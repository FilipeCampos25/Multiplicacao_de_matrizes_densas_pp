# Multiplicação de matrizes densas — Programação Paralela

Projeto acadêmico da disciplina CCO085 (IESB 2026/2) para comparar a
multiplicação de matrizes densas sequencial, OpenMP e MPI. O ambiente Docker e
o baseline fornecidos pelo professor foram preservados.

## Implementações

- Sequencial: `codigos/baselines_sequenciais_CCO085/01_matmul.cpp`;
- OpenMP: `codigos/01_openmp/matmul_openmp.cpp`;
- MPI: `codigos/02_mpi/matmul_mpi.cpp`.

As três versões inicializam as matrizes da mesma forma, preservam a ordem de
laços `i-k-j` e imprimem `N`, `checksum` e `tempo`.

## Reprodução no ambiente oficial

Na raiz do repositório:

```bash
docker compose build
docker compose run --rm pp-lab
```

Dentro do contêiner, em `/lab`:

```bash
make clean
make base
make openmp
make mpi
make

./bin/base/01_matmul 512
OMP_NUM_THREADS=4 ./bin/openmp/matmul_openmp 512
mpirun -np 4 ./bin/mpi/matmul_mpi 512

make test
./scripts/medir_speedup.sh 512 5 "1 2 4 8" "1 2 4 8"
python3 scripts/analisar_resultados.py resultados/benchmarks_automatizados.csv
```

Se o Open MPI informar falta de slots, use `MPI_RUN_ARGS=--oversubscribe`
somente quando necessário e registre essa condição na apresentação; ela afeta
os tempos e não equivale a possuir oito núcleos disponíveis.

Os CSVs e metadados de ambiente ficam em `resultados/`. Resultados medidos em
hosts diferentes não devem ser combinados na mesma comparação.

## Documentação

- [Compilação e comandos](docs/compilacao.md)
- [Auditoria e checklist do Marco 2](docs/auditoria_marco2.md)
- [Roteiro da demonstração](docs/demonstracao.md)
- [Preparação para a arguição](docs/arguicao.md)
- [Perfilamento sequencial](docs/perfilamento_sequencial.md)
- [Autoria e contribuições](AUTORES.md)

O relatório de auditoria registra separadamente o que foi comprovado e o que
ainda depende de execução no ambiente oficial. O projeto não deve ser declarado
pronto enquanto os itens bloqueados desse relatório não forem concluídos.
