# Roteiro de demonstração técnica — até 9 minutos

Planejamento: 7min30s de conteúdo e até 1min30s de margem para comandos, troca
de tela e imprevistos. Use somente resultados registrados com ambiente e
metodologia; não complete lacunas com estimativas.

## 0:00–0:45 — Problema e objetivo

Multiplicar A e B densas para obter C exige aproximadamente `2N³` operações e
`O(N²)` memória. O objetivo é comparar o baseline com paralelismo em memória
compartilhada (OpenMP) e distribuída (MPI), mantendo a mesma entrada e ordem de
laços.

## 0:45–1:30 — Baseline sequencial

Identificar `01_matmul.cpp` como código original do Prof. Rodrigo Gonçalves
Pinto. Mostrar o laço `i-k-j`: para cada linha i, reutiliza-se `A[i,k]` e
percorrem-se B e C contiguamente em j. Inicialização e checksum ficam fora do
tempo.

```bash
./bin/base/01_matmul 512
```

## 1:30–2:30 — OpenMP

Mostrar `parallel`, `single` e `for schedule(static)`. Cada iteração de i
corresponde a uma linha exclusiva de C; A e B são somente leitura, portanto não
há corrida. Relacionar a adaptação aos exemplos oficiais 004 (distribuição do
`for`) e 005 (`omp_get_wtime()` e equipe efetiva). As proteções, testes,
scripts e automações são contribuições do grupo.

```bash
OMP_NUM_THREADS=4 ./bin/openmp/matmul_openmp 512 --verify
```

## 2:30–3:45 — MPI

Explicar que o rank 0 cria A e B; `Scatterv` distribui linhas de A, `Bcast`
replica B, cada rank calcula suas linhas em `i-k-j` e `Gatherv` reúne C.
`counts` e `displacements` tratam N não divisível por P; os primeiros
`N mod P` ranks recebem uma linha extra e, se `P>N`, alguns recebem zero.

```bash
mpirun -np 4 ./bin/mpi/matmul_mpi 7 --verify
```

Apontar `carga_linhas min/max` e os tempos máximos por etapa.

## 3:45–4:35 — Validação

A mesma inicialização determinística permite comparar checksums. Como checksum
não prova igualdade, `--verify` recalcula cada elemento e `MATMUL_DUMP=1`
permite comparação externa para N pequeno.

```bash
make test
make test-correctness
```

Apresentar o registro histórico de 22/10/2026 como tal: 142 PASS, 0 FAIL e 0
SKIP. Não afirmar que a suíte foi reexecutada na versão corrente sem um novo
registro de ambiente.

## 4:35–6:10 — Resultados

Mostrar apenas gráficos reais. Declarar N, repetições, mediana, CPU, compilador,
afinidade e eventual sobresubscrição.

- OpenMP: usar as tabelas e ressalvas de `docs/avaliacao_openmp.md`;
- MPI: usar a coleta conjunta comparável de 22/10/2026, com suas limitações;
- speedup: `S(p)=Tseq/Tpar`;
- eficiência: `E(p)=S(p)/p`;
- variabilidade: apresentar mínimo, máximo, desvio-padrão ou CV reais.

Não usar o CSV sequencial antigo como se fosse diretamente comparável a uma
coleta feita com fonte, flags ou ambiente diferentes.

Os 175 registros conjuntos estão em `resultados/issue10_desempenho.csv`; o
ambiente em `resultados/issue10_desempenho_ambiente.txt`; os 21 SVGs em
`resultados/graficos/`; e a interpretação em
[analise_desempenho.md](analise_desempenho.md).

## 6:10–7:00 — Escalabilidade e Amdahl

Escalabilidade forte mantém N e aumenta recursos; a fraca aumenta o problema
para manter trabalho aproximado por recurso. Pela Lei de Amdahl,
`S(p)=1/((1-f)+f/p)` e `S(infinito)=1/(1-f)`. Só apresentar uma estimativa de
f se ela tiver sido calculada a partir de dados comparáveis e identificados.

## 7:00–7:30 — Conclusão

OpenMP compartilha as matrizes em um nó; MPI explicita distribuição e
comunicação. Ganhos dependem do tamanho, overhead, cache, memória e hardware.
Encerrar distinguindo resultados medidos, hipóteses e pendências.

## Plano de contingência

- Deixar o contêiner aberto, binários compilados e comandos no histórico;
- usar N=7 para corretude e N=512 somente após ensaio;
- manter capturas reais com data e ambiente;
- se ranks excederem os slots, reduzir P ou declarar sobresubscrição;
- nunca editar CSV ou trocar configuração durante a apresentação;
- declarar qualquer regressão bloqueada em vez de tratá-la como aprovação.
