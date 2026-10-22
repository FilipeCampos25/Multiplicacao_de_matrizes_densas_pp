# Auditoria de integração e checklist do Marco 2

Data da auditoria: 21 de outubro de 2026. Branch de trabalho:
`issue-12-integracao-final`. A Issue #12 permanece condicionada à execução no
ambiente oficial e aos resultados das Issues #8 e #9.

## Resultado executivo

O repositório possui baseline preservado, implementações OpenMP e MPI,
compilação integrada, testes automatizados e roteiro reprodutível. Contudo,
**ainda não há comprovação local de compilação/testes das alterações nem dados
comparáveis de OpenMP e MPI**. O host da auditoria não possui Docker, `make`,
`g++`, `mpic++`, `mpirun` ou distribuição WSL. Portanto, o Marco 2 não deve ser
declarado pronto até a conclusão dos itens bloqueados abaixo.

## Itens aprovados por inspeção

- `Dockerfile`, `docker-compose.yml`, `.devcontainer/devcontainer.json` e o
  baseline original não foram alterados.
- O Makefile descobre fontes OpenMP/MPI e gera binários separados em `bin/`.
- OpenMP distribui linhas com `parallel for schedule(static)`; cada thread
  escreve em linhas exclusivas de C, sem redução ou região crítica.
- MPI distribui blocos de linhas com `MPI_Scatterv`, replica B com `MPI_Bcast` e
  reúne C com `MPI_Gatherv`; a diferença de carga é de no máximo uma linha.
- Os testes cobrem múltiplos N, 1/2/4/8 threads, diferentes processos,
  divisões irregulares e `P > N`.
- O benchmark executa aquecimento, repetições, registra checksums e metadados e
  mede baseline, OpenMP e MPI usando os caminhos reais do Makefile.
- O analisador deriva mediana, variação, speedup e eficiência apenas do CSV.
- O `.gitignore` permite versionar CSVs, gráficos e metadados necessários.
- README, reprodução, demonstração, arguição e autoria estão documentados.

## Pendências

- Confirmar em `AUTORES.md` nomes completos e responsabilidades do terceiro
  integrante e da integração final.
- Gerar gráficos a partir dos dados reais de OpenMP/MPI e documentar a
  ferramenta/comandos usados; nenhum gráfico foi inventado nesta auditoria.
- Produzir análise experimental de escalabilidade forte e fraca, cache,
  largura de banda, sincronização e comunicação.
- Estimar a fração paralela com medições e aplicar Amdahl numericamente.
- Decidir se o PDF pessoal localizado dentro da pasta do baseline faz parte da
  entrega. Ele parece alheio ao projeto e contém nome pessoal; revisar e remover
  do Git antes de publicar se não houver justificativa acadêmica.
- Revisar os documentos vazios herdados (`architecture.md`,
  `design_philosoply.md`, `development.md`, `orchestration.md`) e remover ou
  preencher em tarefa própria para evitar links/entregáveis sem conteúdo.

## Bloqueios de comprovação

- `make clean`, `make base`, `make openmp`, `make mpi` e `make` não puderam ser
  executados no host auditado.
- `make test` e verificações de checksum não puderam ser executados.
- Benchmarks OpenMP/MPI, speedup, eficiência, gráficos e ensaio ao vivo ainda
  dependem do ambiente oficial.
- O CSV `resultados/benchmarks_automatizados.csv` contém somente cinco linhas
  sequenciais para N=256; não sustenta comparação paralela.
- `resultados/baseline_sequencial.csv` foi coletado fora do contêiner oficial,
  conforme sua documentação, e não deve ser misturado com uma nova coleta.

## Comandos executados nesta auditoria

```text
git status --short
git branch --show-current
git log --oneline --decorate
git ls-files
git ls-remote --heads origin
git shortlog -sne --all
docker version                 # falhou: comando ausente
docker compose version         # falhou: comando ausente
Get-Command make,g++,mpic++,mpirun  # todos ausentes
wsl --list --verbose           # WSL sem distribuição instalada
```

Também foram lidos todos os códigos, scripts, CSVs, documentos, Makefile,
README, `.gitignore` e arquivos do ambiente. As Issues #8, #9 e #12 foram
conferidas no GitHub em 21 de outubro de 2026.

## Checklist antes da entrega

- [ ] Abrir/reconstruir o Dev Container oficial.
- [ ] Executar a sequência limpa dos cinco alvos do Makefile sem warnings.
- [ ] Executar `make test` e guardar a saída no registro da entrega.
- [ ] Coletar N=256, 512 e 1024, cinco repetições, 1/2/4/8 threads/processos.
- [ ] Manter cada sessão de coleta com seu arquivo de ambiente.
- [ ] Gerar resumos, conferir checksums e revisar outliers, sem descartá-los sem justificativa.
- [ ] Produzir gráficos de tempo, speedup e eficiência com eixos/unidades/legenda.
- [ ] Discutir escalabilidade forte; executar ou declarar explicitamente a lacuna da fraca.
- [ ] Quantificar Amdahl somente após estimar a fração paralela.
- [ ] Explicar custos MPI por etapa e limites de memória/cache/largura de banda.
- [ ] Confirmar autoria com todos os integrantes.
- [ ] Remover arquivo pessoal alheio ao projeto, se confirmado pelo grupo.
- [ ] Ensaiar comandos ao vivo e a apresentação dentro do tempo.
- [ ] Conferir `git status --ignored` para não entregar binários, caches ou temporários.

## Sugestão de separação dos commits

1. `chore: integrar e validar implementacoes do projeto` — código, Makefile,
   testes, benchmark, analisador e `.gitignore`.
2. `docs: preparar roteiro da demonstracao tecnica` — README, auditoria,
   compilação, demonstração, arguição e autoria.

Não foram feitos commit, push, merge ou fechamento de issue.
