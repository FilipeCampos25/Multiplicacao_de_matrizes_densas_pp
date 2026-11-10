# Roteiro reproduzível da demonstração — até 9 minutos

Este roteiro pressupõe Linux ou o contêiner do projeto, com `make`, GCC, OpenMP
e Open MPI. Antes da apresentação, executar a sequência completa em uma cópia
limpa e guardar a saída. A coleta de 22/10/2026 é histórica: o baseline atual
mantém o hash registrado, mas Makefile e fontes OpenMP/MPI mudaram depois dela.

## Preparação (antes do cronômetro)

```bash
docker compose build
docker compose run --rm pp-lab
make clean && make all
make test-correctness
```

Resultado exigido para prosseguir: compilação sem erro e a suíte corrente sem
falhas. Se isso não acontecer, declarar a falha; não usar o registro histórico
de `142 PASS` como se fosse a execução atual. Deixar abertos
`resultados/graficos/speedup_cae72fda_N1024.svg` e
`resultados/issue10_desempenho_ambiente.txt`.

## 0:00–0:45 — problema e origem

Apresentar multiplicação densa `C=A×B`, custo aproximado `2N³` e memória
`O(N²)`. Identificar
`codigos/00_sequencial/01_matmul.cpp` como baseline fornecido
pelo professor e separar esse material das adaptações do grupo.

## 0:45–1:35 — baseline ao vivo

```bash
./bin/base/01_matmul 256
```

Mostrar `N`, tempo e checksum realmente impressos. Para a entrada determinística
de N=256, a evidência histórica registra checksum `4109165.296`; conferir a
saída atual em vez de prometer um tempo, que depende da máquina.

## 1:35–3:00 — OpenMP com duas equipes

Explicar que A e B são somente leitura e cada thread escreve em linhas distintas
de C. Executar o mesmo problema com equipes diferentes:

```bash
OMP_NUM_THREADS=1 ./bin/openmp/matmul_openmp 256 --verify
OMP_NUM_THREADS=4 ./bin/openmp/matmul_openmp 256 --verify
```

Apontar a equipe efetivamente usada, o checksum e o resultado de `--verify`.
Não prometer que quatro threads serão mais rápidas em toda máquina ou execução.

## 3:00–4:30 — MPI com dois números de processos

Explicar `Scatterv` de A, `Bcast` de B, cálculo local e `Gatherv` de C. N=7
torna visível a divisão irregular e permite a comparação rápida:

```bash
mpirun -np 1 ./bin/mpi/matmul_mpi 7 --verify
mpirun -np 4 ./bin/mpi/matmul_mpi 7 --verify
```

Mostrar checksum, verificação, carga mínima/máxima e tempos por etapa. Se o
launcher recusar slots, reduzir para dois processos e registrar a limitação;
não habilitar sobresubscrição silenciosamente.

## 4:30–5:30 — comparação de corretude

```bash
MATMUL_DUMP=1 ./bin/base/01_matmul 3
MATMUL_DUMP=1 OMP_NUM_THREADS=4 ./bin/openmp/matmul_openmp 3 --verify
MATMUL_DUMP=1 mpirun -np 4 ./bin/mpi/matmul_mpi 3 --verify
```

Comparar os nove elementos e o checksum. Explicar que checksum é triagem; a
metodologia em `docs/testes_corretude.md` também compara elementos para casos
pequenos, com tolerância numérica.

## 5:30–7:15 — speedup medido

Abrir `resultados/graficos/speedup_cae72fda_N1024.svg` e declarar:

- dados da coleta conjunta de 22/10/2026, não da execução ao vivo;
- cinco repetições válidas por configuração e mediana como medida principal;
- Intel i7-4770S, oito CPUs lógicas expostas, GCC 13.3.0 e Open MPI 4.1.6;
- `S(p)=Tseq/Tpar(p)` e `E(p)=S(p)/p`;
- a análise registrada informa speedup OpenMP de 3,805 em N=1024/p=8 e
  regressão MPI de p=4 para p=8 em N=512/1024;
- afinidades OpenMP/MPI não eram idênticas e o MPI rodou em uma única máquina.

Não apresentar o gráfico como validação dos fontes atuais: os hashes OpenMP/MPI
registrados em 22/10 diferem dos hashes atuais.

## 7:15–8:20 — escalabilidade e Amdahl

Forte: N fixo e mais recursos. Fraca: trabalho aproximadamente constante por
recurso, usando `N(p)=round(256·p^(1/3))`. Mostrar que ambas existem nos CSVs,
mas a série fraca apresentou crescimento de tempo.

Para Amdahl, usar `S(p)=1/((1-f)+f/p)`. Os gráficos de Amdahl são cenários
hipotéticos para frações seriais de 5%, 10% e 20%; não são ajuste nem estimativa
da fração serial real.

## 8:20–9:00 — conclusão e contingência

Concluir que OpenMP compartilha memória e MPI explicita distribuição e
comunicação; ganhos dependem de tamanho, overhead, cache, afinidade e hardware.
Se a validação corrente não tiver sido concluída, encerrar com essa pendência.
Manter capturas reais como contingência, mas identificá-las com data, commit e
ambiente; nunca editar CSVs durante a apresentação.
