/*
    001 - Informações do ambiente OpenMP
    --------------------------------------------------------
    Objetivo: descobrir quantos processadores lógicos o OpenMP enxerga
              e quantas threads ele usaria por padrão.

    Analogia:
      Antes de dividir uma tarefa em equipe, você pergunta:
        "Quantas mesas tem na sala?"           -> omp_get_num_procs()
        "Quantas pessoas eu posso chamar?"     -> omp_get_max_threads()
        "Quem sou eu e quantos somos agora?"   -> omp_get_thread_num() / omp_get_num_threads()

    --------------------------------------------------------
    Compilar e rodar (Windows, PowerShell):
      g++ -fopenmp 001_info.cpp -o 001_info.exe
      .\001_info.exe

    Teste mudando o número de threads:
      $env:OMP_NUM_THREADS = "4"
      .\001_info.exe
      -> "Maximo de threads padrao" passa a ser 4, mesmo que a CPU tenha mais núcleos.

    Compilar e rodar (Linux):
      g++ -fopenmp -o 001_info 001_info.cpp
      ./001_info
*/


#include <iostream>
#include <omp.h>

int main() {
    // Número de processadores lógicos disponíveis para o programa.
    // Exemplo: CPU com 8 núcleos físicos e Hyper-Threading -> 16.
    int num_procs = omp_get_num_procs();

    // Número máximo de threads que a PRÓXIMA região paralela usaria.
    // Sem OMP_NUM_THREADS definida, costuma ser igual a num_procs.
    // Com $env:OMP_NUM_THREADS = "4", passa a ser 4.
    int max_threads = omp_get_max_threads();

    std::cout << " Processadores logicos disponiveis: " << num_procs << std::endl;
    std::cout << " Maximo de threads padrao (omp_get_max_threads): " << max_threads << std::endl;

    // Só para testar: cada thread imprime seu ID.
    #pragma omp parallel
    {
        // tid   -> "quem sou eu"  (0, 1, 2, ... T-1)
        // total -> "quantos somos" (T)
        // Atenção: omp_get_num_threads() FORA de uma região paralela retorna 1,
        // porque ali só existe a thread principal.
        int tid = omp_get_thread_num();
        int total = omp_get_num_threads();

        // critical: só UMA thread por vez executa a linha abaixo.
        // Assim cada frase sai inteira, sem misturar com a de outra thread.
        // (Sem chaves, o critical vale apenas para a instrução seguinte.)
        #pragma omp critical
        std::cout << "Sou a thread " << tid << " de " << total << std::endl;
    }

    return 0;
}

/*
 Exemplo de saída (CPU com 8 lógicos, sem OMP_NUM_THREADS):
    Processadores logicos disponiveis: 8
    Maximo de threads padrao (omp_get_max_threads): 8
   Sou a thread 3 de 8
   Sou a thread 0 de 8
   Sou a thread 6 de 8
   ...

 Repare:
  - As linhas agora saem INTEIRAS (graças ao critical),
    mas a ORDEM continua variando a cada execução.
  - critical organiza o acesso (um por vez), não a ordem (quem vem primeiro).
*/
