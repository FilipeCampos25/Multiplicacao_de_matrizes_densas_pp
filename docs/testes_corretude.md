# Testes de corretude — Issue #6

## Objetivo e implementações

A suíte `tests/test_correctness.py` usa apenas a biblioteca padrão do Python 3
(`unittest`, `subprocess` e auxiliares). Executa os binários reais, captura stdout
e stderr, valida os campos de saída e compara os resultados com o executável
sequencial. Nenhum checksum esperado dos programas é fixado no script.

| Implementação | Executável | Interface testada |
| --- | --- | --- |
| Sequencial | `bin/base/01_matmul` | `N` |
| OpenMP da Issue #4 | `bin/openmp/01_matmul_openmp` | `N [threads]` ou `OMP_NUM_THREADS` |
| OpenMP com verificação interna | `bin/openmp/matmul_openmp` | `N`, com `OMP_NUM_THREADS` |
| MPI | `bin/mpi/matmul_mpi` | `mpirun -np P ... N` |

As duas versões OpenMP existem no repositório e são testadas. A versão MPI também
está implementada. Todos os programas utilizam a inicialização determinística
`A[i] = (i % 100) * 0.01` e `B[i] = ((i * 7) % 100) * 0.01`.
O baseline, os cálculos das versões paralelas e os testes shell anteriores são
preservados. O novo alvo não substitui `make test`.

## Casos e referências

As dimensões principais são **1, 2, 3, 5, 8, 16, 32 e 64**. Cada versão OpenMP
é executada com **1, 2, 3 e 4 threads** por `OMP_NUM_THREADS`. A versão
`01_matmul_openmp` também é executada com cada uma dessas configurações pelo
argumento opcional, usando um valor diferente no ambiente para testar a prioridade.
O script verifica `threads_configuradas` nessa versão e `threads` (equipe efetiva)
em `matmul_openmp`. Não confunde a configuração com a medição da equipe efetiva.

MPI é executado com **1, 2, 3 e 4 processos** para cada dimensão principal.
Também são incluídos os casos abaixo, sem duplicar os já cobertos:

| N | Processos | Distribuição |
| --- | --- | --- |
| 4 | 2 | Regular |
| 5 | 2 | Irregular |
| 7 | 3 | Irregular |
| 10 | 4 | Irregular |
| 2 | 4 | Processos sem linhas |
| 8 | 4 | Regular |

São **142 casos funcionais** quando todas as fontes estão presentes: 11 do
baseline, 96 OpenMP e 35 MPI. O baseline é executado uma vez por dimensão e seu
checksum é reutilizado nas comparações diretas com as implementações paralelas.
As dimensões 4, 7 e 10 acrescentam referências para os casos especiais MPI.
O baseline recebe somente entradas positivas pequenas; seus argumentos inválidos
não são exercitados, pois o programa original não os valida.

Para **N até 10**, uma referência Python complementar calcula C em um vetor
row-major, com a mesma inicialização e ordem de laços `i-k-j`. Sua soma sequencial
é comparada com o checksum do baseline; cada elemento das matrizes paralelas é
comparado com o valor Python da mesma linha e coluna. Isso permite detectar
permutação, omissão ou duplicação de linhas mesmo quando a soma não muda.
A referência Python não substitui a comparação com o executável sequencial.

MPI exporta a matriz global após `MPI_Gatherv`, somente no rank 0. O teste exige
exatamente N² valores, na ordem correta, e uma única linha de checksum. Também
confere os mínimos e máximos de linhas relatados em `carga_linhas` com a divisão
esperada para N e P, incluindo processos que recebem zero linhas.

## Exportação opcional de C

As três fontes paralelas aceitam a variável **`MATMUL_DUMP=1`**. Quando ativada,
acrescentam uma linha `C=` seguida dos elementos em row-major, com 17 algarismos
significativos (`%.17g`). A posição linear `i*N + j` identifica C[i,j].

```bash
MATMUL_DUMP=1 OMP_NUM_THREADS=2 ./bin/openmp/matmul_openmp 3
MATMUL_DUMP=1 ./bin/openmp/01_matmul_openmp 3 2
MATMUL_DUMP=1 mpirun -np 3 ./bin/mpi/matmul_mpi 7
```

A exportação ocorre após o cálculo e a medição, sem alterar a operação matemática,
os argumentos ou a saída padrão quando a variável está ausente. O baseline não
foi modificado. Os mecanismos `--verify` já existentes continuam disponíveis;
a suíte nova compara diretamente os elementos exportados com sua referência.
Nos testes maiores, sem exportação, a suíte também exige a ausência de `C=`.

## Tolerâncias e limites numéricos

O critério usado é:

```text
abs(esperado - obtido) <= atol + rtol * max(abs(esperado), abs(obtido))
```

| Comparação | atol | rtol |
| --- | --- | --- |
| Checksum | `1e-6` | `1e-9` |
| Elementos | `1e-10` | `1e-9` |

O checksum é impresso com seis casas decimais; `atol=1e-6` considera o
arredondamento dessa saída. O termo relativo acompanha a magnitude da soma.
Os elementos exportados têm precisão maior e usam tolerância absoluta menor.
NaN e infinitos são sempre rejeitados, inclusive se aparecerem dos dois lados.
As mensagens de comparação mostram esperado, obtido, diferença e tolerância.

O checksum sozinho não prova igualdade entre matrizes, pois erros podem se
compensar. Para N acima de 10, esta suíte verifica apenas checksum e metadados;
não declara comparação elemento a elemento nesses tamanhos. O baseline não
exporta C e é validado contra a referência Python somente pela soma em N até 10.

## Execução e ambiente

Na raiz do repositório, em Linux ou no terminal do contêiner:

```bash
make test-correctness
```

No computador hospedeiro:

```bash
docker compose run --rm pp-lab make test-correctness
```

O alvo depende de `all`: compila as implementações disponíveis com o Makefile
existente e executa Python. Falhas de compilação ou de testes propagam retorno
diferente de zero. Para executar sem recompilar:

```bash
python3 tests/test_correctness.py
```

São necessários Python 3.8 ou posterior, os compiladores do projeto e um runtime
MPI para seus casos. O Docker já fornece essas ferramentas e as variáveis
`OMPI_ALLOW_RUN_AS_ROOT=1` e `OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1`; o script preserva
essa configuração. Sem Docker, a política de execução como root depende do
runtime instalado e deve ser configurada pelo responsável pelo ambiente.

O script procura `mpirun` ou `mpiexec`. Ao identificar OpenMPI por `--version`,
adiciona `--oversubscribe` para permitir quatro processos mesmo com poucos slots.
Não passa essa opção automaticamente a outros runtimes. As variáveis existentes
`MPI_RUN` e `MPI_RUN_ARGS` permitem escolher o launcher e seus argumentos:

```bash
MPI_RUN=mpiexec MPI_RUN_ARGS="--oversubscribe" make test-correctness
MATMUL_TEST_TIMEOUT=60 make test-correctness
```

O timeout padrão é **30 segundos por comando**, incluindo MPI. Em Linux, o
script termina o grupo local de processos em caso de timeout, captura os erros
e reprova o caso. A coleta final da saída tem limite adicional de cinco segundos
para evitar bloqueio por pipes mantidos abertos por descendentes. A limpeza automática foi projetada para execuções locais no
Linux/Docker, não para jobs distribuídos entre máquinas remotas.

Para reprodução, a suíte remove configurações `OMP_*` herdadas dos subprocessos
e define explicitamente a quantidade desejada, `OMP_DYNAMIC=FALSE`,
`OMP_THREAD_LIMIT=4` e `LC_ALL=C`. Isso não altera o ambiente do terminal pai.
O Makefile repassa `BIN_DIR` como `MATMUL_BIN_DIR`, permitindo testar binários
em um diretório de compilação alternativo sem mudar as fontes.

## PASS, FAIL, SKIP e fixtures negativas

- **PASS:** caso executado e verificações aprovadas.
- **FAIL:** retorno não zero, timeout, saída inválida, configuração incorreta ou
  resultado incompatível. A mensagem identifica implementação, N e paralelismo;
  diferenças numéricas incluem esperado, obtido e diferença. Erros de execução
  incluem comando, stdout e stderr, pois podem não produzir checksum.
- **SKIP:** fonte paralela ou launcher MPI ausente, com motivo explícito. Não
  representa aprovação. Fonte presente com executável ausente gera FAIL.

O resumo apresenta as quantidades PASS, FAIL e SKIP. Um SKIP torna a validação
incompleta, mesmo quando o script retorna zero por não haver reprovações. Se MPI
for removido ou o runtime estiver indisponível, seus casos ficam explicitamente
pendentes. O baseline é obrigatório: sua ausência provoca falha.

As fixtures em `tests/test_correctness_harness.py` verificam que o mecanismo
rejeita checksum incorreto, permutação de elementos com soma compatível, NaN,
infinitos, dados incompletos/duplicados, erro de processo e timeout. Também
verificam SKIP por dependência ausente e a saída não zero da suíte quando há
reprovação. Usam processos Python controlados e mocks, sem corromper os programas.

```bash
python3 -m unittest discover -s tests -p test_correctness_harness.py -v
docker compose run --rm pp-lab python3 -m unittest discover -s tests -p test_correctness_harness.py -v
```

Essas dez fixtures são testes do detector e são contabilizadas separadamente
dos 142 casos funcionais. A suíte não mede speedup nem substitui benchmarks.
