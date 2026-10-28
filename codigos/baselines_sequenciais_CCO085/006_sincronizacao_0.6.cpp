/*---------------------------------
 006 - 0.6 - Exercícios
---------------------------------*/

/*
 Utilize o código base abaixo para a resolução dos exercícios.
 Ao final de cada código, escreva um parágrafo com suas palavras:

 a) Qual é o objetivo do código, em detalhes.
 b) O que o seu código faz, etapa por etapa, em detalhes.
 c) Ao final de cada exercício, ouvirei uma explicação sua sobre o que está sendo feito.

 */

 /*
  Explicações, datas:

  exercício 1 e 2: 07/10/2025.
  exercício 3 e 4: 14/10/2025.
  exercício 5:     21/10/2025.
 */

/*
 O que o código base já faz:
  - Cria N = 1000 equações de segundo grau.
  - Índices PARES  (i = 0, 2, 4, ...): x² - 7x + 10 = 0
        Δ = 49 - 40 = 9  -> raízes 5 e 2 (TEM raízes reais)
  - Índices ÍMPARES (i = 1, 3, 5, ...): x² + 2x + 5 = 0
        Δ = 4 - 20 = -16 -> NÃO tem raízes reais
  - A função resolver_bhaskara devolve as DUAS raízes em um std::pair.

 Como usar o std::pair (dica):
     auto r = resolver_bhaskara(a[i], b[i], c[i]);
     double x1 = r.first;    // primeira raiz
     double x2 = r.second;   // segunda raiz

 Atenção: quando não há raízes reais a função devolve {0.0, 0.0}.
 Para CONTAR quantas equações têm raízes reais, não use "x1 == 0" como teste
 (uma equação como x² - 4x = 0 tem raiz real igual a 0).
 Calcule o delta, ou altere a função para também informar se Δ >= 0.

 Compilar e rodar (PowerShell):
   g++ -fopenmp 006_sincronizacao_0.6.cpp -o 006_sincronizacao_0.6.exe
   .\006_sincronizacao_0.6.exe
*/


#include <iostream>
#include <vector>
#include <cmath>
#include <utility>   // std::pair
#include <omp.h>

// Função para calcular as raízes de uma equação de segundo grau.
// Retorna um std::pair contendo as raízes x1 e x2.
// Se não houver raízes reais, ambas serão 0.
std::pair<double, double> resolver_bhaskara(double a, double b, double c) {
    double delta = (b * b) - (4 * a * c);

    if (delta < 0) {
        return {0.0, 0.0};
    }

    double x1 = (-b + std::sqrt(delta)) / (2 * a);
    double x2 = (-b - std::sqrt(delta)) / (2 * a);

    return {x1, x2};
}

int main() {
    // Número de equações a serem resolvidas
    const int N = 1000;

    // Vetores para armazenar os coeficientes
    std::vector<double> a(N), b(N), c(N);

    // Preenche os vetores com valores de exemplo.
    // Metade terá raízes reais, a outra metade não.
    for (int i = 0; i < N; ++i) {
        a[i] = 1.0;
        if (i % 2 == 0) {
            // Equação com raízes reais: x^2 - 7x + 10 = 0 (raízes 5 e 2)
            b[i] = -7.0;
            c[i] = 10.0;
        } else {
            // Equação sem raízes reais: x^2 + 2x + 5 = 0 (delta < 0)
            b[i] = 2.0;
            c[i] = 5.0;
        }
    }

    // ===============================================================
    //               ÁREA PARA RESOLVER OS EXERCÍCIOS
    // ===============================================================

    // Adicione aqui o seu código OpenMP para cada exercício.


    // ===============================================================

    return 0;
}

/*------------------------------------------------------
 Instruções dos exercícios:
------------------------------------------------------*/
/*
1. Faça um programa que utilize OpenMP para paralelizar a resolução das equações do segundo
   grau com a fórmula de Bhaskara.

2. Crie uma entrada de dados onde o usuário decidirá a quantidade de equações a serem resolvidas.
   Ao final, mostre o tempo gasto para resolver as equações (use omp_get_wtime, como no script 005),
   o número de equações que possuem raízes reais e quantas não possuem.
   Resultado esperado com N = 1000: 500 com raízes reais e 500 sem raízes reais.

3. Utilize todos os conceitos das aulas 006, na mesma ordem em que foram apresentados:
   critical (0.1) -> atomic (0.2) -> barrier (0.3) -> ordered (0.4) -> locks (0.5).
*/
