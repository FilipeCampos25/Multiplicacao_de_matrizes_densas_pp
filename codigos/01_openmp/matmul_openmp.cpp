/* Multiplicacao de matrizes densas C = A x B com OpenMP.
 *
 * Adaptacao do baseline 01_matmul.cpp, do Prof. Rodrigo Goncalves Pinto.
 * A distribuicao do laco externo e a medicao seguem, conceitualmente, os
 * exemplos 004_loop_for_paralell.cpp e 005_loop_for_paralell_time.cpp.
 *
 * A paralelizacao distribui as linhas de C entre as threads. Cada thread
 * escreve em linhas exclusivas, portanto nao ha compartilhamento de escritas.
 * A ordem i-k-j do baseline foi preservada para manter a localidade de cache.
 *
 * Compilar: make openmp
 * Executar: OMP_NUM_THREADS=4 ./bin/openmp/matmul_openmp 512
 */

#include <omp.h>

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

// Variantes de benchmark usam a mesma fonte; a compilacao normal segue static.
#ifndef MATMUL_BENCH_SCHEDULE
#define MATMUL_BENCH_SCHEDULE static
#endif

namespace {

bool parse_dimension(const char* text, int& dimension) {
    if (text == nullptr || *text == '\0') return false;
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
                expected += static_cast<double>(a_index % 100) * 0.01 *
                            (static_cast<double>((b_index * 7) % 100) * 0.01);
            }
            const std::size_t index =
                static_cast<std::size_t>(i) * dimension + j;
            const double error = std::abs(result[index] - expected);
            maximum_error = std::max(maximum_error, error);
            const double tolerance = 1.0e-12 * (1.0 + std::abs(expected));
            if (error > tolerance) return false;
        }
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    int dimension = 512;
    const bool verify = argc == 3 && std::strcmp(argv[2], "--verify") == 0;
    if (argc > 3 || (argc >= 2 && !parse_dimension(argv[1], dimension)) ||
        (argc == 3 && !verify)) {
        std::fprintf(stderr, "Uso: %s [N positivo] [--verify]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const std::size_t n = static_cast<std::size_t>(dimension);
    if (n > static_cast<std::size_t>(-1) / n) {
        std::fprintf(stderr, "Erro: N*N excede o limite de tamanho.\n");
        return EXIT_FAILURE;
    }

    const std::size_t element_count = n * n;
    std::vector<double> matrix_a;
    std::vector<double> matrix_b;
    std::vector<double> matrix_c;
    try {
        matrix_a.resize(element_count);
        matrix_b.resize(element_count);
        matrix_c.assign(element_count, 0.0);
    } catch (const std::bad_alloc&) {
        std::fprintf(stderr, "Erro: memoria insuficiente para N=%d.\n", dimension);
        return EXIT_FAILURE;
    } catch (const std::length_error&) {
        std::fprintf(stderr, "Erro: dimensao N=%d excede o limite dos vetores.\n",
                     dimension);
        return EXIT_FAILURE;
    }

    for (std::size_t index = 0; index < element_count; ++index) {
        matrix_a[index] = static_cast<double>(index % 100) * 0.01;
        matrix_b[index] = static_cast<double>((index * 7) % 100) * 0.01;
    }

    int thread_count = 1;
    const double start = omp_get_wtime();
#pragma omp parallel
    {
#pragma omp single
        thread_count = omp_get_num_threads();

#pragma omp for schedule(MATMUL_BENCH_SCHEDULE)
        for (int i = 0; i < dimension; ++i) {
            for (int k = 0; k < dimension; ++k) {
                const double a = matrix_a[static_cast<std::size_t>(i) * dimension + k];
                for (int j = 0; j < dimension; ++j) {
                    matrix_c[static_cast<std::size_t>(i) * dimension + j] +=
                        a * matrix_b[static_cast<std::size_t>(k) * dimension + j];
                }
            }
        }
    }
    const double elapsed = omp_get_wtime() - start;

    // Mantem a ordem sequencial do baseline. Uma reduction (exemplo 007)
    // poderia mudar a ordem das somas e introduzir diferencas de arredondamento.
    double checksum = 0.0;
    for (double value : matrix_c) checksum += value;
    std::printf("N=%d  checksum=%.6f  tempo=%.6f s  threads=%d\n", dimension,
                checksum, elapsed, thread_count);

    // Exportacao opcional para testes, em row-major e fora da medicao.
    const char* dump = std::getenv("MATMUL_DUMP");
    if (dump != nullptr && std::strcmp(dump, "1") == 0) {
        std::printf("C=");
        for (double value : matrix_c) std::printf(" %.17g", value);
        std::printf("\n");
    }

    if (verify) {
        double maximum_error = 0.0;
        const bool correct = verify_result(matrix_c, dimension, maximum_error);
        std::printf("verificacao=%s  max_erro=%.3e\n",
                    correct ? "OK" : "FALHOU", maximum_error);
        return correct ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
