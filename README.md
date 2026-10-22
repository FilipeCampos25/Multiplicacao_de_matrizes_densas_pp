# Ambiente de Laboratório – Programação Paralela (CCO085)

**IESB 2026/2 • Prof. Rodrigo Gonçalves Pinto**

Este é o ambiente oficial da disciplina. Todas as medições reportadas no Projeto Acadêmico IESB devem ser feitas aqui, para que os resultados sejam comparáveis entre os trios. 

O objetivo deste projeto é analisar o desempenho e a escalabilidade da multiplicação de matrizes densas, comparando um *baseline* sequencial com futuras implementações paralelas utilizando OpenMP e MPI.

- Sequencial: `codigos/baselines_sequenciais_CCO085/01_matmul.cpp`;
- OpenMP: `codigos/01_openmp/matmul_openmp.cpp`;
- MPI: `codigos/02_mpi/matmul_mpi.cpp`.

A estrutura inicial deste repositório foi baseada no ambiente de desenvolvimento fornecido pelo professor da disciplina CCO085 – Programação Paralela.

## Reprodução no ambiente oficial

## Compilação, Execução e Benchmarks

O ambiente pode ser executado localmente (utilizando `g++`) ou através do Docker / Dev Container (`docker-compose up -d`) para garantir a compatibilidade das bibliotecas e ferramentas (OpenMP e OpenMPI).

**Execução Sequencial Local:**
```bash
g++ -O2 codigos/baselines_sequenciais_CCO085/01_matmul.cpp -o 01_matmul
./01_matmul 512

Estado do projeto
Fornecido pela disciplina
Ambiente Docker e Dev Container;

Makefile e scripts auxiliares;

Documentação do ambiente;

Baseline sequencial de multiplicação de matrizes densas;

Exemplos em Python e notebook.

Desenvolvimento do grupo
[x] Seleção e organização do baseline correspondente ao tema;

[x] Estruturação inicial do repositório;

[x] Análise do algoritmo sequencial;

[x] Perfilamento e identificação dos hotspots;

[x] Medição do baseline para diferentes entradas;

[ ] Estimativa da fração paralelizável e aplicação da Lei de Amdahl;

[ ] Implementação com OpenMP;

[ ] Implementação com MPI;

[ ] Validação de corretude;

[ ] Análise de speedup, eficiência e escalabilidade.
