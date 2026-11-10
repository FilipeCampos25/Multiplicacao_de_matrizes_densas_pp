/*--------------------------------------------
 006 - 0.3 - Exemplos de sincronização de threads.

 Compilar e rodar (PowerShell):
   g++ -fopenmp 006_sincronizacao_0.3.cpp -o 006_sincronizacao_0.3.exe
   $env:OMP_NUM_THREADS = "4"
   .\006_sincronizacao_0.3.exe
---------------------------------*/

/*--------------------------------------------
 3. Pragma omp barrier
-------------------------------------------*/


/*--------------------------------------------
 Contexto para a fórmula de Bhaskara (o mesmo do 0.1)
-------------------------------------------*/
/*
--> A Fórmula de Bhaskara
--------------------------------
Resolve equações de segundo grau do tipo ax² + bx + c = 0:

1.  Calcular o discriminante (delta): Δ = b² - 4ac
2.  Calcular as raízes:
       x1 = (-b + √Δ) / 2a
       x2 = (-b - √Δ) / 2a

Exemplo: x² - 5x + 6 = 0  ->  Δ = 1,  x1 = 3,  x2 = 2.
Neste arquivo usamos só a primeira raiz (x1 = 3).

------------------------------------

3. Pragma omp barrier
--------------------*/
/*
A diretiva #pragma omp barrier cria um PONTO DE ENCONTRO.
Nenhuma thread passa da barreira até que TODAS as threads do time tenham chegado a ela.

Analogia: excursão escolar. Ninguém entra no ônibus para a próxima parada
até todos os alunos voltarem da parada atual.

É útil quando o trabalho é dividido em FASES e a fase seguinte depende
da conclusão da anterior por todas as threads.

No nosso exemplo, um processo de duas fases:
1.  Fase 1: as threads calculam as raízes e guardam em um vetor compartilhado.
2.  Fase 2: depois que TODAS as raízes foram calculadas, as threads verificam os resultados.

Por que a fase 2 precisa esperar?
  Na fase 2, a thread 0 pode verificar o resultado de i = 5, que foi calculado
  por OUTRA thread na fase 1. Se a thread 0 não esperar, pode ler raizes_x1[5]
  antes de ele ser calculado (lendo 0 em vez de 3).
*/

#include <iostream>
#include <vector>
#include <cmath>
#include <cstdio>   // printf
#include <omp.h>

// Função para calcular apenas a primeira raiz de Bhaskara
double primeira_raiz_bhaskara(double a, double b, double c) {
    double delta = (b * b) - (4 * a * c);
    if (delta < 0) return 0.0; // Retorna 0 se não houver raiz real
    return (-b + std::sqrt(delta)) / (2 * a);
}

int main() {
    const int N = 8; // Número pequeno para facilitar a visualização
    // Atalho: vector(N, valor) cria N posições já preenchidas com 'valor'.
    std::vector<double> a(N, 1.0), b(N, -5.0), c(N, 6.0);
    std::vector<double> raizes_x1(N); // Vetor COMPARTILHADO com os resultados

    #pragma omp parallel
    {
        int id = omp_get_thread_num();

        // --- FASE 1: CÁLCULO DAS RAÍZES ---
        // A diretiva 'for' distribui as iterações entre as threads JÁ EXISTENTES
        // (as threads foram criadas pelo 'parallel' acima).
        // Exemplo com 4 threads: thread 0 -> i = 0,1 | thread 1 -> i = 2,3 | ...
        #pragma omp for
        for (int i = 0; i < N; ++i) {
            raizes_x1[i] = primeira_raiz_bhaskara(a[i], b[i], c[i]);
            // printf costuma imprimir a linha inteira de uma vez,
            // por isso aqui as linhas raramente se misturam.
            printf("Thread %d calculou a raiz para i = %d.\n", id, i);
        }

        // --- PONTO DE SINCRONIZAÇÃO (BARREIRA) ---
        // Nenhuma thread continua para a Fase 2 até que TODAS tenham terminado a Fase 1.
        // Isso garante que o vetor 'raizes_x1' esteja completamente preenchido.
        //
        // OBSERVAÇÃO IMPORTANTE:
        // O "omp for" acima JÁ TEM uma barreira implícita no final.
        // Então, neste código, a barreira explícita é redundante: está aqui para
        // deixar a ideia VISÍVEL.
        // Ela passa a ser obrigatória se o for da Fase 1 tiver a cláusula 'nowait'
        // (que remove a barreira implícita):
        //     #pragma omp for nowait
        //     ... Fase 1 ...
        //     #pragma omp barrier      <- aqui ela seria indispensável
        #pragma omp barrier

        // --- FASE 2: VERIFICAÇÃO DOS RESULTADOS ---
        // Agora que todos os cálculos terminaram, podemos usar os resultados.
        // Repare: a thread que verifica o i = 5 pode NÃO ser a mesma que o calculou.
        #pragma omp for
        for (int i = 0; i < N; ++i) {
            printf("Thread %d verificando o resultado de i = %d, que é %.2f.\n", id, i, raizes_x1[i]);
        }
    }

    return 0;
}

/*
 Saída esperada (4 threads; a ordem dentro de cada fase varia):
   Thread 0 calculou a raiz para i = 0.
   Thread 2 calculou a raiz para i = 4.
   ...                                   <- todas as linhas "calculou" aparecem primeiro
   Thread 1 verificando o resultado de i = 2, que é 3.00.
   Thread 3 verificando o resultado de i = 6, que é 3.00.
   ...                                   <- depois todas as linhas "verificando"

 O ponto-chave: NUNCA aparece um "verificando" antes de todos os "calculou",
 e todos os valores verificados são 3.00.
*/
