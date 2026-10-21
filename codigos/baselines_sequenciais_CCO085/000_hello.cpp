
// Objetivo: abrir uma região paralela e imprimir uma mensagem por thread.

#include <iostream>   // std::cout
#include <omp.h>      // API do OpenMP

int main() {
    // A diretiva abaixo cria uma REGIÃO PARALELA.
    // O runtime do OpenMP cria um time (grupo) de threads.
    // Cada thread executa o bloco entre { }.
    #pragma omp parallel // cria um time de threads
    /*
     por padrão, o número de threads é igual ao número de núcleos lógicos da CPU.

     Para alterar o número de threads, usar a variável de ambiente OMP_NUM_THREADS, veremos no próximo script.
    */
    {
        // Cada thread recebe um ID único de 0 a (T-1), onde T = número de threads.
        int thread_id = omp_get_thread_num();

        // Atenção: várias threads escrevendo no mesmo stdout podem "misturar" saídas.
        // Para garantir a impressão linha-a-linha sem entrelaçamento, usar criticals. Próximo script.
        std::cout << "Hello, world from thread " << thread_id << std::endl;
    }
    return 0;

}

/* -------------------------------------------------------------------------------------------------- 
 Analisando a saida 
 ---------------------------------------------------------------------------------------------------*/
 /*
  Como no início das threads eu não especifiquei o número de threads, o OpenMP criou tantas threads quanto núcleos lógicos 
  minha CPU tem. Quantidade de núcleos lógicos = quantidade de threads.

  Ou seja, a linha hello world foi impressa 20 vezes, pois minha CPU tem 20 núcleos lógicos (htreads HW).
  Eu mandei paralelizar, e o OpenMP criou 20 threads para executar o bloco entre { }.

  O que parece uma bagunça na saída são as threads concorrendo para escrever no stdout. A ordem da impressão,
  não é determnística, depende do escalonamento do SO.

  OBSERVAÇÕES.

  Para saber quantos cores lógicos (htreads HW) sua CPU tem, use o comando: 20 no meu caso.
  - No Windows: (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors
  
 Para saber quantos cores físicos sua CPU tem, use o comando: 14 no meu caso.
  - No Windows: (Get-CimInstance Win32_Processor).NumberOfCores

 Para Windows (Gerenciador de Tarefas)
Ctrl+Shift+Esc → aba Desempenho → CPU
Embaixo aparece “Processadores lógicos” (é o número que o OpenMP enxerga como máximo útil).

No linux:
nproc

  No meu caso, minha CPU tem 14 núcleos físicos e 20 núcleos lógicos (com hyperthreading).
  Logo, o OpenMP criou 20 threads.

  A ordem de impressão das mensagens pode variar a cada execução, pois depende do escalonamento do SO.
  Em cada execução, as threads podem ser executadas em ordens diferentes.
  Note que cada thread imprime seu ID único, de 0 a 19 (20 threads no total).

  Se você executar várias vezes, verá que a ordem das mensagens muda a cada execução.
  Isso é normal em programas paralelos, pois o escalonamento das threads é dinâmico e depende do SO.

  FIM.
  ---------------------------------------------------------------------------------------------------
 */