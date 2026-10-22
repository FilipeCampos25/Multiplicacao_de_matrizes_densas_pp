# Avaliacao de desempenho e escalabilidade MPI

## Estado da avaliacao

Este documento prepara a coleta exigida pela Issue #9, mas **nao apresenta
resultados experimentais**. Em 21 de outubro de 2026, o host disponivel nao
possuia compilador C/C++, implementacao/launcher MPI, Docker nem distribuicao
WSL. Portanto, nao foi possivel compilar, validar ou executar a implementacao
nesse ambiente. Nenhum tempo ou CSV de resultados foi inventado.

O host inspecionado tinha:

- Windows 10 Home, build 26200;
- AMD Ryzen 5 4500, 6 nucleos e 12 processadores logicos;
- 17.042.706.432 bytes de memoria fisica (aproximadamente 15,9 GiB);
- `g++`, `make`, `mpic++`, `mpirun`/`mpiexec` e `docker` indisponiveis;
- WSL habilitado, mas sem distribuicao instalada.

Como as medicoes devem ocorrer no ambiente oficial do projeto, esses dados do
host servem apenas para registrar a limitacao e nao constituem um ambiente de
benchmark.

## Metodologia preparada

O script `scripts/benchmark_mpi.sh` compila e executa primeiro a validacao da
Issue #5. Apenas depois de ela passar, coleta escalabilidade forte para
N = 255, 256, 512 e 1024 e p = 1, 2, 4 e 8. N = 255 cobre distribuicoes nao
divisiveis; os outros tres tamanhos sao os candidatos iniciais da tarefa. Cada
configuracao recebe uma execucao de aquecimento e pelo menos cinco medicoes
validas. Uma configuracao admite, por padrao, ate tres tentativas extras; erros
sao registrados no CSV e nao contam como medicao valida.

O baseline sequencial e medido uma vez para cada N (aquecimento mais cinco
repeticoes), pois p nao se aplica a ele. Todas as execucoes MPI sao comparadas
ao checksum desse baseline. O script tambem grava CPU, memoria, sistema,
compiladores, launcher e parametros em um arquivo de ambiente ao lado do CSV.

Comando padrao, a partir da raiz do repositorio em Linux ou no Dev Container:

```bash
chmod +x scripts/benchmark_mpi.sh
./scripts/benchmark_mpi.sh 5 resultados/mpi_escalabilidade.csv
```

Se o Open MPI recusar 8 processos por falta de slots, a execucao pode ser
adaptada sem alterar o script, desde que a sobresubscricao e sua consequencia
sejam documentadas:

```bash
MPI_RUN_ARGS=--oversubscribe ./scripts/benchmark_mpi.sh
```

Sobresubscricao nao equivale a dispor de oito nucleos fisicos e pode piorar os
tempos; por isso, resultados obtidos assim devem ser identificados pelo arquivo
`mpi_escalabilidade_ambiente.txt`. Dimensoes e processos tambem podem ser
ajustados, por exemplo:

```bash
DIMENSOES="255 512 1024" PROCESSOS="1 2 4 8" ./scripts/benchmark_mpi.sh
```

O caso N = 255 complementa a coleta ao expor distribuicoes nao divisiveis por
2, 4 e 8. Ele deve ser coletado no mesmo ambiente e na mesma sessao se for
usado em comparacoes de desempenho.

## Formato dos dados

O CSV preserva os campos essenciais `implementacao`, `n`, `processos`,
`repeticao`, `tempo_s` e `checksum`. Acrescenta `tentativa`, os quatro tempos
de etapa, limites de linhas, `status` e `erro`. Linhas validas usam `status=ok`;
falhas deixam as metricas vazias e guardam a saida do processo em `erro`.

O script da Issue #7 existente no repositorio mede um programa OpenMP de pi,
usa colunas `threads,repeticao,tempo_s` e nao conhece executaveis ou coletivas
MPI. Para evitar misturar responsabilidades, ele nao foi alterado. O novo CSV
mantem `repeticao` e `tempo_s`, mas explicita implementacao, N e processos,
como requerido pela Issue #9 e pela futura consolidacao da Issue #10.

## Interpretacao dos tempos

O tempo sequencial envolve somente o nucleo de multiplicacao: inicializacao e
checksum ficam fora da regiao medida. O tempo MPI comeca depois da
inicializacao e de uma barreira, e termina apos `MPI_Gatherv`; portanto inclui
`MPI_Scatterv`, `MPI_Bcast`, computacao local, `MPI_Gatherv` e esperas entre
processos. Alocacao, inicializacao, checksum e verificacao ficam fora. Os dois
tempos sao uteis para speedup, mas nao sao perfeitamente equivalentes: apenas
o MPI paga distribuicao e comunicacao dentro da regiao medida.

Cada processo mede as quatro etapas com `MPI_Wtime()`. Uma reducao com
`MPI_MAX` registra o maior tempo observado entre os ranks, isto e, o caminho
critico de cada etapa. O tempo total tambem e o maximo entre ranks. Tempos de
coletivas podem incorporar espera por ranks atrasados; por isso, a soma dos
maximos das etapas nao deve ser tratada como decomposicao exata do total.

Para cada N e p, a analise deve usar pelo menos mediana e variacao (minimo,
maximo e, preferencialmente, desvio-padrao). Com a mediana dos tempos:

```text
S(p) = mediana(T_sequencial) / mediana(T_MPI,p)
E(p) = S(p) / p
```

Um aumento de p que eleve o tempo deve ser preservado e discutido, nao
descartado. Para matrizes pequenas, latencia de coletivas, copia de dados e
sobresubscricao podem superar a reducao da computacao local.

## Balanceamento de linhas

Para N linhas e p processos, cada rank recebe `floor(N/p)` linhas e os
primeiros `N mod p` ranks recebem uma linha adicional. Assim, a diferenca entre
o maior e o menor bloco e no maximo uma linha. Se p > N, alguns ranks recebem
zero linhas; a implementacao fornece buffers validos com contagem zero.

Exemplos a confirmar pela saida instrumentada:

| N | p | distribuicao esperada | diferenca |
|---:|---:|---|---:|
| 256 | 8 | 8 ranks x 32 linhas | 0 |
| 255 | 8 | 7 ranks x 32; 1 rank x 31 | 1 |
| 2 | 4 | 2 ranks x 1; 2 ranks x 0 | 1 |

Embora a quantidade de linhas seja equilibrada, tempos diferentes de
computacao podem surgir por interferencia do sistema, afinidade, hierarquia de
memoria ou ranks sem nucleo fisico exclusivo.

## Escalabilidade fraca

A coleta preparada prioriza escalabilidade forte. Uma avaliacao fraca e
opcional e deve ser feita separadamente. Como o trabalho e O(N^3), uma serie
aproximada parte de N0 e usa `N(p) = round(N0 * cubert(p))`. Por exemplo, com
N0 = 256, os valores arredondados seriam 256, 323, 406 e 512 para p = 1, 2, 4
e 8. O arredondamento altera levemente o trabalho por processo e deve ser
registrado; essa serie nao deve ser misturada ao CSV de escalabilidade forte
sem uma coluna que identifique o tipo de experimento.

## Checklist da Issue #9

- [ ] Compilar e validar a implementacao no ambiente oficial.
- [ ] Executar p = 1, 2, 4 e 8 conforme os recursos disponiveis.
- [ ] Medir pelo menos tres dimensoes com aquecimento e cinco repeticoes.
- [ ] Produzir e preservar `resultados/mpi_escalabilidade.csv` e o arquivo de ambiente.
- [ ] Calcular tabela resumida, speedup, eficiencia e variacao com dados reais.
- [ ] Interpretar os custos de `Scatterv`, `Bcast`, computacao e `Gatherv`.
- [ ] Confirmar casos divisiveis, nao divisiveis e com processos sem linhas.
- [x] Preparar instrumentacao por etapa, agregada pelo maximo entre ranks.
- [x] Preparar coleta de erros, checksums e metadados do ambiente.
- [x] Documentar a diferenca metodologica entre baseline e MPI.
- [x] Registrar a limitacao do ambiente atual sem inventar resultados.

A Issue #9 nao deve ser considerada concluida ate os itens de execucao e
analise baseados em dados reais serem preenchidos.
