# Resultados experimentais

Esta árvore separa coletas imutáveis, análises derivadas, gráficos de apresentação e validação. Arquivos de ambientes diferentes não são combinados para calcular speedup.

## Coleta oficial

A coleta principal é [`01_coletas/principal/issue10_desempenho.csv`](01_coletas/principal/issue10_desempenho.csv): 175 observações (15 sequenciais, 80 OpenMP e 80 MPI), cinco repetições e 35 configurações, produzidas em uma única execução Docker em 22/10/2026. A procedência, comandos, ambiente e checksums estão no arquivo `_ambiente.txt` adjacente.

`02_analises/principal/desempenho_consolidado.csv` preserva cada valor bruto e acrescenta procedência. `metricas_desempenho.csv` contém média, mediana, extremos, desvio padrão, CV, speedup sequencial (`Tseq/Tp`) e speedup relativo a um trabalhador. `escalabilidade_fraca.csv` mantém separada a série fraca. O manifesto registra SHA-256 da fonte.

## Coletas complementares e históricas

- `01_coletas/complementares/openmp/`: estudo OpenMP independente; não participa da análise oficial.
- `01_coletas/complementares/perfil_mpi_2026-11-10/`: perfil sequencial e etapas MPI do snapshot `02c0765`; os resumos ficam em `02_analises/complementares/`. O proxy de comunicação MPI é explicitamente não aditivo e não representa partes de 100%.
- `01_coletas/complementares/coleta_conjunta_2026-11-11/`: nova coleta conjunta no WSL2, 175 observações válidas; preservada como conjunto independente e não incorporada aos gráficos oficiais.
- `01_coletas/complementares/openmp_2026-11-11/`: 180 medições OpenMP no mesmo host WSL2; resumo e status correspondentes ficam em `02_analises/complementares/openmp_2026-11-11/`.
- `01_coletas/historicas_nao_comparaveis/`: ensaios antigos sem ambiente comparável; preservados, mas excluídos de speedup.

## Gráficos

`03_graficos/` contém tempos, speedup, eficiência, escalabilidade forte, escalabilidade fraca e Lei de Amdahl. Linhas ideais e de Amdahl são marcadas como teóricas/hipotéticas; as demais são medidas. A procedência permanece no título SVG e no manifesto, não no nome público.

## Reprodução

Na raiz do repositório:

```bash
python3 scripts/analisar_desempenho.py
python3 scripts/analisar_complementacao.py \
  --sequencial resultados/01_coletas/complementares/perfil_mpi_2026-11-10/perfil_sequencial.csv \
  --mpi resultados/01_coletas/complementares/perfil_mpi_2026-11-10/mpi_etapas.csv \
  --expected-sizes 256 512 1024 --expected-processes 1 2 4 8 \
  --minimum-repetitions 5
```

O analisador principal lê somente os caminhos passados por `--input`; sem argumentos, seleciona exclusivamente a coleta oficial. CSVs estranhos em `resultados/` não entram automaticamente.

## Limitações

As coletas pertencem aos ambientes documentados e não devem ser extrapoladas para outras máquinas. A coleta principal é histórica. Tempos máximos por etapa MPI podem vir de ranks diferentes, por isso não são somados como decomposição do tempo total. Consulte `04_validacao/resumo_testes.md` e `04_validacao/hashes_coletas.md`.
