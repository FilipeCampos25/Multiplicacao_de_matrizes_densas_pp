/*-------------------------------------------------------------------------------------------------------------------------
 * File Name : 007_reduction_0.0.cpp
 * Author    : Prof. Rodrigo Gonçalves Pinto
 * Institution: University of São Paulo (USP) / University of Brasília (UnB) /IESB /SENAC
 * Course    : Parallel and Distributed Programming
 * Objective : Demonstration of OpenMP reduction basics
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





/*---------------------------------
 007 - 0.0 - reduction
---------------------------------*/
// Vamos supor que precisamos calcular a soma de milhões de números.
// Uma forma paralela de fazer isso seria dividir o trabalho:
// cada thread soma uma parte dos números em uma variável LOCAL (só dela)
// e, no final, juntamos as somas de cada thread na variável global.
// É exatamente isso que a cláusula reduction faz por nós.
//
// Analogia: contar os votos de uma eleição.
//   - Ruim (critical/atomic): todos os mesários fazem fila para
//     anotar CADA voto no mesmo quadro central.
//   - Bom (reduction): cada mesário conta os votos da SUA urna
//     num papel próprio; no final, só os totais de cada urna
//     são somados no quadro central. Fila quase nenhuma.
//
// Exemplo numérico: somar os números de 1 a 8 com 2 threads.
//   Thread 0 soma 1+2+3+4 = 10   (na sua cópia privada)
//   Thread 1 soma 5+6+7+8 = 26   (na sua cópia privada)
//   No final, o OpenMP combina: 10 + 26 = 36.


// Já vimos que proteger a soma com critical (ou atomic) funciona.
// No entanto, isso pode se tornar um GARGALO de desempenho:
// a cada iteração, as threads disputam a mesma variável e ficam
// esperando na fila para poder atualizar o total.


/*--------------------------------------------------------------
  A cláusula reduction automatiza e otimiza exatamente esse processo.
----------------------------------------------------------------*/
// Ela instrui o OpenMP a:
// 1. Criar uma cópia PRIVADA da variável de redução para cada thread,
//    ou seja, uma variável local para cada thread.
//
// 2. Inicializar essa cópia com o valor NEUTRO da operação:
//      soma (+)          -> 0   (somar 0 não muda nada)
//      multiplicação (*) -> 1   (multiplicar por 1 não muda nada)
//      E lógico (&&)     -> true
//      OU lógico (||)    -> false
//    Atenção: a cópia privada NÃO começa com o valor da variável global;
//    começa com o neutro. O valor original da global entra só na combinação final.
//    Exemplo: se soma_total começasse com 100, o resultado final seria 100 + (somas das threads).
//
// 3. No final da região paralela, combinar os valores das cópias privadas
//    com a variável original, de forma segura e eficiente.

/*--------------------------------------------------
 A reduction funciona com operações ASSOCIATIVAS
 (a ordem de agrupamento não muda o resultado), como:
 - soma:            reduction(+:var)
 - multiplicação:   reduction(*:var)
 - máximo/mínimo:   reduction(max:var), reduction(min:var)
 - operações lógicas:     reduction(&&:var), reduction(||:var)
 - operações bit a bit:   reduction(&:var), reduction(|:var), reduction(^:var)

 Para outros tipos (por exemplo, concatenar strings ou juntar vetores)
 é preciso criar uma redução própria com "#pragma omp declare reduction"
 — assunto mais avançado.

 É a forma preferida, e normalmente a de melhor desempenho, para agregar
 resultados em programas paralelos.
--------------------------------------------------*/

/*
  Vamos utilizar Bhaskara como exemplo, o mesmo que já vimos
  com critical (006 - 0.1) e atomic (006 - 0.2).

  Equação usada: x² - 7x + 10 = 0
      Δ = 49 - 40 = 9
      x1 = (7 + 3) / 2 = 5
      x2 = (7 - 3) / 2 = 2     -> soma das raízes = 7

  Compilar e rodar (PowerShell):
    g++ -fopenmp 007_reduction_0.0.cpp -o 007_reduction_0.0.exe
    .\007_reduction_0.0.exe
*/
#include <iostream>
#include <vector>
#include <cmath>
#include <omp.h>

// Função para retornar a SOMA das raízes de Bhaskara
double soma_raizes_bhaskara(double a, double b, double c) {
    double delta = (b * b) - (4 * a * c);
    if (delta < 0) return 0.0;
    double x1 = (-b + std::sqrt(delta)) / (2 * a);
    double x2 = (-b - std::sqrt(delta)) / (2 * a);
    return x1 + x2;
}

int main() {
    const int N = 10000;
    std::vector<double> a(N, 1.0), b(N, -7.0), c(N, 10.0); // Raízes 5 e 2, soma 7

    double soma_total = 0.0;

    // A mágica acontece aqui: reduction(+:soma_total)
    // 1. O OpenMP cria uma soma_total privada para cada thread, inicializada com 0.
    // 2. Cada thread acumula suas somas nesta variável privada. Não há disputa!
    // 3. No final do loop, o OpenMP soma os resultados de todas as cópias
    //    privadas na soma_total original, de forma segura.
    //
    // Repare que dentro do loop NÃO há critical nem atomic.
    // O código fica mais limpo, mais legível e geralmente mais rápido que as
    // versões com critical ou atomic para este tipo de problema.
    #pragma omp parallel for reduction(+:soma_total)
    for (int i = 0; i < N; ++i) {
        // O resultado da função é somado diretamente à soma_total.
        // Internamente, a soma é feita na cópia privada da thread.
        soma_total += soma_raizes_bhaskara(a[i], b[i], c[i]);
    }

    std::cout << "Soma total das raízes com reduction: " << soma_total << std::endl;
    std::cout << "Resultado esperado: " << N * 7.0 << std::endl;

    return 0;
}

/*
 Saída esperada:
   Soma total das raízes com reduction: 70000
   Resultado esperado: 70000

 Comparação das três formas de somar em paralelo:
   critical  -> fila a cada iteração (mais lento)
   atomic    -> fila a cada iteração, mas uma fila mais rápida
   reduction -> sem fila durante o loop; só uma combinação no final (normalmente o mais rápido)
*/
