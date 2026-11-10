/*-------------------------------
 006 - Sincronização com OpenMP: CONCEITOS
---------------------------------
Neste arquivo estão somente comentários sobre sincronização com OpenMP.
Os trechos de código abaixo são ilustrativos (estão dentro de comentários
para não serem compilados).

Os códigos de exemplo completos estão distribuídos em outros arquivos,
começando pelo 006_sincronizacao_0.1.cpp:

   0.1 -> critical      0.2 -> atomic      0.3 -> barrier
   0.4 -> ordered       0.5 -> locks       0.6 -> exercícios
----------------------------------------------------*/

/*--------------------------------------------------------------
Sincronização com OpenMP - conceitos
1. pragma omp critical
2. pragma omp atomic
3. pragma omp barrier
4. pragma omp ordered
5. locks: omp_init_lock, omp_set_lock, omp_unset_lock, omp_destroy_lock
--------------------------------------------------------------*/

/*--------------------------------------------------------------
Por que precisamos de sincronização?
--------------------------------------------------------------
A sincronização serve para controlar o acesso simultâneo a dados compartilhados,
garantindo consistência e ordem na execução das threads.

O problema clássico: CONDIÇÃO DE CORRIDA (race condition).
A instrução  total += valor;  parece uma coisa só, mas o processador faz 3 passos:
   1) LER    total da memória
   2) SOMAR  valor
   3) GRAVAR o resultado de volta

Exemplo com total = 10 e duas threads somando 1 ao mesmo tempo:
   Thread A lê 10
   Thread B lê 10          (A ainda não gravou!)
   Thread A grava 11
   Thread B grava 11       -> deveria ser 12. Uma soma se perdeu.

Analogia: duas pessoas atualizando o saldo da mesma conta em caixas diferentes,
cada uma olhando o saldo antigo. O último a gravar apaga o trabalho do outro.

As diretivas abaixo são as ferramentas para evitar isso.
--------------------------------------------------------------*/

/*--------------------------------------------------------------
1. pragma omp critical  ("um de cada vez")
--------------------------------------------------------------
A diretiva #pragma omp critical define uma SEÇÃO CRÍTICA:
apenas UMA thread por vez pode executar o trecho de código dentro dela.
Útil para proteger trechos onde há escrita em uma variável compartilhada.

Analogia: o banheiro com uma chave só. Quem está dentro tranca a porta;
os outros esperam na fila.

Exemplo:

    #pragma omp parallel
    {
        #pragma omp critical
        {
            // só uma thread por vez entra aqui.
            total += valor; // variável compartilhada
        }
    }

- Sem o critical, duas threads poderiam atualizar total ao mesmo tempo e sobrescrever resultados.
- Com critical, cada uma espera a outra terminar antes de entrar na seção crítica.
- Serve para QUALQUER bloco de código (várias linhas, chamadas de função, cout...).
- Custo: o trecho vira uma fila. Se ele for executado muitas vezes, as threads
  passam mais tempo esperando do que trabalhando.
--------------------------------------------------------------*/

/*--------------------------------------------------------------
2. pragma omp atomic  ("uma conta indivisível")
--------------------------------------------------------------
Protege UMA operação simples de leitura/modificação/escrita em uma variável:
x += v,  x -= v,  x *= v,  x++,  x--, etc.
É recomendada quando a proteção é só para essa operação simples,
sem o custo de um bloco critical completo.

Analogia: em vez de trancar o banheiro inteiro (critical), você só
garante que ninguém mexa no interruptor enquanto você o aciona.

Exemplo:

    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        #pragma omp atomic
        soma += 1; // operação atômica segura
    }

- atomic costuma ser mais rápido que critical, porque usa instruções especiais
  do próprio processador.
- Limitação: só vale para UMA instrução simples. Não serve para um bloco
  com várias linhas, nem para um std::cout.
--------------------------------------------------------------*/

/*--------------------------------------------------------------
3. pragma omp barrier  ("todo mundo espera todo mundo")
--------------------------------------------------------------
Força todas as threads a esperarem umas pelas outras antes de seguir
para o próximo trecho de código.

Analogia: excursão escolar. Ninguém entra no ônibus para a próxima parada
até TODOS voltarem da parada atual.

Exemplo:

    #pragma omp parallel
    {
        faz_etapa1();

        #pragma omp barrier   // todas as threads esperam aqui

        faz_etapa2();         // código que depende do trabalho da etapa 1
    }

- Útil quando uma etapa depende do resultado completo da etapa anterior.
- Lembrete: "omp for", "omp single" e "omp sections" já têm uma barreira
  IMPLÍCITA no final (a menos que você use a cláusula nowait).
--------------------------------------------------------------*/

/*--------------------------------------------------------------
4. pragma omp ordered  ("na ordem da fila original")
--------------------------------------------------------------
Garante que uma parte do código dentro de um laço paralelo seja executada
na ORDEM original das iterações (i = 0, 1, 2, ...).

Analogia: vários cozinheiros preparam pratos ao mesmo tempo, mas o garçom
só serve as mesas na ordem dos pedidos.

Exemplo:

    #pragma omp parallel for ordered
    for (int i = 0; i < N; i++) {
        double r = calculo_pesado(i);   // roda em paralelo, em qualquer ordem

        #pragma omp ordered
        {
            std::cout << "i = " << i << " -> " << r << "\n";  // sai na ordem 0, 1, 2...
        }
    }

- Serve para manter a sequência de saída, logs ou escrita em arquivo.
- A cláusula 'ordered' precisa aparecer no "for" E a diretiva no bloco.
- Custo: o trecho ordenado anda em fila, então deve ser pequeno.
--------------------------------------------------------------*/

/*--------------------------------------------------------------
5. Locks: omp_init_lock, omp_set_lock, omp_unset_lock, omp_destroy_lock
--------------------------------------------------------------
São funções para criar e controlar "cadeados" (locks) manualmente.
Dão um controle mais fino que as diretivas: você decide QUANDO trancar,
QUANDO destrancar e pode ter VÁRIOS cadeados independentes.

Analogia: um armário com várias gavetas, cada uma com seu cadeado.
Uma pessoa mexendo na gaveta A não impede outra de mexer na gaveta B.
(O critical seria um cadeado único no armário inteiro.)

Exemplo:

    omp_lock_t lock;
    omp_init_lock(&lock);          // 1) cria o cadeado

    #pragma omp parallel
    {
        omp_set_lock(&lock);       // 2) tranca (espera se outra thread estiver usando)
        // seção crítica
        omp_unset_lock(&lock);     // 3) destranca
    }

    omp_destroy_lock(&lock);       // 4) descarta o cadeado

Cuidados:
- Diferente do critical, locks não são automáticos: cada set_lock precisa
  de um unset_lock correspondente. Esquecer o unset trava o programa.
- DEADLOCK: duas threads esperando uma pela outra para sempre.
  Exemplo: Thread A tranca o lock1 e espera o lock2;
           Thread B tranca o lock2 e espera o lock1. Ninguém sai do lugar.
  Regra prática: sempre trancar vários locks na MESMA ordem em todas as threads.
- Sempre inicialize (init) antes de usar e destrua (destroy) no final.
--------------------------------------------------------------*/

/*--------------------------------------------------------------
Resumo rápido: qual usar?
--------------------------------------------------------------
  Quero...                                         Use
  -----------------------------------------------  ------------------
  somar/contar em uma variável compartilhada       reduction (aula 007) ou atomic
  proteger um bloco com várias linhas              critical
  esperar todos terminarem uma fase                barrier
  imprimir/gravar na ordem de i                    ordered
  proteger vários recursos diferentes em paralelo  locks
--------------------------------------------------------------*/

/*---------------------------
 Vamos aos exemplos práticos: 006_sincronizacao_0.1.cpp
-----------------------------------*/

int main() { return 0; } // arquivo só de conceitos: main vazia apenas para compilar
