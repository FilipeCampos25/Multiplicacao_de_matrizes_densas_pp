
/*--------------------------------------------
 006 - 0.2 - Exemplos de sincronização de threads.

 Compilar e rodar (PowerShell):
   g++ -fopenmp 006_sincronizacao_0.2.cpp -o 006_sincronizacao_0.2.exe
   .\006_sincronizacao_0.2.exe
---------------------------------*/

/*--------------------------------------------
 2. Pragma omp atomic
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

Exemplo: x² - 5x + 6 = 0  (a = 1, b = -5, c = 6)
       Δ  = 25 - 24 = 1
       x1 = (5 + 1) / 2 = 3
       x2 = (5 - 1) / 2 = 2      -> soma das raízes = 5

Cenário: temos MUITAS equações para resolver e queremos paralelizar.
Cada thread resolve algumas equações e soma o resultado em soma_total.
Como todas as threads escrevem na MESMA variável soma_total,
precisamos de sincronização — senão algumas somas se perdem.

------------------------------------

2. Pragma omp atomic
--------------------*/
/*
A diretiva #pragma omp atomic é uma forma mais LEVE de proteger UMA operação simples,
como incremento (x++), decremento (x--) ou soma acumulada (x += v).

Analogia:
  - critical = trancar o banheiro inteiro enquanto você usa.
  - atomic   = apertar um botão que o próprio hardware garante que ninguém
               aperta junto com você. É bem mais rápido, mas só serve para "apertar o botão".

Diferença na prática:
  - critical: protege QUALQUER bloco (várias linhas, cout, chamadas de função).
  - atomic:   protege SÓ UMA instrução simples do tipo x op= expr.

    Pode:      #pragma omp atomic
               soma_total += soma_local;

    NÃO pode:  #pragma omp atomic
               { soma_total += v; contador++; }      // duas instruções -> use critical

Vamos repetir o exemplo do 0.1 trocando critical por atomic.
O resultado deve ser o mesmo: N * 5.
*/

#include <iostream>
#include <vector>
#include <cmath>
#include <omp.h>

// Função resolver_bhaskara: retorna a soma das raízes (ou 0 se Δ < 0).
// Roda em paralelo quando chamada de dentro do laço paralelo;
// é segura porque só usa variáveis locais (cada thread tem as suas).
double resolver_bhaskara(double a, double b, double c) {
    double delta = (b * b) - (4 * a * c);
    if (delta < 0) return 0.0;
    double x1 = (-b + std::sqrt(delta)) / (2 * a);
    double x2 = (-b - std::sqrt(delta)) / (2 * a);
    return x1 + x2;
}

int main() {
    const int N = 1000;
    std::vector<double> a(N), b(N), c(N);
    double soma_total = 0.0;   // COMPARTILHADA entre as threads

    // Todas as equações são x² - 5x + 6 = 0 (soma das raízes = 5)
    for (int i = 0; i < N; ++i) {
        a[i] = 1.0;
        b[i] = -5.0;
        c[i] = 6.0;
    }

    #pragma omp parallel for
    for (int i = 0; i < N; ++i) {
        // Parte paralela: cada thread resolve a sua equação.
        double soma_local = resolver_bhaskara(a[i], b[i], c[i]);

        // --- OPERAÇÃO ATÔMICA ---
        // Garante que "ler soma_total, somar, gravar" aconteça como UM passo só,
        // sem que outra thread entre no meio.
        // É a escolha natural para contadores e acumuladores simples.
        #pragma omp atomic
        soma_total += soma_local;
    }

    std::cout << "Soma total (com #pragma omp atomic): " << soma_total << std::endl;
    std::cout << "Resultado esperado: " << N * 5.0 << std::endl;

    return 0;
}

/*
 Saída esperada:
   Soma total (com #pragma omp atomic): 5000
   Resultado esperado: 5000

 Experimento para a aula:
   Apague a linha "#pragma omp atomic", compile e rode várias vezes com
   $env:OMP_NUM_THREADS = "8". Às vezes o resultado sai MENOR que 5000:
   são as somas perdidas pela condição de corrida.
   (Com N pequeno o erro pode não aparecer sempre — aumente N para 1.000.000.)

 E ainda dá para melhorar: na aula 007 veremos 'reduction', em que cada thread
 soma numa variável PRÓPRIA e o OpenMP junta tudo só no final, sem fila.
*/
