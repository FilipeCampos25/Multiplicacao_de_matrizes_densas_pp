/* Multiplicacao de matrizes densas C = A x B com OpenMP.
 *
 * Adaptacao do baseline 01_matmul.cpp, do Prof. Rodrigo Goncalves Pinto.
 * A distribuicao do laco externo e a medicao seguem, conceitualmente, os
 * exemplos 004_loop_for_paralell.cpp e 005_loop_for_paralell_time.cpp.
 * A inicializacao, o armazenamento row-major e a ordem i-k-j foram preservados.
 *
 * Compilar: make openmp
 * Executar: ./bin/openmp/01_matmul_openmp [N] [threads]
 */
#include <charconv>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <string_view>
#include <vector>
#include <omp.h>

// Converte um argumento textual para int e aceita somente um inteiro positivo.
// std::from_chars informa tanto erros de conversao (resultado.ec) quanto o ponto
// em que parou (resultado.ptr). Exigir que ptr chegue ao fim rejeita entradas
// parcialmente numericas, como "512x". O valor e devolvido por referencia.
static bool inteiro_positivo(const char* argumento, int& valor) {
    std::string_view texto(argumento);
    auto resultado = std::from_chars(texto.data(), texto.data() + texto.size(), valor);
    return resultado.ec == std::errc{} &&
           resultado.ptr == texto.data() + texto.size() && valor > 0;
}

int main(int argc, char** argv) {
    // Fase 1 - parametros. Sem argumentos, usa-se uma matriz 512 x 512 e a
    // quantidade de threads escolhida pelo ambiente OpenMP. argv[1] redefine N;
    // argv[2], quando presente, redefine a quantidade solicitada de threads.
    int N = 512;
    int threads = 0;
    if (argc > 3 ||
        (argc > 1 && !inteiro_positivo(argv[1], N)) ||
        (argc > 2 && !inteiro_positivo(argv[2], threads))) {
        std::fprintf(stderr, "Uso: %s [N inteiro positivo] [threads inteiro positivo]\n", argv[0]);
        return 1;
    }

    // Antes de reservar memoria, garante que N*N e (N*N-1)*7 caibam em int.
    // Esses valores aparecem nos indices e na inicializacao de B. Sem a guarda,
    // overflow de inteiro poderia produzir indices/valores incorretos antes mesmo
    // de o vector detectar que a matriz e grande demais.
    if (N > INT_MAX / N || N*N - 1 > INT_MAX / 7) {
        std::fprintf(stderr, "Erro: N excede o limite dos indices e da inicializacao com int.\n");
        return 1;
    }

    // omp_set_num_threads configura o tamanho desejado para equipes posteriores.
    // O runtime ainda pode criar menos threads (por limite do sistema, modo
    // dinamico etc.); por isso o programa registra separadamente o valor efetivo.
    if (argc > 2)
        omp_set_num_threads(threads);
    // omp_get_max_threads retorna o limite que seria usado na proxima regiao
    // paralela neste ponto, nao uma contagem de threads ja criadas.
    const int threads_configuradas = omp_get_max_threads();
    int threads_efetivas = 1;

    // Fase 2 - alocacao e inicializacao. As matrizes N x N sao armazenadas como
    // vetores lineares em row-major: todos os elementos de uma linha ficam
    // contiguos. Assim, o elemento [linha][coluna] esta em linha*N + coluna.
    // C comeca em zero porque cada C[i][j] sera um acumulador de produtos.
    std::vector<double> A(N*N), B(N*N), C(N*N, 0.0);
    // A entrada deterministica permite comparar execucoes e o baseline: o mesmo
    // indice sempre gera o mesmo valor, independentemente do numero de threads.
    for (int i = 0; i < N*N; i++) {
        A[i] = (i % 100) * 0.01;
        B[i] = ((i * 7) % 100) * 0.01;
    }

    // Fases 3 a 5 - execucao paralela. omp_get_wtime devolve tempo de parede em
    // segundos. O intervalo medido inclui criacao/encerramento da equipe,
    // distribuicao do trabalho, calculo e sincronizacao; nao inclui inicializacao,
    // checksum nem impressao.
    const double t0 = omp_get_wtime();
    // "parallel" cria uma equipe de threads dentro do mesmo processo. Ao contrario
    // de processos MPI, threads compartilham o mesmo espaco de memoria. Ao fim do
    // bloco, ha sincronizacao e a equipe e encerrada/reutilizada pelo runtime.
    // default(none) obriga a declarar o compartilhamento das variaveis externas,
    // evitando que uma regra implicita esconda enganos. A, B, C, N e o contador
    // observado referem-se aos mesmos objetos para toda a equipe. Indices declarados
    // dentro dos lacos/blocos (i, k, j e a) sao privados a cada thread.
    #pragma omp parallel default(none) shared(A, B, C, N, threads_efetivas)
    {
        // "single" faz exatamente uma thread da equipe executar a atribuicao.
        // Isso evita varias escritas simultaneas em threads_efetivas. Sua barreira
        // implicita tambem faz todas as threads aguardarem antes do omp for.
        #pragma omp single
        threads_efetivas = omp_get_num_threads();

        // "omp for" nao cria outra equipe: reparte entre a equipe existente as
        // iteracoes do laco imediatamente seguinte. schedule(static) faz uma divisao
        // previamente determinada em blocos quase iguais, apropriada porque toda
        // linha executa N*N produtos. Exemplo: N=4 e duas threads normalmente dao
        // i={0,1} a thread 0 e i={2,3} a thread 1.
        //
        // Matematicamente, C[i][j] = soma, para k=0..N-1, de A[i][k]*B[k][j].
        // Os lacos i-k-j implementam essa soma por acumulacao:
        //   A[i*N+k] e A[i][k], B[k*N+j] e B[k][j], C[i*N+j] e C[i][j].
        // Para cada par (i,k), "a" guarda A[i][k] e o laco j acrescenta sua
        // contribuicao a toda a linha i de C. Ha N escolhas para cada um dos tres
        // indices, portanto o custo dominante e O(N^3).
        //
        // Distribuir i e especialmente seguro: cada i pertence a uma unica thread,
        // que escreve exclusivamente a linha i de C; A e B sao somente lidas.
        // Logo nao existe atualizacao concorrente da mesma celula e nao se requer
        // critical, atomic ou reduction. Paralelizar k sem outra estrategia faria
        // threads disputarem o mesmo C[i][j].
        //
        // A ordem i-k-j tambem favorece cache: para k fixo, j percorre de forma
        // contigua a linha k de B e a linha i de C em armazenamento row-major; o
        // valor de A e reutilizado durante todo o laco j.
        #pragma omp for schedule(static)
        for (int i = 0; i < N; i++)
            for (int k = 0; k < N; k++) {
                const double a = A[i*N + k];
                for (int j = 0; j < N; j++)
                    C[i*N + j] += a * B[k*N + j];
            }
        }
    // O omp for possui barreira implicita: uma thread que termina suas linhas cedo
    // espera as demais. A propria saida de "parallel" tambem sincroniza a equipe.
    // Portanto nenhuma thread continua existindo neste bloco e C esta completa
    // antes de t1 e do checksum sequencial.
    const double t1 = omp_get_wtime();

    // Fases 7 e 8 - verificacao resumida e apresentacao. O checksum percorre C na
    // mesma ordem sequencial do baseline. Uma reduction paralela poderia reagrupar
    // somas; como ponto flutuante nao e exatamente associativo, isso pode alterar
    // os ultimos bits mesmo quando todos os elementos de C estao corretos.
    double soma = 0.0;
    for (int i = 0; i < N*N; i++)
        soma += C[i];
    std::printf("N=%d  threads_configuradas=%d  checksum=%.6f  tempo=%.6f s"
                "  threads=%d\n",
                N, threads_configuradas, soma, t1 - t0, threads_efetivas);
    // MATMUL_DUMP=1 exporta todos os elementos na ordem row-major para testes. A
    // operacao fica fora da regiao cronometrada e nao interfere no paralelismo.
    const char* dump = std::getenv("MATMUL_DUMP");
    if (dump != nullptr && std::string_view(dump) == "1") {
        std::printf("C=");
        for (double value : C) std::printf(" %.17g", value);
        std::printf("\n");
    }
    return 0;
}
