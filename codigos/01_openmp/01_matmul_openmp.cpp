/* Multiplicacao de matrizes densas C = A x B com OpenMP.
 * Compilar: make openmp
 * Executar: ./bin/openmp/01_matmul_openmp [N] [threads]
 */
#include <charconv>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string_view>
#include <vector>
#include <omp.h>

static bool inteiro_positivo(const char* argumento, int& valor) {
    std::string_view texto(argumento);
    auto resultado = std::from_chars(texto.data(), texto.data() + texto.size(), valor);
    return resultado.ec == std::errc{} &&
           resultado.ptr == texto.data() + texto.size() && valor > 0;
}

static double agora() {
    timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

int main(int argc, char** argv) {
    int N = 512;
    int threads = 0;
    if (argc > 3 ||
        (argc > 1 && !inteiro_positivo(argv[1], N)) ||
        (argc > 2 && !inteiro_positivo(argv[2], threads))) {
        std::fprintf(stderr, "Uso: %s [N inteiro positivo] [threads inteiro positivo]\n", argv[0]);
        return 1;
    }

    // Preserva os indices int e a formula i * 7 sem overflow.
    if (N > INT_MAX / N || N*N - 1 > INT_MAX / 7) {
        std::fprintf(stderr, "Erro: N excede o limite dos indices e da inicializacao com int.\n");
        return 1;
    }

    if (argc > 2)
        omp_set_num_threads(threads);
    // Limite configurado para a proxima regiao, nao o tamanho efetivo da equipe.
    const int threads_configuradas = omp_get_max_threads();

    std::vector<double> A(N*N), B(N*N), C(N*N, 0.0);
    for (int i = 0; i < N*N; i++) {
        A[i] = (i % 100) * 0.01;
        B[i] = ((i * 7) % 100) * 0.01;
    }

    double t0 = agora();
    // Cada iteracao escreve em uma linha exclusiva de C; k, j e a sao locais.
    #pragma omp parallel for schedule(static) default(none) shared(A, B, C, N)
    for (int i = 0; i < N; i++)
        for (int k = 0; k < N; k++) {
            double a = A[i*N + k];
            for (int j = 0; j < N; j++)
                C[i*N + j] += a * B[k*N + j];
        }
    // A barreira implicita garante a conclusao de todas as threads.
    double t1 = agora();

    double soma = 0.0;
    for (int i = 0; i < N*N; i++)
        soma += C[i];
    std::printf("N=%d  threads_configuradas=%d  checksum=%.6f  tempo=%.6f s\n",
                N, threads_configuradas, soma, t1 - t0);
    // Exportacao opcional para testes, em row-major e fora da medicao.
    const char* dump = std::getenv("MATMUL_DUMP");
    if (dump != nullptr && std::string_view(dump) == "1") {
        std::printf("C=");
        for (double value : C) std::printf(" %.17g", value);
        std::printf("\n");
    }
    return 0;
}
