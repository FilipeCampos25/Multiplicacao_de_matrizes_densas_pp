# Checklist final de entrega — Marco 2

Auditoria atualizada em **10/11/2026** na branch
`fix/ajustes-finais-marco2`, após os commits `b558803`, `e388003` e `06d9b88`.
As classificações abaixo distinguem evidência histórica, inspeção da revisão atual
e validação executada neste host. “Concluído com evidência” não significa que uma
medição histórica foi repetida no código atual.

| Requisito | Classificação | Arquivo ou resultado que fundamenta a classificação | Limite ou pendência |
| --- | --- | --- | --- |
| Baseline original | **Concluído com evidência** | `codigos/baselines_sequenciais_CCO085/01_matmul.cpp`; `docs/architecture.md`; hash preservado conforme auditoria anterior | Manter atribuição ao professor e não confundir os dois `.cpp` locais não rastreados com material confirmado |
| Perfilamento e hotspots | **Parcialmente concluído** | `docs/perfilamento_sequencial.md`; `codigos/instrumentacao/01_matmul_perfil.cpp`; `scripts/coletar_perfil_sequencial.py` | Instrumentação e protocolo existem, mas não há coleta quantitativa por etapa; o kernel `O(N³)` é hotspot fundamentado por complexidade, não por nova medição |
| Fração paralelizável e Lei de Amdahl | **Parcialmente concluído** | `docs/analise_desempenho.md`, seção 9; `resultados/metricas_desempenho.csv`; `resultados/graficos/amdahl_*.svg` | Há estimativa efetiva invertida de speedup histórico e cenários teóricos; falta fração por etapas medida e a inferência incorpora overheads |
| OpenMP e condições de corrida | **Concluído com evidência** | `codigos/01_openmp/*.cpp`; `docs/metodologia.md`; `docs/avaliacao_openmp.md`; testes históricos de 05/11 no `README.md` | A inspeção confirma divisão por linhas e escritas disjuntas; os testes não foram repetidos neste host |
| MPI, decomposição e coletivas | **Concluído com evidência** | `codigos/02_mpi/matmul_mpi.cpp`; `docs/escalabilidade_mpi.md`; uso de `Scatterv`, `Bcast` e `Gatherv`; testes históricos de casos irregulares | Validação atual bloqueada por ausência de toolchain |
| Corretude | **Parcialmente concluído** | `tests/test_correctness.py`; `tests/test_correctness_harness.py`; `docs/testes_corretude.md`; registro histórico `142 PASS` em 05/11 | Os hashes/fontes atuais são posteriores à coleta histórica; é necessária reexecução limpa dos testes obrigatórios |
| Speedup e eficiência | **Concluído com evidência** | `resultados/metricas_desempenho.csv`; `docs/analise_desempenho.md`; gráficos `speedup_*` e `eficiencia_*` | Resultados pertencem à coleta histórica de 22/10, não às fontes atuais |
| Escalabilidade forte | **Concluído com evidência** | `resultados/issue10_desempenho.csv`; `resultados/metricas_desempenho.csv`; `docs/analise_desempenho.md` | Evidência histórica em máquina única/Docker |
| Escalabilidade fraca | **Concluído com evidência** | `resultados/escalabilidade_fraca.csv`; gráfico `fraca_*`; metodologia cúbica em `docs/analise_desempenho.md` | Explicar arredondamento de N e limites experimentais |
| Overhead de comunicação MPI | **Parcialmente concluído** | `docs/escalabilidade_mpi.md`; `docs/analise_desempenho.md`; `scripts/benchmark_mpi.sh`; `resultados/mpi_etapas_complementar_bloqueio_2026-11-10.txt` | Instrumentação por etapas foi preparada, mas a coleta foi bloqueada; não há quantificação experimental separada de comunicação |
| Gráficos e resultados | **Concluído com evidência** | `resultados/graficos/` (21 SVGs); `resultados/analise_manifesto.json`; teste de procedência de artefatos | Preservar dados históricos e não apresentá-los como coleta atual |
| Makefile e Docker | **Parcialmente concluído** | `Makefile`, `Dockerfile`, `docker-compose.yml`, `docs/compilacao.md`; comandos cruzados por inspeção | Docker/Make não estão disponíveis neste host; validação limpa atual não foi executada |
| Scripts e CSVs | **Concluído com evidência** | scripts de coleta, benchmark e análise; `resultados/*.csv` | Nova instrumentação de perfil/MPI ainda carece de execução no ambiente oficial |
| README | **Concluído com evidência** | `README.md`; comandos e caminhos conferidos com o `Makefile`; medições históricas separadas da auditoria atual | Revalidar comandos em Docker antes da entrega |
| Autoria | **Parcialmente concluído** | `AUTORES.md`; `git shortlog -sne --all`; commits e PRs citados no documento | A divisão proposta precisa de confirmação humana dos três integrantes; o Git não prova acordo interno nem exclusividade |
| Demonstração e arguição | **Parcialmente concluído** | `docs/demonstracao.md`; `docs/arguicao.md` | Falta ensaio cronometrado e confirmação de preparação individual |
| Higiene e escopo da branch | **Parcialmente concluído** | `git diff main...HEAD` contém somente complementações de perfilamento, Amdahl, MPI e auditoria | Há dois `.cpp` não rastreados na pasta do professor; origem e destino precisam de decisão humana e eles não integram o commit |

## Validação de integração de 10/11/2026

Host: Windows/PowerShell. Não estão disponíveis `make`, `g++`, `mpic++`,
`mpirun`, Bash ou Docker; `python3` é um alias inoperante da Microsoft Store.
Assim, não foi possível criar um checkout limpo funcional nem usar o ambiente
Docker documentado.

| Comando solicitado | Resultado observado |
| --- | --- |
| `make clean` | Bloqueado: `make` ausente |
| `make all` | Bloqueado: `make` ausente |
| `make test` | Bloqueado: `make` e Bash ausentes |
| `make test-correctness` | Bloqueado: `make`, compiladores e binários ausentes |
| `python3 -m unittest discover -s tests -v` | Bloqueado: alias `python3` sem interpretador |
| `python -m unittest discover -s tests -v` | Diagnóstico alternativo: 168 testes; 7 aprovações, 142 falhas por executáveis ausentes e 19 erros, sobretudo por permissão em temporários. Resultado global: **FAILED** |

O registro de 05/11/2026 no `README.md` (build, 142 casos funcionais e 167
testes gerais aprovados) permanece como evidência histórica. Ele não deve ser
usado para afirmar que esta revisão foi validada, porque os três commits de
complementação são posteriores.

## Parecer de prontidão

O Marco 2 está **bem coberto documentalmente, porém não pronto para declaração
de conformidade integral**. Antes da entrega, permanecem necessárias:

1. reexecução, em checkout limpo e no Docker oficial, dos cinco comandos
   obrigatórios e arquivamento da saída com o hash do commit;
2. coleta quantitativa do perfil sequencial por etapas;
3. coleta MPI por etapas para quantificar comunicação, ou apresentação explícita
   desse item como limitação;
4. confirmação humana da divisão de autoria pelos três integrantes;
5. decisão documentada sobre os dois fontes não rastreados;
6. ensaio da demonstração em até nove minutos.

Não é necessário sobrescrever os resultados históricos. Qualquer nova coleta
deve usar outro nome e registrar ambiente, commit e parâmetros.
