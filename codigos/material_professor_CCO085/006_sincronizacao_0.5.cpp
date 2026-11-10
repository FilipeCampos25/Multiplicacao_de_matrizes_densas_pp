/*--------------------------------------------
 006 - 0.5 - Exemplos de sincronização de threads.

 Compilar e rodar (PowerShell):
   g++ -fopenmp 006_sincronizacao_0.5.cpp -o 006_sincronizacao_0.5.exe
   .\006_sincronizacao_0.5.exe
---------------------------------*/

/*--------------------------------------------
 5. Utilizando locks
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

5. Funções de lock
   omp_init_lock, omp_set_lock, omp_unset_lock, omp_destroy_lock

Um lock é um "cadeado" que você tranca e destranca manualmente.
É um mecanismo mais flexível e de mais baixo nível que o critical.

Analogia: um armário com DUAS gavetas, cada uma com seu cadeado.
  - Gaveta "pares"   -> cadeado lock_pares
  - Gaveta "ímpares" -> cadeado lock_impares
  Uma thread mexendo na gaveta dos pares NÃO impede outra de mexer
  na gaveta dos ímpares ao mesmo tempo. Só espera quem quer a MESMA gaveta.

  Com um único critical seria um cadeado só para o armário inteiro:
  quem quisesse a gaveta dos ímpares esperaria quem está na dos pares, sem necessidade.

Ciclo de vida de um lock (sempre nesta ordem):
  1) omp_init_lock    -> cria o cadeado (antes da região paralela)
  2) omp_set_lock     -> tranca (se já estiver trancado, a thread ESPERA)
  3) omp_unset_lock   -> destranca (libera para a próxima thread)
  4) omp_destroy_lock -> descarta o cadeado (depois da região paralela)

Locks são úteis quando:
  - há VÁRIOS recursos que podem ser protegidos separadamente; ou
  - a lógica de trancar/destrancar não cabe em um único bloco de código.

Exemplo deste arquivo: duas somas (equações de índice par e de índice ímpar),
cada uma protegida pelo seu próprio lock.
--------------------*/


#include <iostream>
#include <vector>
#include <cmath>
#include <omp.h>

// Função resolver_bhaskara
double resolver_bhaskara(double a, double b, double c) {
    double delta = (b * b) - (4 * a * c);
    if (delta < 0) return 0.0;
    double x1 = (-b + std::sqrt(delta)) / (2 * a);
    double x2 = (-b - std::sqrt(delta)) / (2 * a);
    return x1 + x2;
}

int main() {
    const int N = 1000;
    std::vector<double> a(N, 1.0), b(N, -5.0), c(N, 6.0);

    // Dois recursos compartilhados diferentes (as duas "gavetas")
    double soma_pares = 0.0;
    double soma_impares = 0.0;

    // Um cadeado para cada recurso
    omp_lock_t lock_pares;
    omp_lock_t lock_impares;

    // 1. Inicializa os locks ANTES de usá-los.
    omp_init_lock(&lock_pares);
    omp_init_lock(&lock_impares);

    #pragma omp parallel for
    for (int i = 0; i < N; ++i) {
        double soma_local = resolver_bhaskara(a[i], b[i], c[i]);

        // Decide qual recurso/lock usar: i par -> pares; i ímpar -> ímpares.
        if (i % 2 == 0) {
            // 2. Tranca o lock dos pares.
            // Se outra thread estiver com ele, esta thread espera aqui.
            omp_set_lock(&lock_pares);
            soma_pares += soma_local; // acesso seguro ao recurso

            // 3. Destranca. É CRUCIAL: sem isso, as outras threads
            // ficam esperando para sempre e o programa trava.
            omp_unset_lock(&lock_pares);
        } else {
            // Mesmo padrão, mas com o OUTRO cadeado.
            // Uma thread aqui não bloqueia uma thread que está nos pares.
            omp_set_lock(&lock_impares);
            soma_impares += soma_local;
            omp_unset_lock(&lock_impares);
        }
    }

    // 4. Destrói os locks após o uso, liberando os recursos que eles ocupam.
    omp_destroy_lock(&lock_pares);
    omp_destroy_lock(&lock_impares);

    std::cout << "Soma das equações pares: " << soma_pares << std::endl;
    std::cout << "Soma das equações ímpares: " << soma_impares << std::endl;
    std::cout << "Soma total (locks): " << soma_pares + soma_impares << std::endl;

    return 0;
}

/*
 Saída esperada (N = 1000 -> 500 equações pares e 500 ímpares, cada uma com soma 5):
   Soma das equações pares: 2500
   Soma das equações ímpares: 2500
   Soma total (locks): 5000

 Cuidado com DEADLOCK (travamento mútuo):
   Se uma thread precisar dos DOIS locks ao mesmo tempo, tranque sempre na mesma ordem.
     Errado:  Thread A: set(lock_pares)   -> set(lock_impares)
              Thread B: set(lock_impares) -> set(lock_pares)
              -> A segura pares e espera ímpares; B segura ímpares e espera pares. Ninguém sai.
     Certo:   todas as threads trancam primeiro lock_pares, depois lock_impares.

 Curiosidade: o mesmo efeito deste exemplo pode ser obtido com critical NOMEADOS:
     #pragma omp critical(pares)    { soma_pares   += soma_local; }
     #pragma omp critical(impares)  { soma_impares += soma_local; }
   Criticals com nomes diferentes não bloqueiam um ao outro.
*/
