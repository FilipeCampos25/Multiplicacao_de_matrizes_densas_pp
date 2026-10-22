# Roteiro de demonstração técnica — até 9 minutos

Planejamento: 7min30s de conteúdo e até 1min30s de margem para comandos,
troca de tela e imprevistos. Substitua os campos `[DADO REAL]` somente depois
da coleta no ambiente oficial.

## 0:00–0:45 — Problema e objetivo

Multiplicar A e B densas para obter C exige aproximadamente `2N³` operações e
`O(N²)` memória. O objetivo é comparar o baseline com paralelismo em memória
compartilhada (OpenMP) e distribuída (MPI), mantendo a mesma entrada e ordem
de laços para uma comparação controlada.

## 0:45–1:30 — Baseline sequencial

Mostrar o laço `i-k-j`: para cada linha i, reutiliza-se `A[i,k]` e percorre-se
B e C contiguamente em j. Explicar que inicialização e checksum ficam fora do
tempo. Executar:

```bash
./bin/base/01_matmul 512
```

## 1:30–2:30 — OpenMP

Mostrar `parallel`, `single` e `for schedule(static)`. Cada iteração de i
corresponde a uma linha exclusiva de C; A e B são somente leitura, então não há
corrida. A divisão estática tem baixo overhead e carga uniforme por linha.

```bash
OMP_NUM_THREADS=4 ./bin/openmp/matmul_openmp 512 --verify
```

## 2:30–3:45 — MPI

Explicar que o rank 0 cria A e B; `Scatterv` distribui linhas de A, `Bcast`
replica B, cada rank calcula suas linhas e `Gatherv` reúne C. `counts` e
`displacements` tratam N não divisível por P; os primeiros `N mod P` ranks
recebem uma linha extra.

```bash
mpirun -np 4 ./bin/mpi/matmul_mpi 7 --verify
```

Apontar `carga_linhas min/max` e os tempos máximos por etapa.

## 3:45–4:35 — Validação

Explicar que a mesma inicialização determinística permite comparar checksums.
Além disso, `--verify` recalcula cada elemento esperado e aplica tolerância
`1e-12(1+|esperado|)`. Mostrar rapidamente:

```bash
make test
```

Não esperar toda a suíte se o ensaio indicar que ultrapassa a margem; nesse
caso, apresentar uma saída já registrada e executar apenas N=7 ao vivo.

## 4:35–6:10 — Resultados

Mostrar os gráficos reais de tempo, speedup e eficiência. Declarar N,
repetições, mediana, CPU, núcleos, compilador e eventual sobresubscrição.

- OpenMP: `[DADO REAL: melhor configuração e limite observado]`.
- MPI: `[DADO REAL: melhor configuração e proporção comunicação/computação]`.
- Speedup: `S(p)=Tseq/Tpar`.
- Eficiência: `E(p)=S(p)/p`.
- Variabilidade: `[DADO REAL: min–máx ou desvio-padrão]`.

Não usar o CSV sequencial antigo como se fosse comparável às novas execuções.

## 6:10–7:00 — Escalabilidade e Amdahl

Escalabilidade forte mantém N e aumenta recursos; a eficiência normalmente cai
quando overhead, comunicação e banda de memória dominam. Escalabilidade fraca
aumenta o problema para manter trabalho aproximado por recurso. Pela Lei de
Amdahl, com fração paralelizável f, o limite é
`S(p)=1/((1-f)+f/p)` e `S(infinito)=1/(1-f)`. Inserir somente a estimativa de f
obtida dos dados: `[DADO REAL]`.

## 7:00–7:30 — Conclusão

OpenMP é direto em um nó e compartilha as matrizes; MPI explicita distribuição
e comunicação e pode escalar entre nós. O ganho depende de N ser grande o
suficiente para amortizar criação de threads/coletivas e dos limites de cache,
memória e hardware. Encerrar com as lacunas honestamente documentadas.

## Plano de contingência

- Deixar o contêiner aberto, binários compilados e comandos no histórico.
- Usar N=7 para corretude e N=512 apenas se o ensaio confirmar tempo seguro.
- Ter capturas/saídas reais de uma execução anterior, com data e ambiente.
- Se 8 ranks falharem por slots, não improvisar: usar 4 ou declarar a
  sobresubscrição e seu impacto.
- Nunca editar CSV, trocar gráfico ou ajustar thread/processo durante a fala.
