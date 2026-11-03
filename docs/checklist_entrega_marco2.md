# Checklist final de entrega — Marco 2

Auditoria realizada em **03/11/2026**, na branch
`task2/complementacao-experimental-marco2`, revisão `eae1ac9`. “Concluído”
significa comprovado por inspeção ou evidência reproduzível; não significa que
os executáveis atuais foram validados neste host.

| Requisito do professor | Status | Arquivo ou evidência | Validação realizada em 03/11/2026 | Ação manual necessária |
| --- | --- | --- | --- | --- |
| Baseline sequencial fornecido | concluído | `codigos/baselines_sequenciais_CCO085/01_matmul.cpp` | Presença, histórico e SHA-256 conferidos; hash atual `af87…507a` coincide com o registro de 22/10 | Confirmar licença/atribuição exigida pela disciplina |
| Implementações OpenMP | parcial | `codigos/01_openmp/*.cpp` | Duas fontes presentes e integração no Makefile inspecionada; não compiladas neste host | Compilar e executar a revisão atual no contêiner |
| Implementação MPI | parcial | `codigos/02_mpi/matmul_mpi.cpp` | Fonte, `Scatterv/Bcast/Gatherv` e casos irregulares inspecionados; não compilada | Compilar e executar com 1/2/4 processos |
| Makefile | parcial | `Makefile` | Alvos esperados presentes; execução bloqueada por ausência de `make` | Executar todos os alvos obrigatórios em Linux/contêiner |
| Scripts de benchmark | concluído | `scripts/benchmark_openmp.py`, `benchmark_mpi.sh`, `coletar_desempenho.py` | Presença, separação coleta/análise e proteção contra sobrescrita inspecionadas | Fazer ensaio final dos comandos da demonstração |
| CSVs de resultados | concluído | `resultados/*.csv`, `analise_manifesto.json` | 175 linhas na coleta conjunta (15 seq., 80 OpenMP, 80 MPI), todas `status=ok`; manifesto registra 435 observações consolidadas | Preservar brutos e metadados sem regravar |
| Gráficos de tempo, speedup e eficiência | concluído | `resultados/graficos/` | 21 SVGs presentes; teste de artefatos/procedência passou; links locais conferidos | Abrir os SVGs no ambiente da apresentação |
| Escalabilidade forte | concluído | `metricas_desempenho.csv`, `docs/analise_desempenho.md` | N=256/512/1024 e 1/2/4/8 trabalhadores conferidos na evidência histórica | Repetir somente se os resultados forem atribuídos aos fontes atuais |
| Escalabilidade fraca | concluído | `escalabilidade_fraca.csv` | Oito grupos e fórmula cúbica documentados; série separada da forte | Explicar arredondamento de N e limites na apresentação |
| Lei de Amdahl | concluído | `docs/analise_desempenho.md`, gráficos `amdahl_*` | Fórmula e três cenários hipotéticos conferidos; documento não os trata como ajuste medido | Não declarar fração serial estimada |
| README e compilação | concluído | `README.md`, `docs/compilacao.md` | Caminhos e comandos cruzados com Makefile | Atualizar qualquer saída somente após nova validação |
| Autoria | parcial | `AUTORES.md`, histórico Git | Três integrantes e contribuições históricas documentados; 24 commits Filipe, 11 Fabricio (duas identidades, mesmo e-mail), 3 Davi | Os três integrantes devem validar a divisão formal |
| Corretude e metodologia | parcial | `docs/testes_corretude.md`, `docs/metodologia.md`, registro de ambiente | Método de comparação e tolerâncias documentados; 142 PASS são de 22/10 | Reexecutar após as mudanças posteriores aos hashes registrados |
| Demonstração até 9 minutos | parcial | `docs/demonstracao.md` | Roteiro revisado com comandos reais, tempos e contingência | Ensaio cronometrado no ambiente oficial |
| Preparação para arguição cruzada | concluído | `docs/arguicao.md` | Conceitos sequencial/OpenMP/MPI, corretude, métricas e limites cobertos | Cada integrante deve praticar todas as três versões |
| Higiene do repositório | parcial | `.gitignore`, `git ls-files`, `git status --ignored` | Nenhum binário, objeto, cache, PDF, chave ou arquivo >1 MiB rastreado; caches ignorados encontrados | Remover temporários locais antes de empacotar; decidir sobre os dois `.cpp` não rastreados |

## Validação executada agora

Host: Windows, PowerShell, Python 3.12. Não estavam disponíveis `make`, `g++`,
`mpic++`, `mpirun`, Docker ou Bash. `python3` era apenas o alias inoperante da
Microsoft Store.

| Comando solicitado | Resultado de 03/11/2026 |
| --- | --- |
| `make clean` | bloqueado: `make` ausente |
| `make all` | bloqueado: `make` ausente |
| `make test` | bloqueado: `make` e Bash ausentes |
| `make test-correctness` | bloqueado: `make`; binários não existem |
| `python3 -m unittest discover -s tests -v` | bloqueado: alias `python3` sem interpretador |
| `python -m unittest discover -s tests -v` | executou 167 testes: 142 falharam por executáveis ausentes e 18 deram erro de permissão em temporários; 7 passaram; não é aprovação |
| `make benchmark-openmp-build` | bloqueado: `make` e `g++` ausentes |
| Execuções manuais baseline/OpenMP/MPI | bloqueadas: binários e toolchains ausentes |

Uma segunda tentativa dos testes Python, apontando temporários para o workspace,
continuou bloqueada pela política de escrita do sandbox: 25 testes, 18 erros de
permissão e 7 aprovações. Separadamente, o teste somente de leitura
`RepositoryArtifactsTests.test_real_statistics_provenance_and_graphs` passou.

## Integridade e divergências encontradas

- A coleta de 22/10 registra GCC 13.3.0, Open MPI 4.1.6, Ubuntu 24.04 em
  WSL2/Docker, i7-4770S e oito CPUs lógicas. Registra 175 medições válidas e
  `142 PASS, 0 FAIL, 0 SKIP` naquela data.
- O hash atual do baseline coincide com o registrado. Os hashes atuais do
  Makefile, OpenMP e MPI **não coincidem** com os da coleta. Logo, CSVs e
  gráficos permanecem íntegros como resultados históricos, mas não validam os
  executáveis atuais.
- `metricas_desempenho.csv` não apresentou divergência em `E=S/p` dentro de
  `1e-6`. A procedência e correspondência dos gráficos também são cobertas pelo
  teste de artefatos aprovado.
- Foram encontrados 21 SVGs e nenhum link Markdown local quebrado nos arquivos
  rastreados. `docs/auditoria_marco2.md` é uma fotografia antiga de 21/10 e sua
  nota retrospectiva deve ser respeitada; não usar suas pendências antigas como
  estado atual.
- `benchmarks_automatizados.csv` e `baseline_sequencial.csv` são históricos
  isolados e não devem ser combinados como referência comparável à coleta
  conjunta.
- Não há arquivo rastreado maior que 1 MiB nem binário/objeto/cache/documento
  pessoal rastreado. Existem dois fontes não rastreados na pasta do professor;
  a equipe precisa decidir sua origem e inclusão sem atribuí-los ao professor
  automaticamente.

## Parecer e ações bloqueantes

O Marco 2 está **documental e experimentalmente bem coberto, mas parcialmente
validado na revisão atual**. Antes da entrega, a equipe deve:

1. executar, em checkout limpo do commit candidato e no contêiner oficial,
   `make clean`, `make all`, `make test`, `make test-correctness`,
   `python3 -m unittest discover -s tests -v` e
   `make benchmark-openmp-build`;
2. executar manualmente baseline, OpenMP (1 e 4 threads) e MPI (1 e 4 ranks),
   comparar dumps/checksums e arquivar saída, commit, hashes e ambiente;
3. confirmar `AUTORES.md` com os três integrantes;
4. resolver os dois `.cpp` não rastreados e limpar caches/temporários locais;
5. ensaiar `docs/demonstracao.md` em até nove minutos.

Não é necessário alterar os resultados de 22/10. Se a equipe quiser afirmar
desempenho dos fontes atuais, deve produzir uma coleta nova, com outro nome e
metadados completos.
