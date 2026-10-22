# Análise do algoritmo sequencial de multiplicação de matrizes densas

## 1. Introdução e objetivo

Este documento analisa o baseline sequencial de multiplicação de matrizes densas da disciplina de Programação Paralela (CCO085), implementado em [`01_matmul.cpp`](../codigos/baselines_sequenciais_CCO085/01_matmul.cpp). O objetivo é descrever seu funcionamento, custo computacional, comportamento de memória e possibilidades de paralelização futura, mantendo a implementação original.

O programa calcula **C = A × B**, em que A, B e C são matrizes quadradas de dimensão N × N, com elementos do tipo `double`. Cada elemento do resultado é o produto escalar da linha correspondente de A com a coluna correspondente de B:

```text
C[i, j] = Σ (A[i, k] × B[k, j]), para k = 0, ..., N − 1.
```

Os índices `i` e `j` também variam de 0 a N − 1. A representação é densa: todos os N² elementos de cada matriz são armazenados e processados, inclusive aqueles cujo valor é zero. Não há uma estrutura específica para explorar esparsidade.

A análise é estática, fundamentada no código-fonte. Não foram realizados experimentos de desempenho, e as considerações sobre cache e paralelismo não constituem ganhos medidos.

## 2. Estruturas de dados e inicialização

### Dimensão e armazenamento

A dimensão é obtida pelo primeiro argumento da linha de comando. Quando esse argumento não existe, o programa utiliza N = 512:

```cpp
int N = (argc > 1) ? atoi(argv[1]) : 512;
vector<double> A(N*N), B(N*N), C(N*N, 0.0);
```

Assim, `./01_matmul 512` solicita matrizes de dimensão 512 × 512, enquanto `./01_matmul` utiliza a mesma dimensão por padrão. A conversão é feita por `atoi`, e o código não valida a entrada. A análise pressupõe N positivo, memória disponível e operações inteiras dentro da faixa representável. Isso inclui expressões como `N*N`, os índices e `i * 7` na inicialização; dimensões arbitrariamente grandes não são garantidas pelo baseline.

O código usa `using namespace std`, de modo que `vector<double>` corresponde a `std::vector<double>`. Cada vetor contém N² elementos contíguos e representa uma matriz em armazenamento por linhas (*row-major*). O elemento da linha `i` e da coluna `j` ocupa a posição:

```text
índice linear = i*N + j
```

O termo `i*N` avança sobre as `i` linhas anteriores, e `j` seleciona a coluna dentro da linha atual. Por exemplo, para N = 3, o elemento da linha 1 e coluna 2, com índices iniciando em zero, ocupa a posição `1*3 + 2 = 5`. Portanto, os elementos de uma mesma linha são vizinhos na memória.

### Valores iniciais

As matrizes de entrada recebem valores pelo seguinte laço:

```cpp
for (int i = 0; i < N*N; i++) {
    A[i] = (i % 100) * 0.01;
    B[i] = ((i * 7) % 100) * 0.01;
}
```

Nesse trecho, `i` é um índice linear que percorre o vetor inteiro, e não apenas o número de uma linha. Em A, o resto de `i` por 100 produz a sequência de inteiros de 0 a 99, repetida ao longo do vetor. A multiplicação por `0.01` fornece valores de 0,00 a 0,99, sujeitos à representação em ponto flutuante.

Em B, o índice é primeiro multiplicado por 7 e depois reduzido módulo 100. Os primeiros valores são 0,00; 0,07; 0,14; e assim por diante, com retorno ao intervalo de 0,00 a 0,99 após a operação de resto. Como 7 e 100 são coprimos, os 100 resíduos aparecem em uma ordem diferente antes da repetição. As sequências seguem os índices lineares e não são reiniciadas explicitamente a cada linha.

A matriz C é inicializada integralmente com `0.0` pelo construtor `C(N*N, 0.0)`. Isso é necessário porque o núcleo utiliza `+=` para acumular contribuições. Os vetores A e B também têm seus elementos inicialmente zerados pelo construtor com tamanho, mas todos esses valores são substituídos pelo laço acima antes da multiplicação.

Não há geração aleatória. Para a mesma dimensão e as mesmas condições de representação e avaliação numérica, a inicialização é determinística. Isso permite reproduzir as entradas e comparar a corretude de futuras implementações com o baseline sequencial.

## 3. Funcionamento do algoritmo sequencial

O fluxo do programa consiste em definir N, alocar e inicializar as matrizes, registrar o instante inicial, executar a multiplicação, registrar o instante final, calcular o checksum e imprimir os resultados.

A multiplicação utiliza três laços na ordem `i-k-j`. O laço externo seleciona uma linha de C; o intermediário seleciona uma contribuição para essa linha; e o interno aplica essa contribuição a todas as suas colunas:

| Índice | Papel no cálculo | Elementos envolvidos |
| --- | --- | --- |
| `i` | Seleciona a linha do resultado | Linha `i` de A e de C |
| `k` | Percorre a dimensão de acumulação | Elemento A[i, k] e linha `k` de B |
| `j` | Percorre as colunas do resultado | Elementos B[k, j] e C[i, j] |

Para cada par `(i, k)`, o valor de A[i, k] é armazenado na variável local `a`. Em seguida, cada coluna `j` recebe a contribuição `a * B[k*N + j]`. Após a conclusão de todos os valores de `k`, cada elemento da linha `i` de C contém a soma dos N produtos exigidos pela definição de multiplicação matricial.

Embora o programa atualize uma linha inteira de C a cada valor de `k`, cada elemento C[i, j] recebe suas contribuições em ordem crescente de `k`. A inicialização com zero fornece o ponto de partida da soma.

## 4. Análise de complexidade temporal e espacial

Cada um dos três laços da multiplicação executa N iterações. Portanto, o comando de atualização de C é executado:

```text
N × N × N = N³ vezes.
```

Cada atualização contém uma multiplicação e uma adição em ponto flutuante, além dos acessos e cálculos de índices. No modelo em que essas operações têm custo constante, o núcleo apresenta complexidade temporal **Θ(N³)** e, consequentemente, **O(N³)**. São N³ multiplicações e N³ adições no nível do algoritmo; essa contagem não determina o número de instruções de máquina, pois o compilador pode transformar as operações.

A inicialização de A e B percorre N² posições, e o preenchimento inicial dos vetores também tem custo proporcional à quantidade de elementos. Essa etapa tem complexidade O(N²), distinta do custo cúbico da multiplicação. O checksum percorre C uma vez e também custa O(N²). Assim, para entradas válidas e suficientemente grandes, o termo cúbico domina o crescimento do trabalho total.

O armazenamento principal corresponde a três matrizes com N² elementos cada:

```text
memória dos elementos = 3 × N² × sizeof(double) bytes.
```

Nas plataformas em que `sizeof(double) = 8`, isso equivale a 24N² bytes. Os objetos de controle dos vetores e as variáveis escalares acrescentam espaço constante em relação a N. A complexidade espacial total é, portanto, **Θ(N²)**, também expressa como **O(N²)**. O núcleo não cria outra matriz ou um vetor temporário proporcional ao tamanho da entrada.

## 5. Região computacionalmente intensiva

O principal candidato a concentrar o tempo de execução é o núcleo abaixo, delimitado no código pelas chamadas que obtêm `t0` e `t1`:

```cpp
for (int i = 0; i < N; i++)
    for (int k = 0; k < N; k++) {
        double a = A[i*N + k];
        for (int j = 0; j < N; j++)
            C[i*N + j] += a * B[k*N + j];
    }
```

A expressão `a * B[k*N + j]` produz uma contribuição para C[i, j], e o operador `+=` soma essa contribuição ao valor acumulado. Como esse comando é executado N³ vezes, a região concentra o maior volume de trabalho para matrizes suficientemente grandes, em comparação com a inicialização e o checksum quadráticos. A fração efetiva do tempo consumida por ela depende da execução e precisaria ser confirmada por perfilamento.

A leitura `A[i*N + k]` aparece fora do laço interno. No nível do código, são N² atribuições a `a`, cada uma reutilizada em N atualizações de C. O compilador pode manter esse escalar em um registrador, reduzindo a necessidade de buscar repetidamente o mesmo elemento de A. A quantidade real de acessos à memória depende do código de máquina gerado e da hierarquia de memória.

## 6. Ordem dos laços e hierarquia de memória

### Localidade na ordem i-k-j

Com `i` e `k` fixos, o incremento de `j` faz os índices `B[k*N + j]` e `C[i*N + j]` avançarem de uma posição por vez. O laço interno percorre sequencialmente a linha `k` de B e a linha `i` de C. Como os vetores usam armazenamento por linhas, elementos usados em sequência ocupam endereços adjacentes.

Esse padrão favorece a **localidade espacial**: uma linha de cache que contém um elemento pode conter também os próximos elementos acessados. O percurso regular ainda pode facilitar a busca antecipada de dados pelo hardware, dependendo do processador.

A **localidade temporal** aparece na reutilização de `a` durante todo o laço interno. O mesmo valor de A participa de N produtos sem mudar, e sua permanência em registrador pode atender a essa reutilização. A linha atual de C também é reutilizada a cada novo `k`; o benefício depende de seus dados permanecerem em algum nível de cache durante as varreduras.

### Hierarquia de cache e limitações

Em uma hierarquia típica, a cache L1 é menor e de menor latência, a L2 oferece capacidade intermediária e a L3, quando presente, tem maior capacidade e costuma ser compartilhada entre núcleos. A organização concreta varia conforme o processador. Dados ausentes em um nível precisam ser buscados em níveis posteriores ou, quando necessário, na memória principal.

Um *cache miss* pode introduzir espera pelo dado. Além da latência, a largura de banda limita a taxa de transferência de dados entre os níveis de memória. Assim, o custo das operações aritméticas não explica sozinho o tempo de execução; o fornecimento dos operandos e o tráfego de leitura e escrita também podem restringir o desempenho.

A ordem `i-k-j` favorece acessos contíguos, mas não garante que o conjunto de dados caiba nas caches. Para cada novo `i`, B é percorrida novamente; se seus dados não permanecerem em cache, serão necessários novos carregamentos de níveis mais distantes. A linha de C também é lida e atualizada repetidamente ao longo de `k`. Sua retenção depende da capacidade, da associatividade e da competição com outros dados. O código não divide o cálculo em blocos para ajustar explicitamente o conjunto de trabalho à cache.

### Comparação com a ordem i-j-k

Na ordem tradicional `i-j-k`, cada par `(i, j)` é fixado enquanto `k` varia para calcular um elemento de C. A tabela compara os padrões lógicos das duas ordens sob o mesmo armazenamento por linhas:

| Aspecto | Ordem i-k-j do baseline | Ordem i-j-k |
| --- | --- | --- |
| A no laço interno | Reutiliza um escalar A[i, k] | Percorre a linha `i` de A |
| B no laço interno | Percorre uma linha contígua | Percorre a coluna `j`, com salto de N elementos |
| C no laço interno | Atualiza colunas consecutivas da linha `i` | Acumula sobre um único elemento C[i, j] |
| Principal oportunidade de reutilização | Escalar de A e linha de C | Acumulador de um elemento de C |

O acesso por colunas de B em `i-j-k` pode aproveitar menos os elementos trazidos em cada linha de cache, especialmente para N grande. Em contrapartida, calcular um único elemento de C por vez pode favorecer sua manutenção em um acumulador escalar. Na ordem do baseline, as atualizações de C são distribuídas pela linha e repetidas a cada `k`.

Ambas as ordens mantêm complexidade O(N³). A vantagem efetiva de uma ordem depende da dimensão, das caches, das otimizações do compilador e do hardware. Os padrões descritos fundamentam uma expectativa de melhor localidade espacial para B em `i-k-j`, mas não demonstram um ganho de desempenho sem medições comparáveis.

## 7. Oportunidades futuras de paralelização

O laço externo `i` permite distribuir linhas distintas de C entre trabalhadores. A iteração de índice `i` escreve apenas nas posições de `i*N` a `i*N + N − 1`. Iterações com índices de linha diferentes escrevem em conjuntos disjuntos, enquanto A e B são apenas lidas durante o núcleo. Por isso, diferentes linhas de C podem ser calculadas independentemente.

Em memória compartilhada, **OpenMP** poderia distribuir as iterações do laço externo entre threads. Para preservar a corretude, cada linha de C deve ter um único responsável; os índices de trabalho e a variável `a` devem ser privados de cada execução; e a inicialização deve estar concluída antes do cálculo. Também é necessária sincronização ao final do trabalho antes de registrar o término e ler C para validação.

Paralelizar diretamente `k` exige cuidado adicional: dois trabalhadores poderiam executar `+=` sobre o mesmo C[i, j], provocando condição de corrida. Uma abordagem desse tipo precisaria combinar somas parciais ou adotar sincronização apropriada. Se o checksum também fosse paralelizado no futuro, a atualização de `soma` exigiria uma redução ou mecanismo equivalente.

Linhas distintas não criam uma dependência lógica de escrita, mas elementos próximos atribuídos a threads diferentes podem compartilhar uma linha de cache. Esse possível compartilhamento falso é uma consideração de desempenho, sem alterar a independência matemática entre as linhas.

Com **MPI**, uma possibilidade é distribuir blocos de linhas de A entre processos, disponibilizar B aos processos participantes e calcular os blocos correspondentes de C localmente. A distribuição de entradas e a reunião dos resultados, quando necessária, introduziriam comunicação e sincronização. A replicação de B também teria custo de memória por processo. A escolha da distribuição e sua escalabilidade dependeriam de avaliação posterior.

As futuras versões devem utilizar as mesmas entradas e comparar seus resultados com o baseline. Recomenda-se a comparação elemento a elemento, com tolerâncias absoluta e relativa adequadas ao uso de `double`, pois alterações na ordem das somas ou nas otimizações podem modificar o arredondamento. A independência das linhas oferece uma oportunidade de paralelismo, mas não estabelece speedup, eficiência ou escalabilidade sem experimentação. Esta análise não implementa OpenMP nem MPI.

## 8. Medição de desempenho e validação

### Região cronometrada

A função auxiliar `agora()` consulta um relógio monotônico e converte o resultado para segundos:

```cpp
static double agora() {
    timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}
```

`CLOCK_MONOTONIC` permite medir intervalos sem depender de ajustes descontínuos do relógio civil. O campo `tv_sec` fornece segundos, e `tv_nsec * 1e-9` converte a parte em nanossegundos. O intervalo `t1 - t0` representa tempo decorrido, e não apenas tempo de CPU consumido pelo processo; portanto, pode refletir interferências de escalonamento.

A chamada que define `t0` ocorre depois da alocação e da inicialização. A chamada que define `t1` ocorre imediatamente após os três laços. Assim, a região medida corresponde ao núcleo de multiplicação, com a pequena sobrecarga associada à instrumentação. Ela não inclui a inicialização, o cálculo do checksum ou a impressão. A unidade de saída é o segundo, e o código imprime seis casas decimais, o que não equivale a garantir precisão de um microssegundo.

### Checksum e limites da validação

Após a medição, o programa soma todos os elementos de C e apresenta a dimensão, o checksum e o tempo:

```cpp
double soma = 0.0;
for (int i = 0; i < N*N; i++) soma += C[i];
printf("N=%d  checksum=%.6f  tempo=%.6f s\n", N, soma, t1 - t0);
```

O checksum oferece uma verificação básica de consistência entre execuções com a mesma entrada e pode revelar erros que alterem a soma final. Entretanto, matrizes diferentes podem ter a mesma soma: permutações dos elementos ou erros que se compensem podem passar despercebidos. Além disso, a impressão com seis casas decimais oculta diferenças menores, e a própria soma está sujeita a arredondamento.

Consequentemente, a igualdade do checksum não prova igualdade entre matrizes. Para validar futuras implementações, a comparação elemento a elemento deve complementar essa verificação resumida, levando em conta tolerâncias numéricas. Nenhum tempo, checksum experimental ou resultado de perfilamento é apresentado neste documento.

## 9. Conclusão técnica

O baseline calcula o produto de matrizes densas em vetores contíguos, com entradas determinísticas e acumulação na ordem `i-k-j`. Seu custo temporal é O(N³), enquanto a inicialização, o checksum e o armazenamento apresentam ordem O(N²). O núcleo de três laços é a região de maior crescimento computacional e o alvo natural de futuras análises de desempenho.

A ordem adotada combina reutilização de um escalar de A com acessos sequenciais às linhas de B e C. Esse padrão favorece a localidade, mas não elimina limitações de cache ou de largura de banda. A independência entre linhas do resultado permite estudar distribuição de trabalho com OpenMP e MPI, desde que sejam preservadas a exclusividade das escritas e a validação numérica. A análise fornece a base técnica para essas etapas posteriores, sem modificar o código sequencial nem atribuir ganhos experimentais a possibilidades ainda não avaliadas.
