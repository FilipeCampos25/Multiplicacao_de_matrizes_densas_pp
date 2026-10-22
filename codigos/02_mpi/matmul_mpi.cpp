/* Multiplicacao distribuida de matrizes densas C = A x B com MPI.
 *
 * Estrategia: decomposicao equilibrada por blocos de linhas de A e C.
 * A matriz B e compartilhada integralmente por todos os processos.
 *
 * Compilar: make mpi
 * Executar: mpirun -np 4 ./bin/mpi/matmul_mpi 512
 * Verificar: mpirun -np 4 ./bin/mpi/matmul_mpi 7 --verify
 */

#include <mpi.h>

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <stdexcept>
#include <vector>

namespace {

bool parse_dimension(const char* text, int& dimension) {
    if (text == nullptr || *text == '\0') {
        return false;
    }

    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0' || parsed <= 0 ||
        parsed > INT_MAX) {
        return false;
    }

    dimension = static_cast<int>(parsed);
    return true;
}

void build_distribution(int dimension, int process_count,
                        std::vector<int>& counts,
                        std::vector<int>& displacements) {
    const int base_rows = dimension / process_count;
    const int extra_rows = dimension % process_count;
    int offset = 0;

    for (int process = 0; process < process_count; ++process) {
        const int rows = base_rows + (process < extra_rows ? 1 : 0);
        counts[process] = rows * dimension;
        displacements[process] = offset;
        offset += counts[process];
    }
}

bool verify_result(const std::vector<double>& result, int dimension,
                   double& maximum_error) {
    maximum_error = 0.0;

    for (int i = 0; i < dimension; ++i) {
        for (int j = 0; j < dimension; ++j) {
            double expected = 0.0;
            for (int k = 0; k < dimension; ++k) {
                const std::size_t a_index =
                    static_cast<std::size_t>(i) * dimension + k;
                const std::size_t b_index =
                    static_cast<std::size_t>(k) * dimension + j;
                const double a = static_cast<double>(a_index % 100) * 0.01;
                const double b =
                    static_cast<double>((b_index * 7) % 100) * 0.01;
                expected += a * b;
            }

            const std::size_t index =
                static_cast<std::size_t>(i) * dimension + j;
            const double error = std::abs(result[index] - expected);
            maximum_error = std::max(maximum_error, error);
            const double tolerance = 1.0e-12 * (1.0 + std::abs(expected));
            if (error > tolerance) {
                return false;
            }
        }
    }

    return true;
}

}  // namespace

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int process_count = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &process_count);

    int dimension = 512;
    int input_is_valid = 1;
    int verify = 0;

    if (rank == 0) {
        if (argc > 3 || (argc >= 2 && !parse_dimension(argv[1], dimension)) ||
            (argc == 3 && std::strcmp(argv[2], "--verify") != 0)) {
            input_is_valid = 0;
            std::fprintf(stderr, "Uso: %s [N positivo] [--verify]\n", argv[0]);
        } else {
            verify = (argc == 3) ? 1 : 0;

            // MPI_Scatterv, MPI_Bcast e MPI_Gatherv usam contagens int.
            const long long element_count =
                static_cast<long long>(dimension) * dimension;
            if (element_count > INT_MAX) {
                input_is_valid = 0;
                std::fprintf(stderr,
                             "Erro: N*N deve ser menor ou igual a INT_MAX "
                             "para as coletivas MPI usadas.\n");
            }
        }
    }

    MPI_Bcast(&input_is_valid, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (!input_is_valid) {
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    MPI_Bcast(&dimension, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&verify, 1, MPI_INT, 0, MPI_COMM_WORLD);

    const std::size_t element_count =
        static_cast<std::size_t>(dimension) * dimension;
    std::vector<int> counts(process_count);
    std::vector<int> displacements(process_count);
    build_distribution(dimension, process_count, counts, displacements);

    const int local_element_count = counts[rank];
    std::vector<double> matrix_a;
    std::vector<double> matrix_b;
    std::vector<double> matrix_c;
    std::vector<double> local_a;
    std::vector<double> local_c;
    int allocation_succeeded = 1;

    try {
        matrix_b.resize(element_count);
        local_a.resize(static_cast<std::size_t>(local_element_count));
        local_c.assign(static_cast<std::size_t>(local_element_count), 0.0);
        if (rank == 0) {
            matrix_a.resize(element_count);
            matrix_c.assign(element_count, 0.0);
        }
    } catch (const std::bad_alloc&) {
        allocation_succeeded = 0;
    } catch (const std::length_error&) {
        allocation_succeeded = 0;
    }

    int all_allocations_succeeded = 0;
    MPI_Allreduce(&allocation_succeeded, &all_allocations_succeeded, 1, MPI_INT,
                  MPI_MIN, MPI_COMM_WORLD);
    if (!all_allocations_succeeded) {
        if (rank == 0) {
            std::fprintf(stderr,
                         "Erro: memoria insuficiente para N=%d com %d processos.\n",
                         dimension, process_count);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (rank == 0) {
        for (std::size_t index = 0; index < element_count; ++index) {
            matrix_a[index] = static_cast<double>(index % 100) * 0.01;
            matrix_b[index] = static_cast<double>((index * 7) % 100) * 0.01;
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    const double start = MPI_Wtime();

    // Alguns processos podem receber zero linhas quando P > N. Um endereco
    // valido com contagem zero evita depender do tratamento de nullptr do MPI.
    double unused_buffer = 0.0;
    double* local_a_buffer =
        local_element_count == 0 ? &unused_buffer : local_a.data();
    double* local_c_buffer =
        local_element_count == 0 ? &unused_buffer : local_c.data();

    double stage_start = MPI_Wtime();
    MPI_Scatterv(rank == 0 ? matrix_a.data() : nullptr, counts.data(),
                 displacements.data(), MPI_DOUBLE, local_a_buffer,
                 local_element_count, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    const double scatter_elapsed = MPI_Wtime() - stage_start;

    stage_start = MPI_Wtime();
    MPI_Bcast(matrix_b.data(), static_cast<int>(element_count), MPI_DOUBLE, 0,
              MPI_COMM_WORLD);
    const double broadcast_elapsed = MPI_Wtime() - stage_start;

    const int local_rows = local_element_count / dimension;
    stage_start = MPI_Wtime();
    for (int i = 0; i < local_rows; ++i) {
        for (int k = 0; k < dimension; ++k) {
            const double a = local_a[static_cast<std::size_t>(i) * dimension + k];
            for (int j = 0; j < dimension; ++j) {
                local_c[static_cast<std::size_t>(i) * dimension + j] +=
                    a * matrix_b[static_cast<std::size_t>(k) * dimension + j];
            }
        }
    }
    const double computation_elapsed = MPI_Wtime() - stage_start;

    stage_start = MPI_Wtime();
    MPI_Gatherv(local_c_buffer, local_element_count, MPI_DOUBLE,
                rank == 0 ? matrix_c.data() : nullptr, counts.data(),
                displacements.data(), MPI_DOUBLE, 0, MPI_COMM_WORLD);
    const double gather_elapsed = MPI_Wtime() - stage_start;

    const double local_elapsed = MPI_Wtime() - start;
    double elapsed = 0.0;
    MPI_Reduce(&local_elapsed, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0,
               MPI_COMM_WORLD);

    // O maximo entre ranks representa o caminho critico. Tempos de coletivas
    // tambem podem incorporar espera causada por desbalanceamento.
    const double local_stage_times[] = {scatter_elapsed, broadcast_elapsed,
                                        computation_elapsed, gather_elapsed};
    double maximum_stage_times[4] = {};
    MPI_Reduce(local_stage_times, maximum_stage_times, 4, MPI_DOUBLE, MPI_MAX,
               0, MPI_COMM_WORLD);

    int minimum_rows = 0;
    int maximum_rows = 0;
    MPI_Reduce(&local_rows, &minimum_rows, 1, MPI_INT, MPI_MIN, 0,
               MPI_COMM_WORLD);
    MPI_Reduce(&local_rows, &maximum_rows, 1, MPI_INT, MPI_MAX, 0,
               MPI_COMM_WORLD);

    int exit_code = EXIT_SUCCESS;
    if (rank == 0) {
        double checksum = 0.0;
        for (double value : matrix_c) {
            checksum += value;
        }
        std::printf("N=%d  checksum=%.6f  tempo=%.6f s\n", dimension, checksum,
                    elapsed);
        std::printf(
            "tempos_max_s scatterv=%.9f bcast=%.9f computacao=%.9f "
            "gatherv=%.9f total=%.9f\n",
            maximum_stage_times[0], maximum_stage_times[1],
            maximum_stage_times[2], maximum_stage_times[3], elapsed);
        std::printf("carga_linhas min=%d max=%d diferenca=%d\n", minimum_rows,
                    maximum_rows, maximum_rows - minimum_rows);

        if (verify) {
            double maximum_error = 0.0;
            const bool result_is_correct =
                verify_result(matrix_c, dimension, maximum_error);
            std::printf("verificacao=%s  max_erro=%.3e\n",
                        result_is_correct ? "OK" : "FALHOU", maximum_error);
            if (!result_is_correct) {
                exit_code = EXIT_FAILURE;
            }
        }
    }

    MPI_Bcast(&exit_code, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Finalize();
    return exit_code;
}
