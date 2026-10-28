/*----------------------------------------------------------
 000 - Hello World paralelo
------------------------------------------------------------
 Objetivo: abrir uma região paralela e imprimir uma mensagem por thread.

 Analogia:
   Pense no programa como um professor sozinho na sala (a thread principal).
   Quando ele chega no "#pragma omp parallel", ele chama uma equipe de
   ajudantes. Cada ajudante (thread) recebe um crachá com um número (0, 1, 2...)
   e TODOS executam o mesmo bloco { } ao mesmo tempo.
   No fim do bloco, a equipe é dispensada e só o professor continua.

 Compilar e rodar (PowerShell, na pasta do arquivo):
   g++ -fopenmp 000_hello.cpp -o 000_hello.exe
   .\000_hello.exe

 IMPORTANTE: sem a flag -fopenmp o compilador IGNORA o #pragma
 e o programa roda com uma única thread.
----------------------------------------------------------*/

#include <iostream>   // std::cout
#include <omp.h>      // API do OpenMP (omp_get_thread_num, etc.)

int main() {
    // Até aqui existe UMA thread só (a thread principal, de ID 0).

    // A diretiva abaixo cria uma REGIÃO PARALELA.
    // O runtime do OpenMP cria um time (grupo) de threads e
    // CADA thread executa o bloco entre { } uma vez.
    #pragma omp parallel // cria um time de threads
    /*
     Quantas threads? Por padrão, o número de threads é igual ao número de
     núcleos LÓGICOS da CPU.
       Exemplo: CPU com 8 núcleos lógicos -> 8 threads -> 8 mensagens.

     Para alterar, use a variável de ambiente OMP_NUM_THREADS (próximos scripts):
       $env:OMP_NUM_THREADS = "4"   -> 4 threads -> 4 mensagens.
    */
    {
        // Cada thread recebe um ID único de 0 a (T-1), onde T = número de threads.
        // Exemplo com T = 4: os IDs serão 0, 1, 2 e 3.
        // 'thread_id' é declarada DENTRO do bloco, então cada thread tem a sua cópia.
        int thread_id = omp_get_thread_num();

        // Atenção: todas as threads escrevem no MESMO console (std::cout).
        // Como elas escrevem ao mesmo tempo, as linhas podem sair "misturadas",
        // por exemplo:
        //     Hello, world from thread Hello, world from thread 31
        //
        // Isso se resolve com #pragma omp critical (scripts 001 e 002).
        std::cout << "Hello, world from thread " << thread_id << std::endl;
    }
    // Fim da região paralela: o OpenMP espera todas as threads terminarem
    // (barreira implícita) e daqui em diante volta a existir só a thread principal.

    return 0;
}

/* --------------------------------------------------------------------------------------------------
 Analisando a saída
 ---------------------------------------------------------------------------------------------------*/
 /*
  Como eu não especifiquei o número de threads, o OpenMP criou tantas threads quanto
  núcleos lógicos a minha CPU tem. Quantidade de núcleos lógicos = quantidade de threads.

  Ou seja, a linha "Hello world" foi impressa 20 vezes, pois minha CPU tem 20 núcleos
  lógicos (threads de hardware). Eu mandei paralelizar, e o OpenMP criou 20 threads
  para executar o bloco entre { }.

  Exemplo de saída possível (com 4 threads):
      Hello, world from thread 2
      Hello, world from thread 0
      Hello, world from thread 3
      Hello, world from thread 1

  Rodando de novo, a ordem pode mudar:
      Hello, world from thread 1
      Hello, world from thread 3
      ...

  O que parece uma bagunça na saída são as threads concorrendo para escrever no stdout.
  A ordem da impressão NÃO é determinística: depende do escalonamento do sistema
  operacional (quem ganha o processador primeiro). Isso é normal em programas paralelos.

  -------------------------------------------------
  Núcleos físicos x núcleos lógicos
  -------------------------------------------------
  - Núcleo físico: um "processador" de verdade dentro do chip.
  - Núcleo lógico: o que o sistema operacional enxerga. Com Hyper-Threading,
    cada núcleo físico pode aparecer como 2 núcleos lógicos.

  No meu caso: 14 núcleos físicos e 20 núcleos lógicos -> o OpenMP criou 20 threads.

  Para descobrir na sua máquina:
  - Núcleos lógicos (Windows, PowerShell):
        (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors
  - Núcleos físicos (Windows, PowerShell):
        (Get-CimInstance Win32_Processor).NumberOfCores
  - Gerenciador de Tarefas: Ctrl+Shift+Esc -> aba Desempenho -> CPU.
    Embaixo aparece "Processadores lógicos" (é o número que o OpenMP usa por padrão).
  - Linux:
        nproc

  Note que cada thread imprime seu ID único, de 0 a 19 (20 threads no total).

  FIM.
  ---------------------------------------------------------------------------------------------------
 */
