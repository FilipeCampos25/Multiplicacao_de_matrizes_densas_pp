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
make test-correctness
python3 -m unittest discover -s tests -v
OMP_NUM_THREADS=4 ./bin/openmp/matmul_openmp 7 --verify
mpirun -np 4 ./bin/mpi/matmul_mpi 7 --verify
MATMUL_DUMP=1 OMP_NUM_THREADS=2 ./bin/openmp/matmul_openmp 3
MATMUL_DUMP=1 mpirun -np 3 ./bin/mpi/matmul_mpi 3
```

`make test` compara os checksums das três versões para diferentes dimensões,
threads e processos. Os casos MPI incluem `N=7, p=4`, `N=10, p=3` e `N=2,
p=4`, cobrindo divisão irregular e processos que recebem zero linhas. A
verificação elemento a elemento usa erro absoluto/relativo combinado de
`1e-12 * (1 + |esperado|)`; a comparação dos checksums impressos usa
`1e-9 * (1 + |baseline|)` porque a saída tem seis casas decimais.

O programa aceita somente matrizes quadradas N×N geradas internamente. Não há
uma API de dimensões independentes. Zero, negativos, texto não numérico,
argumentos extras e dimensões fora dos limites válidos devem produzir retorno
não zero; o baseline original não é alterado para incorporar essas proteções.

## Coletar e analisar benchmarks

```bash
./scripts/medir_speedup.sh 256 5 "1 2 4 8" "1 2 4 8" resultados/benchmark_256.csv
./scripts/medir_speedup.sh 512 5 "1 2 4 8" "1 2 4 8" resultados/benchmark_512.csv
./scripts/medir_speedup.sh 1024 5 "1 2 4 8" "1 2 4 8" resultados/benchmark_1024.csv
python3 scripts/analisar_resultados.py resultados/benchmark_512.csv
```

Para as variantes OpenMP da mesma fonte:

```bash
make benchmark-openmp-build
make benchmark-openmp \
  BENCHMARK_ARGS="--sizes 256 512 1024 --threads 1 2 4 8 \
  --schedules static dynamic guided \
  --output resultados/openmp_nova_desempenho.csv"
```

Escolha sempre um arquivo novo. O coletor recusa sobrescrever saídas existentes;
os CSVs históricos não devem ser editados ou substituídos.

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

O laço externo `i` usa uma região `parallel` e um `for schedule(static)`, com A,
B, C e N explicitamente compartilhados. Cada thread atualiza linhas exclusivas de C;
os indices de iteracao e o escalar `a` sao privados pelo escopo ou pelas regras
do OpenMP. A ordem `i-k-j` e a inicializacao deterministica sao preservadas.
Como cada linha exige a mesma quantidade de operacoes, `schedule(static)` tende
a equilibrar o trabalho sem a distribuicao dinamica de iteracoes. Isso justifica
a escolha inicial, sem representar um ganho de desempenho medido.

A medição com `omp_get_wtime()` segue o exemplo oficial
`005_loop_for_paralell_time.cpp` e envolve apenas a multiplicação,
incluindo os custos de entrada na regiao paralela e de sincronizacao. A barreira
implicita garante que todas as threads terminaram antes do instante final.
Inicializacao e checksum ficam fora do intervalo; a soma de C continua sequencial,
na mesma ordem do baseline.

A versão de benchmark registra a equipe efetiva com `omp_get_num_threads()` em
uma região `single`. A versão didática preserva também `threads_configuradas`,
obtido com `omp_get_max_threads()`. Não há cronômetros individuais por thread,
pois depois da barreira eles incluiriam espera e acrescentariam overhead.

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

## Estado da última validação

Passaram 8 testes de análise, 6 testes do coletor OpenMP e 10 testes do detector
de corretude. Os comandos `make clean`, `make all`, `make test`,
`make test-correctness`, a descoberta completa, o build das variantes e o smoke
benchmark permaneceram bloqueados no host disponível por ausência de Make,
compiladores, MPI, Docker e WSL. Esses comandos ainda precisam terminar com
sucesso no ambiente oficial; casos bloqueados não são considerados aprovados.
