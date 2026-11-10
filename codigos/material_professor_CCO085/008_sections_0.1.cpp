/*-------------------------------------------------------------------------------------------------------------------------
 * File Name : 008_sections_0.1.cpp
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
 008 - 0.1 - sections: paralelismo de TAREFAS
-----------------------------------------------------
 A diretiva sections divide o trabalho em BLOCOS DIFERENTES (tarefas),
 e o OpenMP entrega cada bloco a uma thread.

   #pragma omp parallel sections
   {
       #pragma omp section
       { tarefa_A(); }       // uma thread faz A

       #pragma omp section
       { tarefa_B(); }       // outra thread faz B (ao mesmo tempo)
   }                         // barreira implícita: espera A e B terminarem

 Diferença para o "parallel for":
   - parallel for: MESMA tarefa, dados diferentes (cada thread pega um pedaço do loop).
   - sections:     tarefas DIFERENTES, cada uma feita por uma thread.

 Analogia: preparar um almoço.
   Sequencial: um cozinheiro faz arroz (20 min), depois feijão (30 min),
               depois salada (10 min) -> 60 minutos.
   Com sections: 3 cozinheiros, cada um com um prato, ao mesmo tempo
               -> o almoço fica pronto quando o prato MAIS DEMORADO termina: 30 minutos.

---------------------------------------------------
Exemplo: simulação de inicialização de sistema
-----------------------------------------------------
Imaginem que estamos desenvolvendo um software grande,
como um jogo ou um sistema de análise de dados.
Quando o programa inicia, ele precisa realizar várias tarefas de preparação que demoram um certo tempo:

1.  Carregar dados de um banco de dados.          (2,0 s)
2.  Conectar-se a um serviço externo (uma API).   (1,5 s)
3.  Inicializar um sistema de cache em memória.   (1,0 s)
4.  Ler e processar arquivos de configuração.     (0,5 s)

Essas quatro tarefas são INDEPENDENTES umas das outras.
Não precisamos esperar o banco de dados carregar para começar a ler os arquivos de configuração.
Portanto, este é um caso típico de paralelismo por tarefas com sections.

-------
O objetivo é reduzir o tempo total de "boot" do programa.
  Sequencial: 2,0 + 1,5 + 1,0 + 0,5 = 5,0 segundos (a soma de todas).
  Paralelo com 4 threads: ≈ 2,0 segundos (o tempo da tarefa MAIS LONGA).
------

Cada função abaixo simula uma tarefa demorada usando sleep_for ("dormir" por um tempo)
e retorna true se a "inicialização" foi bem-sucedida.
O main usa sections para chamar as quatro funções em paralelo e, no final,
verifica se todos os subsistemas estão prontos.

 Compilar e rodar (PowerShell):
   g++ -fopenmp 008_sections_0.1.cpp -o 008_sections_0.1.exe
   .\008_sections_0.1.exe

 Experimento: rode com $env:OMP_NUM_THREADS = "2".
   Com 2 threads, uma thread precisa fazer duas tarefas em sequência,
   então o tempo total sobe (em torno de 2,5 a 3 s, dependendo de como o OpenMP distribuir).
*/

#include <iostream>
#include <string>
#include <vector>
#include <chrono> // Para medir/simular tempo
#include <thread> // Para a função sleep_for
#include <omp.h>

// Nossas 4 funções de inicialização.
// Cada uma simula uma tarefa demorada e independente.
// omp_get_thread_num() dentro delas mostra QUAL thread recebeu a tarefa.

/**
 * Simula o carregamento de dados de um banco de dados.
 * Retorna true se o carregamento foi bem-sucedido.
 */
bool carregarBancoDeDados() {
    int thread_id = omp_get_thread_num();
    std::cout << "[Thread " << thread_id << "] Iniciando carregamento do Banco de Dados... (demora 2s)\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "[Thread " << thread_id << "] >>> Banco de Dados CARREGADO.\n";
    return true;
}

/**
 * Simula a conexão com um serviço de rede externo.
 * Retorna true se a conexão foi estabelecida.
 */
bool conectarServicoExterno() {
    int thread_id = omp_get_thread_num();
    std::cout << "[Thread " << thread_id << "] Iniciando conexão com API externa... (demora 1.5s)\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    std::cout << "[Thread " << thread_id << "] >>> Conectado à API externa.\n";
    return true;
}

/**
 * Simula a criação e o preenchimento de um cache em memória.
 * Retorna true se o cache foi inicializado.
 */
bool inicializarCache() {
    int thread_id = omp_get_thread_num();
    std::cout << "[Thread " << thread_id << "] Iniciando inicialização do cache em memória... (demora 1s)\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "[Thread " << thread_id << "] >>> Cache PRONTO.\n";
    return true;
}

/**
 * Simula a leitura de múltiplos arquivos de configuração do disco.
 * Retorna true se os arquivos foram lidos com sucesso.
 */
bool lerArquivosDeConfiguracao() {
    int thread_id = omp_get_thread_num();
    std::cout << "[Thread " << thread_id << "] Iniciando leitura dos arquivos de configuração... (demora 0.5s)\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "[Thread " << thread_id << "] >>> Configurações LIDAS.\n";
    return true;
}


int main() {
    // Variáveis para guardar o status de cada subsistema.
    // Cada section escreve em UMA variável diferente -> não há conflito.
    bool db_ok = false;
    bool api_ok = false;
    bool cache_ok = false;
    bool config_ok = false;

    std::cout << "--- Iniciando a simulação de boot do sistema ---\n";
    double start_time = omp_get_wtime();

    // Cada 'section' é uma tarefa independente que o OpenMP atribui a uma thread.
    // Todas as seções dentro deste bloco podem ser executadas em paralelo.
    // Se houver menos threads que seções, alguma thread executa mais de uma, em sequência.
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            db_ok = carregarBancoDeDados();
        }

        #pragma omp section
        {
            api_ok = conectarServicoExterno();
        }

        #pragma omp section
        {
            cache_ok = inicializarCache();
        }

        #pragma omp section
        {
            config_ok = lerArquivosDeConfiguracao();
        }
    } // Barreira implícita: o programa espera aqui até TODAS as 4 seções terminarem.

    double end_time = omp_get_wtime();
    double duration = end_time - start_time;

    std::cout << "\n--- Processo de boot finalizado --- \n";
    std::cout << "Tempo total de inicialização: " << duration << " segundos.\n";

    double tempo_serial_esperado = 2.0 + 1.5 + 1.0 + 0.5;
    std::cout << "Tempo serial estimado seria: " << tempo_serial_esperado << " segundos.\n";

    // Verificação final do status do sistema.
    // Só continua se TODOS os subsistemas estiverem OK.
    if (db_ok && api_ok && cache_ok && config_ok) {
        std::cout << "\n[SUCESSO] Todos os subsistemas foram inicializados corretamente. O programa pode continuar.\n";
    } else {
        std::cout << "\n[FALHA] Um ou mais subsistemas falharam na inicialização. O programa será encerrado.\n";
    }

    return 0;
}


/*
-----------------------------------------------
Observações sobre a execução do código:
-----------------------------------------------

1.  Ordem de início e fim: as mensagens "Iniciando..." aparecem quase todas ao mesmo tempo.
    As mensagens ">>> ..." aparecem na ordem em que as tarefas terminam:
    a de 0,5 s provavelmente primeiro, e a de 2 s por último.
    Cada tarefa é executada por uma thread diferente (veja o número entre colchetes).
    Como todas usam o mesmo cout, uma linha pode eventualmente sair misturada com outra.

2.  Tempo total de execução: o ponto mais importante.
    O tempo total fica um pouco acima do tempo da tarefa mais demorada (2 segundos),
    e não a soma de todos os tempos (5 segundos).
    Isso mostra o ganho do paralelismo de tarefas.
    Speedup ideal aqui: 5,0 / 2,0 = 2,5 — limitado pela tarefa mais longa,
    não importa quantas threads a mais você tenha.

3.  Estrutura do código: separar as tarefas em funções deixa o bloco sections
    limpo e legível. Cada section tem uma responsabilidade clara.

4.  Quando usar sections: poucas tarefas DIFERENTES e INDEPENDENTES, conhecidas
    na hora de escrever o código. Para muitas tarefas iguais, use parallel for.
*/
