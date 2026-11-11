# Avaliação de desempenho e escalabilidade MPI

> **Atualização de 10/11/2026, após a tentativa bloqueada:** foram coletadas
> 60 medições MPI por etapas, para N=256/512/1024 e p=1/2/4/8, com cinco
> repetições válidas por configuração, além de 15 referências sequenciais.
> Veja [dados, resumos e limitações](complementacao_validada.md), incluindo
> o aviso de falha de vinculação de memória emitido pelo Open MPI.
> A pendência de coleta por etapas descrita abaixo é histórica e foi resolvida
> nesta máquina; a avaliação entre nós distintos continua fora desta coleta.

## Evolução do estado

Em 21/10/2026, a avaliação MPI estava preparada, mas o host então disponível não possuía compilador, launcher MPI, Docker ou distribuição WSL. Naquela data não havia medições MPI, e nenhuma foi inventada.

Essa pendência foi resolvida em **22/10/2026** por uma coleta conjunta no serviço Docker `pp-lab`. O arquivo `resultados/issue10_desempenho.csv` contém **175 observações**: 15 sequenciais, 80 OpenMP e 80 MPI. Antes da coleta, a suíte funcional registrou **142 PASS, 0 FAIL e 0 SKIP**. Esses registros são evidências históricas e não foram executados novamente nesta revisão.

Ambiente, comandos, saída da suíte e hashes estão em `resultados/issue10_desempenho_ambiente.txt`; tabelas, estatísticas e interpretação estão em [analise_desempenho.md](analise_desempenho.md).

## Ambiente da coleta conjunta

A coleta ocorreu em Ubuntu 24.04 sobre WSL2/Docker, com oito CPUs lógicas expostas (Intel Core i7-4770S), GCC 13.3.0 e Open MPI 4.1.6. O launcher usado foi `mpirun --use-hwthread-cpus --bind-to hwthread`; não houve sobresubscrição nos benchmarks. MPI foi executado em uma única máquina, sem comunicação de rede entre nós.

As afinidades dos runtimes OpenMP e MPI não são idênticas. Portanto, diferenças observadas não podem ser atribuídas exclusivamente ao paradigma de programação.

## Metodologia executada

Na escalabilidade forte foram usados N=256, 512 e 1024, com p=1, 2, 4 e 8 para OpenMP/MPI e uma referência sequencial por dimensão. A série fraca usou N base 256 e `N(p)=round(256 p^(1/3))`. Cada configuração teve um aquecimento, seguido de cinco medições válidas; aquecimentos não entraram no CSV.

O tempo MPI começa após uma barreira e inclui `MPI_Scatterv`, `MPI_Bcast`, computação local, `MPI_Gatherv` e esperas, usando o máximo entre ranks. Alocação, inicialização, checksum, Python e partida do launcher ficam fora. Assim, o speedup MPI inclui os custos de comunicação do algoritmo, mas não toda a latência de invocação.

Para cada N e p, a análise usa a mediana como comparação principal e também preserva média, mínimo, máximo, desvio-padrão e coeficiente de variação:

```text
S(p) = mediana(T_sequencial) / mediana(T_MPI,p)
E(p) = S(p) / p
```

Nenhum outlier foi descartado. Checksums foram comparados à execução sequencial da mesma dimensão; eles complementam, mas não substituem, a comparação elemento a elemento da suíte funcional.

## Distribuição e limites

Para N linhas e p processos, cada rank recebe `floor(N/p)` linhas, e os primeiros `N mod p` ranks recebem uma linha adicional. A diferença de carga é de no máximo uma linha; se p>N, alguns ranks recebem zero linhas.

| N | p | Distribuição esperada |
| ---: | ---: | --- |
| 256 | 8 | 8 ranks × 32 linhas |
| 255 | 8 | 7 ranks × 32; 1 rank × 31 |
| 2 | 4 | 2 ranks × 1; 2 ranks × 0 |

A matriz B é replicada integralmente por rank, e o rank 0 mantém A, B e C completas. A coleta conjunta não registra separadamente os tempos de cada etapa MPI, portanto não quantifica quanto de uma regressão vem de comunicação, computação ou espera. Há alta dispersão em alguns grupos, especialmente MPI com oito processos.

## Reprodução

Para produzir uma nova coleta sem alterar os resultados históricos:

```bash
make collect-performance BENCHMARK_ARGS="--sizes 256 512 1024 \
  --workers 1 2 4 8 --repetitions 5 --weak-base 256 \
  --output resultados/nova_coleta.csv"
python3 scripts/analisar_desempenho.py
```

O coletor exige um caminho novo e cria o arquivo de ambiente correspondente. Resultados existentes não devem ser sobrescritos. O script específico `scripts/benchmark_mpi.sh` permanece disponível para uma coleta MPI com tempos por etapa; suas saídas devem ficar separadas da coleta conjunta.

## Pendências reais

- repetir MPI entre nós distintos para estudar custos de rede;
- coletar etapas MPI separadas em ambiente comparável para decompor comunicação e computação;
- repetir grupos de alta variabilidade com controle adicional de ruído e afinidade;
- executar nova regressão e nova coleta após qualquer mudança executável.

## Complementação das etapas MPI

`matmul_mpi.cpp` já mede `MPI_Scatterv`, `MPI_Bcast`, computação local,
`MPI_Gatherv` e total com `MPI_Wtime`; reduz os quatro tempos e o total com
`MPI_MAX`. `scripts/benchmark_mpi.sh` já extrai esses campos, realiza aquecimento,
exige repetições válidas, verifica checksum e grava carga mínima/máxima. Portanto,
não é necessária alteração do algoritmo MPI para obter a decomposição.

O CSV conjunto histórico não contém esses campos. Para obter evidência nova sem
sobrescrevê-lo, execute em Linux com GCC, Open MPI e recursos adequados:

```bash
DIMENSOES="256 512 1024" PROCESSOS="1 2 4 8" \
MPI_RUN_ARGS="--use-hwthread-cpus --bind-to hwthread" \
bash scripts/benchmark_mpi.sh 5 resultados/mpi_etapas_complementar.csv

python3 scripts/analisar_complementacao.py \
  --mpi resultados/mpi_etapas_complementar.csv \
  --expected-sizes 256 512 1024 \
  --expected-processes 1 2 4 8 --minimum-repetitions 5 \
  --output-dir resultados/complementacao_marco2
```

O analisador valida a presença das 12 configurações, ao menos cinco repetições
válidas, identificadores de repetição únicos e os extremos de carga esperados.
Ele produz `etapas_mpi_resumo.csv` e `configuracoes_mpi_resumo.csv`, com medianas,
speedup/eficiência relativos a p=1 e um proxy de comunicação explicitamente
marcado como não aditivo.

O resumo produzido contém medianas e razões etapa/tempo total para cada N e p.
Essas razões ajudam a comparar configurações, mas não formam uma decomposição
aditiva: o máximo de cada etapa pode pertencer a ranks diferentes, e uma coletiva
inclui espera causada por desbalanceamento anterior. Neste host não havia
`mpic++`, `mpirun`, Docker ou distribuição WSL, então a coleta complementar não
foi executada e o CSV histórico permaneceu intacto. A nova tentativa de
**10/11/2026** confirmou o mesmo bloqueio; o diagnóstico está em
`resultados/mpi_etapas_complementar_bloqueio_2026-11-10.txt`. Portanto, a análise
quantitativa por etapa continua pendente e nenhum gráfico de etapas foi gerado.
