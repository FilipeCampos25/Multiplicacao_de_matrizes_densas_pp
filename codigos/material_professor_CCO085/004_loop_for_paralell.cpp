/*---------------------------------------
 004 - Paralelizar o for básico com OpenMP
-----------------------------------------*/
/*
 Mesmo problema do script 003:
        a[i] = x[i]*x[i] + y[i]*y[i] + z[i]*z[i]
 agora dividindo as iterações do for entre várias threads.

 Compilar e rodar (GCC/MinGW):
   g++ -O2 -fopenmp 004_loop_for_paralell.cpp -o 004_loop_for_paralell.exe
   .\004_loop_for_paralell.exe

 Para escolher o número de threads (PowerShell):
   $env:OMP_NUM_THREADS = "2"
   .\004_loop_for_paralell.exe
 */

#include <iostream>
#include <vector>
#include <omp.h>

int main() {
    const int N = 10;                 // tamanho dos vetores
    std::vector<int> x(N), y(N), z(N), a(N);

    // Inicializa valores simples (sequencial)
    for (int i = 0; i < N; i++) {
        x[i] = i;
        y[i] = i + 1;
        z[i] = i + 2;
    }

    std::cout << "Calculando a = x*x + y*y + z*z para vetores de tamanho " << N << "\n";

    /*--------------------------------------------------
     Paralelizar o loop for com OpenMP
    ----------------------------------------------------*/
    // A diretiva "omp parallel for" paraleliza o loop for logo abaixo dela.
    // Ela diz ao compilador: "crie um time de threads e divida as iterações do for entre elas".
    // Cada thread recebe um pedaço do intervalo [0, N) para calcular.
    //
    // É a junção de duas diretivas:
    //   #pragma omp parallel   -> cria as threads
    //   #pragma omp for        -> divide as iterações entre elas

    /*--------------------------------------------------
     A cláusula schedule(static)
     --------------------------------------------------
     Define COMO as iterações são distribuídas entre as threads.
     - static: o OpenMP divide as iterações em blocos de tamanho (quase) igual,
       ANTES de o loop começar, e cada thread fica com um bloco fixo.

       Exemplo: N = 10, 2 threads
       -->  Thread 0: i = 0,1,2,3,4
            Thread 1: i = 5,6,7,8,9

       Exemplo: N = 10, 3 threads (10 não divide por 3)
       -->  Thread 0: i = 0,1,2,3
            Thread 1: i = 4,5,6
            Thread 2: i = 7,8,9

     - É rápido (quase nenhum custo de organização) e ótimo quando todas as
       iterações têm o mesmo custo — como aqui, em que toda iteração faz
       3 multiplicações e 2 somas.
     - Se as iterações tivessem custos muito diferentes, schedule(dynamic)
       poderia equilibrar melhor (exercício 5 da lista 005).

     --------------------------------------------------
     O corpo: a[i] = x[i]*x[i] + y[i]*y[i] + z[i]*z[i];
     - Cada thread calcula a[i] apenas para os índices que recebeu.
     - Não há risco de conflito: cada posição a[i] é escrita por UMA única thread,
       e x, y, z são apenas lidos.
       Exemplo: a Thread 0 escreve a[0..4] e a Thread 1 escreve a[5..9].
       Ninguém escreve na mesma posição.
     - O 'i' do for é automaticamente PRIVADO: cada thread tem o seu contador.
     --------------------------------------------------*/

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < N; i++) {
        a[i] = x[i]*x[i] + y[i]*y[i] + z[i]*z[i];
        // (evite imprimir aqui para não embaralhar a saída)
    }
    // Ao final do "parallel for" existe uma BARREIRA IMPLÍCITA:
    // o programa só segue quando TODAS as threads terminaram suas iterações.
    // Por isso, abaixo, o vetor 'a' já está completo.

    // --- IMPRESSÃO EM ORDEM (SEQUENCIAL) ---
    // Mantemos a impressão fora da região paralela para garantir ordem por i.
    for (int i = 0; i < N; i++) {
        std::cout << "i = " << i
                  << " | x[i] = " << x[i]
                  << " | y[i] = " << y[i]
                  << " | z[i] = " << z[i]
                  << " | a[i] = " << a[i]
                  << std::endl;
    }

    // Resumo tabular
    std::cout << "\nResumo dos resultados:\n";
    std::cout << "----------------------------------------\n";
    std::cout << "  i |   x[i] |   y[i] |   z[i] |   a[i]\n";
    std::cout << "----------------------------------------\n";
    for (int i = 0; i < N; i++) {
        std::cout << " " << i
                  << "  | " << x[i]
                  << "     | " << y[i]
                  << "     | " << z[i]
                  << "     | " << a[i]
                  << std::endl;
    }
    std::cout << "----------------------------------------\n";
}

/*--------------------------------------------------
 Analisando a saída
 --------------------------------------------------*/
/*
  A saída mostra que o cálculo foi feito corretamente e é IDÊNTICA à do script 003:
     i = 0 -> a = 5,  i = 1 -> a = 14,  i = 2 -> a = 29, ...,  i = 9 -> a = 302.
  Cada a[i] corresponde a x[i]^2 + y[i]^2 + z[i]^2.

  A paralelização do loop for com OpenMP é simples e eficaz para acelerar cálculos independentes.
  Cada thread trabalha em seu próprio subconjunto de dados, evitando conflitos de escrita.

  Observações:
  - Evite imprimir dentro do loop paralelo, pois isso pode embaralhar a saída.
  - Mantenha a impressão fora da região paralela para garantir ordem e clareza.
  - O OpenMP gerencia a criação e a sincronização das threads automaticamente.
  - O OpenMP escolhe o número de threads com base no número de núcleos lógicos da CPU, por padrão.
  - Para controlar o número de threads, use a variável de ambiente OMP_NUM_THREADS.
  - Com N = 10 o paralelo NÃO fica mais rápido: criar as threads custa mais que
    fazer 10 contas. O ganho só aparece com N grande (script 005, N = 1.000.000).
  */
