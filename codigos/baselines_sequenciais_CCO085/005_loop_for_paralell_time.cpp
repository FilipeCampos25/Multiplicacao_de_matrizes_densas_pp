/*
  a = x*x + y*y + z*z  (paralelo com OpenMP)
  Ajustes pedidos:
    - Mostrar quantas threads foram utilizadas
    - Tempo total de execução
    - Tempo de execução de cada thread

  Compile (GCC/MinGW/WSL):
    g++ -O2 -fopenmp paralelo_timing.cpp -o paralelo_timing.exe

  Execute (exemplo com 8 threads):
    (PowerShell)  $env:OMP_NUM_THREADS = "8"; .\paralelo_timing.exe
*/

#include <iostream>
#include <vector>
#include <iomanip>
#include <omp.h>

int main() {
    // Aumente N para tempos mais visíveis
    const int N = 1'000'000;

    std::vector<double> x(N), y(N), z(N), a(N);

    // Inicialização sequencial (fora do timing principal)
    for (int i = 0; i < N; ++i) {
        x[i] = i * 0.5;
        y[i] = i * 0.5 + 1.0;
        z[i] = i * 0.5 + 2.0;
    }

    // Vetor para armazenar o tempo gasto por thread
    // Tamanho = máximo de threads possível neste processo
    std::vector<double> thread_time(omp_get_max_threads(), 0.0);

    int threads_usadas = 0;

    // Tempo total da região paralela (somente o cálculo)
    double T0 = omp_get_wtime();

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();

        // Descobrir o número de threads efetivamente utilizado
        #pragma omp single
        {
            threads_usadas = omp_get_num_threads();
        }

        // Medir o tempo somente do trabalho desta thread
        double t0 = omp_get_wtime();

        // Divisão do trabalho por índices (cada thread pega um subconjunto de i)
        #pragma omp for schedule(static)
        for (int i = 0; i < N; ++i) {
            a[i] = x[i]*x[i] + y[i]*y[i] + z[i]*z[i];
        }

        double t1 = omp_get_wtime();
        thread_time[tid] += (t1 - t0);
    }

    double T1 = omp_get_wtime();
    double tempo_total = T1 - T0;

    // Checagem simples de corretude
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "a[0]     = " << a[0] << "\n";
    std::cout << "a[N-1]   = " << a[N-1] << "\n\n";

    // Relatorio solicitado
    std::cout << std::setprecision(6);
    std::cout << "Threads utilizadas: " << threads_usadas << "\n";
    std::cout << "Tempo TOTAL (s):    " << tempo_total << "\n";

    std::cout << "\nTempo por thread (s):\n";
    for (int t = 0; t < threads_usadas; ++t) {
        std::cout << "  Thread " << t << ": " << thread_time[t] << "\n";
    }

    return 0;
}
