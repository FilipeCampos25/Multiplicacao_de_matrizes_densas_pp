# Avaliação de desempenho e escalabilidade OpenMP — Issue #8

## 1. Objetivo da avaliação

Investigar escalabilidade forte da multiplicação densa: para cada N fixo,
variar a equipe e o escalonamento, comparando tempo do kernel e variabilidade.
O executável escolhido é `matmul_openmp`, que recebe N e configura a equipe por
`OMP_NUM_THREADS`; sua saída informa `N`, `checksum`, `tempo` e `threads`
efetivamente utilizadas. A versão `01_matmul_openmp` também existe e continua
coberta pela suíte de corretude, mas não é misturada aos tempos desta coleta.

O script anterior `medir_speedup.sh` é compatível com matrizes, porém coleta
também MPI, não varia escalonamentos e escreve em um CSV preexistente por padrão.
A Issue #8 usa a automação específica `scripts/benchmark_openmp.py`.

## 2. Ambiente de execução

Experimentos no serviço Docker `pp-lab`, imagem `iesb/pp-lab:2026.2`, Ubuntu
24.04 sobre Linux/WSL2. CPU identificada: **13th Gen Intel Core i5-1334U**.
`lscpu` expõe 12 CPUs lógicas e topologia virtual de 6 núcleos com 2 threads;
essa descrição não deve ser interpretada como inventário físico completo do
processador hospedeiro.

Foram consideradas a afinidade do processo, cpusets e quotas dos cgroups e seus
ancestrais visíveis. O contêiner expôs CPUs **0–11**, `cpu.max = max 100000`
e `memory.max = max`: capacidade visível de **12 CPUs**, sem quota explícita.
A memória disponível no início foi **7.211.327.488 bytes**, aproximadamente
**6,72 GiB**. A virtualização não garante uso exclusivo desses recursos.

Compilador: **g++ 13.3.0**, com flags comuns
`-O2 -std=c++17 -Wall -Wextra -Wpedantic -fopenmp` e ligação `-lm`.
As variantes diferem somente em `-DMATMUL_BENCH_SCHEDULE=static|dynamic|guided`.
Os comandos de compilação, hashes SHA-256, sistema, cgroups, recursos antes/depois
e instantes UTC estão em [`openmp_ambiente.txt`](../resultados/openmp_ambiente.txt).

Configuração controlada: `OMP_DYNAMIC=FALSE`, `OMP_PROC_BIND=close`,
`OMP_PLACES=threads`, `OMP_WAIT_POLICY=PASSIVE`, `OMP_THREAD_LIMIT=8` e `LC_ALL=C`.
As demais variáveis herdadas `OMP_*`, `GOMP_*`, `KMP_*` e `MATMUL_DUMP` são
removidas dos subprocessos. O ambiente do terminal pai não é modificado.

## 3. Metodologia experimental

O Makefile compila três binários em `bin/benchmark_openmp/`, a partir da mesma
fonte `codigos/01_openmp/matmul_openmp.cpp`. A macro de escalonamento tem
`static` como valor padrão; a compilação habitual e as interfaces permanecem
compatíveis. Não há cópias do algoritmo nem alterações na ordem `i-k-j` ou na
inicialização determinística.

Antes de medir, a automação executa `make test-correctness` e compara também os
elementos das variantes para N=5. Depois obtém um checksum sequencial por N,
reutilizado em todas as repetições. Cada execução deve terminar normalmente,
ter dimensão e equipe corretas, tempo positivo e checksum finito compatível:
`abs(a-b) <= 1e-6 + 1e-9 * max(abs(a), abs(b))`, conforme a Issue #6.

O tempo registrado é o campo `tempo` do próprio programa, obtido por
`omp_get_wtime()`. Inclui entrada e sincronização da região paralela; exclui
alocação, inicialização, checksum, compilação, Python e partida do contêiner.
Cada medição usa um processo novo, preservando o custo da região paralela.

## 4. Dimensões e quantidades de threads

| N | Memória dos elementos de A, B e C | Threads solicitadas |
| --- | ---: | --- |
| 256 | 1,5 MiB | 1, 2, 4, 8 |
| 512 | 6 MiB | 1, 2, 4, 8 |
| 1024 | 24 MiB | 1, 2, 4, 8 |
| 2048 | 96 MiB | 1, 2, 4, 8 |

Duplicar N multiplica por oito o trabalho O(N³) e por quatro o armazenamento
O(N²). N=2048 foi incluído porque cabia com folga na memória disponível.
O coletor rejeita dimensões incompatíveis com os índices do baseline e não
executa tamanhos que excedam 50% da memória disponível inicial. Configurações
acima da capacidade de CPU visível recebem SKIP, sem oversubscription.

## 5. Repetições e tratamento estatístico

Um aquecimento por combinação precede as **cinco repetições válidas**.
O aquecimento ocorre em outro processo: pode preparar caches/frequência, mas
não mantém uma equipe OpenMP viva entre medições. Em cada rodada, a ordem das
12 configurações é embaralhada com semente fixa **8506**, reduzindo o efeito
de sempre executar uma estratégia primeiro. As dimensões seguem ordem crescente.
Não há descarte de outliers nem substituição seletiva de medições lentas.

São calculados média, mediana, mínimo, máximo, desvio-padrão **amostral** (n−1)
e CV = 100 × desvio-padrão / média. A mediana é a comparação principal.
Para cada par (N, escalonamento), `S(p)=mediana(T1)/mediana(Tp)` e `E(p)=S(p)/p`.
A referência é **OpenMP com uma thread do mesmo escalonamento**, não o baseline
sequencial. Sem referência válida, speedup/eficiência ficam vazios; grupos
incompletos permanecem no bruto, mas não recebem resumo final.

Arquivos da coleta:

- [`openmp_desempenho.csv`](../resultados/openmp_desempenho.csv): cada repetição
  válida, equipe solicitada (`threads`), efetiva (`threads_utilizadas`) e ordem.
- [`openmp_resumo.csv`](../resultados/openmp_resumo.csv): estatísticas e métricas.
- [`openmp_status.csv`](../resultados/openmp_status.csv): PASS, SKIP ou FAIL e motivo.
- [`openmp_ambiente.txt`](../resultados/openmp_ambiente.txt): metadados em JSON.

## 6. Comparação das estratégias de escalonamento

As 48 combinações foram executadas, sem SKIP ou falhas, gerando **240 medições
válidas**. As equipes efetivas coincidiram com as solicitadas. A suíte anterior
produziu **142 PASS, 0 FAIL, 0 SKIP**, e os 12 casos pequenos das variantes também
passaram, com comparação elemento a elemento. Houve 48 aquecimentos, não
incluídos no CSV bruto, e nenhum outlier foi removido.

A tabela apresenta **medianas em milissegundos**. S, E e CV nesta tabela se
referem exclusivamente a `static`; os resultados completos das três estratégias
estão no CSV de resumo.

| N | Threads | Static (ms) | Dynamic (ms) | Guided (ms) | S static | E static (%) | CV static (%) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 256 | 1 | 5,436 | 5,461 | 6,179 | 1,00 | 100,0 | 9,0 |
| 256 | 2 | 3,129 | 4,316 | 3,154 | 1,74 | 86,9 | 60,4 |
| 256 | 4 | 3,380 | 3,466 | 3,551 | 1,61 | 40,2 | 53,5 |
| 256 | 8 | 3,242 | 3,429 | 3,087 | 1,68 | 21,0 | 16,8 |
| 512 | 1 | 44,392 | 44,023 | 44,370 | 1,00 | 100,0 | 16,2 |
| 512 | 2 | 25,526 | 27,718 | 23,423 | 1,74 | 87,0 | 19,8 |
| 512 | 4 | 20,131 | 19,419 | 16,522 | 2,21 | 55,1 | 11,1 |
| 512 | 8 | 14,362 | 13,365 | 12,425 | 3,09 | 38,6 | 11,4 |
| 1024 | 1 | 462,757 | 468,136 | 517,759 | 1,00 | 100,0 | 5,4 |
| 1024 | 2 | 233,150 | 242,444 | 264,092 | 1,98 | 99,2 | 20,1 |
| 1024 | 4 | 155,017 | 197,659 | 152,479 | 2,99 | 74,6 | 13,9 |
| 1024 | 8 | 106,294 | 110,733 | 110,297 | 4,35 | 54,4 | 6,3 |
| 2048 | 1 | 6421,492 | 6064,247 | 6177,284 | 1,00 | 100,0 | 6,4 |
| 2048 | 2 | 3305,549 | 3500,654 | 3289,623 | 1,94 | 97,1 | 3,7 |
| 2048 | 4 | 2101,887 | 2010,652 | 2148,995 | 3,06 | 76,4 | 5,9 |
| 2048 | 8 | 1471,572 | 1298,362 | 1534,753 | 4,36 | 54,5 | 8,7 |

`Static` divide antecipadamente as linhas; `dynamic` distribui trabalho durante
a execução; `guided` reduz progressivamente os blocos. Não foi especificado
chunk nas diretivas. Cada iteração externa calcula uma linha, com O(N²) trabalho
regular. Essa regularidade favorece teoricamente baixo custo de distribuição
estática, mas os tempos medidos não demonstram vantagem universal dessa política.
As diferenças de tempo total não isolam o overhead do escalonador.

## 7. Resultados e métricas de escalabilidade

Em `static`, N=256 atinge sua menor mediana com **duas threads**, e usar quatro
ou oito não melhora esse valor. Seu CV com duas threads é 60,4%, com tempos de
3,091 a 9,442 ms: a mediana sozinha esconderia a instabilidade. Em N=512, 1024 e
2048, oito threads apresentam as menores medianas de cada estratégia testada.

As menores medianas globais por N foram:

| N | Estratégia / threads | Mediana (ms) | CV (%) | S da própria estratégia | E (%) |
| --- | --- | ---: | ---: | ---: | ---: |
| 256 | guided / 8 | 3,087 | 13,0 | 2,00 | 25,0 |
| 512 | guided / 8 | 12,425 | 7,6 | 3,57 | 44,6 |
| 1024 | static / 8 | 106,294 | 6,3 | 4,35 | 54,4 |
| 2048 | dynamic / 8 | 1298,362 | 11,3 | 4,67 | 58,4 |

O CV de todos os grupos varia de **1,34% a 60,38%**. N=1024 ilustra por que é
necessário comparar também tempos absolutos: `guided/8` tem S=4,69, maior que
S=4,35 de `static/8`, mas sua mediana de 110,297 ms é mais lenta. A referência
`guided/1` também é mais lenta; portanto, speedups de estratégias diferentes não
classificam diretamente seu desempenho absoluto.

A auditoria independente confirmou 240 linhas, 48 grupos com repetições 1–5,
nenhuma duplicação, equipes corretas e todos os checksums compatíveis. Médias,
medianas, extremos, desvios amostrais, CV, speedup e eficiência foram recalculados
a partir do bruto e conferidos com o resumo.

## 8. Análise dos possíveis gargalos

**Observações diretas:** em `static/8`, a eficiência cresce de 21,0% em N=256
para cerca de 54,5% em N=2048, ainda distante do ideal de 100%. Na configuração
`static/1`, passar de 512 para 1024 eleva a mediana em aproximadamente 10,42 vezes;
de 1024 para 2048, em 13,88 vezes, acima do fator oito do trabalho aritmético.
Essas observações indicam crescimento de custo e escalabilidade sublinear, sem
identificar isoladamente sua causa.

**Explicações plausíveis:** para matrizes pequenas, iniciar e sincronizar threads
e distribuir linhas pode representar parcela relevante do kernel. Em N=256,
isso é compatível com o ganho limitado acima de duas threads em `static`, mas
não foi medido separadamente. Nenhuma mediana paralela desta coleta foi pior
que a referência de uma thread do mesmo escalonamento; não se demonstrou que
o custo de criação excedeu todo o benefício do paralelismo.

O kernel reutiliza um escalar de A e acessa sequencialmente linhas de B e C.
Em N=2048, B sozinha ocupa 32 MiB, acima dos 12 MiB de L3 relatados pelo ambiente;
isso torna efeitos de capacidade de cache uma hipótese relevante. Disputa por
recursos de execução, memória, interferência do hospedeiro e mudanças de frequência
também podem explicar parte dos ganhos decrescentes e da variabilidade.

**Hipóteses ainda não demonstradas:** não é possível afirmar quantidade de cache
misses, saturação da largura de banda, contenção específica ou limitação térmica
a partir desses tempos. Seriam necessários contadores de hardware, perfilamento
de memória e controle/monitoramento do hospedeiro para distinguir essas causas.

## 9. Justificativa da estratégia escolhida

Para priorizar **menor latência mediana neste ambiente**, os candidatos são
`guided/8` para N=256 e 512, `static/8` para N=1024 e `dynamic/8` para N=2048.
Há ressalvas importantes na escolha:

- **N=256:** `guided/8` ficou apenas 0,042 ms abaixo de `static/2`, embora com
  menor CV. Não há evidência suficiente para justificar oito threads apenas por
  essa pequena diferença. Duas threads com `static` são uma alternativa econômica
  a reavaliar, pois a dispersão de 60,4% impede recomendá-la como opção estável.
- **N=512:** `guided/8` combina menor mediana com CV de 7,6%; `dynamic/8` tem CV
  ligeiramente menor, 6,7%, mas mediana de 13,365 ms contra 12,425 ms.
- **N=1024:** `static/8` é a opção de menor mediana; `guided/8` tem CV de 3,0%
  contra 6,3%, sendo alternativa quando estabilidade pesa mais que a diferença
  de aproximadamente 3,8% no tempo mediano.
- **N=2048:** `dynamic/8` tem mediana cerca de 11,8% menor que `static/8`, com CV
  maior (11,3% contra 8,7%). É o candidato orientado a latência; `static/8` oferece
  menor dispersão nesta amostra.

O comportamento padrão permanece **`static`**, conforme o requisito da Issue,
pela compatibilidade e regularidade do trabalho. Essa preservação não significa
que os dados o apontem como vencedor em todos os tamanhos. As escolhas específicas
acima são provisórias e não constituem prova de superioridade estatística.

## 10. Limitações, reprodução e conclusão

O contêiner compartilha um computador virtualizado com outros processos.
Não foram controlados carga do hospedeiro, frequência ou temperatura. Afinidade
e ausência de quota não equivalem a CPUs físicas exclusivas. Cinco repetições
permitem descrever dispersão, mas não demonstram significância estatística de
pequenas diferenças. A saída temporal tem resolução de seis casas decimais.

Não foram coletados contadores de cache, tráfego de memória, energia ou traces
de escalonamento. As causas dos gargalos permanecem hipóteses. Nesta escala,
a validação de cada repetição é por checksum; a comparação elemento a elemento
é restrita aos testes pequenos. As recomendações valem para esta fonte, flags,
afinidade, máquina e condições da coleta.

Comandos na raiz do repositório, dentro de Linux/Docker:

```bash
make benchmark-openmp
# Para incluir a dimensão adicional usada nesta avaliação:
make benchmark-openmp BENCHMARK_ARGS="--sizes 256 512 1024 2048"
```

No hospedeiro, o comando da coleta foi:

```bash
docker compose run --rm -T pp-lab make benchmark-openmp 'BENCHMARK_ARGS=--sizes 256 512 1024 2048'
```

Os arquivos entregues já existem: **uma repetição desse comando será recusada
para protegê-los**. Para uma nova coleta, escolha outro nome, por exemplo:

```bash
docker compose run --rm -T pp-lab make benchmark-openmp 'BENCHMARK_ARGS=--sizes 256 512 1024 2048 --output resultados/openmp_nova_desempenho.csv'
```

Também é possível compilar com `make benchmark-openmp-build` e executar
`python3 scripts/benchmark_openmp.py --sizes 256 512 1024 --threads 1 2 4 8
--repetitions 5 --schedules static dynamic guided --output resultados/outra_desempenho.csv`
em uma única linha. Requer Python 3.9+ e apenas bibliotecas padrão. O timeout
padrão é 120 s por execução (`--timeout`); a suíte anterior tem limite de 300 s.
Falhas encerram a coleta com código não zero e preservam os dados já válidos.
SKIP produz status PARTIAL, sem apresentar casos não executados como aprovados.

Os testes da automação, com fixtures sintéticas explicitamente separadas dos
dados experimentais, são executados por:

```bash
python3 -m unittest discover -s tests -p test_benchmark_openmp.py -v
```

A avaliação executou todas as dimensões e configurações propostas, inclusive
N=2048, sem oversubscription visível e sem casos pendentes. Os seis testes da
automação passaram. O baseline, as medições anteriores, MPI e os testes de
corretude preexistentes permaneceram intactos. O contêiner não dispõe do comando
Git; a identificação reproduzível dos artefatos foi registrada por hashes, sem
inventar um identificador de commit.

Os critérios 1–17 da Issue foram contemplados: inspeção, automação, recursos,
repetições, escalonamentos, tempo do kernel, corretude, estatísticas, CSVs reais,
análise, recomendação, integração e verificação final. Os resultados mostram
ganhos de paralelismo dependentes de N, dispersão relevante em casos pequenos e
ausência de uma política de escalonamento universalmente superior nesta coleta.
