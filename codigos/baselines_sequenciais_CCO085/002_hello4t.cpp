
/*----------------------------------------------------------
   Hello OpenMP com saída organizada (uma linha por thread), usando apenas 4 treads
   --------------------------------------------------------
   Objetivo: mostrar como evitar "linhas coladas" usando 
   número limitado de treads no buffer local e região crítica.

   Para isso, usamos a cláusula num_threads(4) na região
   imprimem ao mesmo tempo, usando:
     1) um buffer local (std::ostringstream) para montar a linha inteira; e
     2) uma região crítica (#pragma omp critical) na hora de escrever no stdcout.

   Como compilar (GCC/MinGW-w64, VS Code terminal / PowerShell):
     g++ -fopenmp hello_organizado.cpp -o hello_organizado.exe

   Como executar com N threads (PowerShell):
     $env:OMP_NUM_THREADS = "4"
     .\hello_organizado.exe

   Observações:
   - A ordem das linhas pode variar entre execuções (escalonamento do SO).
   - #pragma omp parallel cria um *time* de threads; cada thread executa o bloco uma vez.
*/

#include <iostream>   // std::cout
#include <sstream>    // std::ostringstream (buffer local de string)
#include <omp.h>      // API do OpenMP

int main() {
    // Região paralela: o runtime do OpenMP cria um time de T threads.
    // Cada thread executa o bloco entre { } exatamente uma vez.
    #pragma omp parallel num_threads(4) // cria um time de 4 threads
    {
        //  Descobrir quem sou eu dentro do time:
        //    - tid: Id da thread (0...3)
        //    - tot: Total de threads no time (4).
        int tid = omp_get_thread_num();
        int tot = omp_get_num_threads();

      // cria uma stream de saída para string (buffer local)
      // ele mantém um buffer em memória, onde tudo que imprimimos com << será acumulado.
      // o oss é local de cada thread, não há concorrência, compartilhamento nem corrida nessa etapa.
      // monta a string.
        std::ostringstream oss;

        
        oss << "Hello, world from thread " << tid
            << " of " << tot << "\n";

        /*----------------------------------------------------------
         Região crítica: 
         ----------------------------------------------------------*/
        // Só uma thread por vez pode executar a região crítica.
        // Isso evita que várias threads escrevam ao mesmo tempo no stdcout,
        // o que causaria linhas "coladas" e ilegíveis.
        // A thread que entrar na região crítica escreve a linha inteira de uma vez.
        // As outras threads ficam esperando sua vez.
        // A ordem de entrada na região crítica não é garantida, depende do escalonamento do SO.
        // O std::cout é um recurso compartilhado. Ou seja, todas as threads escrevem no mesmo console.
        // Se duas ou  mais threads chamarem std::cout ao mesmo tempo, a saída podem se misturar.
        // Para evitar isso, usamos a região crítica.
        // Com a região crítica, cada thread escreve sua linha inteira de uma vez, sem ser interrompida no meio
           /*--------
            Resumindo
            Região crítica é uma trava de exclusão mútua para proteger a escrita no std::cout e manter as mensagens legíveis.
            organizadas, uma thread após a outra
            ----------*/ 


        #pragma omp critical
        {
            std::cout << oss.str();
        }

        // Nota: A região critical é pequena (apenas a escrita),
        // o trabalho "pesado" — se existisse — ficaria fora dela.
    }

    return 0;
}








/*--------------------------------------------------------------
Código sem comentários
---------------------------------------------------------------*/

/* <---------------------- para visualizar, retira o comentário

#include <iostream>
#include <sstream>
#include <omp.h>

int main() {
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int tot = omp_get_num_threads();

       
        std::ostringstream oss;
        oss << "Hello, world from thread " << tid << " of " << tot << "\n";

       
        #pragma omp critical
        std::cout << oss.str();
    }
}
*/