# Metodologia e Arquitetura do Projeto

## Metodologia Experimental
As avaliações de desempenho baseiam-se na medição do tempo de execução da multiplicação de matrizes densas ($N \times N$). 
* **Tamanhos de Matriz:** Foram definidos os tamanhos $N = 256$, $512$ e $1024$.
* **Repetições:** Cada dimensão é executada 5 vezes para isolar interferências do sistema operacional, adotando-se a mediana dos tempos como métrica principal comparativa.
* **Critérios de Comparação:** O *baseline* sequencial (`01_matmul.cpp`) serve de referência ($T_{seq}$) para o cálculo de *Speedup* ($S = T_{seq}/T_p$) e Eficiência nas futuras implementações paralelas.

## Paralelismo: OpenMP vs MPI
O projeto explora dois paradigmas distintos de programação paralela aplicados à multiplicação de matrizes:
1. **OpenMP (Memória Compartilhada):** Utiliza *threads* que compartilham o mesmo espaço de memória da máquina. É ideal para otimizar os laços aninhados (`for`) dentro de um único computador, reduzindo o tempo de execução através da paralelização no nível do nó.
2. **MPI (Memória Distribuída):** Baseia-se em processos isolados com sua própria memória, que se comunicam explicitamente através do envio e recebimento de mensagens. É o padrão para sistemas distribuídos e *clusters*, onde o trabalho da matriz precisa ser particionado e enviado para diferentes computadores.

## Relação com os Exemplos do Professor
Os exemplos didáticos disponibilizados pelo professor orientam as estratégias do projeto:
* **`omp_get_wtime()`:** Assim como no exemplo oficial `005_loop_for_paralell_time.cpp`, esta função do OpenMP será utilizada para capturar o tempo do relógio da parede (*wall clock time*) com alta precisão e de forma consistente nos executáveis paralelos.
* **`schedule(static)`:** Com base nas práticas do exemplo `004_loop_for_paralell.cpp`, a estratégia de divisão estática é ideal para a multiplicação de matrizes densas. Como o esforço computacional para multiplicar os elementos é uniforme, a distribuição estática do trabalho pelas *threads* evita o custo extra (*overhead*) gerado por um escalonamento dinâmico.