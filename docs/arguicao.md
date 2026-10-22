# Preparação para arguição individual

## Perguntas e respostas essenciais

**Qual é a diferença entre processo e thread?**  Processos têm espaços de
endereçamento independentes e comunicam explicitamente; threads do mesmo
processo compartilham memória e outros recursos, mas têm pilhas próprias.

**Memória compartilhada e distribuída?**  OpenMP trabalha normalmente em um
nó e todas as threads acessam os mesmos vetores. Em MPI, cada rank possui seu
próprio endereço; por isso A é distribuída, B é transmitida e C é reunida.

**O que é uma condição de corrida?**  É quando o resultado depende da ordem de
acessos concorrentes, com pelo menos uma escrita sem sincronização adequada.
Aqui cada thread/rank calcula linhas exclusivas de C; A e B são somente leitura.

**O que faz `#pragma omp parallel for`?**  Cria/usa uma equipe de threads e
distribui as iterações do laço seguinte. No código, `parallel` foi separado de
`for` para registrar quantas threads foram realmente criadas.

**Por que `schedule(static)`?**  Cada linha tem praticamente o mesmo trabalho
`O(N²)`. Divisão estática equilibra a carga com overhead menor que escalonamento
dinâmico ou guiado. Esses outros modos só devem ser comparados experimentalmente.

**Para que servem `MPI_Init` e `MPI_Finalize`?**  Inicializam e encerram o
ambiente MPI. Nenhuma operação MPI deve ocorrer antes da inicialização ou após
o encerramento.

**O que faz `MPI_Bcast`?**  Copia o mesmo buffer do rank raiz para todos os
ranks. Aqui fornece a matriz B inteira a cada processo.

**Por que `MPI_Scatterv` e `MPI_Gatherv`, não `Scatter/Gather`?**  As versões
com `v` aceitam contagens e deslocamentos distintos. Isso permite distribuir N
linhas mesmo quando N não é múltiplo de P e reunir blocos de tamanhos diferentes.

**Como funciona a divisão irregular?**  Cada rank recebe `floor(N/P)` linhas;
os primeiros `N mod P` recebem mais uma. A diferença máxima é uma linha. Se
`P>N`, alguns ranks recebem zero linhas, caso coberto pelos buffers válidos.

**Computação versus comunicação?**  A multiplicação é `O(N³)`, enquanto o
volume principal de dados é `O(N²)`. Para N pequeno, latência e coletivas podem
dominar; para N maior, a computação tende a amortizá-las, até limites de memória
e largura de banda surgirem.

**O que são speedup e eficiência?**  `S(p)=Tseq/Tpar(p)` e `E(p)=S(p)/p`.
Speedup ideal seria p e eficiência ideal 1, mas overheads e a parte serial
reduzem ambos. Superlinearidade exige análise de cache/variabilidade, não deve
ser presumida.

**Escalabilidade forte e fraca?**  Na forte, o tamanho do problema permanece
fixo e aumentam os recursos. Na fraca, aumenta-se o problema para manter o
trabalho por recurso aproximadamente constante. Como o trabalho é `N³`, uma
série fraca pode usar aproximadamente `N(p)=N0*cuberoot(p)`.

**O que diz a Lei de Amdahl?**  Se f é paralelizável,
`S(p)=1/((1-f)+f/p)`. Mesmo com recursos infinitos, o limite é `1/(1-f)`. Ela
mostra que a fração serial e overheads limitam o ganho.

**Quais são os gargalos?**  Criação/sincronização de threads, cache e banda de
memória no OpenMP; latência, cópias e coletivas no MPI; alocação `O(N²)` e
trabalho `O(N³)` em ambos. Afinidade, frequência dinâmica, carga do sistema e
sobresubscrição também afetam os experimentos.

**Por que o tempo MPI usa o máximo dos ranks?**  A aplicação só termina quando
o processo mais lento conclui. O máximo representa o caminho crítico; uma média
entre ranks esconderia espera ou desbalanceamento.

**Por que checksum não basta?**  Colisões são possíveis. Ele é uma triagem
rápida; os testes pequenos também verificam elemento por elemento com tolerância
numérica justificada.

**Por que os tempos das etapas MPI podem não somar o total?**  Cada etapa é
reduzida pelo máximo, possivelmente vindo de ranks diferentes, e coletivas
incluem espera. A soma dos máximos não é uma decomposição exata do caminho total.

**Qual limitação experimental deve ser declarada?**  O baseline histórico foi
medido fora do contêiner; ainda faltam medições paralelas comparáveis no
ambiente oficial. Até essa coleta ocorrer, não há conclusão quantitativa válida.
