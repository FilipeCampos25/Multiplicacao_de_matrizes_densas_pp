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

## Executar a multiplicacao com OpenMP

O Makefile detecta `codigos/01_openmp/01_matmul_openmp.cpp` e compila com
C++17 e `-fopenmp`. No Linux ou no terminal do ambiente Docker:

```sh
make base
make openmp
./bin/openmp/01_matmul_openmp           # N = 512; threads definidas pelo runtime
./bin/openmp/01_matmul_openmp 64
./bin/openmp/01_matmul_openmp 256 4     # N = 256; solicita 4 threads
OMP_NUM_THREADS=2 ./bin/openmp/01_matmul_openmp 128
OMP_NUM_THREADS=2 ./bin/openmp/01_matmul_openmp 128 4  # argumento tem prioridade
```

Os argumentos devem ser inteiros decimais positivos. Valores invalidos, argumentos
extras e dimensoes que causariam overflow nos indices `int` ou na expressao
`i * 7` da inicializacao sao rejeitados com mensagem em stderr e retorno diferente
de zero. A memoria disponivel tambem limita o tamanho das matrizes.

A saida preserva os campos `N`, `checksum` e `tempo` e acrescenta
`threads_configuradas`, obtido com `omp_get_max_threads()` depois da configuracao
opcional por argumento. Esse campo informa o limite configurado para a regiao;
nao mede a quantidade efetiva de threads. Ajustes do runtime, como `OMP_DYNAMIC`
e `OMP_THREAD_LIMIT`, podem reduzir a equipe utilizada. Sem o segundo argumento,
o programa respeita a configuracao do runtime, incluindo `OMP_NUM_THREADS`.

O laço externo `i` usa `parallel for schedule(static) default(none)`, com A, B, C
e N explicitamente compartilhados. Cada thread atualiza linhas exclusivas de C;
os indices de iteracao e o escalar `a` sao privados pelo escopo ou pelas regras
do OpenMP. A ordem `i-k-j` e a inicializacao deterministica sao preservadas.
Como cada linha exige a mesma quantidade de operacoes, `schedule(static)` tende
a equilibrar o trabalho sem a distribuicao dinamica de iteracoes. Isso justifica
a escolha inicial, sem representar um ganho de desempenho medido.

A medicao com `clock_gettime(CLOCK_MONOTONIC)` envolve apenas a multiplicacao,
incluindo os custos de entrada na regiao paralela e de sincronizacao. A barreira
implicita garante que todas as threads terminaram antes do instante final.
Inicializacao e checksum ficam fora do intervalo; a soma de C continua sequencial,
na mesma ordem do baseline.

Para comparar os checksums nas dimensoes da Issue #4, execute:

```sh
for n in 1 2 8 64 128 256; do
    ./bin/base/01_matmul "$n"
    for t in 1 2 4; do
        ./bin/openmp/01_matmul_openmp "$n" "$t"
    done
done
```

Compare o campo `checksum` de cada execucao com o baseline da mesma dimensao.
Prefira igualdade quando obtida; qualquer diferenca deve ser investigada antes
de aceitar tolerancias absolutas e relativas justificadas. A igualdade da soma
e uma verificacao de consistencia, nao uma prova de igualdade de todos os elementos.
Para conferir a configuracao de threads, use tambem os exemplos com
`OMP_NUM_THREADS` acima e verifique `threads_configuradas`.

Pelo servico Docker existente, os comandos equivalentes incluem:

```sh
docker compose run --rm pp-lab make base
docker compose run --rm pp-lab make openmp
docker compose run --rm -e OMP_NUM_THREADS=4 pp-lab ./bin/openmp/01_matmul_openmp 128
docker compose run --rm pp-lab ./bin/openmp/01_matmul_openmp 128 2
```
