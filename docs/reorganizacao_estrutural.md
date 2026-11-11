# Relatório da reorganização estrutural — 10/11/2026

> **Validação posterior:** o bloqueio de execução registrado neste relatório
> foi superado em uma cópia temporária da revisão `02c0765`, extraída do Git.
> Build, integração, 142 casos de corretude e 168 testes gerais passaram.
> As novas coletas sequencial e MPI também foram executadas. Consulte a
> [complementação validada](complementacao_validada.md) para os logs e limites.
> O relato abaixo preserva a situação no momento da refatoração.

Branch: `refactor/organizacao-sequencial-openmp-mpi`.
Estado inicial: branch já existente, HEAD `635f81d`, sem diferenças em relação
à main, sem alterações locais nem arquivos não rastreados informados por
`git status --short --untracked-files=all`. Nenhum merge realizado.

## Estrutura e movimentações

Antes, `codigos/baselines_sequenciais_CCO085/` reunia o baseline
`01_matmul.cpp` e 21 exemplos de aula. OpenMP, MPI e instrumentação já
estavam em diretórios próprios.

Depois:

```text
codigos/
├── 00_sequencial/01_matmul.cpp
├── 01_openmp/
│   ├── 01_matmul_openmp.cpp
│   └── matmul_openmp.cpp
├── 02_mpi/matmul_mpi.cpp
├── instrumentacao/01_matmul_perfil.cpp
└── material_professor_CCO085/ (21 exemplos)
```

O baseline foi movido da pasta antiga para `00_sequencial/`.
Todos os exemplos abaixo foram movidos da mesma pasta antiga para
`material_professor_CCO085/`, mantendo os nomes e os bytes, com `git mv`:

- `000_hello.cpp`
- `001_info.cpp`
- `002_hello4t.cpp`
- `003_loop_for_basic.cpp`
- `004_loop_for_paralell.cpp`
- `005_loop_for_paralell_time.cpp`
- `006_sincronizacao_0.0.cpp`
- `006_sincronizacao_0.1.cpp`
- `006_sincronizacao_0.2.cpp`
- `006_sincronizacao_0.3.cpp`
- `006_sincronizacao_0.4.cpp`
- `006_sincronizacao_0.5.cpp`
- `006_sincronizacao_0.6.cpp`
- `006_sincronizacao_bhaskara_critical.cpp`
- `007_reduction_0.0.cpp`
- `007_reduction_0.1.cpp`
- `007_reduction_0.2.cpp`
- `008_sections_0.0.cpp`
- `008_sections_0.1.cpp`
- `010_selecionar_thread.cpp`
- `011_matriz_paralela.cpp`

A atribuição está documentada em `AUTORES.md` e nos cabeçalhos didáticos.
O commit anterior `952d9bd` documenta especificamente a inclusão de
`010_selecionar_thread.cpp` e `011_matriz_paralela.cpp` como exemplos do
professor; portanto, a pendência sobre arquivos não rastreados no checklist
histórico não descreve o estado inicial desta refatoração.
`001_hello.cpp` não foi encontrado e não foi criado.

Inspeção do Makefile, Docker, scripts e testes: nenhum desses exemplos
participa da compilação, testes ou benchmarks principais. As fontes avaliadas
são o baseline explícito e os arquivos das pastas OpenMP/MPI. Referências aos
exemplos nos comentários OpenMP são atribuição didática, não dependências de
compilação. Nenhum arquivo original foi excluído.

## Referências atualizadas

- `Makefile`: somente `BASE_SOURCE`; alvos, flags e binários preservados.
- `scripts/coletar_desempenho.py` e `scripts/coletar_perfil_sequencial.py`:
  localização do baseline.
- `tests/test_correctness.py`: localização da fonte sequencial.
- `README.md`: árvore, caminhos e explicação das três abordagens e auxiliares.
- `AUTORES.md`, `docs/architecture.md`, `docs/demonstracao.md`,
  `docs/metodologia.md` e `docs/perfilamento_sequencial.md`: caminhos atuais.
- `docs/analise_desempenho.md` e `docs/checklist_entrega_marco2.md`:
  notas de localização atual, preservando o relato histórico.

A busca no repositório não encontrou dependências funcionais do caminho antigo
remanescentes. Ele permanece nos registros históricos, nas notas deste relatório
e no comentário original de `codigos/instrumentacao/01_matmul_perfil.cpp`;
esse comentário não é usado em execução e foi mantido para preservar integralmente
o arquivo auxiliar. Os metadados de `resultados/` não foram editados.

## Preservação verificada

SHA-256 do baseline antes e depois:

```text
af87de5c9c58d20429eaf73d89b8d0379a86a5c656227f0ead2a967d8aca507a
```

Os 60 arquivos existentes em `codigos/` e `resultados/` foram comparados
por SHA-256 antes e depois, considerando os destinos das movimentações:
nenhuma divergência. Isso inclui os 21 exemplos, as duas implementações
OpenMP, MPI, instrumentação e todos os resultados. O Git reconhece as
22 movimentações com similaridade de 100%.

Nenhum algoritmo, diretiva, laço, inicialização, checksum, temporização,
fórmula ou resultado experimental foi alterado. Nenhum benchmark foi executado.
Os coletores continuam apontando para `bin/base/01_matmul`,
`bin/openmp/`, `bin/benchmark_openmp/` e `bin/mpi/matmul_mpi`.
A expansão de fontes do Makefile permanece restrita às pastas OpenMP e MPI.

## Validação e limitações

| Comando ou verificação | Resultado nesta refatoração |
| --- | --- |
| `docker compose build` | Tentado; bloqueado porque o pipe `dockerDesktopLinuxEngine` não existe (motor Docker Linux indisponível). A primeira tentativa restrita também encontrou acesso negado à configuração; a repetição com acesso autorizado confirmou o bloqueio do motor. |
| `docker compose run --rm pp-lab make clean` | Não executado: motor Docker indisponível. |
| `docker compose run --rm pp-lab make all` | Não executado: motor Docker indisponível. |
| `docker compose run --rm pp-lab make test` | Não executado: motor Docker indisponível. |
| `docker compose run --rm pp-lab make test-correctness` | Não executado: motor Docker indisponível. |
| `docker compose run --rm pp-lab python3 -m unittest discover -s tests -v` | Não executado: motor Docker indisponível. |
| `docker compose config --quiet` | Código de saída 0; aviso de acesso restrito à configuração pessoal do Docker. |
| `git diff --check` | Aprovado, sem erros de whitespace. |
| SHA-256 antes/depois | 60 arquivos preservados, nenhuma divergência. |
| Inspeção de caminhos e escopo do Makefile | Novo baseline existente; somente o caminho de `BASE_SOURCE` mudou; exemplos fora dos alvos. |

Não há `make`, `g++` ou `mpic++` no PATH verificado; a tentativa
`python --version` também falhou por acesso ao alias de WindowsApps.
Nenhum serviço foi iniciado e nenhuma dependência foi instalada.
Compilação, execução e comparação de checksums numéricos permanecem pendentes
no ambiente Docker operacional; não são declaradas aprovadas.
