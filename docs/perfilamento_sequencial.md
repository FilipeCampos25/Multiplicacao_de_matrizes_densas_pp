# Perfilamento do algoritmo sequencial

> **Atualização de 10/11/2026, após a tentativa bloqueada:** coleta executada
> no Docker sobre a revisão `02c0765`, com 15 medições válidas (cinco por dimensão).
> A multiplicação representou 90,60%, 94,12% e 97,39% do total mediano para
> N=256, 512 e 1024. A [complementação validada](complementacao_validada.md)
> reúne CSV, estatísticas por etapa, ambiente e hashes. As seções de estado
> abaixo registram a tentativa anterior, cuja pendência de coleta foi resolvida.

## Evidência histórica

O baseline original do professor (`codigos/00_sequencial/01_matmul.cpp`)
permanece inalterado. O arquivo histórico `resultados/01_coletas/historicas_nao_comparaveis/baseline_sequencial.csv` contém
cinco execuções para cada dimensão, realizadas em Windows/MINGW64, com GCC `-O2`,
em um AMD Ryzen 5 3400G com 8 GB de RAM. Esses dados medem somente a região da
multiplicação:

| N | Checksum | Média (s) | Mediana (s) | Mínimo–máximo (s) |
|---:|---:|---:|---:|---:|
| 256 | 4109165.296000 | 0.004799 | 0.004798 | 0.004777–0.004832 |
| 512 | 32883431.310401 | 0.048124 | 0.049412 | 0.040243–0.055978 |
| 1024 | 263088437.159955 | 0.546761 | 0.542017 | 0.536388–0.566012 |

Esses valores não separam alocação, inicialização e checksum. Portanto, não são
evidência quantitativa de perfilamento por etapa e não permitem calcular a
participação percentual real de cada trecho.

## Instrumentação e preservação do algoritmo

A cópia `codigos/instrumentacao/01_matmul_perfil.cpp` mede separadamente alocação,
inicialização, multiplicação e checksum. Ela preserva as fórmulas de inicialização,
a inicialização de `C` com zero e a ordem de laços `i-k-j` do baseline. O coletor
compila o baseline e a cópia instrumentada com as mesmas flags e compara o checksum
de cada aquecimento e repetição com o baseline da mesma dimensão. Uma divergência
encerra a coleta sem publicar o CSV.

## Metodologia de coleta preparada

- Dimensões: `N=256`, `512` e `1024`.
- Um aquecimento separado por dimensão, excluído do CSV.
- Cinco repetições medidas por dimensão.
- Compilação: `g++ -O2 -std=c++17 -Wall -Wextra -Wpedantic -lm`.
- Relógio monotônico (`CLOCK_MONOTONIC`).
- Registro de plataforma, processador, CPUs lógicas, compilador, flags, intervalo
  UTC, hashes SHA-256 das fontes e checksums esperados.
- Resumo por etapa com média, mediana, desvio-padrão amostral, mínimo, máximo e
  participação da mediana da etapa no tempo total mediano.

A mediana é a referência para a participação percentual por ser menos sensível
a interferências do sistema. O desvio-padrão, mínimo e máximo expõem a dispersão.

## Estado da complementação e hotspots

Uma primeira tentativa em 10 de novembro de 2026 foi bloqueada no host Windows.
Depois, a coleta foi executada em Docker sobre a revisão `02c0765`; os dados
brutos e o ambiente estão em
`resultados/01_coletas/complementares/perfil_mpi_2026-11-10/` e o resumo em
`resultados/02_analises/complementares/perfil_sequencial_resumo.csv`.

| N | Etapa | Média | Mediana | Desvio-padrão | Participação |
|---:|---|---:|---:|---:|---:|
| 256 | multiplicação | 0,007178 s | 0,007390 s | 0,000813 s | 90,60% |
| 512 | multiplicação | 0,051235 s | 0,051046 s | 0,001801 s | 94,12% |
| 1024 | multiplicação | 0,532086 s | 0,538348 s | 0,040020 s | 97,39% |

A complexidade indica a multiplicação (`O(N³)`) como hotspot provável, enquanto
as demais etapas são `O(N²)`. Isso é uma hipótese baseada na estrutura do código,
não uma conclusão experimental. Sem o CSV complementar não há identificação
quantitativa de hotspot, e esta tarefa permanece parcialmente concluída.

## Reprodução no Docker do projeto

Na raiz do repositório, use um nome novo para não sobrescrever resultados históricos:

```bash
docker compose build pp-lab
docker compose run --rm pp-lab python3 scripts/coletar_perfil_sequencial.py \
  --sizes 256 512 1024 --repetitions 5 \
  --output resultados/01_coletas/complementares/perfil_sequencial_nova.csv

docker compose run --rm pp-lab python3 scripts/analisar_complementacao.py \
  --sequencial resultados/01_coletas/complementares/perfil_sequencial_nova.csv \
  --output-dir resultados/02_analises/complementares/perfil_sequencial_nova
```

Os arquivos esperados são o CSV bruto, o arquivo
`perfil_sequencial_etapas_2026-11-10_ambiente.txt` e o CSV de resumo. Os scripts
usam criação exclusiva e recusam sobrescrever destinos existentes.

## Limitações da instrumentação

- Trechos muito curtos podem se aproximar da resolução do relógio e sofrer maior
  influência proporcional do custo de instrumentação.
- `total_s` inclui os pequenos intervalos entre regiões; as participações podem
  somar menos de 100%.
- A alocação de `C` inclui sua inicialização com zero, como no baseline.
- O perfil mede tempo de parede e não fornece contadores de hardware ou cache misses.
- Resultados dependem da carga, frequência e política de energia do host; o arquivo
  de ambiente deve acompanhar qualquer CSV publicado.

## Uso futuro como baseline

Depois da coleta real e da revisão dos checksums, a mediana da multiplicação pode
ser usada como `T_seq` nos cálculos de speedup e eficiência. Ela não deve ser
misturada com o tempo total instrumentado sem declarar a região comparada.
