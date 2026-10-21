/*
    Hello OpenMP - Informações do ambiente OpenMP
    --------------------------------------------------------
    Objetivo: mostrar como descobrir o número de processadores
                 lógicos e o número máximo de threads que o OpenMP
                 usaria por padrão.
    --------------------------------------------------------
    Compilação (Linux):
      g++ -fopenmp -o 001_info 001_info.cpp
    Execução:
      ./001_info
*/


#include <iostream>
#include <omp.h>

int main() {
    // Número de processadores lógicos disponíveis para o OpenMP
    int num_procs = omp_get_num_procs();

    // Número máximo de threads que o OpenMP usaria por padrão
    int max_threads = omp_get_max_threads();

    std::cout << " Processadores logicos disponíveis: " << num_procs << std::endl;
    std::cout << " Maximo de threads padrao (omp_get_max_threads): " << max_threads << std::endl;

    // Só para testar: cada thread imprime seu ID
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int total = omp_get_num_threads();
        #pragma omp critical
        std::cout << "Sou a thread " << tid << " de " << total << std::endl;
    }

    return 0;
}
