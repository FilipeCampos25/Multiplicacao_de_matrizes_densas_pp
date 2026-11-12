# Resumo de testes

## Evidência histórica da complementação

- Comandos: `docker compose build`, `make clean`, `make all`, `make test`, `make test-correctness` e `python3 -m unittest discover -s tests -v`.
- Data: 10/11/2026 (America/Sao_Paulo).
- Commit: `02c0765d26881a9bc07602faf77af1cfe9fb71c9`.
- Ambiente: imagem `iesb/pp-lab:2026.2`, ID `sha256:c987e7098115a132c78a82e11ca66525e6813ded2e35359bcfb6a962aa0968a8`.
- Resultado: todos os comandos encerraram com código zero; 142 casos de corretude e 168 testes gerais aprovados.
- Avisos: Open MPI informou falha ao fixar memória; a validação usou snapshot Git com LF.

Os seis logs extensos numerados e `testes.json` foram removidos da árvore principal. Seus hashes e conteúdos permanecem verificáveis no commit `3eca2a8` e no histórico Git.

## Regressão da reorganização

- Análise oficial antes/depois: 175/175 observações e 35/35 grupos.
- Chaves, medianas, médias, desvios padrão, coeficientes de variação e séries brutas: equivalência exata.
- Checksums: equivalência exata.
- Speedup, eficiência e escalabilidade: mesmas fórmulas e mesmas entradas; valores matematicamente equivalentes.
- CSV estranho: a seleção é explícita por `--input`, portanto arquivos não selecionados não alteram a análise oficial.

## Execução após a refatoração (11/11/2026)

- `python -m unittest discover -s tests -v`: **falhou** no Windows, 168 testes executados, 26 aprovados e 142 falhas. As 142 falhas são da suíte de corretude e informam binários Linux ausentes em `bin/`; não se declara sucesso para a suíte completa.
- Os 16 testes unitários de análise, validação de CSV, artefatos e benchmark passaram quando executados no mesmo processo; os 10 testes do próprio harness também passaram.
- Limitação restante: executar `make all && make test-correctness && python3 -m unittest discover -s tests -v` no ambiente Linux/Docker suportado para validar os executáveis sequencial, OpenMP e MPI.

## Validação final (12/11/2026)

- `docker compose run --rm -T pp-lab make all`: código zero.
- `docker compose run --rm -T pp-lab make test`: código zero; 24 comparações
  OpenMP e 9 configurações MPI consistentes com o baseline.
- `docker compose run --rm -T pp-lab make test-correctness`: **142 PASS, 0 FAIL,
  0 SKIP**.
- `docker compose run --rm -T pp-lab python3 -m unittest discover -s tests -v`:
  **169 testes, OK**.
- Regeneração controlada da análise em `%TEMP%`: 175 observações, 35 grupos,
  quatro arquivos derivados e 16 SVGs; nenhum artefato oficial sobrescrito.
- Ensaio mínimo do benchmark OpenMP em `/tmp`: bruto/metadados em diretório de
  coleta, resumo/status no diretório de análise e proteção contra sobrescrita.
- Validação estrutural: quatro CSVs novos com 175, 180, 36 e 36 registros; 16
  SVGs bem formados; nenhum link local quebrado; nenhum arquivo experimental
  solto na raiz de `resultados/`.

A execução direta da suíte completa no Python do Windows não é uma validação do
binário Linux: retornou 27 testes independentes aprovados, 107 falhas por
`WinError 193` e 35 casos MPI ignorados por ausência do launcher. O resultado
aprovado acima é o do ambiente Docker suportado.
