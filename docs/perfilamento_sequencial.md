# Perfilamento do Algoritmo Sequencial (Baseline)

## 1. Ambiente de Execução
Para garantir a reprodutibilidade dos experimentos, os testes foram executados no seguinte ambiente de hardware e software:
* **Processador (CPU):** AMD Ryzen 5 3400G (3.70 GHz)
* **Memória RAM:** 8,00 GB (2666 MT/s)
* **Sistema Operacional / Terminal:** Windows (execução via MINGW64, sem uso de container Docker nesta etapa)
* **Compilador:** g++ (GCC)
* **Flags de compilação:** `-O2` (Otimização de nível 2)

## 2. Metodologia
O código original do professor (`01_matmul.cpp`) foi utilizado sem qualquer alteração. O programa realiza a multiplicação de duas matrizes densas $N \times N$ gerando uma matriz resultante $C$. 

As dimensões escolhidas para o benchmark foram $N = 256$, $N = 512$ e $N = 1024$. Para cada dimensão, o executável foi rodado 5 vezes para isolar oscilações do sistema operacional. O tempo de execução e o *checksum* determinístico foram registrados.

## 3. Resultados Obtidos
Abaixo estão os cálculos de média, mediana e variação extraídos das 5 repetições registradas em `resultados/baseline_sequencial.csv`:

| Dimensão (N) | Checksum | Tempo Médio | Tempo Mediano | Variação (Mín - Máx) |
| :--- | :--- | :--- | :--- | :--- |
| **256** | 4109165.296000 | 0.004799 s | 0.004798 s | 0.004777 s – 0.004832 s |
| **512** | 32883431.310401 | 0.048124 s | 0.049412 s | 0.040243 s – 0.055978 s |
| **1024** | 263088437.159955 | 0.546761 s | 0.542017 s | 0.536388 s – 0.566012 s |

*A mediana foi escolhida como métrica de referência principal, pois é menos sensível a eventuais picos de atraso causados por processos em segundo plano no Windows.*

## 4. Limite das evidências históricas

Os tempos acima medem somente a região de multiplicação delimitada pelo código
original. Eles não separam alocação, inicialização e checksum e, portanto, **não
constituem perfilamento de hotspots por trecho**. A inspeção dos três laços e a
complexidade O(N³) indicam onde se espera maior trabalho, mas não são tempos
medidos nem permitem publicar uma participação percentual real.

O algoritmo utiliza uma **ordem de laços i-k-j**. Isso foi projetado especificamente para otimizar os acessos à memória:
1. Ao invés da tradicional abordagem `i-j-k`, que causaria saltos na memória (gerando muitos *cache misses* ao ler colunas de forma vertical), a ordem `i-k-j` navega de forma sequencial nos vetores contíguos alocados na memória.
2. A leitura sequencial maximiza a localidade espacial. O processador traz blocos inteiros de dados da RAM para a memória Cache (L1/L2/L3) de uma só vez, reduzindo o gargalo de I/O de memória.

## 5. Procedimento complementar preparado

A cópia `codigos/instrumentacao/01_matmul_perfil.cpp` preserva a entrada, a ordem
`i-k-j` e o checksum do baseline, acrescentando relógios em torno de alocação,
inicialização, multiplicação e checksum. O arquivo original do professor não é
modificado. O coletor faz um aquecimento por dimensão, cinco repetições, valida
checksums, recusa sobrescrita e registra ambiente, compilador, flags e hash da
fonte:

```bash
python3 scripts/coletar_perfil_sequencial.py \
  --sizes 256 512 1024 --repetitions 5 \
  --output resultados/perfil_sequencial_complementar.csv

python3 scripts/analisar_complementacao.py \
  --sequencial resultados/perfil_sequencial_complementar.csv \
  --output-dir resultados/complementacao_marco2
```

O segundo comando gera `perfil_sequencial_resumo.csv`, com mediana por trecho e
participação relativa à mediana do total. Tempos muito curtos devem ser tratados
com cautela por causa da resolução do relógio e do custo da própria
instrumentação. Neste host de revisão não havia compilador C++, Docker nem uma
distribuição WSL; logo, o procedimento não foi executado e nenhuma tabela de
tempos foi preenchida artificialmente.

## 6. Uso como baseline
Estes tempos medianos do algoritmo puramente sequencial representam o nosso *Baseline* ($T_{seq}$). Eles serão a referência fundamental para as medições futuras. Quando os códigos paralelizados forem integrados, utilizaremos estes exatos números para calcular:
* **Speedup ($S$):** O quão mais rápido o código ficou ($S = T_{seq} / T_{paralelo}$).
* **Eficiência:** O aproveitamento real de cada thread OpenMP ou processo MPI adicionado.
