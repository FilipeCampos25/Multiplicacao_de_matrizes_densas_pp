/*-------------------------------------------------------------------------------------------------------------------------
 * File Name : 008_sections_0.0.cpp
 * Author    : Prof. Rodrigo Gonçalves Pinto
 * Institution: University of Brasília (UnB) / University of São Paulo (USP) /IESB /SENAC
 * Course    : Parallel and Distributed Programming
 * Objective :  Demonstration of OpenMP sections for parallel execution of independent tasks
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
 008 - 0.0 - Paralelismo de TAREFAS: a ideia antes do sections
-----------------------------------------------------
 Até aqui, com "parallel for", TODAS as threads faziam o MESMO trabalho
 em dados diferentes (paralelismo de DADOS).
   Exemplo: 4 threads calculando a[i] = x² + y² + z², cada uma num pedaço do vetor.

 Agora queremos que cada thread faça um trabalho DIFERENTE
 (paralelismo de TAREFAS).
   Analogia: numa cozinha, um cozinheiro faz o arroz, outro o feijão,
   outro a salada e outro arruma a mesa. Tarefas diferentes, ao mesmo tempo.

 Este primeiro código faz isso "na mão": cada thread olha o próprio ID
 e decide o que fazer com if/else.

 Problemas desta abordagem manual:
   1) Depende de existirem EXATAMENTE 4 threads.
      Se o sistema der só 2 threads, as tarefas das threads 2 e 3
      simplesmente NÃO acontecem (ninguém tem o ID 2 ou 3).
   2) Se houver mais tarefas que threads, temos que reescrever os ifs.
   3) O código mistura "quem faz" com "o que fazer".

 No próximo arquivo (008_sections_0.1.cpp), a diretiva "sections" resolve isso:
 nós só listamos as tarefas e o OpenMP distribui entre as threads disponíveis.

 Compilar e rodar (PowerShell):
   g++ -fopenmp 008_sections_0.0.cpp -o 008_sections_0.0.exe
   .\008_sections_0.0.exe
*/

#include <iostream>
#include <omp.h>
using namespace std;

int main() {
    // num_threads(4): pede um time de 4 threads para esta região.
    #pragma omp parallel num_threads(4)
    {
        int id = omp_get_thread_num();

        // Cada thread escolhe a SUA tarefa pelo ID.
        if (id == 0) {
            cout << "Thread " << id << " carrega dados.\n";
        }
        else if (id == 1) {
            cout << "Thread " << id << " processa dados.\n";
        }
        else if (id == 2) {
            cout << "Thread " << id << " salva resultados.\n";
        }
        else {
            cout << "Thread " << id << " faz logging e limpeza.\n";
        }
    }

    return 0;
}

/*
 Exemplo de saída (a ordem varia a cada execução):
   Thread 2 salva resultados.
   Thread 0 carrega dados.
   Thread 3 faz logging e limpeza.
   Thread 1 processa dados.

 Observações:
  - As 4 tarefas rodam ao mesmo tempo, por isso a ordem muda.
  - Num programa real, "processar" dependeria de "carregar" ter terminado.
    Aqui são só mensagens; tarefas DEPENDENTES precisariam de sincronização
    (barrier, aula 006). O sections é indicado para tarefas INDEPENDENTES.
  - As linhas podem sair misturadas porque todas usam o mesmo cout
    (veja 002_hello4t.cpp para a solução com critical).
*/
