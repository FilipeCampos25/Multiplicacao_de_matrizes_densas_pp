# Multiplicação de Matrizes Densas — Programação Paralela (CCO085)

Projeto acadêmico desenvolvido no **Instituto de Educação Superior de Brasília (IESB)**, durante o segundo semestre de 2026, na disciplina de **Programação Paralela (CCO085)**, sob orientação do **Prof. Rodrigo Gonçalves Pinto**.

O projeto implementa, executa e avalia a multiplicação de matrizes densas quadradas utilizando três abordagens:

- **Sequencial (C++):** implementação de referência fornecida pelo professor.
- **OpenMP:** paralelização em memória compartilhada por meio de threads.
- **MPI:** paralelização por processos com comunicação e distribuição de dados.

O objetivo é investigar a corretude, o desempenho, o speedup e a eficiência das implementações, considerando diferentes dimensões de matrizes e quantidades de trabalhadores.

## 1. Integrantes

| Integrante | Matrícula |
|---|---|
| Filipe de Campos Duarte | 2412130132 |
| Fabrício de Campos Duarte | 2412130139 |
| Davi Vinícius de Sousa Costa | 2412130133 |

A divisão de responsabilidades, as contribuições identificadas no histórico Git e as pendências de validação da autoria estão documentadas em [AUTORES.md](AUTORES.md).

## 2. Objetivos

### Objetivo geral

Implementar e avaliar estratégias de paralelização para multiplicação de matrizes densas, comparando seus resultados com uma implementação sequencial de referência.

### Objetivos específicos

1. Preservar a lógica matemática e a inicialização determinística do código sequencial fornecido.
2. Implementar a distribuição do processamento entre threads usando OpenMP.
3. Implementar a distribuição de dados e processamento entre processos usando MPI.
4. Verificar a equivalência dos resultados numéricos.
5. Avaliar speedup e eficiência em experimentos de escalabilidade forte e fraca.
6. Comparar políticas de escalonamento OpenMP.
7. Documentar ambiente, metodologia, limitações e resultados.
8. Disponibilizar uma estrutura reproduzível de compilação, execução e testes com Docker.

## 3. Tecnologias utilizadas

| Tecnologia | Finalidade |
|---|---|
| C++17 | Implementação dos algoritmos |
| OpenMP | Paralelismo com memória compartilhada |
| MPI / OpenMPI | Paralelismo com processos |
| Python 3 | Validação, testes, coleta e análise |
| GNU Make | Automação de compilação e execução |
| Docker / Docker Compose | Padronização do ambiente |
| Ubuntu 24.04 | Sistema-base do contêiner |
| Git / GitHub | Versionamento e colaboração |

O ambiente Docker contém os compiladores e as dependências necessárias para compilar e executar as implementações.

## 4. Organização do repositório

```text
Multiplicacao_de_matrizes_densas_pp/
│
├── codigos/
│   ├── 00_sequencial/
│   │   └── 01_matmul.cpp
│   ├── 01_openmp/
│   │   ├── 01_matmul_openmp.cpp
│   │   └── matmul_openmp.cpp
│   ├── 02_mpi/
│   │   └── matmul_mpi.cpp
│   ├── instrumentacao/
│   │   └── 01_matmul_perfil.cpp
│   └── material_professor_CCO085/
│       └── ... (21 exemplos didáticos preservados)
│
├── scripts/
│   ├── test_matmul.sh
│   ├── test_matmul_openmp.sh
│   ├── test_matmul_mpi.sh
│   ├── benchmark_mpi.sh
│   ├── benchmark_openmp.py
│   ├── coletar_desempenho.py
│   ├── analisar_desempenho.py
│   └── ...
│
├── tests/
├── notebooks/
├── python/
├── docs/
├── resultados/
│   └── graficos/
│
├── Dockerfile
├── docker-compose.yml
├── Makefile
├── AUTORES.md
└── README.md
```

Os executáveis são gerados em `bin/` durante a compilação. Essa pasta pode ser removida e recriada com o Makefile.

As três abordagens estão em `00_sequencial` (referência sequencial de corretude e desempenho fornecida pelo professor), `01_openmp` (threads e memória compartilhada) e `02_mpi` (processos e comunicação MPI). A relação didática é **Sequencial → OpenMP → MPI → Comparação experimental**.

`instrumentacao` contém instrumentos auxiliares de análise e perfilamento, não uma quarta implementação independente. `material_professor_CCO085` preserva os exemplos de aula, separados das implementações avaliadas e excluídos da compilação, dos testes e dos benchmarks principais. O executável sequencial continua em `bin/base/01_matmul`.

A pasta `resultados/` armazena dados experimentais, metadados e gráficos produzidos pelas ferramentas de análise.

## 5. Implementações

### 5.1. Implementação sequencial

O algoritmo sequencial é a referência matemática do projeto.

Arquivo principal:

`codigos/00_sequencial/01_matmul.cpp`

São utilizadas matrizes quadradas de dimensão N × N, armazenadas em memória contígua no formato *row-major*.

A multiplicação segue a ordem de laços:

```cpp
for (int i = 0; i < N; i++) {
    for (int k = 0; k < N; k++) {
        for (int j = 0; j < N; j++) {
            C[i * N + j] += A[i * N + k] * B[k * N + j];
        }
    }
}
```

A complexidade computacional do algoritmo é O(N³).

A inicialização determinística permite comparar os resultados das implementações sem depender de entradas aleatórias.

O código sequencial original é atribuído ao professor e foi preservado como referência.

### 5.2. Implementação OpenMP

Arquivos:

- `codigos/01_openmp/01_matmul_openmp.cpp`
- `codigos/01_openmp/matmul_openmp.cpp`

O OpenMP distribui as iterações do laço externo entre threads.

Cada thread processa linhas específicas da matriz C, enquanto as matrizes A e B permanecem compartilhadas e são utilizadas somente para leitura.

Características:

- Preservação da ordem interna `i-k-j`.
- Distribuição de linhas entre threads.
- Escrita em regiões distintas da matriz resultante.
- Ausência de necessidade de locks no kernel de multiplicação.
- Medição de tempo com `omp_get_wtime()`.
- Verificação de resultados com a implementação sequencial.

A versão didática utiliza distribuição estática. A versão de benchmark permite comparar as políticas `static`, `dynamic` e `guided` por meio de compilações configuradas para cada política.

### 5.3. Implementação MPI

Arquivo:

`codigos/02_mpi/matmul_mpi.cpp`

A implementação MPI utiliza processos independentes para distribuir a multiplicação de matrizes.

A estratégia emprega:

| Operação MPI | Responsabilidade |
|---|---|
| `MPI_Scatterv` | Distribuir blocos de linhas de A |
| `MPI_Bcast` | Disponibilizar B aos processos |
| Cálculo local | Multiplicar os blocos atribuídos |
| `MPI_Gatherv` | Reunir os resultados na matriz C |

O processo de rank 0 é responsável pela inicialização e pela reunião do resultado global.

A implementação contempla dimensões que não são divisíveis pela quantidade de processos e casos em que alguns processos recebem zero linhas.

## 6. Pré-requisitos

### Ambiente recomendado: Windows 10/11 + Docker Desktop

Requisitos:

- Git.
- Docker Desktop instalado e funcionando.
- Backend WSL2 habilitado.
- Docker configurado para contêineres Linux.
- Recursos computacionais suficientes para as dimensões escolhidas.

**Não é necessário instalar GCC, GNU Make, OpenMPI ou Python diretamente no Windows.** Essas ferramentas já estão disponíveis na imagem Docker.

O arquivo `docker-compose.yml` monta a pasta do repositório em `/lab`, que é o diretório de trabalho do contêiner.

### Clonagem do projeto

No terminal:

```powershell
git clone https://github.com/FilipeCampos25/Multiplicacao_de_matrizes_densas_pp.git
cd Multiplicacao_de_matrizes_densas_pp
```

## 7. Compilação e execução

### 7.1. Método recomendado — PowerShell + Docker

Execute os comandos no diretório raiz do repositório.

Construir a imagem:

```powershell
docker compose build
```

Limpar compilações anteriores:

```powershell
docker compose run --rm pp-lab make clean
```

Compilar todas as implementações:

```powershell
docker compose run --rm pp-lab make all
```

A compilação gera os executáveis correspondentes em:

```text
bin/
├── base/
│   └── 01_matmul
├── openmp/
│   ├── 01_matmul_openmp
│   └── matmul_openmp
└── mpi/
    └── matmul_mpi
```

### 7.2. Executar os testes automatizados

Ainda no PowerShell:

```powershell
docker compose run --rm pp-lab make test
docker compose run --rm pp-lab make test-correctness
docker compose run --rm pp-lab python3 -m unittest discover -s tests -v
```

Todos esses comandos são executados no ambiente Linux do Docker.

**Importante:** os executáveis produzidos dentro do contêiner são binários Linux. Não tente executá-los diretamente no PowerShell ou Git Bash do Windows.

### 7.3. Método alternativo — Terminal interativo do Docker

Para executar vários comandos na mesma sessão, abra o Bash do contêiner:

```powershell
docker compose run --rm pp-lab
```

Dentro do contêiner, execute:

```bash
make clean
make all
make test
make test-correctness
python3 -m unittest discover -s tests -v
```

Nesse ambiente também é possível executar os programas individualmente.

**Sequencial:**

```bash
./bin/base/01_matmul 512
```

**OpenMP — 4 threads:**

```bash
OMP_NUM_THREADS=4 ./bin/openmp/matmul_openmp 512 --verify
```

**OpenMP — versão didática:**

```bash
./bin/openmp/01_matmul_openmp 256 4
```

**MPI — 4 processos:**

```bash
mpirun -np 4 ./bin/mpi/matmul_mpi 512 --verify
```

A opção `--verify`, disponível nas versões paralelas de benchmark, solicita uma verificação adicional do resultado.

Também é possível executar diretamente em Linux/WSL2 quando todos os compiladores e runtimes necessários estiverem instalados.

## 8. Testes e validação de corretude

O repositório possui três mecanismos complementares de validação.

### 8.1. Testes de integração — `make test`

Executa os scripts Bash responsáveis por comparar a implementação sequencial com OpenMP e MPI.

São avaliadas diferentes dimensões, quantidades de threads e processos, incluindo configurações com distribuição irregular de linhas.

### 8.2. Suíte de corretude — `make test-correctness`

A suíte Python executa os binários reais e verifica:

- Checksum em relação ao baseline sequencial.
- Consistência entre resultados esperados e obtidos.
- Diferentes dimensões e configurações de paralelismo.
- Resultados elemento a elemento para dimensões pequenas.
- Distribuição de linhas entre processos MPI.
- Saídas inválidas, erros numéricos e comportamentos de execução.

Para dimensões até 10, são realizadas comparações complementares com uma referência Python e verificação dos elementos exportados pelas versões paralelas.

Para dimensões maiores, a validação concentra-se em checksums e metadados. Portanto, igualdade de checksum não é apresentada como prova isolada de igualdade de todos os elementos.

### 8.3. Suíte geral — `unittest`

Executa os testes de corretude, testes do mecanismo de validação e verificações das ferramentas de benchmark e análise.

Os testes incluem verificações que asseguram a rejeição de resultados incorretos, dados incompletos, valores não finitos e condições de timeout.

## 9. Resultados da validação — 05/11/2026

> **Atualização de 10/11/2026:** a versão reorganizada (`02c0765`) foi compilada
> e validada em cópia temporária extraída do Git: `make test` aprovado,
> 142 casos de corretude e 168 testes gerais aprovados. Também foram executadas
> as novas coletas de perfilamento sequencial e MPI por etapas. Consulte as
> [evidências da complementação](docs/complementacao_validada.md), incluindo
> a distinção entre a cópia versionada LF e os scripts CRLF da pasta local.
> A seção abaixo mantém o registro histórico de 05/11.
>
> **Validação final de 12/11/2026:** no serviço Docker atual, `make all`,
> `make test`, `make test-correctness` e a suíte geral passaram. Foram 142 casos
> de corretude e 169 testes gerais, sem falhas ou casos ignorados.

As implementações foram compiladas e testadas em ambiente Windows com Docker Desktop e contêiner Linux.

Os resultados abaixo foram observados em execução local documentada no terminal em **05 de novembro de 2026**.

### 9.1. Compilação e testes

| Etapa | Resultado |
|---|---|
| `docker compose build` | Sucesso |
| `make all` | Compilação concluída |
| `make test` | Sequencial, OpenMP e MPI consistentes |
| `make test-correctness` | 142 PASS, 0 FAIL, 0 SKIP |
| `python3 -m unittest discover -s tests -v` | 167 testes, OK |

Na execução de `make test`, foram validadas as configurações OpenMP com 1, 2, 4 e 8 threads para as dimensões 1, 2, 7, 10, 64 e 128, além de nove configurações MPI.

A execução terminou com:

```text
OK: baseline, OpenMP e MPI produziram resultados consistentes.
```

### 9.2. Testes de corretude

```text
Ran 142 tests in 15.883s

OK

Resumo: PASS=142 FAIL=0 SKIP=0
```

A distribuição dos casos funcionais foi:

| Implementação | Casos aprovados |
|---|---:|
| Sequencial | 11 |
| OpenMP | 96 |
| MPI | 35 |
| **Total** | **142** |

### 9.3. Testes gerais

```text
Ran 167 tests in 16.957s

OK
```

A suíte geral inclui os 142 casos funcionais anteriores, além de testes adicionais das ferramentas e verificadores. Esses resultados não representam 309 testes independentes.

### 9.4. Observações

A compilação MPI apresentou avisos `-Wcast-function-type` provenientes de cabeçalhos C++ da biblioteca OpenMPI. Esses avisos não impediram a criação do executável nem a aprovação dos testes.

Os testes confirmam a corretude das implementações nos casos avaliados. Eles não substituem medições de desempenho nem comprovam ganhos universais de paralelização.

## 10. Avaliação experimental de desempenho

O projeto também contém ferramentas para coleta e análise de desempenho.

As análises consideram:

- Tempo de execução.
- Speedup.
- Eficiência paralela.
- Escalabilidade forte.
- Escalabilidade fraca.
- Políticas de escalonamento OpenMP.
- Cenários teóricos da Lei de Amdahl.
- Possíveis gargalos e limitações experimentais.

### 10.1. Speedup

O speedup é definido por:

\[
S(p) = \frac{T_{\text{referência}}}{T_p}
\]

Onde:

- \(T_{\text{referência}}\) é o tempo de referência explicitado no experimento.
- \(T_p\) é o tempo da execução com p trabalhadores.

A referência adotada deve ser preservada ao comparar os resultados.

### 10.2. Eficiência

A eficiência paralela é calculada por:

\[
E(p) = \frac{S(p)}{p}
\]

O indicador relaciona o ganho de desempenho à quantidade de trabalhadores utilizados.

### 10.3. Escalabilidade

**Escalabilidade forte:** mantém a dimensão do problema constante e altera a quantidade de trabalhadores.

**Escalabilidade fraca:** aumenta o tamanho do problema em função da quantidade de trabalhadores, procurando manter aproximadamente constante o trabalho computacional por trabalhador.

Esses experimentos são analisados separadamente, respeitando suas diferentes referências.

## 11. Como reproduzir benchmarks

**Atenção:** os comandos desta seção devem ser executados dentro do Bash do contêiner Docker ou em ambiente Linux/WSL2 configurado.

### 11.1. Coleta conjunta — Sequencial, OpenMP e MPI

```bash
make collect-performance BENCHMARK_ARGS="--sizes 256 512 1024 \
  --workers 1 2 4 8 --repetitions 5 --weak-base 256 \
  --output resultados/01_coletas/complementares/coleta_conjunta_nova.csv"
```

O coletor utiliza uma organização experimental que inclui medições das implementações disponíveis e verificações de consistência.

O caminho informado em `--output` deve ser novo. O coletor recusa sobrescrever uma coleta existente.

### 11.2. Benchmark específico OpenMP

```bash
make benchmark-openmp BENCHMARK_ARGS="--sizes 256 512 1024 \
  --threads 1 2 4 8 --repetitions 5 \
  --schedules static dynamic guided \
  --output resultados/01_coletas/complementares/openmp_nova/openmp_desempenho.csv \
  --analysis-dir resultados/02_analises/complementares/openmp_nova"
```

O benchmark permite investigar o comportamento das diferentes políticas de escalonamento.

Configurações que ultrapassem recursos disponíveis podem ser ignoradas explicitamente pela ferramenta.

### 11.3. Analisar resultados existentes

```bash
make analyze-performance
```

Ou:

```bash
python3 scripts/analisar_desempenho.py
```

Esse procedimento utiliza arquivos de resultados existentes e não executa novamente as multiplicações.

A análise pode gerar arquivos derivados e gráficos na pasta `resultados/`. Para separar uma análise nova dos artefatos atuais, utilize diretórios de saída próprios conforme os parâmetros do analisador.

## 12. Resultados experimentais históricos

O repositório preserva uma coleta conjunta datada de **22/10/2026**, contendo 175 observações.

Arquivos principais:

- [Coleta conjunta — CSV](resultados/01_coletas/principal/issue10_desempenho.csv)
- [Registro do ambiente experimental](resultados/01_coletas/principal/issue10_desempenho_ambiente.txt)
- [Análise experimental](docs/analise_desempenho.md)
- [Gráficos de desempenho](resultados/03_graficos/)

A documentação experimental apresenta 16 gráficos SVG, incluindo resultados de speedup, eficiência, escalabilidade e cenários de Amdahl.

As complementações de novembro estão registradas separadamente:

- [Perfilamento sequencial](docs/perfilamento_sequencial.md): instrumentação e protocolo por etapas; a [nova coleta quantitativa](docs/complementacao_validada.md) foi executada em 10/11/2026.
- [Análise experimental](docs/analise_desempenho.md): estimativa efetiva de Amdahl derivada do speedup histórico, com suas limitações.
- [Escalabilidade MPI](docs/escalabilidade_mpi.md): instrumentação e 60 medições complementares por etapas, preservadas separadamente da coleta principal.
- [Checklist final do Marco 2](docs/checklist_entrega_marco2.md): estado por requisito, evidências, testes e pendências em 10/11/2026.
- [Referências técnicas](docs/referencias.md): fontes acadêmicas e padrões oficiais usados na fundamentação.

Nas medições documentadas, o OpenMP apresentou ganhos de desempenho com múltiplas threads, mas com redução de eficiência em parte das configurações. O MPI apresentou comportamento dependente da dimensão e do número de processos, com regressões ao aumentar de quatro para oito processos em determinados casos.

Essas observações pertencem ao ambiente e à coleta registrados. Não representam garantia de desempenho em outros computadores.

**Importante:** as validações funcionais de 05/11/2026 confirmaram a execução e a corretude dos códigos, mas não constituem uma nova coleta completa de benchmarks. Os resultados experimentais de outubro permanecem identificados como históricos.

Os commits de complementação de 10/11/2026 são posteriores à validação de 05/11. A primeira tentativa de revalidação foi bloqueada no host da auditoria. Posteriormente, a revisão reorganizada `02c0765` passou no ambiente Docker, conforme os [logs e resultados da nova validação](docs/complementacao_validada.md). Os registros antigos não substituem essa evidência específica da revisão.

## 13. Solução de problemas

### Docker Desktop indisponível

Erros envolvendo `dockerDesktopLinuxEngine`, conexão recusada ou named pipe geralmente indicam indisponibilidade do Docker Engine.

Confirme que o Docker Desktop foi iniciado e que o modo de contêineres Linux está habilitado.

```powershell
docker version
docker compose version
```

### Erro `set: pipefail: invalid option name`

Esse erro pode ocorrer quando arquivos Bash possuem finais de linha Windows (`CRLF`) em vez de Linux (`LF`).

Verifique os atributos e os finais de linha do checkout:

```powershell
git check-attr text eol -- scripts/test_matmul.sh
git ls-files --eol
```

Para evitar recorrência, recomenda-se manter os scripts `.sh` em LF e configurar `.gitattributes` com:

```gitattributes
*.sh text eol=lf
```

Caso o checkout local ainda contenha scripts em CRLF, normalize os arquivos preservando alterações locais antes de repetir os testes.

### Erro `Exec format error`

Esse erro pode ocorrer quando se tenta executar no Windows um binário Linux compilado dentro do Docker.

Exemplo de execução correta:

```powershell
docker compose run --rm pp-lab ./bin/base/01_matmul 512
```

Para executar comandos como `make`, `python3` ou `mpirun`, utilize o contêiner ou um ambiente Linux configurado.

### Executáveis não encontrados

O comando:

```bash
make clean
```

remove a pasta `bin/`.

Após a limpeza, compile novamente:

```powershell
docker compose run --rm pp-lab make all
```

### MPI não encontrado no Windows

O MPI está disponível dentro da imagem Docker do projeto.

Execute os programas MPI pelo contêiner, sem depender de uma instalação nativa no Windows.

### Avisos durante compilação MPI

Avisos oriundos de cabeçalhos internos do OpenMPI não significam necessariamente falha de compilação.

Verifique o código de retorno da compilação e execute a suíte de validação antes de considerar uma implementação aprovada.

## 14. Limitações conhecidas

O projeto possui algumas limitações importantes:

- As matrizes são quadradas e geradas internamente; não há suporte a dimensões retangulares ou leitura externa de matrizes.
- A referência sequencial original é preservada e não incorpora todas as validações adicionais das versões paralelas.
- As medições MPI documentadas foram realizadas em ambiente de máquina única com Docker/WSL2, não em um cluster distribuído.
- A replicação da matriz B e a utilização de memória pelos processos MPI afetam os recursos necessários.
- Os resultados dependem de hardware, limites do contêiner, escalonamento e condições do ambiente.
- A comparação entre OpenMP e MPI não demonstra superioridade universal de uma abordagem.
- A coleta conjunta histórica não apresenta separadamente os tempos de todas as etapas de comunicação MPI.
- Os testes de corretude não equivalem a uma demonstração formal de ausência de erros para todas as entradas possíveis.

## 15. Documentação complementar

| Documento | Conteúdo |
|---|---|
| [AUTORES.md](AUTORES.md) | Integrantes, responsabilidades e histórico de contribuições |
| [Metodologia](docs/metodologia.md) | Fundamentação das implementações e experimentos |
| [Compilação e execução](docs/compilacao.md) | Procedimentos técnicos detalhados |
| [Testes de corretude](docs/testes_corretude.md) | Casos, tolerâncias e mecanismos de validação |
| [Análise experimental](docs/analise_desempenho.md) | Métricas, gráficos, resultados e limitações |

## 16. Conclusão

O projeto integra três abordagens de multiplicação de matrizes densas e fornece mecanismos para compilação, testes automatizados, coleta de dados e análise experimental.

Na validação local realizada em **05/11/2026**, as implementações sequencial, OpenMP e MPI compilaram e passaram pelos testes de integração. A suíte de corretude registrou **142 casos aprovados, sem falhas ou casos ignorados**, enquanto a suíte geral `unittest` concluiu **167 testes com resultado OK**.

Esses resultados oferecem evidência de funcionamento e consistência matemática nas condições verificadas.

A análise experimental preservada no repositório permite examinar como paralelismo, dimensão das matrizes, distribuição de trabalho e características do ambiente influenciam o desempenho observado.

O projeto busca, assim, relacionar a implementação prática de programação paralela à validação rigorosa de resultados e à interpretação cuidadosa de métricas experimentais.

---

**Instituto de Educação Superior de Brasília — IESB**

**Disciplina:** Programação Paralela — CCO085

**Professor:** Rodrigo Gonçalves Pinto

**Período:** 2026/2

**Repositório:** [Multiplicacao_de_matrizes_densas_pp](https://github.com/FilipeCampos25/Multiplicacao_de_matrizes_densas_pp)
