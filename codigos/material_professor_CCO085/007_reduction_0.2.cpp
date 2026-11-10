/*-------------------------------------------------------------------------------------------------------------------------
 * File Name : 007_reduction_0.2.cpp
 * Author    : Prof. Rodrigo Gonçalves Pinto
 * Institution: University of Brasília (UnB) / University of São Paulo (USP) /IESB /SENAC
 * Course    : Parallel and Distributed Programming
 * Objective :  Demonstration of OpenMP reduction with logical operators (&&, ||)
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


/*-------------------------------------------------
 007 - 0.2 - Reduction com operadores lógicos (&&, ||)
---------------------------------------------------
- A cláusula reduction não se limita a operações aritméticas.
  Ela também funciona com operadores lógicos, como && (E) e || (OU).
  Isso é útil quando cada thread verifica uma parte dos dados e
  precisamos combinar as respostas (true/false) de todas elas.

  Temos dois casos principais:

- reduction(|| : flag)  -> "ALGUÉM encontrou?"
  O resultado final é true se o flag for true em PELO MENOS UMA thread.
  Cada cópia começa com false; basta uma thread marcar true.

  Analogia: procurar uma chave perdida em casa, cada pessoa num cômodo.
  Se UMA pessoa gritar "achei!", a resposta final é "achamos".

- reduction(&& : flag)  -> "TODOS concordam?"
  O resultado final é true somente se o flag for true em TODAS as threads.
  Cada cópia começa com true; basta uma thread marcar false.

  Analogia: checklist de decolagem. O avião só sai se TODOS os itens
  estiverem OK. Um único "não" cancela a decolagem.

  Exemplo com 3 threads:
     Thread 0: false   Thread 1: true   Thread 2: false
     ||  -> false || true || false = true
     &&  -> false && true && false = false

   --------------------------------------------------
   Objetivo do código exemplo:

   Auditar o vetor de salários verificando três regras
   de negócio da empresa ao mesmo tempo:
   ------------------------------------------------
1. Regra 1: Existe algum salário abaixo do piso de R$ 1500,00?  (||)
2. Regra 2: Existe algum salário acima do teto de R$ 20000,00?  (||)
3. Regra 3: Todos os salários registrados são valores positivos? (&&)
    --------------------------------------------------
    Cada regra é representada por uma variável booleana.
    A cláusula reduction combina os resultados de todas as
    threads e entrega o resultado final de cada regra.

 Compilar e rodar (PowerShell):
   g++ -fopenmp 007_reduction_0.2.cpp -o 007_reduction_0.2.exe
   .\007_reduction_0.2.exe
*/

#include <iostream>
#include <vector>
#include <omp.h>

int main() {
    const int N = 100000;
    std::vector<double> salarios(N);

    // Preenche com dados "bons" (todos R$ 5000,00)
    #pragma omp parallel for
    for (int i = 0; i < N; ++i) {
        salarios[i] = 5000.0;
    }

    // Injeta alguns dados "ruins" para a auditoria encontrar
    salarios[1000]  = 1400.0;    // Viola a Regra 1 (abaixo do piso)
    salarios[20000] = -50.0;     // Viola a Regra 3 (salário negativo) — e também a Regra 1 (-50 < 1500)
    salarios[50000] = 25000.0;   // Viola a Regra 2 (acima do teto)

    /*---------------------------------------------
        Variáveis chamadas de "flag" (bandeira) para a auditoria.
        1. piso_violado
        2. teto_violado
        3. dados_validos
    ---------------------------------------------*/

    /*-------------------------------------------------------
     Os valores iniciais assumem que "está tudo certo" até prova em contrário:
     -------------------------------------------------
     1. piso_violado = false: ainda não encontramos nenhum salário abaixo do piso.
     2. teto_violado = false: ainda não encontramos nenhum salário acima do teto.
     3. dados_validos = true: ainda não encontramos nenhum dado inválido.

     Repare que esses valores iniciais são justamente os NEUTROS de cada operador:
       false para || e true para &&.
    */

    bool piso_violado = false;   // Regra 1: existe algum salário abaixo do piso de R$ 1500,00?
    bool teto_violado = false;   // Regra 2: existe algum salário acima do teto de R$ 20000,00?
    bool dados_validos = true;   // Regra 3: todos os salários registrados são positivos?

    // Constantes para o piso e o teto salarial.
    const double PISO_SALARIAL = 1500.0;
    const double TETO_SALARIAL = 20000.0;

   /*-------------------------------------------------
    reduction com 3 variáveis e operadores diferentes.
    ---------------------------------------------------
    - Variáveis com o MESMO operador podem ir juntas na mesma cláusula:
          reduction(||:piso_violado, teto_violado)
    - Para outro operador, usamos outra cláusula:
          reduction(&&:dados_validos)
    - A barra invertida (\) no fim da linha só serve para quebrar
      a diretiva em várias linhas e facilitar a leitura.
    */
    #pragma omp parallel for \
        reduction(||:piso_violado, teto_violado) \
        reduction(&&:dados_validos)
    for (int i = 0; i < N; ++i) {
        double s = salarios[i];

        // Regra 1: se encontrarmos UM salário abaixo, o piso foi violado.
        if (s < PISO_SALARIAL) {
            piso_violado = true;   // a thread "grita": encontrei!
        }

        // Regra 2: se encontrarmos UM salário acima, o teto foi violado.
        if (s > TETO_SALARIAL) {
            teto_violado = true;   // a thread "grita": encontrei!
        }

        // Regra 3: se encontrarmos UM salário inválido, nem todos os dados são válidos.
        if (s <= 0) {
            dados_validos = false; // a thread "vota": nem tudo está OK!
        }
    }

    /*-------------------------------------------------
     Apresentação dos resultados da auditoria
    --------------------------------------------------*/
    std::cout << "Relatório de auditoria salarial" << std::endl;
    if (piso_violado) {
        std::cout << "[ALERTA] Regra 1: Encontrado pelo menos um salário abaixo do piso de R$ " << PISO_SALARIAL << std::endl;
    } else {
        std::cout << "[OK] Regra 1: Nenhum salário abaixo do piso foi encontrado." << std::endl;
    }

    if (teto_violado) {
        std::cout << "[ALERTA] Regra 2: Encontrado pelo menos um salário acima do teto de R$ " << TETO_SALARIAL << std::endl;
    } else {
        std::cout << "[OK] Regra 2: Nenhum salário acima do teto foi encontrado." << std::endl;
    }

    if (!dados_validos) {
        std::cout << "[ALERTA] Regra 3: Encontrados dados inválidos (salários negativos ou zero)." << std::endl;
    } else {
        std::cout << "[OK] Regra 3: Todos os registros de salário são válidos (positivos)." << std::endl;
    }

    return 0;
}

/*
 Saída esperada (as três regras são violadas pelos dados injetados):
   Relatório de auditoria salarial
   [ALERTA] Regra 1: Encontrado pelo menos um salário abaixo do piso de R$ 1500
   [ALERTA] Regra 2: Encontrado pelo menos um salário acima do teto de R$ 20000
   [ALERTA] Regra 3: Encontrados dados inválidos (salários negativos ou zero).

 Experimento: comente as três linhas de "dados ruins" e rode de novo.
 As três regras devem aparecer como [OK].

 Por que não basta escrever "piso_violado = true" sem reduction?
   Como todas as threads só escrevem "true", o resultado até sairia certo na prática,
   mas várias threads escrevendo na mesma variável sem proteção é uma condição de corrida
   (comportamento indefinido em C++). A reduction deixa o código correto e explícito.
*/
