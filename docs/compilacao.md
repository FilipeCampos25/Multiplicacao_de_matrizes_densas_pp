# Compilacao e execucao

O projeto usa C++ e grava todos os executaveis em `bin/`. O alvo padrao compila
somente as implementacoes que possuem fontes no repositorio.

## Alvos do Makefile

```sh
make          # compila todas as implementacoes disponiveis
make base     # compila o baseline sequencial com g++
make openmp   # compila as fontes OpenMP com g++ -fopenmp
make mpi      # compila as fontes MPI com mpic++
make clean    # remove somente o diretorio de artefatos bin/
```

Enquanto as implementacoes paralelas nao existirem, `make` compila o baseline e
informa que OpenMP e MPI foram ignorados. Os alvos explicitos `make openmp` e
`make mpi` encerram com uma mensagem de erro clara. Quando forem adicionados,
os arquivos `.cpp` devem ficar diretamente em `codigos/01_openmp/` e
`codigos/02_mpi/`, respectivamente; o Makefile os detectara automaticamente.

## Executar o baseline

Depois de compilar, informe a ordem `N` da matriz:

```sh
./bin/base/01_matmul 64
```

O programa imprime `N`, `checksum` e `tempo`. No ambiente Docker oficial, os
mesmos comandos podem ser executados no servico existente:

```sh
docker compose run --rm pp-lab make
docker compose run --rm pp-lab ./bin/base/01_matmul 64
```
