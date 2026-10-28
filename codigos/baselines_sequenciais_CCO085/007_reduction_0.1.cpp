/*-------------------------------------------------------------------------------------------------------------------------
 * File Name : 007_reduction_0.1.cpp
 * Author    : Prof. Rodrigo Gonçalves Pinto
 * Institution: University of Brasília (UnB) / University of São Paulo (USP) /IESB /SENAC
 * Course    : Parallel and Distributed Programming
 * Objective :  Demonstration of OpenMP reduction to compute population mean and variance (sum and sum of squares)
 * Semester  : 2025/2
 * Version   : 2.0
 * 
 * History:
 *   Creation date : 2023-05-15
 *   Update date   : 2024-04-26
 *   Updated by    : Rodrigo Gonçalves Pinto
 *   Changes made  : Adapted for C++17; general code revision and documentation update.
 --------------------------------------------------------------------------------------------------------------------------*/

/*-------------------------------------
 * Apache License, Version 2.0
 *-------------------------------------
 * Copyright (c) 2023–2025 Rodrigo Gonçalves Pinto
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Modifications: Please maintain all change logs in the "History" section above.
 */

/*-------------------------------------
 * NOTICE
 *-------------------------------------
 * Parallel Programming Examples in C++ (OpenMP)
 * Copyright (c) 2023–2025 Rodrigo Gonçalves Pinto
 *
 * This product includes software developed by Rodrigo Gonçalves Pinto.
 *
 * Academic Attribution:
 * If you use this software in academic or scientific work, please cite:
 *   Pinto, R. G. (2025). Parallel Programming Examples in C++ using OpenMP.
 *   ORCID: https://orcid.org/0009-0008-8360-9538
 *   University of São Paulo (USP) or University of Brasília (UnB).
 *
 * Any distribution of this software or derivative works must reproduce
 * this NOTICE file.
 */





/*---------------------------------------------------
 007 - 0.1 - Mais exemplos de uso da cláusula reduction do OpenMP:
             média, variância e desvio-padrão populacionais.
-----------------------------------------------------*/

// Média populacional:
//            N
//      μ = ( Σ xᵢ ) / N
//           i=1
//
// Variância populacional:
//             N
//     σ² =  ( Σ (xᵢ - μ)² ) / N
//            i=1
//
// Desvio-padrão populacional:
//     σ = √σ²
//
// onde:
//     σ² = variância
//     xᵢ = cada valor individual do conjunto de dados
//     μ  = média populacional do conjunto de dados
//     N  = número total de valores no conjunto de dados

/*---------------------------------------------------
 Exemplo pequeno, à mão (salários de 4 pessoas):
     x = 2000, 4000, 4000, 6000      N = 4

 1) Média:     μ = (2000 + 4000 + 4000 + 6000) / 4 = 16000 / 4 = 4000
 2) Desvios:   (2000-4000) = -2000, 0, 0, (6000-4000) = 2000
    Ao quadrado: 4.000.000, 0, 0, 4.000.000   -> soma = 8.000.000
 3) Variância: σ² = 8.000.000 / 4 = 2.000.000
 4) Desvio-padrão: σ = √2.000.000 ≈ 1414,21

 Interpretação: em média, os salários se afastam cerca de R$ 1.414 da média.
-----------------------------------------------------*/

// Observe que para calcular isso precisamos de DUAS somas:
//   1) a soma de todos os valores (Σ xᵢ), para obter μ;
//   2) a soma dos desvios ao quadrado (Σ (xᵢ - μ)²), para obter σ².
//
// A segunda soma DEPENDE de μ, que só é conhecido depois da primeira.
// Por isso o código usa DOIS loops paralelos, cada um com sua reduction
// (método das "duas passadas"):
//   Passada 1: reduction(+:soma)           -> μ
//   Passada 2: reduction(+:soma_desvios2)  -> σ²
//
// Alternativa com UM loop só: acumular Σx e Σx² ao mesmo tempo e usar
//     σ² = (Σx²)/N - μ²
// Funciona, mas com números grandes pode perder precisão (subtração de dois
// valores enormes e parecidos). As duas passadas são mais seguras numericamente.

/*-----------------------------------------
 Cenário: temos os salários de uma empresa fictícia:
 1. 100 DEPARTAMENTOS x 500 FUNCIONÁRIOS = 50.000 salários.
 2. Queremos a média, a variância e o desvio-padrão de todos os salários da empresa.
------------------------------------------------*/


/*-------------------------------------------------
Atenção: cada cláusula reduction tem UM operador.
--------------------------------------------------
- Se precisar de mais de um operador, utilize várias
  cláusulas reduction na mesma diretiva.

  Exemplo:
    #pragma omp parallel for reduction(+:soma) reduction(*:produto)
    for (int i = 0; i < N; ++i) {
        soma += vec[i];
        produto *= vec[i];
    }

  Isso diz ao OpenMP:
  1. Gerenciar uma redução do tipo SOMA para a variável soma.
  2. Gerenciar uma redução do tipo PRODUTO para a variável produto.

- Com o MESMO operador, dá para listar várias variáveis na mesma cláusula:
    reduction(+:soma, soma_quadrados)

  O OpenMP cuida de cada variável de forma independente, combinando as
  cópias privadas no final de acordo com a operação de cada uma.

 Compilar e rodar (PowerShell):
   g++ -fopenmp 007_reduction_0.1.cpp -o 007_reduction_0.1.exe
   .\007_reduction_0.1.exe
*/


// Vamos ao exemplo.
#include <iostream>
#include <vector>
#include <cmath>
#include <omp.h>
#include <iomanip>

int main() {
    // Exemplo: salários de uma empresa fictícia
    const int DEPARTAMENTOS = 100;
    const int FUNCIONARIOS  = 500;
    const int N = DEPARTAMENTOS * FUNCIONARIOS;   // 50.000 salários

    std::vector<double> salarios(N);

    // Inicialização em paralelo apenas para produzir dados de exemplo.
    // Salário = 4000 + (i % 100) * 20  ->  valores entre 4000 e 5980, em degraus de 20.
    // Cada thread escreve em posições diferentes do vetor -> sem conflito.
    #pragma omp parallel for
    for (int i = 0; i < N; ++i) {
        salarios[i] = 4000.0 + (i % 100) * 20.0;
    }

    // ---------------------------
    // 1) MÉDIA POPULACIONAL (μ)  — primeira passada
    // ---------------------------
    double soma = 0.0;

    // Cada thread soma a sua parte dos salários numa cópia privada de 'soma';
    // no fim, o OpenMP junta as cópias.
    #pragma omp parallel for reduction(+:soma)
    for (int i = 0; i < N; ++i) {
        soma += salarios[i];
    }

    const double mu = soma / static_cast<double>(N);

    // ----------------------------------------------------
    // 2) VARIÂNCIA POPULACIONAL (σ²) — segunda passada
    //     σ² = (1/N) * Σ (xᵢ − μ)²
    //    Aqui μ já é conhecido e é apenas LIDO pelas threads.
    // ----------------------------------------------------
    double soma_desvios2 = 0.0;

    #pragma omp parallel for reduction(+:soma_desvios2)
    for (int i = 0; i < N; ++i) {
        const double d = salarios[i] - mu;   // desvio deste salário em relação à média
        soma_desvios2 += d * d;              // desvio ao quadrado
    }

    double variancia_pop = soma_desvios2 / static_cast<double>(N);

    // (opcional) Proteção contra um valor negativo minúsculo causado por arredondamento.
    // Nesta fórmula (soma de quadrados) isso não acontece; a proteção é útil
    // na alternativa σ² = (Σx²)/N - μ².
    if (variancia_pop < 0.0 && std::fabs(variancia_pop) < 1e-12) {
        variancia_pop = 0.0;
    }

    const double desvio_padrao_pop = std::sqrt(variancia_pop);

    // Saída
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Analise Salarial (Populacional)\n";
    std::cout << "N = " << N << "\n";
    std::cout << "Media (μ)                  : R$ " << mu << "\n";
    // Observação: a unidade da variância é "R$ ao quadrado"; a do desvio-padrão é R$.
    std::cout << "Variancia populacional (σ²): R$ " << variancia_pop << "\n";
    std::cout << "Desvio-padrao populacional : R$ " << desvio_padrao_pop << "\n";



    return 0;
}

/*
 Saída esperada:
   N = 50000
   Media (μ)                  : R$ 4990.00
   Variancia populacional (σ²): R$ 333300.00
   Desvio-padrao populacional : R$ 577.32

 Conferindo a média: os salários vão de 4000 a 5980 de forma uniforme,
 então a média é o ponto do meio: (4000 + 5980) / 2 = 4990.
*/
