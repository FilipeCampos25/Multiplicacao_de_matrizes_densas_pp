/* Copia instrumentada do baseline sequencial para perfilamento por trechos.
 * O original em codigos/baselines_sequenciais_CCO085/01_matmul.cpp permanece
 * inalterado. A computacao e a entrada deterministica sao equivalentes.
 */
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>

static double agora() {
    timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

int main(int argc, char** argv) {
    const int n = (argc > 1) ? std::atoi(argv[1]) : 512;
    if (n <= 0) {
        std::fprintf(stderr, "N deve ser positivo.\n");
        return EXIT_FAILURE;
    }

    const double inicio_total = agora();
    const double inicio_alocacao = agora();
    std::vector<double> a(static_cast<std::size_t>(n) * n);
    std::vector<double> b(static_cast<std::size_t>(n) * n);
    std::vector<double> c(static_cast<std::size_t>(n) * n, 0.0);
    const double fim_alocacao = agora();

    const double inicio_inicializacao = agora();
    for (std::size_t i = 0; i < a.size(); ++i) {
        a[i] = static_cast<double>(i % 100) * 0.01;
        b[i] = static_cast<double>((i * 7) % 100) * 0.01;
    }
    const double fim_inicializacao = agora();

    const double inicio_multiplicacao = agora();
    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < n; ++k) {
            const double valor_a = a[static_cast<std::size_t>(i) * n + k];
            for (int j = 0; j < n; ++j) {
                c[static_cast<std::size_t>(i) * n + j] +=
                    valor_a * b[static_cast<std::size_t>(k) * n + j];
            }
        }
    }
    const double fim_multiplicacao = agora();

    const double inicio_checksum = agora();
    double checksum = 0.0;
    for (double valor : c) checksum += valor;
    const double fim_checksum = agora();
    const double fim_total = agora();

    std::printf("N=%d checksum=%.6f total_s=%.9f alocacao_s=%.9f "
                "inicializacao_s=%.9f multiplicacao_s=%.9f checksum_s=%.9f\n",
                n, checksum, fim_total - inicio_total,
                fim_alocacao - inicio_alocacao,
                fim_inicializacao - inicio_inicializacao,
                fim_multiplicacao - inicio_multiplicacao,
                fim_checksum - inicio_checksum);
    return EXIT_SUCCESS;
}
