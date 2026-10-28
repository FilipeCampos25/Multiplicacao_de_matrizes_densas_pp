/*--------------------------------------------
 006 - 0.1 - Exemplos de sincronização de threads.

 Compilar e rodar (PowerShell):
   g++ -fopenmp 006_sincronizacao_0.1.cpp -o 006_sincronizacao_0.1.exe
   .\006_sincronizacao_0.1.exe

 Analogia do critical: o banheiro com uma chave só.
 Quem está dentro tranca a porta; os outros esperam na fila.
---------------------------------*/

/*--------------------------------------------
 1. Pragma omp critical
-------------------------------------------*/


/*--------------------------------------------
 Contexto para a fórmula de Bhaskara
-------------------------------------------*/
/*
--> A Fórmula de Bhaskara
--------------------------------
A fórmula de Bhaskara é usada para encontrar as raízes de uma equação de segundo grau
do tipo ax² + bx + c = 0.
As raízes (x1 e x2) são calculadas da seguinte forma:

1.  Calcular o discriminante (delta): Δ = b² - 4ac
2.  Calcular as raízes:
       x1 = (-b + √Δ) / 2a
       x2 = (-b - √Δ) / 2a

Exemplo: x² - 5x + 6 = 0  (a = 1, b = -5, c = 6)
       Δ  = 25 - 24 = 1
       x1 = (5 + 1) / 2 = 3
       x2 = (5 - 1) / 2 = 2      -> soma das raízes = 5

Vamos supor o seguinte cenário:
Temos um grande número de equações de segundo grau para resolver e
queremos paralelizar o processo.

Para isso, armazenaremos os coeficientes a, b e c em arrays, calcularemos as raízes
de cada equação e somaremos tudo em uma variável soma_total para verificar o resultado.

É neste ponto que a sincronização se torna crucial:
- Resolver cada equação é um trabalho INDEPENDENTE (a equação i não depende da j).
  Isso paraleliza sem nenhum cuidado especial.
- Somar em soma_total NÃO é independente: todas as threads escrevem na MESMA
  variável. Sem sincronização, duas threads podem ler o mesmo valor antigo,
  somar cada uma a sua parcela e gravar por cima uma da outra -> uma parcela
  se perde e o valor final fica incorreto (condição de corrida).

Por que "soma_total += x" não é seguro? Porque, para o processador, ela é feita em
três passos separados:
      1) LER   soma_total da memória
      2) SOMAR x
      3) GRAVAR o resultado de volta
Se a thread B fizer o passo 1 enquanto a thread A ainda está entre 1 e 3,
B trabalha com um valor desatualizado.

Exemplo com soma_total = 10, thread A somando 5 e thread B somando 5:
      A lê 10 | B lê 10 | A grava 15 | B grava 15   -> deveria ser 20!

------------------------------------*/


#include <iostream> // Para entrada e saída
#include <vector>   // std::vector
#include <cmath>    // Para operações matemáticas (std::sqrt)
#include <omp.h>    // API do OpenMP

/*--------------------------------------------
 Função para resolver uma equação de segundo grau usando Bhaskara
 e retornar a soma das raízes (ou 0 se não houver raízes reais).

 ESTA FUNÇÃO RODA EM PARALELO?
 Sim. Ela está escrita fora do #pragma, mas é CHAMADA de dentro do laço
 paralelo. No OpenMP, o que importa é ONDE o código executa, não onde ele
 está escrito: tudo o que é executado durante uma região paralela (inclusive
 funções chamadas a partir dela) faz parte dessa região -- é a chamada
 "extensão dinâmica" (dynamic extent) da região paralela.

 Cada thread que chama resolver_bhaskara() ganha a SUA PRÓPRIA cópia das
 variáveis locais (delta, x1, x2), porque elas vivem na pilha (stack) da
 thread. Por isso várias threads podem executar esta função ao mesmo tempo
 sem uma atrapalhar a outra.

 Ela é segura para threads (thread-safe) porque:
   - só lê os próprios parâmetros (passados por valor);
   - só escreve em variáveis locais;
   - não usa variáveis globais nem 'static'.
 Se ela, por exemplo, acumulasse o resultado numa variável global, teríamos
 a mesma condição de corrida de soma_total, só que escondida aqui dentro.
-------------------------------------------*/
double resolver_bhaskara(double a, double b, double c) {
    // Calcula delta (variável local: cada thread tem a sua)
    double delta = (b * b) - (4 * a * c);

    // Se o delta for negativo, não existem raízes reais.
    if (delta < 0) {
        return 0.0;
    }

    // Calcula as duas raízes
    double x1 = (-b + std::sqrt(delta)) / (2 * a);
    double x2 = (-b - std::sqrt(delta)) / (2 * a);

    // Retorna a soma das raízes
    return x1 + x2;
}

int main() {
    // Número de equações a serem resolvidas em paralelo
    const int N = 1000;

    // Vetores para armazenar os coeficientes de N equações.
    // Serão COMPARTILHADOS entre as threads, mas apenas LIDOS
    // dentro do laço paralelo -> leitura simultânea não causa problema.
    std::vector<double> a(N), b(N), c(N);

    // Declarada fora da região paralela -> COMPARTILHADA (shared) por padrão.
    // Todas as threads escrevem nela: é o ponto que precisa de sincronização.
    double soma_total = 0.0;

    // Preenche os vetores com valores de exemplo (trecho sequencial)
    for (int i = 0; i < N; ++i) {
        a[i] = 1.0;
        b[i] = -5.0;
        c[i] = 6.0; // x² - 5x + 6 = 0 -> raízes 2 e 3, a soma é 5
    }

    // Inicia a região paralela. As N iterações do laço são divididas
    // entre as threads (cada thread recebe um bloco de valores de i).
    //
    // Escopo das variáveis nesta região:
    //   i          -> PRIVADA (o OpenMP torna o índice do laço privado automaticamente)
    //   soma_local -> PRIVADA (declarada dentro do laço, cada thread tem a sua)
    //   a, b, c, N -> COMPARTILHADAS, apenas lidas -> seguro
    //   soma_total -> COMPARTILHADA e ESCRITA      -> precisa de proteção
    #pragma omp parallel for
    for (int i = 0; i < N; ++i) {
        // Parte PARALELA de verdade: cada thread resolve a sua equação,
        // ao mesmo tempo que as outras, chamando resolver_bhaskara().
        double soma_local = resolver_bhaskara(a[i], b[i], c[i]);

        // --- INÍCIO DA SEÇÃO CRÍTICA ---
        // A diretiva 'critical' garante que apenas UMA thread por vez
        // possa executar o bloco a seguir. As demais ficam esperando na
        // "porta" até a thread que está dentro sair.
        // Isso evita que duas threads atualizem 'soma_total' ao mesmo
        // tempo, o que poderia causar a perda de uma das atualizações.
        //
        // Custo: este trecho deixa de ser paralelo (vira uma fila).
        // Aqui o cálculo de Bhaskara é tão barato que as threads passam boa
        // parte do tempo esperando na fila -- por isso, neste exemplo, a
        // versão paralela pode não ficar mais rápida que a sequencial.
        // Veremos a seguir alternativas mais eficientes (atomic e reduction).
        #pragma omp critical
        {
            soma_total += soma_local;
        }
        // --- FIM DA SEÇÃO CRÍTICA ---
    }
    // Barreira implícita: ao final do 'parallel for', todas as threads
    // terminam antes de o programa seguir. Daqui em diante, só a thread
    // principal executa, e soma_total já está completa.

    // O resultado esperado é N * 5, pois a soma das raízes de cada equação é 5.
    std::cout << "Soma total (com #pragma omp critical): " << soma_total << std::endl;
    std::cout << "Resultado esperado: " << N * 5.0 << std::endl;

    return 0;
}
