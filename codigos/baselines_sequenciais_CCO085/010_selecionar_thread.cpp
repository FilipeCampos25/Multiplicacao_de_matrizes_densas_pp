/*----------------------------------------------------------
 010 - Como escolher UMA thread específica para rodar uma função
------------------------------------------------------------
 Pergunta da aula:
   "Dentro de uma região paralela, como eu faço SÓ a thread 2
    (ou só a thread 0, ou só uma qualquer) executar uma função?"

 Analogia:
   Uma equipe de 4 pessoas (threads 0, 1, 2, 3) trabalhando juntas.
   - "Fulano, só você faz isso"          -> escolher pelo ID (if tid == 2)
   - "O chefe faz isso"                  -> master (sempre a thread 0)
   - "Alguém faz isso, tanto faz quem"   -> single (a primeira que chegar)

 As 3 formas que veremos:
   1) if (omp_get_thread_num() == ALVO) -> thread ESPECÍFICA, escolhida por você
   2) #pragma omp master                -> sempre a thread 0 (a principal)
   3) #pragma omp single                -> UMA thread qualquer (não é específica!)

 Compilar e rodar (PowerShell):
   g++ -fopenmp 010_selecionar_thread.cpp -o 010_selecionar_thread.exe
   .\010_selecionar_thread.exe
----------------------------------------------------------*/

#include <iostream>
#include <sstream>
#include <omp.h>

// Imprime uma linha inteira de uma vez (critical evita linhas misturadas, como no 002).
void imprimir(const std::string& msg) {
    #pragma omp critical
    std::cout << msg << std::endl;
}

// Funções "de trabalho" que queremos entregar a threads específicas.
void salvar_relatorio() {
    std::ostringstream oss;
    oss << "   [Thread " << omp_get_thread_num() << "] salvar_relatorio(): gravando o arquivo...";
    imprimir(oss.str());
}

void mostrar_progresso() {
    std::ostringstream oss;
    oss << "   [Thread " << omp_get_thread_num() << "] mostrar_progresso(): 50% concluido";
    imprimir(oss.str());
}

void ler_configuracao() {
    std::ostringstream oss;
    oss << "   [Thread " << omp_get_thread_num() << "] ler_configuracao(): arquivo lido";
    imprimir(oss.str());
}

int main() {
    const int NUM_THREADS = 4;
    const int THREAD_ALVO = 2;   // <- troque aqui para escolher outra thread

    /*======================================================
     FORMA 1: escolher pelo ID com if
     ======================================================
     Todas as threads chegam no if, mas só a que tem o ID igual
     a THREAD_ALVO entra. As outras simplesmente pulam o bloco.

     Exemplo com 4 threads e THREAD_ALVO = 2:
        thread 0 -> 0 == 2? não -> pula
        thread 1 -> 1 == 2? não -> pula
        thread 2 -> 2 == 2? SIM -> executa salvar_relatorio()
        thread 3 -> 3 == 2? não -> pula

     É a única das três formas em que VOCÊ escolhe exatamente qual thread.

     CUIDADO: se o time tiver menos threads que o alvo, NINGUÉM executa.
       Exemplo: THREAD_ALVO = 5 com 4 threads (IDs 0..3) -> a função nunca roda.
       Por isso o código abaixo confere se o alvo existe.

     Não há barreira automática depois do if: as outras threads
     seguem em frente sem esperar a thread alvo terminar.
     ======================================================*/
    std::cout << "FORMA 1 - if (omp_get_thread_num() == " << THREAD_ALVO << ")\n";

    #pragma omp parallel num_threads(NUM_THREADS)
    {
        int tid = omp_get_thread_num();
        int total = omp_get_num_threads();

        // Todas as threads fazem o trabalho "comum"
        std::ostringstream oss;
        oss << "   Thread " << tid << " fazendo o trabalho comum";
        imprimir(oss.str());

        // Só a thread alvo executa a função
        if (tid == THREAD_ALVO) {
            salvar_relatorio();
        }

        // Proteção: avisa se a thread alvo não existe neste time
        if (tid == 0 && THREAD_ALVO >= total) {
            imprimir("   AVISO: a thread alvo nao existe neste time; ninguem executou a funcao!");
        }
    }

    /*======================================================
     FORMA 2: #pragma omp master  (sempre a thread 0)
     ======================================================
     Só a thread 0 (a "principal", que existia antes da região paralela)
     executa o bloco. É o mesmo que "if (tid == 0)", escrito como diretiva.

     Uso típico: mostrar progresso na tela, interagir com o usuário,
     coisas que devem sempre partir da thread principal.

     Também NÃO tem barreira no final: as outras threads não esperam.

     Observação: no OpenMP 5.1 'master' foi renomeado para 'masked'.
     Com compiladores novos (GCC 12 ou superior) dá para escolher outra thread:
         #pragma omp masked filter(2)   // só a thread 2 executa
     'master' continua funcionando em qualquer GCC, por isso o usamos aqui.
     ======================================================*/
    std::cout << "\nFORMA 2 - #pragma omp master (sempre a thread 0)\n";

    #pragma omp parallel num_threads(NUM_THREADS)
    {
        #pragma omp master
        {
            mostrar_progresso();
        }
    }

    /*======================================================
     FORMA 3: #pragma omp single  (UMA thread qualquer)
     ======================================================
     Só UMA thread executa o bloco, mas o OpenMP escolhe qual
     (normalmente a primeira que chegar). Rodando várias vezes,
     o ID impresso pode mudar: às vezes 0, às vezes 3...

     Portanto: single NÃO serve para escolher uma thread específica.
     Serve quando o importante é que a tarefa rode UMA vez só,
     não importa por quem. Exemplo: ler um arquivo de configuração
     que todas as threads vão usar depois.

     Diferente de if e master, o single TEM barreira implícita no final:
     as outras threads esperam até o bloco terminar. Assim, depois do single,
     todas podem usar com segurança o que ele preparou.
     (Para tirar a espera: #pragma omp single nowait)
     ======================================================*/
    std::cout << "\nFORMA 3 - #pragma omp single (uma thread qualquer)\n";

    #pragma omp parallel num_threads(NUM_THREADS)
    {
        #pragma omp single
        {
            ler_configuracao();
        }
        // barreira implícita aqui: todas esperam o single terminar

        std::ostringstream oss;
        oss << "   Thread " << omp_get_thread_num() << " ja pode usar a configuracao";
        imprimir(oss.str());
    }

    return 0;
}

/*----------------------------------------------------------
 Saída esperada (a ordem das linhas "comuns" varia):

 FORMA 1 - if (omp_get_thread_num() == 2)
    Thread 0 fazendo o trabalho comum
    Thread 2 fazendo o trabalho comum
    [Thread 2] salvar_relatorio(): gravando o arquivo...     <- sempre a thread 2
    Thread 1 fazendo o trabalho comum
    Thread 3 fazendo o trabalho comum

 FORMA 2 - #pragma omp master (sempre a thread 0)
    [Thread 0] mostrar_progresso(): 50% concluido            <- sempre a thread 0

 FORMA 3 - #pragma omp single (uma thread qualquer)
    [Thread 3] ler_configuracao(): arquivo lido              <- pode ser qualquer ID
    Thread 1 ja pode usar a configuracao
    ...                                                      <- sempre DEPOIS do single

----------------------------------------------------------
 Resumo para o quadro:

   Quero...                              Use                         Barreira no fim?
   ------------------------------------  --------------------------  ----------------
   uma thread ESPECÍFICA (ex.: a 2)      if (tid == 2)               não
                                         masked filter(2) (GCC 12+)  não
   sempre a thread principal (0)         master                      não
   uma vez só, qualquer thread           single                      SIM

 Experimento: troque THREAD_ALVO para 5 e rode. O aviso aparece
 e salvar_relatorio() não é executada por ninguém.
----------------------------------------------------------*/
