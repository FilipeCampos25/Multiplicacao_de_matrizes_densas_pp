# Complementação validada — 10/11/2026

Os três itens solicitados foram executados sobre o commit
`02c0765d26881a9bc07602faf77af1cfe9fb71c9`, da branch
`refactor/organizacao-sequencial-openmp-mpi`. Nenhum algoritmo ou fórmula
foi modificado para produzir as evidências.

## Escopo, ambiente e reversão dos scripts

A conversão local de CRLF para LF feita durante a investigação foi revertida
nos quatro arquivos: `scripts/medir_speedup.sh`, `scripts/test_matmul.sh`,
`scripts/test_matmul_openmp.sh` e `scripts/test_matmul_mpi.sh`.
Seus bytes de quebra de linha voltaram ao formato Windows anterior; não houve
reversão da reorganização de diretórios. O conteúdo versionado desses scripts
já era LF e não recebeu alteração de lógica nem novo patch.

Para não alterar novamente essa cópia local, foi usado `git archive` do commit
acima, extraído em diretório temporário. O Compose dessa cópia montou suas fontes
em `/lab`. Essa é a versão reorganizada versionada, não a pasta Windows com CRLF.
Executar `make test` diretamente sobre aquela pasta ainda pode reproduzir o erro
original; não foi feita mudança de configuração do Git, editor ou sincronização.

Ambiente observado: Docker Desktop 29.1.3, Ubuntu 24.04 sobre WSL2,
Intel Core i5-1334U, 12 CPUs lógicas expostas e aproximadamente 8 GB de memória
na VM Docker; GCC 13.3.0, Open MPI 4.1.6 e Python 3.12.3.
O contêiner não apresentou quota adicional de CPU/memória nos arquivos cgroup
consultados. Horários brutos estão em UTC: 11/11/2026 01:15–01:17 corresponde
a 10/11/2026 22:15–22:17 em America/Sao_Paulo.

## Testes executados

| Comando no Docker | Resultado | Log |
| --- | --- | --- |
| `docker compose build` | Saída 0 | [build](../resultados/complementacao_2026-11-10_02c0765/teste_0.txt) |
| `make clean` | Saída 0, na cópia temporária | [clean](../resultados/complementacao_2026-11-10_02c0765/teste_1.txt) |
| `make all` | Sequencial, duas versões OpenMP e MPI compilados | [compilação](../resultados/complementacao_2026-11-10_02c0765/teste_2.txt) |
| `make test` | 24 comparações OpenMP e 9 MPI aprovadas | [integração](../resultados/complementacao_2026-11-10_02c0765/teste_3.txt) |
| `make test-correctness` | PASS=142, FAIL=0, SKIP=0 | [corretude](../resultados/complementacao_2026-11-10_02c0765/teste_4.txt) |
| `python3 -m unittest discover -s tests -v` | 168 testes, OK | [suíte geral](../resultados/complementacao_2026-11-10_02c0765/teste_5.txt) |

As cinco últimas linhas foram executadas com `docker compose run --rm pp-lab`.
Todos os comandos usaram `-f` apontando ao Compose da cópia temporária.
Os [códigos de saída e horários](../resultados/complementacao_2026-11-10_02c0765/testes.json)
complementam os logs. Os 168 testes incluem os 142 casos funcionais; não devem
ser somados como testes independentes.
Algumas mensagens de stderr do Docker aparecem no log do PowerShell com o rótulo
`NativeCommandError`; os códigos de saída registrados foram zero.

## Perfilamento sequencial

[CSV bruto](../resultados/complementacao_2026-11-10_02c0765/perfil_sequencial.csv),
[ambiente e checksums](../resultados/complementacao_2026-11-10_02c0765/perfil_sequencial_ambiente.txt)
e [resumo completo](../resultados/complementacao_2026-11-10_02c0765/resumos/perfil_sequencial_resumo.csv).

Foram executados um aquecimento e cinco repetições por N=256, 512 e 1024,
com 15 observações válidas. Cada checksum foi comparado ao baseline original,
compilado no mesmo ambiente. Tempos abaixo em segundos; desvio-padrão amostral.

| N | Etapa | Média (s) | Mediana (s) | Desvio-padrão (s) | Mediana da etapa / total mediano (%) |
| ---: | --- | ---: | ---: | ---: | ---: |
| 256 | alocacao | 0.000672894 | 0.000604401 | 0.000170501 | 7.41 |
| 256 | inicializacao | 0.000118880 | 0.000119236 | 0.000002802 | 1.46 |
| 256 | multiplicacao | 0.007178201 | 0.007389706 | 0.000813445 | 90.60 |
| 256 | checksum | 0.000075752 | 0.000048747 | 0.000046383 | 0.60 |
| 512 | alocacao | 0.002342173 | 0.002178216 | 0.000316190 | 4.02 |
| 512 | inicializacao | 0.000589451 | 0.000550802 | 0.000108597 | 1.02 |
| 512 | multiplicacao | 0.051234736 | 0.051045586 | 0.001800565 | 94.12 |
| 512 | checksum | 0.000260416 | 0.000292668 | 0.000069798 | 0.54 |
| 1024 | alocacao | 0.009084126 | 0.008789794 | 0.000760715 | 1.59 |
| 1024 | inicializacao | 0.002393949 | 0.002060263 | 0.000570599 | 0.37 |
| 1024 | multiplicacao | 0.532085533 | 0.538347604 | 0.040019755 | 97.39 |
| 1024 | checksum | 0.000762429 | 0.000726670 | 0.000061690 | 0.13 |

Nesta coleta, a multiplicação foi o trecho dominante nas três dimensões.
As razões entre medianas não são uma decomposição exata de uma execução individual
e não equivalem, por si só, a uma fração paralelizável universal.
Não foram recalculados os gráficos ou as fórmulas de Amdahl dos dados históricos.

## MPI por etapas

[CSV bruto](../resultados/complementacao_2026-11-10_02c0765/mpi_etapas.csv),
[ambiente](../resultados/complementacao_2026-11-10_02c0765/mpi_etapas_ambiente.txt),
[resumo por etapa](../resultados/complementacao_2026-11-10_02c0765/resumos/etapas_mpi_resumo.csv)
e [resumo por configuração](../resultados/complementacao_2026-11-10_02c0765/resumos/configuracoes_mpi_resumo.csv).

Foram executadas 12 configurações: N=256/512/1024, p=1/2/4/8,
com um aquecimento e cinco repetições por configuração. O arquivo contém
60 observações MPI e 15 sequenciais, todas com status `ok`.
O analisador confirmou cobertura, número de repetições, tempos válidos e
distribuição mínima/máxima de linhas. Os testes prévios também incluem N não
divisível por p e p>N.

As etapas medidas são Scatterv, Bcast, computação e Gatherv, além do total.
Os tempos usam máximos entre ranks. O proxy de comunicação fornecido pelo
analisador soma medianas de máximos de etapas: **não é aditivo nem reconstrói
o caminho crítico**. Speedup e eficiência neste resumo são relativos a MPI p=1,
não ao baseline sequencial.

Foi solicitado `--use-hwthread-cpus --bind-to hwthread`, sem
`--oversubscribe`; p máximo 8, abaixo das 12 CPUs lógicas expostas.
O Open MPI emitiu **failed to bind memory** durante a verificação anterior à
coleta. As execuções continuaram e os checksums passaram, mas não se afirma
afinidade plenamente garantida. Isso limita inferências de desempenho e
comparações com outras máquinas. A execução foi em um único nó virtualizado,
sem controle exclusivo da carga do host.

## Reprodução e rastreabilidade

O [protocolo executado](../resultados/complementacao_2026-11-10_02c0765/reproduzir_coletas.sh)
registra os comandos e parâmetros das coletas. O [log](../resultados/complementacao_2026-11-10_02c0765/coletas.txt)
inclui ambiente, avisos e conclusão do analisador. Para repetir, use uma nova
cópia extraída do Git e altere o destino `OUT` no protocolo para um diretório
novo; não o execute sobre a pasta de evidências existente.

O [manifesto desta complementação](../resultados/complementacao_2026-11-10_02c0765/manifesto.json)
registra o commit, imagem Docker, hashes das fontes executadas, binários,
coletores e artefatos. Os logs de testes foram convertidos de UTF-16 do
PowerShell para UTF-8, preservando o texto, antes do cálculo dos hashes.

Os 34 arquivos preexistentes em `resultados/` foram conferidos por SHA-256
antes e depois e permanecem intactos, incluindo `analise_manifesto.json`.
As novas evidências estão em diretório próprio e não foram misturadas às
estatísticas históricas. A exceção adicionada ao `.gitignore` torna esse
diretório visível para versionamento.

Esta complementação não reavalia autoria, ensaio de apresentação, experimentos
entre nós ou outras pendências externas aos três itens solicitados.
