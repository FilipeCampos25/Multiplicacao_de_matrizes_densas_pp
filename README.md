# Ambiente de Laboratório — Programação Paralela (CCO085)

**IESB 2026/2 · Prof. Rodrigo Gonçalves Pinto**

Este é o ambiente oficial da disciplina. Todas as medições reportadas no
Projeto Acadêmico IESB devem ser feitas aqui, para que os resultados sejam
comparáveis entre os trios.

## Origem do ambiente

A estrutura inicial deste repositório foi baseada no ambiente de desenvolvimento fornecido pelo professor da disciplina CCO085 — Programação Paralela.

O grupo preservou o ambiente fornecido e selecionou os componentes relevantes ao problema de multiplicação de matrizes densas. O código sequencial de referência está localizado em:
`codigos/baselines_sequenciais_CCO085/01_matmul.cpp`

## Estado do projeto

### Fornecido pela disciplina

- Ambiente Docker e Dev Container;
- Makefile e scripts auxiliares;
- Documentação do ambiente;
- Baseline sequencial de multiplicação de matrizes densas;
- Exemplos em Python e notebook.

### Desenvolvimento do grupo

- [x] Seleção e organização do baseline correspondente ao tema;
- [x] Estruturação inicial do repositório;
- [ ] Análise do algoritmo sequencial;
- [ ] Perfilamento e identificação dos hotspots;
- [ ] Medição do baseline para diferentes entradas;
- [ ] Estimativa da fração paralelizável e aplicação da Lei de Amdahl;
- [ ] Implementação com OpenMP;
- [ ] Implementação com MPI;
- [ ] Validação de corretude;
- [ ] Análise de speedup, eficiência e escalabilidade.