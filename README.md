# Multiplicação de matrizes densas — Programação Paralela (CCO085)

Projeto do IESB 2026/2, orientado pelo Prof. Rodrigo Gonçalves Pinto, para comparar multiplicação sequencial de matrizes densas com versões OpenMP e MPI, preservando a entrada determinística, o armazenamento row-major e a ordem `i-k-j` do código fornecido.

## Integrantes

- Filipe de Campos Duarte — 2412130132
- Fabrício de Campos Duarte — 2412130139
- Davi Vinícius de Sousa Costa — 2412130133

Responsabilidades e autoria: [AUTORES.md](AUTORES.md).

## Organização

| Caminho | Conteúdo |
| --- | --- |
| `codigos/baselines_sequenciais_CCO085/` | código sequencial e exemplos didáticos do professor |
| `codigos/01_openmp/` | versões OpenMP didática e de benchmark |
| `codigos/02_mpi/` | implementação MPI |
| `scripts/` e `tests/` | testes, coletores e análise |
| `resultados/` | CSVs, metadados, resumos e manifesto |
| `resultados/graficos/` | gráficos SVG gerados |
| `docs/` | metodologia, reprodução, auditoria e análise |

## Implementações

- **Sequencial:** `01_matmul.cpp`, baseline original e referência matemática.
- **OpenMP:** divide linhas de C entre threads; A e B são somente leitura e cada thread escreve em linhas exclusivas. `matmul_openmp.cpp` integra a coleta conjunta; `01_matmul_openmp.cpp` é didático.
- **MPI:** distribui linhas de A com `MPI_Scatterv`, replica B com `MPI_Bcast` e reúne C com `MPI_Gatherv`, inclusive para `N % P != 0` e `P > N`.

Consulte [metodologia](docs/metodologia.md) e [compilação e execução](docs/compilacao.md).

## Compilar, executar e validar

```bash
docker compose build
docker compose run --rm pp-lab
make clean
make all

./bin/base/01_matmul 512
OMP_NUM_THREADS=4 ./bin/openmp/matmul_openmp 512 --verify
mpirun -np 4 ./bin/mpi/matmul_mpi 512 --verify

make test
make test-correctness
python3 -m unittest discover -s tests -v
```

O registro preservado de 22/10/2026 informa **142 PASS, 0 FAIL e 0 SKIP** para baseline, OpenMP e MPI. É evidência histórica em `resultados/issue10_desempenho_ambiente.txt`, não uma execução desta revisão documental. Cobertura e tolerâncias: [testes de corretude](docs/testes_corretude.md).

## Reproduzir benchmarks

Use nomes novos: o coletor recusa sobrescrever resultados existentes.

```bash
make collect-performance BENCHMARK_ARGS="--sizes 256 512 1024 \
  --workers 1 2 4 8 --repetitions 5 --weak-base 256 \
  --output resultados/nova_coleta.csv"
python3 scripts/analisar_desempenho.py

make benchmark-openmp BENCHMARK_ARGS="--sizes 256 512 1024 \
  --threads 1 2 4 8 --schedules static dynamic guided \
  --output resultados/openmp_nova_desempenho.csv"
```

A coleta comparável de 22/10/2026 está em [`resultados/issue10_desempenho.csv`](resultados/issue10_desempenho.csv), com 175 observações, e seu ambiente em [`resultados/issue10_desempenho_ambiente.txt`](resultados/issue10_desempenho_ambiente.txt). CSVs derivados ficam em `resultados/`, os 21 gráficos em [`resultados/graficos/`](resultados/graficos/) e a interpretação em [análise experimental](docs/analise_desempenho.md).

## Limitações e pendências

- Matrizes são quadradas e geradas internamente; não há entrada externa nem dimensões retangulares.
- MPI foi medido em uma máquina via WSL2/Docker, sem rede entre nós; B é replicada e o rank 0 mantém A, B e C completas.
- Afinidades OpenMP/MPI não são idênticas e há alta variabilidade em alguns grupos; não há superioridade universal demonstrada.
- A coleta conjunta não separa tempos das etapas MPI; essa decomposição permanece futura.
- Mudanças de código exigem nova regressão: os resultados descrevem os artefatos e condições de 22/10/2026.
- A divisão formal em `AUTORES.md` ainda requer validação explícita dos integrantes.
