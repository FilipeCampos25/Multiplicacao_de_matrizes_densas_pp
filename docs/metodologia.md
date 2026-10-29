# Metodologia e arquitetura do projeto

## Referência matemática

O baseline original é `codigos/baselines_sequenciais_CCO085/01_matmul.cpp`, do
Prof. Rodrigo Gonçalves Pinto. Ele gera internamente matrizes quadradas N×N,
armazenadas em vetores row-major, e calcula C=A×B na ordem `i-k-j`. A mesma
inicialização determinística é mantida nas implementações paralelas.

O escopo não inclui matrizes retangulares nem leitura de matrizes externas.
Consequentemente, a validação de entrada verifica um único N inteiro positivo e
os limites de representação, alocação e contagens MPI aplicáveis.

## Adaptação OpenMP

O exemplo oficial `004_loop_for_paralell.cpp` orienta a divisão das iterações do
laço externo com `omp for schedule(static)`. Na multiplicação, cada valor de `i`
representa uma linha de C e executa a mesma quantidade de trabalho O(N²). A
distribuição estática é, portanto, o padrão de baixo overhead.

A e B são apenas lidas. Cada thread escreve exclusivamente em `C[i*N+j]` da
linha que recebeu, preservando os laços internos `k-j`. Não há necessidade de
`critical`, `atomic`, locks ou redução no kernel. A barreira implícita assegura
que C esteja completa antes do checksum e da saída.

O exemplo `005_loop_for_paralell_time.cpp` orienta `omp_get_wtime()` e o uso de
`single`/`omp_get_num_threads()` para registrar a equipe efetiva. Não se mede
tempo individual por thread: após a barreira ele incluiria espera e, antes dela,
introduziria instrumentação sem utilidade para o benchmark oficial.

As variantes `static`, `dynamic` e `guided` são compiladas da mesma fonte para
comparação experimental. `static` permanece o comportamento normal; resultados
medidos, e não a teoria isolada, determinam qualquer comparação entre schedules.

## Adaptação MPI

O rank 0 inicializa A e B. `MPI_Scatterv` distribui blocos contíguos de linhas de
A, inclusive quando N não é divisível por P. `MPI_Bcast` replica B porque todas
as linhas de C dependem de todas as linhas de B. Cada rank aplica localmente o
kernel `i-k-j`, e `MPI_Gatherv` recompõe C no rank 0 na ordem global.

Os primeiros `N mod P` ranks recebem uma linha adicional. Quando `P>N`, alguns
ranks recebem zero linhas e participam das coletivas com contagem zero e buffer
válido. O rank 0 calcula o checksum, exporta C opcionalmente e executa `--verify`.

## Medição e comparação

Inicialização, checksum, impressão e verificação ficam fora do tempo do kernel.
No OpenMP, o intervalo inclui criação/entrada da equipe, cálculo e sincronização.
No MPI, começa após uma barreira e inclui `Scatterv`, `Bcast`, cálculo, `Gatherv`
e esperas. O máximo entre ranks representa o caminho crítico.

As coletas usam aquecimento, repetições e mediana. Speedup só deve comparar
tempos obtidos com fonte, flags, máquina e metodologia compatíveis. Os resultados
históricos permanecem nos documentos e CSVs próprios; não se infere ganho a
partir da simples presença de paralelismo.

## Corretude

O checksum é uma triagem e permanece sequencial para conservar a ordem da soma
do baseline. Embora o exemplo `007_reduction_0.0.cpp` seja conceitualmente
aplicável, uma redução paralela poderia mudar o arredondamento. Para matrizes
pequenas, `MATMUL_DUMP=1` permite comparar cada elemento com uma referência
Python independente. `--verify` recalcula os valores esperados.

## Validação realizada e pendências

Passaram no host disponível 8 testes de análise, 6 testes do coletor OpenMP e
10 testes do detector de corretude. A regressão dos executáveis ficou bloqueada
por ausência de Make, GCC, MPI, Docker e WSL; os 142 casos funcionais não foram
aprovados nem ignorados. Ainda são necessários, no ambiente oficial, build
limpo, suíte funcional, casos de argumentos inválidos e smoke benchmark em
arquivo novo.
