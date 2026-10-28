/*
  005 - a = x*x + y*y + z*z (paralelo com OpenMP) medindo TEMPO
  ------------------------------------------------------------
  O que este código acrescenta em relação ao 004:
    - Mostrar quantas threads foram utilizadas
    - Tempo total de execução
    - Tempo de execução de cada thread

  Analogia:
    Uma equipe de 4 pessoas carregando 1.000.000 de caixas.
    - Tempo TOTAL: do apito de início até a ÚLTIMA pessoa terminar.
    - Tempo POR PESSOA: quanto cada uma trabalhou.
    Se uma pessoa demora muito mais que as outras, o total fica preso nela
    (desbalanceamento de carga).

  Compile (GCC/MinGW/WSL):
    g++ -O2 -fopenmp 005_loop_for_paralell_time.cpp -o 005_loop_for_paralell_time.exe

  Execute (exemplo com 8 threads):
    (PowerShell)  $env:OMP_NUM_THREADS = "8"; .\005_loop_for_paralell_time.exe
*/

#include <iostream>
#include <vector>
#include <iomanip>
#include <omp.h>

int main() {
    // N grande para que os tempos fiquem visíveis.
    // (1'000'000 é o mesmo que 1000000; o apóstrofo só facilita a leitura — C++14.)
    const int N = 1'000'000;

    std::vector<double> x(N), y(N), z(N), a(N);

    // Inicialização sequencial (fora da medição de tempo)
    for (int i = 0; i < N; ++i) {
        x[i] = i * 0.5;
        y[i] = i * 0.5 + 1.0;
        z[i] = i * 0.5 + 2.0;
    }

    // Vetor para armazenar o tempo gasto por thread.
    // Uma posição por thread: thread_time[0] é da thread 0, thread_time[1] da thread 1...
    // Tamanho = máximo de threads possível neste processo.
    std::vector<double> thread_time(omp_get_max_threads(), 0.0);

    int threads_usadas = 0;

    // omp_get_wtime() devolve o "relógio de parede" em segundos.
    // Tempo = (relógio depois) - (relógio antes).
    double T0 = omp_get_wtime();

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();

        // single: apenas UMA thread (qualquer uma) executa este bloco.
        // Basta uma thread anotar quantas threads existem; se todas fizessem isso,
        // estariam escrevendo na mesma variável ao mesmo tempo sem necessidade.
        #pragma omp single
        {
            threads_usadas = omp_get_num_threads();
        }

        // Cada thread dispara o SEU cronômetro.
        double t0 = omp_get_wtime();

        // Divisão do trabalho por índices (cada thread pega um bloco de i).
        // Exemplo com 4 threads e N = 1.000.000:
        //   Thread 0: i = 0       ...   249.999
        //   Thread 1: i = 250.000 ...   499.999
        //   Thread 2: i = 500.000 ...   749.999
        //   Thread 3: i = 750.000 ... 999.999
        #pragma omp for schedule(static)
        for (int i = 0; i < N; ++i) {
            a[i] = x[i]*x[i] + y[i]*y[i] + z[i]*z[i];
        }
        // ATENÇÃO: o "omp for" tem uma BARREIRA IMPLÍCITA no final.
        // Uma thread que termina cedo fica ESPERANDO as outras aqui.
        // Por isso o tempo medido abaixo é "trabalho + espera", e os tempos
        // por thread tendem a sair parecidos.
        // Para medir só o trabalho, use: #pragma omp for schedule(static) nowait

        double t1 = omp_get_wtime();
        // Cada thread escreve na SUA posição (thread_time[tid]) -> sem conflito.
        thread_time[tid] += (t1 - t0);
    }

    double T1 = omp_get_wtime();
    double tempo_total = T1 - T0;

    // Checagem simples de corretude
    // a[0]   = 0² + 1² + 2² = 5
    // a[N-1] = conta com i = 999.999
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "a[0]     = " << a[0] << "\n";
    std::cout << "a[N-1]   = " << a[N-1] << "\n\n";

    // Relatório
    std::cout << std::setprecision(6);
    std::cout << "Threads utilizadas: " << threads_usadas << "\n";
    std::cout << "Tempo TOTAL (s):    " << tempo_total << "\n";

    std::cout << "\nTempo por thread (s):\n";
    for (int t = 0; t < threads_usadas; ++t) {
        std::cout << "  Thread " << t << ": " << thread_time[t] << "\n";
    }

    return 0;
}

/*
 Como ler o resultado:
  - Rode com 1, 2, 4 e 8 threads e anote o Tempo TOTAL de cada um.
  - Speedup = Tempo com 1 thread / Tempo com p threads.
    Exemplo de leitura: se 4 threads levam metade do tempo de 1 thread,
    o speedup é 2 (e não 4). Meça na SUA máquina antes de tirar conclusões.
  - Por que o speedup costuma ficar abaixo do número de threads? Esta conta é tão simples que o gargalo vira a
    MEMÓRIA (buscar x, y, z na RAM), e não o processador. Além disso,
    criar e sincronizar as threads tem um custo fixo.
  - Os tempos variam a cada execução: rode algumas vezes e use a média.
*/
