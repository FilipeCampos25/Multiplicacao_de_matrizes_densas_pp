/*---------------------------------------
Paralelizar o for básico com OpenMP
-----------------------------------------*/
/*
 Compilar (GCC/MinGW):
   g++ -O2 -fopenmp basic_parallel.cpp -o basic_parallel.exe (windows)
   g++ -O2 -fopenmp codigos/01_openmp/loop_for_paralell.cpp -o codigos/01_openmp/004_loop_for_paralell

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

/* ============================================================================
 * schedule — COMO O OPENMP DIVIDE AS ITERAÇÕES DO LAÇO ENTRE AS THREADS
 * ============================================================================
 * O "parallel for" cria as threads e reparte as iterações do for entre elas.
 * A cláusula schedule define COMO essa repartição é feita.
 *
 * Exemplos abaixo: laço com N = 16 iterações (i = 0..15) e 4 threads.
 *
 * ----------------------------------------------------------------------------
 * schedule(static)  → blocos contíguos e iguais, definidos ANTES de começar
 * ----------------------------------------------------------------------------
 *   T0: 0  1  2  3
 *   T1: 4  5  6  7
 *   T2: 8  9  10 11
 *   T3: 12 13 14 15
 *   + overhead praticamente zero (a divisão é calculada uma única vez)
 *   - se as últimas iterações forem mais caras, a T3 trabalha sozinha no fim
 *   Use quando: todas as iterações custam o mesmo (ex.: soma de vetores).
 *
 * ----------------------------------------------------------------------------
 * schedule(static, k)  → blocos de k iterações, passo de distribuição de  de 2 em 2 e assim por diante.
 * ----------------------------------------------------------------------------
 *   static, 2:            static, 1 (cíclico):
 *   T0: 0 1   8 9         T0: 0 4 8  12
 *   T1: 2 3   10 11       T1: 1 5 9  13
 *   T2: 4 5   12 13       T2: 2 6 10 14
 *   T3: 6 7   14 15       T3: 3 7 11 15
 *   Use quando: o custo cresce ao longo do laço (ex.: laço triangular),
 *   pois cada thread recebe iterações "baratas" e "caras" misturadas.
 *
 /* ----------------------------------------------------------------------------
 * schedule(dynamic, k)  → FILA DE TRABALHO: quem termina, pega o próximo
 * ----------------------------------------------------------------------------
 * As ITERAÇÕES ficam numa fila. As THREADS vão até a fila buscar trabalho:
 * cada uma pega k iterações (padrão k = 1); quando termina, volta e pega
 * o próximo bloco disponível, até a fila esvaziar.
 *
 * Exemplo: dynamic, 1 com 4 threads
 *
 *   Fila de iterações:  [0][1][2][3][4][5][6][7] ...
 *
 *   Início:  T0 pega 0 | T1 pega 1 | T2 pega 2 | T3 pega 3
 *   T2 terminou primeiro  → volta à fila e pega a 4
 *   T0 terminou           → pega a 5
 *   T3 terminou           → pega a 6
 *   T2 terminou de novo   → pega a 7
 *   ... e assim por diante, até acabarem as iterações.
 *
 * Quem pega qual iteração depende de quem termina primeiro, então a divisão
 * MUDA a cada execução. O resultado do cálculo é o mesmo; só muda QUEM fez
 * cada parte.
 *
 * + balanceia bem quando as iterações têm custos diferentes (ex.: Mandelbrot)
 * - cada ida à fila exige sincronização (overhead); com iterações muito
 *   rápidas, dynamic, 1 pode ficar MAIS LENTO que static.
 *   Aumentar k reduz as idas à fila, mas piora um pouco o balanceamento.
 * -------------------------------------------------------------------------
  *

* ----------------------------------------------------------------------------
 * schedule(guided, k)  → FILA DE TRABALHO com blocos que DIMINUEM
 * ----------------------------------------------------------------------------
 * Funciona como o dynamic (quem termina, pega o próximo bloco da fila),
 * mas o tamanho do bloco vai diminuindo ao longo da execução:
 *
 *     tamanho do bloco ≈ iterações restantes ÷ número de threads
 *
 * (nunca menor que k; padrão k = 1)
 *
 * Exemplo: guided com 16 iterações e 4 threads
 *
 *   restam 16 → bloco = 16 ÷ 4 = 4    pega 0 1 2 3
 *   restam 12 → bloco = 12 ÷ 4 = 3    pega 4 5 6
 *   restam  9 → bloco =  9 ÷ 4 ≈ 3    pega 7 8 9
 *   restam  6 → bloco =  6 ÷ 4 ≈ 2    pega 10 11
 *   restam  4 → bloco =  4 ÷ 4 = 1    pega 12
 *   restam  3 → bloco = 1             pega 13
 *   restam  2 → bloco = 1             pega 14
 *   restam  1 → bloco = 1             pega 15
 *
 *   Blocos:  [ 4 ][ 3 ][ 3 ][2][1][1][1][1]
 *            grandes no início → pequenos no fim
 *
 * Quem pega cada bloco é a thread que ficar livre primeiro (como no dynamic),
 * então a divisão muda a cada execução.
 *
 * Por que é um meio-termo entre static e dynamic:
 *   + blocos grandes no início → poucas idas à fila (pouco overhead)
 *   + blocos pequenos no fim   → ninguém fica preso num bloco grande
 *                                enquanto as outras threads esperam
 *   Idas à fila neste exemplo: static = 0 | guided = 8 | dynamic,1 = 16
 *
 * Com guided, k: o bloco nunca fica menor que k (ex.: guided, 2 termina
 * em blocos de 2), o que evita muitas idas à fila no final.
 *
 * Cuidado: se as iterações MAIS CARAS estiverem no INÍCIO do laço, o guided
 * atrapalha — as primeiras threads pegam blocos grandes justamente das
 * iterações pesadas. Nesse caso, prefira dynamic com k pequeno.
 * ------------------------------------------------------------------------- *






 * ----------------------------------------------------------------------------
 * schedule(runtime)  → a política é lida da variável de ambiente OMP_SCHEDULE
 * ----------------------------------------------------------------------------
 *   Permite testar políticas SEM recompilar:
 *     OMP_SCHEDULE="static"     ./programa
 *     OMP_SCHEDULE="dynamic,4"  ./programa
 *     OMP_SCHEDULE="guided"     ./programa
 *
 * schedule(auto)  → o compilador/runtime escolhe (pouco usado na prática).
 * Sem a cláusula schedule, o g++ usa static.
 *
 * NESTE CÓDIGO: cada iteração faz exatamente a mesma conta, então o custo é
 * uniforme e static é a melhor escolha — balanceado e sem overhead.
 * ========================================================================== */




















    /*--------------------------------------------------
     Parallelizar o loop for com OpenMP
    ----------------------------------------------------*/
    // a diretiva omp parallel for paraleliza o loop for mais próximo.
    // essa diretiva diz ao compilador: crie um time de threads e divida as iterações do for entre elas.
    // cada thread recebe um pedaço do intervalo[0,M) para calcular.

    /*--------------------------------------------------
     A cláusula schedule(static) --> a divisão é feita antes
     --------------------------------------------------
     define como as iterações do loop são distribuídas entre as threads.
     - static: o compilador divide as iterações em blocos de tamanho igual e atribui fixamente a 
       cada thread.
       Exemplo: N = 10, 2 Threads
       -->  Thread 0: i = 0,1,2,3,4
            Thread 1: i = 5,6,7,8,9 
     -  isso é rápido, ótimo quando todas as iterações tem o mesmo custo.

     --------------------------------------------------
     O corpo a[i] = x[i]*x[i] + y[i]*y[i] + z[i]*z[i];
    - cada thread calcula o valor de a[i] para o subconjunto de índices que recebeu.
    - Não há risco de conflito porque cada a[i] é escrito apenas por uma thread.
     --------------------------------------------------*/

    
    
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < N; i++) {
        a[i] = x[i]*x[i] + y[i]*y[i] + z[i]*z[i];
        // (evite imprimir aqui para não embaralhar a saída)
    }

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
  A saída mostra que o cálculo foi feito corretamente.
  Cada a[i] corresponde a x[i]^2 + y[i]^2 + z[i]^2.

  A paralelização do loop for com OpenMP é simples e eficaz para acelerar cálculos independentes.
  Cada thread trabalha em seu próprio subconjunto de dados, evitando conflitos de escrita.

  Observação:
  - Evite imprimir dentro do loop paralelo, pois isso pode embaralhar a saída.
  - Mantenha a impressão fora da região paralela para garantir ordem e clareza.
  - o OpenMP gerencia a criação e sincronização das threads automaticamente.
  - o OpenMP escolhe o número de threads baseado no número de núcleos lógicos da CPU por padrão.
  - para controlar o número de threads, use a variável de ambiente OMP_NUM_THREADS.
  */