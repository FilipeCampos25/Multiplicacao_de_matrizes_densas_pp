# Compilação, execução e validação

Todos os comandos abaixo devem ser executados na raiz do repositório. O
ambiente oficial é o serviço `pp-lab` definido no `docker-compose.yml`.

## Entrar no ambiente

```bash
docker compose build
docker compose run --rm pp-lab
```

O volume do repositório é montado em `/lab`, que já é o diretório de trabalho.
Como alternativa, abra a pasta no VS Code e escolha **Reopen in Container**.

## Compilar

```bash
make clean
make base
make openmp
make mpi
make
```

Os executáveis são gerados somente em `bin/`. O alvo `make` compila todas as
fontes disponíveis e `make clean` remove somente esse diretório.

## Executar

```bash
./bin/base/01_matmul 512
OMP_NUM_THREADS=1 ./bin/openmp/matmul_openmp 512
OMP_NUM_THREADS=2 ./bin/openmp/matmul_openmp 512
OMP_NUM_THREADS=4 ./bin/openmp/matmul_openmp 512
mpirun -np 1 ./bin/mpi/matmul_mpi 512
mpirun -np 2 ./bin/mpi/matmul_mpi 512
mpirun -np 4 ./bin/mpi/matmul_mpi 512
```

## Verificar corretude

```bash
make test
OMP_NUM_THREADS=4 ./bin/openmp/matmul_openmp 7 --verify
mpirun -np 4 ./bin/mpi/matmul_mpi 7 --verify
```

`make test` compara os checksums das três versões para diferentes dimensões,
threads e processos. Os casos MPI incluem `N=7, p=4`, `N=10, p=3` e `N=2,
p=4`, cobrindo divisão irregular e processos que recebem zero linhas. A
verificação elemento a elemento usa erro absoluto/relativo combinado de
`1e-12 * (1 + |esperado|)`; a comparação dos checksums impressos usa
`1e-9 * (1 + |baseline|)` porque a saída tem seis casas decimais.

## Coletar e analisar benchmarks

```bash
./scripts/medir_speedup.sh 256 5 "1 2 4 8" "1 2 4 8" resultados/benchmark_256.csv
./scripts/medir_speedup.sh 512 5 "1 2 4 8" "1 2 4 8" resultados/benchmark_512.csv
./scripts/medir_speedup.sh 1024 5 "1 2 4 8" "1 2 4 8" resultados/benchmark_1024.csv
python3 scripts/analisar_resultados.py resultados/benchmark_512.csv
```

Cada coleta faz um aquecimento, registra cinco repetições e salva um arquivo
`*_ambiente.txt`. O analisador calcula mediana, mínimo, máximo, desvio-padrão,
speedup `Tseq/Tpar` e eficiência `speedup/p`. Não misture execuções de máquinas
ou sessões diferentes. Os CSVs e eventuais gráficos produzidos a partir deles
devem permanecer em `resultados/`.
