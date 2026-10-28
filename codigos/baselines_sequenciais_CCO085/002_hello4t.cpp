
/*----------------------------------------------------------
   002 - Hello OpenMP com saída organizada, usando exatamente 4 threads
   --------------------------------------------------------
   Objetivo: mostrar como evitar "linhas coladas" quando várias threads
   imprimem ao mesmo tempo, usando:
     1) a cláusula num_threads(4), para fixar o time em 4 threads;
     2) um buffer local (std::ostringstream) para montar a linha inteira; e
     3) uma região crítica (#pragma omp critical) na hora de escrever no std::cout.

   Analogia:
     Quatro alunos precisam escrever uma frase cada no MESMO quadro.
     - Sem regra: todos escrevem juntos e as letras se misturam.
     - Com esta solução: cada aluno escreve a frase no SEU caderno (buffer local),
       depois faz fila e vai ao quadro UM POR VEZ (critical) só para copiar.
     Escrever no caderno é rápido e pode ser simultâneo; a fila é curta.

   Como compilar (GCC/MinGW-w64, terminal do VS Code / PowerShell):
     g++ -fopenmp 002_hello4t.cpp -o 002_hello4t.exe

   Como executar:
     .\002_hello4t.exe

   Observações:
   - A ordem das linhas pode variar entre execuções (escalonamento do SO).
   - #pragma omp parallel cria um *time* de threads; cada thread executa o bloco uma vez.
   - num_threads(4) tem PRIORIDADE sobre a variável OMP_NUM_THREADS.
     Exemplo: mesmo com $env:OMP_NUM_THREADS = "8", esta região usa 4 threads.
*/

#include <iostream>   // std::cout
#include <sstream>    // std::ostringstream (buffer local de string)
#include <omp.h>      // API do OpenMP

int main() {
    // Região paralela: o runtime do OpenMP cria um time de 4 threads.
    // Cada thread executa o bloco entre { } exatamente uma vez.
    #pragma omp parallel num_threads(4) // cria um time de 4 threads
    {
        //  Descobrir quem sou eu dentro do time:
        //    - tid: ID da thread (0...3)
        //    - tot: total de threads no time (4)
        int tid = omp_get_thread_num();
        int tot = omp_get_num_threads();

        // ETAPA 1 - Montar a frase no "caderno" (buffer local).
        // std::ostringstream é uma "string em construção": tudo que mandamos com <<
        // vai sendo acumulado na memória, sem aparecer na tela ainda.
        // O 'oss' é declarado dentro do bloco paralelo, então CADA thread tem o seu.
        // Não há compartilhamento, logo não há disputa nessa etapa.
        std::ostringstream oss;

        oss << "Hello, world from thread " << tid
            << " of " << tot << "\n";
        // Exemplo: a thread 2 monta a string "Hello, world from thread 2 of 4\n".

        /*----------------------------------------------------------
         ETAPA 2 - Região crítica: ir ao "quadro" um por vez.
         ----------------------------------------------------------*/
        // O std::cout é um recurso COMPARTILHADO: todas as threads escrevem no mesmo console.
        // Se duas ou mais threads chamarem std::cout ao mesmo tempo, as saídas podem se misturar:
        //     Hello, world from thread Hello, world from thread 1 of 4
        //     3 of 4
        //
        // Com o critical, só UMA thread por vez executa o bloco abaixo.
        // As outras ficam esperando a vez. Assim cada linha sai inteira:
        //     Hello, world from thread 1 of 4
        //     Hello, world from thread 3 of 4
        //
        // A ORDEM de entrada no critical não é garantida (depende do SO).
        /*--------
         Resumindo:
         Região crítica é uma trava de exclusão mútua ("um de cada vez")
         que protege a escrita no std::cout e mantém as mensagens legíveis.
         ----------*/
        #pragma omp critical
        {
            std::cout << oss.str();
        }

        // Boa prática: deixar a região critical PEQUENA (apenas a escrita).
        // O trabalho "pesado" — se existisse — ficaria fora dela, rodando em paralelo.
        // Quanto maior o critical, mais tempo as threads passam na fila.
    }

    return 0;
}

/*
 Exemplo de saída:
    Hello, world from thread 0 of 4
    Hello, world from thread 2 of 4
    Hello, world from thread 3 of 4
    Hello, world from thread 1 of 4

 Compare com o 000_hello.cpp: lá as linhas podiam sair coladas; aqui cada linha sai inteira.
*/




/*--------------------------------------------------------------
Código sem comentários
---------------------------------------------------------------*/

/* <---------------------- para visualizar, retire o comentário

#include <iostream>
#include <sstream>
#include <omp.h>

int main() {
    #pragma omp parallel num_threads(4)
    {
        int tid = omp_get_thread_num();
        int tot = omp_get_num_threads();

        std::ostringstream oss;
        oss << "Hello, world from thread " << tid << " of " << tot << "\n";

        #pragma omp critical
        std::cout << oss.str();
    }
}
*/
