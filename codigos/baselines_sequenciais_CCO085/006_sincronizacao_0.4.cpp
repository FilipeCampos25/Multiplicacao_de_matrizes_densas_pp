/*--------------------------------------------
 006 - 0.4 - Exemplos de sincronização de threads.

 Compilar e rodar (PowerShell):
   g++ -fopenmp 006_sincronizacao_0.4.cpp -o 006_sincronizacao_0.4.exe
   .\006_sincronizacao_0.4.exe
---------------------------------*/

/*--------------------------------------------
 4. Pragma omp ordered
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

Exemplo: x² - 5x + 6 = 0  ->  x1 = 3, x2 = 2, soma das raízes = 5.

------------------------------------

4. Pragma omp ordered

A diretiva #pragma omp ordered faz um trecho de dentro do loop paralelo
ser executado na MESMA ORDEM de uma execução sequencial (i = 0, 1, 2, ...).

Analogia: uma cozinha com vários cozinheiros.
  - Os pratos são PREPARADOS ao mesmo tempo, em qualquer ordem (parte paralela).
  - Mas o garçom SERVE as mesas na ordem dos pedidos: mesa 0, mesa 1, mesa 2...
    Se o prato da mesa 3 ficar pronto antes do da mesa 2, ele espera no balcão.

Útil para: imprimir resultados, escrever em arquivo, gerar logs — tudo em que a ordem importa.

Regra de uso (são DOIS lugares):
  1) a cláusula 'ordered' no "parallel for";
  2) a diretiva '#pragma omp ordered' no bloco que deve sair em ordem.

Neste exemplo vamos calcular as raízes e imprimir o resultado de cada equação
na ordem correta, de i = 0 até N-1.

Compare com o 000_hello.cpp, em que a ordem era aleatória.
*/

#include <iostream>
#include <vector>
#include <cmath>
#include <omp.h>

// Função resolver_bhaskara (a mesma dos exemplos anteriores)
double resolver_bhaskara(double a, double b, double c) {
    double delta = (b * b) - (4 * a * c);
    if (delta < 0) return 0.0;
    double x1 = (-b + std::sqrt(delta)) / (2 * a);
    double x2 = (-b - std::sqrt(delta)) / (2 * a);
    return x1 + x2;
}

int main() {
    const int N = 10;
    std::vector<double> a(N, 1.0), b(N, -5.0), c(N, 6.0);

    // A cláusula 'ordered' no loop 'for' é necessária para usar
    // a diretiva '#pragma omp ordered' dentro do loop.
    #pragma omp parallel for ordered
    for (int i = 0; i < N; ++i) {
        // Parte PARALELA: as threads calculam em qualquer ordem.
        double soma_local = resolver_bhaskara(a[i], b[i], c[i]);

        // --- BLOCO ORDENADO ---
        // Mesmo que o cálculo do i = 7 termine antes do i = 3,
        // o bloco abaixo só roda para o i = 7 depois que o i = 3 (e todos os anteriores) já rodaram.
        #pragma omp ordered
        {
            std::cout << "Resultado para a equação " << i << ": Soma das raízes = " << soma_local << std::endl;
        }
    }

    return 0;
}

/*
 Saída esperada (SEMPRE nesta ordem, com qualquer número de threads):
   Resultado para a equação 0: Soma das raízes = 5
   Resultado para a equação 1: Soma das raízes = 5
   ...
   Resultado para a equação 9: Soma das raízes = 5

 Cuidado com o custo:
   O bloco ordered anda em fila. Se ele for grande comparado ao cálculo,
   o programa fica quase sequencial. Use ordered só para o trecho
   que realmente precisa de ordem (normalmente a impressão/gravação).
*/
