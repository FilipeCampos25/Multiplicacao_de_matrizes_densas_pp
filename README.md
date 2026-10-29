# Multiplicação de matrizes densas — Programação Paralela (CCO085)

**IESB 2026/2 — Prof. Rodrigo Gonçalves Pinto**

O projeto compara a multiplicação sequencial de matrizes densas com adaptações
OpenMP e MPI, preservando a mesma entrada determinística, o armazenamento
row-major e a ordem de laços `i-k-j`.

## Implementações

- Baseline original: `codigos/baselines_sequenciais_CCO085/01_matmul.cpp`;
- OpenMP didático: `codigos/01_openmp/01_matmul_openmp.cpp`;
- OpenMP usado nos benchmarks: `codigos/01_openmp/matmul_openmp.cpp`;
- MPI: `codigos/02_mpi/matmul_mpi.cpp`.

O baseline e os demais arquivos em `codigos/baselines_sequenciais_CCO085/`
foram disponibilizados pelo professor e permanecem como referências originais.
As versões paralelas, as proteções de entrada, a verificação, a instrumentação,
os scripts e os testes são adaptações ou implementações do grupo.

## Rastreabilidade dos exemplos oficiais

| Exemplo oficial | Conceito aproveitado | Aplicação no projeto |
| --- | --- | --- |
| `01_matmul.cpp` | Inicialização, row-major, kernel e checksum | Referência matemática das versões OpenMP e MPI |
| `004_loop_for_paralell.cpp` | `omp parallel for` e `schedule(static)` | Distribuição do laço externo `i`, uma linha de C por iteração |
| `005_loop_for_paralell_time.cpp` | `omp_get_wtime()`, `single` e equipe efetiva | Tempo do kernel e registro de threads realmente utilizadas |
| `007_reduction_0.0.cpp` | Redução de acumuladores | Avaliado para o checksum, mas não adotado para preservar a ordem sequencial da soma |

Os exemplos vetoriais 004, 005 e 007 fornecem conceitos de OpenMP; eles não são
tratados como implementações prontas de multiplicação de matrizes.

## Por que a paralelização é correta

Cada iteração do laço externo calcula uma linha completa de C. Threads ou ranks
diferentes escrevem em linhas distintas, enquanto A e B são somente leitura.
Assim, não são necessários `critical`, `atomic` ou mutex no kernel. A ordem
interna `k-j` é preservada.

No MPI, `MPI_Scatterv` distribui blocos possivelmente desiguais de linhas de A,
`MPI_Bcast` entrega B completa a todos os ranks e `MPI_Gatherv` recompõe C em
ordem. A forma variável das coletivas cobre `N % P != 0` e ranks sem linhas
quando `P > N`.

## Ambiente oficial e comandos principais

```bash
docker compose build
docker compose run --rm pp-lab

make clean
make all
make test
make test-correctness
python3 -m unittest discover -s tests -v
make benchmark-openmp-build
```

Execuções diretas:

```bash
./bin/base/01_matmul 512
OMP_NUM_THREADS=4 ./bin/openmp/matmul_openmp 512 --verify
mpirun -np 4 ./bin/mpi/matmul_mpi 512 --verify
```

Use `MATMUL_DUMP=1` somente para matrizes pequenas quando for necessário
inspecionar C elemento por elemento. Consulte `docs/compilacao.md` para os
comandos de benchmark e para a política de arquivos de saída.

## Estado de validação

Na validação mais recente, os módulos Python independentes passaram: 8 testes
de análise, 6 do coletor OpenMP e 10 do detector de corretude. A compilação e os
142 casos funcionais de baseline/OpenMP/MPI permaneceram bloqueados no host de
validação, que não possuía Make, compiladores, MPI, Docker ou WSL instalado.
Bloqueio não equivale a aprovação; a sequência completa acima ainda deve ser
executada no ambiente oficial antes da avaliação final.

## Estado do projeto

- [x] Baseline original identificado e preservado;
- [x] Kernel sequencial analisado e perfilado;
- [x] Implementações OpenMP e MPI desenvolvidas;
- [x] Testes de checksum e comparação elemento por elemento preparados;
- [x] Automação de benchmarks OpenMP e análise de resultados preparada;
- [x] Resultados OpenMP existentes documentados com ambiente e metodologia;
- [ ] Regressão funcional final no Docker/Dev Container;
- [ ] Coleta e análise MPI comparável no ambiente oficial;
- [ ] Conclusão quantitativa conjunta de speedup e escalabilidade.

O escopo usa apenas matrizes quadradas N×N geradas internamente. Matrizes
retangulares e entrada de matrizes externas não fazem parte desta entrega.
