/*----------------------------------------------------------
 011 - Paralelizando operações com MATRIZES
------------------------------------------------------------
 Objetivo: multiplicar duas matrizes, C = A x B, em paralelo,
 e comparar com a versão sequencial (tempo e resultado).

 Analogia:
   Uma matriz é uma planilha. Para preencher a planilha C, cada célula
   C[i][j] é calculada com a LINHA i de A e a COLUNA j de B.
   Nenhuma célula depende de outra célula de C.
   -> Dá para dividir as LINHAS de C entre as threads, como dividir
      as linhas de uma planilha entre vários digitadores.

 ----------------------------------------------------------
 Multiplicação de matrizes, passo a passo (exemplo 2x2):

        A = | 1  2 |      B = | 5  6 |
            | 3  4 |          | 7  8 |

   C[i][j] = soma de A[i][k] * B[k][j], para k = 0 .. N-1
             (linha i de A "vezes" coluna j de B)

   C[0][0] = 1*5 + 2*7 = 5 + 14  = 19
   C[0][1] = 1*6 + 2*8 = 6 + 16  = 22
   C[1][0] = 3*5 + 4*7 = 15 + 28 = 43
   C[1][1] = 3*6 + 4*8 = 18 + 32 = 50

        C = | 19  22 |
            | 43  50 |

   Custo: para uma matriz N x N são N³ multiplicações.
   N = 500 -> 125 milhões de multiplicações. Aqui o paralelismo compensa.
 ----------------------------------------------------------
 Como guardamos a matriz:
   Usamos um vetor "achatado" de N*N posições.
   A posição da linha i, coluna j é:  i * N + j
   Exemplo com N = 3:  [0][0]=0 [0][1]=1 [0][2]=2 | [1][0]=3 [1][1]=4 ... [2][2]=8
   (Mais rápido e simples que vector<vector<double>>.)

 Compilar e rodar (PowerShell):
   g++ -O2 -fopenmp 011_matriz_paralela.cpp -o 011_matriz_paralela.exe
   .\011_matriz_paralela.exe

 Teste com outros números de threads:
   $env:OMP_NUM_THREADS = "2"; .\011_matriz_paralela.exe
   $env:OMP_NUM_THREADS = "8"; .\011_matriz_paralela.exe
----------------------------------------------------------*/

#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <omp.h>

// Imprime uma matriz pequena (só para o exemplo 2x2)
void imprimir_matriz(const std::vector<double>& M, int N, const char* nome) {
    std::cout << nome << " =\n";
    for (int i = 0; i < N; ++i) {
        std::cout << "   |";
        for (int j = 0; j < N; ++j) std::cout << std::setw(6) << M[i * N + j];
        std::cout << " |\n";
    }
}

// ---------------------------------------------------------
// Versão SEQUENCIAL: uma thread faz todas as células.
// ---------------------------------------------------------
void multiplicar_sequencial(const std::vector<double>& A, const std::vector<double>& B,
                            std::vector<double>& C, int N) {
    for (int i = 0; i < N; ++i) {            // cada linha de C
        for (int j = 0; j < N; ++j) {        // cada coluna de C
            double soma = 0.0;
            for (int k = 0; k < N; ++k) {    // linha i de A "vezes" coluna j de B
                soma += A[i * N + k] * B[k * N + j];
            }
            C[i * N + j] = soma;
        }
    }
}

// ---------------------------------------------------------
// Versão PARALELA: as LINHAS de C são divididas entre as threads.
// ---------------------------------------------------------
void multiplicar_paralelo(const std::vector<double>& A, const std::vector<double>& B,
                          std::vector<double>& C, int N) {
    /*
     O que cada variável é:
       i     -> índice do for paralelizado: PRIVADO automaticamente.
       j, k  -> declarados DENTRO do loop: cada thread tem os seus (privados).
       soma  -> declarada DENTRO do loop: privada. Cada célula tem sua soma.
       A, B  -> compartilhadas, apenas LIDAS: seguro.
       C     -> compartilhada, mas cada thread escreve em LINHAS diferentes: seguro.

     Exemplo com N = 500 e 4 threads (schedule static):
       Thread 0 -> linhas   0 a 124
       Thread 1 -> linhas 125 a 249
       Thread 2 -> linhas 250 a 374
       Thread 3 -> linhas 375 a 499

     Não precisa de critical, atomic nem reduction:
     nenhuma thread escreve na mesma posição de outra.

     Por que paralelizar o loop de FORA (i) e não o de dentro (k)?
       - Fora: as threads são criadas UMA vez e cada uma recebe muito trabalho.
       - Dentro (k): todas as threads somariam na MESMA célula (precisaria de reduction)
         e a região paralela seria aberta N² vezes -> muito custo e pouco ganho.
       Regra prática: paralelize o loop mais EXTERNO que for independente.
    */
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            double soma = 0.0;
            for (int k = 0; k < N; ++k) {
                soma += A[i * N + k] * B[k * N + j];
            }
            C[i * N + j] = soma;
        }
    }
    /*
     Variação: #pragma omp parallel for collapse(2)
       "Junta" os loops i e j num só, com N*N iterações, e divide ESSAS entre as threads.
       Útil quando há poucas linhas (N pequeno) e muitas threads.
       Exemplo: N = 6 e 16 threads -> só 6 linhas, então 10 threads ficariam paradas;
                com collapse(2) são 36 células para dividir entre as 16.
    */
}

int main() {
    /*==========================================
     PARTE 1 - Exemplo 2x2 (o mesmo feito à mão acima)
     ==========================================*/
    {
        const int N = 2;
        std::vector<double> A = {1, 2,
                                 3, 4};
        std::vector<double> B = {5, 6,
                                 7, 8};
        std::vector<double> C(N * N);

        multiplicar_paralelo(A, B, C, N);

        std::cout << "PARTE 1 - Exemplo 2x2\n";
        imprimir_matriz(A, N, "A");
        imprimir_matriz(B, N, "B");
        imprimir_matriz(C, N, "C = A x B");
        std::cout << "   (esperado: 19 22 / 43 50)\n\n";
    }

    /*==========================================
     PARTE 2 - Matriz grande: sequencial x paralelo
     ==========================================*/
    const int N = 500;
    std::vector<double> A(N * N), B(N * N), C_seq(N * N), C_par(N * N);

    // Preenche A e B com valores simples (a inicialização em si também pode ser paralela)
    #pragma omp parallel for
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            A[i * N + j] = (i + j) % 10;        // valores de 0 a 9
            B[i * N + j] = (i * 2 + j) % 7;     // valores de 0 a 6
        }
    }

    std::cout << "PARTE 2 - Matriz " << N << "x" << N
              << " (" << (double)N * N * N / 1e6 << " milhoes de multiplicacoes)\n";
    std::cout << "Threads disponiveis: " << omp_get_max_threads() << "\n";

    double t0 = omp_get_wtime();
    multiplicar_sequencial(A, B, C_seq, N);
    double t_seq = omp_get_wtime() - t0;

    t0 = omp_get_wtime();
    multiplicar_paralelo(A, B, C_par, N);
    double t_par = omp_get_wtime() - t0;

    // Conferência: as duas versões precisam dar EXATAMENTE o mesmo resultado
    double maior_diferenca = 0.0;
    for (int i = 0; i < N * N; ++i) {
        maior_diferenca = std::max(maior_diferenca, std::fabs(C_seq[i] - C_par[i]));
    }

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Tempo sequencial: " << t_seq << " s\n";
    std::cout << "Tempo paralelo:   " << t_par << " s\n";
    std::cout << std::setprecision(2);
    std::cout << "Speedup (seq/par): " << t_seq / t_par << "x\n";
    std::cout << "Maior diferenca entre C_seq e C_par: " << maior_diferenca
              << (maior_diferenca == 0.0 ? "  -> resultados identicos\n" : "  -> ERRO!\n");

    return 0;
}

/*----------------------------------------------------------
 Como ler o resultado:
  - Os tempos dependem da SUA máquina: rode algumas vezes e anote a média.
  - Speedup = tempo sequencial / tempo paralelo.
    Com p threads, o ideal seria p; na prática fica abaixo, porque:
      * criar e sincronizar threads tem custo;
      * as threads disputam a memória (A e B são lidas o tempo todo);
      * núcleos lógicos (Hyper-Threading) não valem um núcleo físico inteiro.
  - Aumente N para 800 ou 1000: o trabalho cresce com N³ e o ganho tende a ficar mais claro.
  - A "maior diferença" deve ser 0: paralelizar não pode mudar o resultado.
----------------------------------------------------------*/
