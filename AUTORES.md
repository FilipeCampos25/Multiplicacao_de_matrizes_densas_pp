# Autores e contribuições

- **Prof. Rodrigo Gonçalves Pinto:** autor do baseline sequencial
  `codigos/baselines_sequenciais_CCO085/01_matmul.cpp` e dos exemplos didáticos
  mantidos em `codigos/baselines_sequenciais_CCO085/`.
- **Davi:** perfilamento sequencial, automação inicial de benchmarks em Bash,
  estruturação da extração CSV e documentação técnica e de reprodutibilidade.
- **Grupo do projeto:** adaptação do baseline para OpenMP e MPI, validação de
  argumentos, instrumentação, verificação numérica, scripts de coleta e análise,
  testes automatizados e documentação de integração.

## Limite de atribuição

O kernel matemático, a inicialização determinística e a ordem `i-k-j` têm como
referência o baseline do professor. A paralelização, a distribuição MPI, as
proteções, os testes e a automação foram desenvolvidos ou adaptados pelo grupo.
Os arquivos originais do professor não devem ser modificados.
