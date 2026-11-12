# Autores e contribuições

## Integrantes e divisão confirmada

| Integrante | Matrícula | Responsabilidade principal confirmada pela equipe |
| --- | --- | --- |
| Filipe de Campos Duarte | 2412130132 | implementação MPI e integração final |
| Fabrício de Campos Duarte | 2412130139 | OpenMP, testes de corretude e análise experimental |
| Davi Vinícius de Sousa Costa | 2412130133 | baseline, perfilamento, scripts de benchmark e documentação |

A divisão acima foi confirmada pelo grupo. O histórico Git sustenta atividades compatíveis, mas não é usado para inferir exclusividade ou medir esforço individual.

## Contribuições adicionais sustentadas pelo histórico

- **Filipe:** implementação MPI e testes iniciais (PR #15), instrumentação MPI (PR #17), integração das implementações, testes, análise e documentação (PR #19), além de ajustes posteriores.
- **Fabrício:** commits das Issues #4, #6, #8 e #10 adicionaram OpenMP, suíte de corretude, coleta OpenMP e consolidação experimental com CSVs, gráficos e análise. Aparecem sob `Fabricio2407` e `FabricioCampos`, com o mesmo e-mail registrado.
- **Davi:** commits de `davivcosta` documentaram perfilamento e ambiente sequencial, adaptaram o benchmark e a coleta CSV e registraram metodologia, referências e documentação.

Na auditoria de 10/11/2026, `git shortlog -sne --all` apresentou 30 commits sob a identidade de Filipe, 12 sob as duas identidades de Fabrício (mesmo e-mail) e 3 sob a identidade de Davi. Essas contagens incluem merges e não medem esforço, relevância ou aprovação da divisão proposta.

Commits de merge ou revisão não provam autoria exclusiva do conteúdo incorporado. Atividades coletivas sem evidência individual suficiente permanecem atribuídas ao grupo.

## Código fornecido pelo professor

O **Prof. Rodrigo Gonçalves Pinto** é autor do código sequencial original `codigos/00_sequencial/01_matmul.cpp` e dos exemplos didáticos em `codigos/material_professor_CCO085/`. São material fornecido ao grupo, não contribuição autoral dos integrantes.

O kernel, a inicialização determinística, o armazenamento row-major e a ordem `i-k-j` derivam desse baseline. Paralelização OpenMP, distribuição MPI, validações, testes, automação, experimentos e documentação foram desenvolvidos ou adaptados pelo grupo.
