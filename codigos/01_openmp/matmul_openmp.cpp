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

// A macro permite compilar a mesma implementacao com politicas diferentes para
// experimentos, sem mudar o algoritmo. Sem -DMATMUL_BENCH_SCHEDULE=..., usa-se
// static. No pragma abaixo ela se expande, por exemplo, para schedule(dynamic).
// static distribui blocos previamente; dynamic/guided entregam trabalho durante a
// execucao e tem maior custo de escalonamento. Como cada linha custa quase o mesmo,
// static tende a ser a escolha natural deste programa.
#ifndef MATMUL_BENCH_SCHEDULE
#define MATMUL_BENCH_SCHEDULE static
#endif

namespace {

bool parse_dimension(const char* text, int& dimension) {
    // strtol permite distinguir texto invalido, valor fora do alcance e sufixos.
    // A funcao so altera dimension depois que toda a entrada foi validada.
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
    // Recalcula sequencialmente cada elemento a partir da inicializacao conhecida.
    // Isto verifica o resultado sem confiar no mesmo vetor A/B usado pelo calculo
    // paralelo. Para cada (i,j), expected acumula os N produtos A[i][k]*B[k][j].
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
            // Ponto flutuante sofre arredondamento. A tolerancia combina uma parte
            // absoluta (1e-12) e outra relativa ao tamanho do resultado, evitando
            // exigir igualdade bit a bit e ainda detectando erros relevantes.
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
    // Fase 1 - argumentos. A dimensao padrao e 512. O segundo argumento opcional
    // so pode ser --verify; qualquer combinacao diferente produz erro de uso.
    int dimension = 512;
    const bool verify = argc == 3 && std::strcmp(argv[2], "--verify") == 0;
    if (argc > 3 || (argc >= 2 && !parse_dimension(argv[1], dimension)) ||
        (argc == 3 && !verify)) {
        std::fprintf(stderr, "Uso: %s [N positivo] [--verify]\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Confere a multiplicacao antes de calcular N*N em size_t. Sem essa guarda,
    // overflow poderia levar a uma alocacao menor e acessos fora do vetor.
    const std::size_t n = static_cast<std::size_t>(dimension);
    if (n > static_cast<std::size_t>(-1) / n) {
        std::fprintf(stderr, "Erro: N*N excede o limite de tamanho.\n");
        return EXIT_FAILURE;
    }

    // Fase 2 - cada matriz N x N possui N*N doubles contiguos. A reserva fica em
    // try porque o tamanho pode exceder tanto a memoria quanto o limite do vector.
    // C e preenchida com zero: cada posicao sera o acumulador da soma de produtos.
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

    // Entrada deterministica igual ao baseline, preparada sequencialmente e fora
    // da medicao para que o tempo represente apenas a parte paralela principal.
    for (std::size_t index = 0; index < element_count; ++index) {
        matrix_a[index] = static_cast<double>(index % 100) * 0.01;
        matrix_b[index] = static_cast<double>((index * 7) % 100) * 0.01;
    }

    // Fases 3 a 5 - OpenMP usa varias threads dentro de um unico processo. Elas
    // compartilham matrix_a, matrix_b e matrix_c, ao passo que variaveis declaradas
    // dentro do bloco/lacos, como i, k, j e a, sao privadas por thread.
    // omp_get_wtime mede tempo de parede. O trecho inclui criacao da equipe,
    // escalonamento, calculo e barreiras, mas exclui alocacao, verificacao e saida.
    int thread_count = 1;
    const double start = omp_get_wtime();
// "parallel" cria a equipe; sair do bloco sincroniza e encerra/recolhe a equipe.
#pragma omp parallel
    {
// "single" permite que apenas uma thread registre o tamanho real da equipe. Essa
// quantidade pode diferir de OMP_NUM_THREADS/omp_get_max_threads por decisoes do
// runtime. A barreira implicita de single ocorre antes de todas seguirem ao for.
#pragma omp single
        thread_count = omp_get_num_threads();

// "omp for" distribui apenas as iteracoes de i entre a equipe ja criada. A macro
// seleciona o schedule no build. Com static e N=4/duas threads, uma divisao tipica
// e thread 0 -> linhas 0,1; thread 1 -> linhas 2,3.
//
// Em row-major, [linha][coluna] vira linha*N+coluna. Logo:
//   matrix_a[i*N+k] = A[i][k]
//   matrix_b[k*N+j] = B[k][j]
//   matrix_c[i*N+j] = C[i][j]
// A formula C[i][j] = soma(k=0..N-1) A[i][k]*B[k][j] e executada
// incrementalmente. C comeca em zero; para cada i e k, "a" guarda A[i][k] e
// acrescenta a contribuicao a todos os j. Os tres intervalos tem N valores, por
// isso a complexidade de calculo e O(N^3).
//
// Cada iteracao i escreve uma linha exclusiva de C. Como uma mesma linha nunca e
// entregue simultaneamente a duas threads, nao ha corrida e critical/atomic seriam
// desnecessarios e caros; reduction tambem nao se aplica, pois nao se agrega uma
// unica variavel compartilhada. A e B sao somente lidas.
//
// A ordem i-k-j percorre B[k][j] e C[i][j] contiguamente quando j varia, aproveita
// linhas de cache e reutiliza "a" N vezes. Ao final do omp for existe uma barreira
// implicita: threads que acabam antes esperam, garantindo C completa na saida.
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

    // Fases 7 e 8 - checksum e relatorio. Mantem-se deliberadamente a ordem
    // sequencial do baseline. Uma reduction (como no exemplo 007) criaria somas
    // privadas e as combinaria em uma ordem dependente da execucao. Como a soma em
    // ponto flutuante nao e exatamente associativa, isso pode mudar os ultimos bits.
    double checksum = 0.0;
    for (double value : matrix_c) checksum += value;
    std::printf("N=%d  checksum=%.6f  tempo=%.6f s  threads=%d\n", dimension,
                checksum, elapsed, thread_count);

    // MATMUL_DUMP=1 exporta C em row-major com precisao suficiente para testes. A
    // exportacao e sequencial, opcional e ocorre fora da regiao cronometrada.
    const char* dump = std::getenv("MATMUL_DUMP");
    if (dump != nullptr && std::strcmp(dump, "1") == 0) {
        std::printf("C=");
        for (double value : matrix_c) std::printf(" %.17g", value);
        std::printf("\n");
    }

    // --verify solicita a verificacao elemento a elemento descrita acima. O maior
    // erro observado ajuda a diagnosticar diferencas; o status tambem define o
    // codigo de saida, permitindo que scripts detectem uma falha.
    if (verify) {
        double maximum_error = 0.0;
        const bool correct = verify_result(matrix_c, dimension, maximum_error);
        std::printf("verificacao=%s  max_erro=%.3e\n",
                    correct ? "OK" : "FALHOU", maximum_error);
        return correct ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
