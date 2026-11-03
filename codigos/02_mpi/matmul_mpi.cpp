/* Multiplicacao distribuida de matrizes densas C = A x B com MPI.
 *
 * Adaptacao do baseline 01_matmul.cpp, do Prof. Rodrigo Goncalves Pinto.
 * A inicializacao deterministica, o armazenamento row-major e a ordem de
 * lacos i-k-j foram preservados; o MPI distribui apenas blocos de linhas.
 *
 * Estrategia: decomposicao equilibrada por blocos de linhas de A e C.
 * A matriz B e compartilhada integralmente por todos os processos.
 *
 * Compilar: make mpi
 * Executar: mpirun -np 4 ./bin/mpi/matmul_mpi 512
 * Verificar: mpirun -np 4 ./bin/mpi/matmul_mpi 7 --verify
 */

#include <mpi.h>

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <stdexcept>
#include <vector>

namespace {

bool parse_dimension(const char* text, int& dimension) {
    // Aceita somente um inteiro decimal positivo que caiba em int. end deve
    // apontar para o terminador, o que rejeita entradas parciais como "100x".
    if (text == nullptr || *text == '\0') {
        return false;
    }

    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0' || parsed <= 0 ||
        parsed > INT_MAX) {
        return false;
    }

    dimension = static_cast<int>(parsed);
    return true;
}

void build_distribution(int dimension, int process_count,
                        std::vector<int>& counts,
                        std::vector<int>& displacements) {
    // Divide linhas inteiras em blocos quase iguais. base_rows e o quociente da
    // divisao N/P; extra_rows e o resto. Os primeiros "extra_rows" ranks recebem
    // uma linha adicional, de modo que as cargas diferem por no maximo uma linha.
    const int base_rows = dimension / process_count;
    const int extra_rows = dimension % process_count;
    int offset = 0;

    // counts e displacements sao expressos em ELEMENTOS double, nao em linhas,
    // porque Scatterv/Gatherv trabalham com unidades do datatype informado.
    // counts[p] = linhas_do_rank_p * N; displacements[p] indica o primeiro elemento
    // do bloco no vetor global row-major.
    //
    // Exemplo N=7, P=3: base_rows=2 e extra_rows=1.
    //   rank:             0    1    2
    //   linhas:           3    2    2
    //   counts (doubles):21   14   14
    //   displacements:    0   21   35
    // Assim, os 49 elementos sao contiguos e nenhum e perdido ou repetido.
    // Se P>N, base_rows=0: os N primeiros ranks recebem uma linha e os demais
    // recebem zero; contagens repetidas/zero continuam validas nas coletivas.
    for (int process = 0; process < process_count; ++process) {
        const int rows = base_rows + (process < extra_rows ? 1 : 0);
        counts[process] = rows * dimension;
        displacements[process] = offset;
        offset += counts[process];
    }
}

bool verify_result(const std::vector<double>& result, int dimension,
                   double& maximum_error) {
    // Reconstroi sequencialmente o valor esperado de cada C[i][j] a partir das
    // formulas deterministicamente usadas para A e B. Isso oferece uma referencia
    // independente da distribuicao MPI e identifica o maior erro encontrado.
    maximum_error = 0.0;

    for (int i = 0; i < dimension; ++i) {
        for (int j = 0; j < dimension; ++j) {
            double expected = 0.0;
            for (int k = 0; k < dimension; ++k) {
                const std::size_t a_index =
                    static_cast<std::size_t>(i) * dimension + k;
                const std::size_t b_index =
                    static_cast<std::size_t>(k) * dimension + j;
                const double a = static_cast<double>(a_index % 100) * 0.01;
                const double b =
                    static_cast<double>((b_index * 7) % 100) * 0.01;
                expected += a * b;
            }

            const std::size_t index =
                static_cast<std::size_t>(i) * dimension + j;
            // A tolerancia tem componentes absoluta e relativa. Ela admite o
            // arredondamento normal de double sem aceitar uma diferenca numerica
            // relevante; igualdade exata seria inadequada para ponto flutuante.
            const double error = std::abs(result[index] - expected);
            maximum_error = std::max(maximum_error, error);
            const double tolerance = 1.0e-12 * (1.0 + std::abs(expected));
            if (error > tolerance) {
                return false;
            }
        }
    }

    return true;
}

}  // namespace

int main(int argc, char** argv) {
    // Fase 1 - inicializacao MPI. MPI (Message Passing Interface) executa processos
    // separados, em geral com espacos de memoria independentes e ate em maquinas
    // diferentes. Isso contrasta com OpenMP, cujas threads compartilham a memoria
    // de um processo. MPI_Init deve preceder as demais chamadas MPI e pode consumir
    // argumentos proprios do runtime.
    MPI_Init(&argc, &argv);

    // MPI_COMM_WORLD e o comunicador que inicialmente reune todos os processos.
    // Comm_rank fornece o identificador 0..P-1 deste processo; Comm_size fornece P.
    // O rank 0 atua como raiz: valida entrada, cria matrizes globais, reune e exibe C.
    int rank = 0;
    int process_count = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &process_count);

    int dimension = 512;
    int input_is_valid = 1;
    int verify = 0;

    // Somente a raiz interpreta argv e imprime erros, evitando mensagens repetidas.
    if (rank == 0) {
        if (argc > 3 || (argc >= 2 && !parse_dimension(argv[1], dimension)) ||
            (argc == 3 && std::strcmp(argv[2], "--verify") != 0)) {
            input_is_valid = 0;
            std::fprintf(stderr, "Uso: %s [N positivo] [--verify]\n", argv[0]);
        } else {
            verify = (argc == 3) ? 1 : 0;

            // MPI_Scatterv, MPI_Bcast e MPI_Gatherv desta API usam contagens int.
            // N*N deve caber nesse tipo para que nenhuma conversao silenciosa torne
            // a contagem negativa ou trunque a comunicacao.
            const long long element_count =
                static_cast<long long>(dimension) * dimension;
            if (element_count > INT_MAX) {
                input_is_valid = 0;
                std::fprintf(stderr,
                             "Erro: N*N deve ser menor ou igual a INT_MAX "
                             "para as coletivas MPI usadas.\n");
            }
        }
    }

    // Bcast e uma coletiva: todos os ranks do comunicador devem chama-la na mesma
    // ordem e com parametros compativeis. A raiz (argumento 0) envia um MPI_INT;
    // os demais o recebem no mesmo buffer. Assim todos abandonam juntos uma entrada
    // invalida, em vez de alguns prosseguirem e bloquearem em coletivas posteriores.
    MPI_Bcast(&input_is_valid, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (!input_is_valid) {
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    // Propaga os parametros validados: endereco do buffer, quantidade 1, datatype
    // MPI_INT, rank raiz 0 e comunicador MPI_COMM_WORLD, nesta ordem conceitual.
    MPI_Bcast(&dimension, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&verify, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Fase 2 - cada rank calcula previamente sua carga. A mesma regra usada por
    // build_distribution garante que local_element_count concorda com counts[rank].
    // A linha local 0 nao e necessariamente a linha global 0: seu indice global e
    // displacements[rank]/N; contudo, o calculo usa apenas o bloco local de A e B
    // completa, e Gatherv recoloca o bloco na posicao global correta.
    const std::size_t element_count =
        static_cast<std::size_t>(dimension) * dimension;
    const int base_rows = dimension / process_count;
    const int extra_rows = dimension % process_count;
    const int local_rows_expected =
        base_rows + (rank < extra_rows ? 1 : 0);
    const int local_element_count = local_rows_expected * dimension;
    std::vector<int> counts;
    std::vector<int> displacements;
    std::vector<double> matrix_a;
    std::vector<double> matrix_b;
    std::vector<double> matrix_c;
    std::vector<double> local_a;
    std::vector<double> local_c;
    int allocation_succeeded = 1;

    // Todos os ranks armazenam B inteira, local_a e local_c. Apenas o rank 0
    // armazena A e C globais. counts/displacements sao criados em todos os ranks
    // porque seus ponteiros sao fornecidos as coletivas (ainda que algumas
    // implementacoes so consultem certos argumentos na raiz).
    try {
        // Estas estruturas tambem precisam participar do tratamento coletivo:
        // uma excecao isolada antes do Allreduce deixaria outros ranks bloqueados.
        counts.resize(static_cast<std::size_t>(process_count));
        displacements.resize(static_cast<std::size_t>(process_count));
        build_distribution(dimension, process_count, counts, displacements);
        matrix_b.resize(element_count);
        local_a.resize(static_cast<std::size_t>(local_element_count));
        local_c.assign(static_cast<std::size_t>(local_element_count), 0.0);
        if (rank == 0) {
            matrix_a.resize(element_count);
            matrix_c.assign(element_count, 0.0);
        }
    } catch (const std::bad_alloc&) {
        allocation_succeeded = 0;
    } catch (const std::length_error&) {
        allocation_succeeded = 0;
    }

    // Allreduce combina e devolve o resultado a TODOS os ranks. Cada processo envia
    // um int; MPI_MIN funciona como um "E" para flags 0/1: o resultado so e 1 se
    // todas as alocacoes tiveram sucesso. Uma verificacao apenas local seria
    // perigosa: o rank sem memoria sairia e os outros ficariam presos numa coletiva.
    int all_allocations_succeeded = 0;
    MPI_Allreduce(&allocation_succeeded, &all_allocations_succeeded, 1, MPI_INT,
                  MPI_MIN, MPI_COMM_WORLD);
    if (!all_allocations_succeeded) {
        if (rank == 0) {
            std::fprintf(stderr,
                         "Erro: memoria insuficiente para N=%d com %d processos.\n",
                         dimension, process_count);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    // A raiz inicializa A e B com os mesmos valores deterministicos do baseline.
    // B sera replicada depois; A sera dividida, portanto nenhum outro rank precisa
    // construir entradas globais independentemente.
    if (rank == 0) {
        for (std::size_t index = 0; index < element_count; ++index) {
            matrix_a[index] = static_cast<double>(index % 100) * 0.01;
            matrix_b[index] = static_cast<double>((index * 7) % 100) * 0.01;
        }
    }

    // Fase 3 - Barrier bloqueia cada rank ate todos alcancarem este ponto. Ela nao
    // transfere as matrizes; apenas alinha o inicio da medicao para que preparacao e
    // diferencas de partida nao contaminem o tempo total. MPI_Wtime devolve tempo de
    // parede local em segundos; diferencas do mesmo relogio medem cada etapa.
    MPI_Barrier(MPI_COMM_WORLD);
    const double start = MPI_Wtime();

    // Alguns processos podem receber zero linhas quando P>N. Embora contagem zero
    // signifique que o conteudo nao sera acessado, fornecer endereco valido evita
    // depender de como uma implementacao trata nullptr. unused_buffer nunca passa a
    // representar dados reais: e apenas um argumento seguro para a chamada.
    double unused_buffer = 0.0;
    double* local_a_buffer =
        local_element_count == 0 ? &unused_buffer : local_a.data();
    double* local_c_buffer =
        local_element_count == 0 ? &unused_buffer : local_c.data();

    // Fase 4 - Scatterv distribui blocos de A de tamanhos possivelmente diferentes.
    // Na raiz: sendbuf=matrix_a, sendcounts=counts, displs=displacements e tipo
    // MPI_DOUBLE descrevem a origem. Em cada rank: local_a_buffer, sua contagem
    // local e MPI_DOUBLE descrevem o destino. O 0 e a raiz e COMM_WORLD e o grupo.
    // Os argumentos de envio dos ranks nao raiz nao sao usados; todos, entretanto,
    // devem participar coordenadamente da mesma coletiva.
    double stage_start = MPI_Wtime();
    MPI_Scatterv(rank == 0 ? matrix_a.data() : nullptr, counts.data(),
                 displacements.data(), MPI_DOUBLE, local_a_buffer,
                 local_element_count, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    const double scatter_elapsed = MPI_Wtime() - stage_start;

    // Bcast replica B completa a partir do rank 0. Todo processo precisa de cada
    // linha B[k][:], pois mesmo calculando poucas linhas de C a formula percorre
    // todos os k. O cast e seguro pela verificacao anterior de N*N <= INT_MAX.
    stage_start = MPI_Wtime();
    MPI_Bcast(matrix_b.data(), static_cast<int>(element_count), MPI_DOUBLE, 0,
              MPI_COMM_WORLD);
    const double broadcast_elapsed = MPI_Wtime() - stage_start;

    // Fase 5 - multiplicacao local. Em row-major, [linha][coluna] ocupa
    // linha*N+coluna. local_a[i*N+k] e o A da linha global pertencente a este rank;
    // matrix_b[k*N+j] e B[k][j]; local_c[i*N+j] e o C local correspondente.
    //
    // Para cada celula, C[i][j] = soma(k=0..N-1) A[i][k]*B[k][j]. C local foi
    // zerada para servir de acumulador. O laco k seleciona cada parcela e o laco j
    // atualiza a linha inteira. Globalmente ha N*N*N produtos/somas: O(N^3).
    // A ordem i-k-j percorre B e C contiguamente no laco j e reutiliza "a", o que
    // melhora a localidade de cache. Nao ha corrida: processos escrevem memorias
    // locais independentes e blocos de linhas globais distintos.
    const int local_rows = local_element_count / dimension;
    // Fase 6 - Gatherv faz a operacao inversa: cada rank envia local_element_count
    // doubles de local_c; na raiz, counts e displacements determinam onde cada bloco
    // entra em matrix_c. Desse modo C global volta a ordem original das linhas.
    // recvbuf dos ranks nao raiz nao e usado, por isso nullptr e apropriado ali.
    stage_start = MPI_Wtime();
    for (int i = 0; i < local_rows; ++i) {
        for (int k = 0; k < dimension; ++k) {
            const double a = local_a[static_cast<std::size_t>(i) * dimension + k];
            for (int j = 0; j < dimension; ++j) {
                local_c[static_cast<std::size_t>(i) * dimension + j] +=
                    a * matrix_b[static_cast<std::size_t>(k) * dimension + j];
            }
        }
    }
    const double computation_elapsed = MPI_Wtime() - stage_start;

    stage_start = MPI_Wtime();
    MPI_Gatherv(local_c_buffer, local_element_count, MPI_DOUBLE,
                rank == 0 ? matrix_c.data() : nullptr, counts.data(),
                displacements.data(), MPI_DOUBLE, 0, MPI_COMM_WORLD);
    const double gather_elapsed = MPI_Wtime() - stage_start;

    // Fase 7 - cada duracao de etapa e local: pode incluir espera interna da
    // coletiva e variar por rank. Reduce combina valores na raiz. MPI_MAX do tempo
    // total representa o caminho critico, pois a execucao distribuida so termina
    // quando o processo mais lento termina; uma media esconderia esse gargalo.
    // Parametros: endereco de envio, endereco de recepcao, quantidade, datatype,
    // operacao, raiz e comunicador. Apenas a raiz usa o buffer de recepcao final.
    const double local_elapsed = MPI_Wtime() - start;
    double elapsed = 0.0;
    MPI_Reduce(&local_elapsed, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0,
               MPI_COMM_WORLD);

    // Reduzir o vetor de quatro tempos com MPI_MAX produz o pior tempo observado em
    // cada etapa. Esses maximos podem vir de ranks diferentes e, portanto, nao devem
    // ser somados para reconstruir o total. Tempos de coletivas incluem comunicacao
    // e podem incorporar espera causada por desbalanceamento anterior.
    const double local_stage_times[] = {scatter_elapsed, broadcast_elapsed,
                                        computation_elapsed, gather_elapsed};
    double maximum_stage_times[4] = {};
    MPI_Reduce(local_stage_times, maximum_stage_times, 4, MPI_DOUBLE, MPI_MAX,
               0, MPI_COMM_WORLD);

    // Duas reducoes calculam os extremos da carga em linhas. max-min quantifica o
    // desbalanceamento estrutural: pela distribuicao adotada e no maximo 1 quando
    // P<=N; com P>N, alguns ranks tem 0 e outros 1. Mais linhas significam mais
    // trabalho O(linhas_locais*N^2), e ranks mais carregados podem limitar o tempo.
    int minimum_rows = 0;
    int maximum_rows = 0;
    MPI_Reduce(&local_rows, &minimum_rows, 1, MPI_INT, MPI_MIN, 0,
               MPI_COMM_WORLD);
    MPI_Reduce(&local_rows, &maximum_rows, 1, MPI_INT, MPI_MAX, 0,
               MPI_COMM_WORLD);

    // Fase 8 - somente a raiz possui C global e gera a saida. O checksum sequencial
    // preserva a ordem do baseline; reagrupar somas em paralelo poderia alterar os
    // ultimos bits, pois soma em ponto flutuante nao e exatamente associativa.
    int exit_code = EXIT_SUCCESS;
    if (rank == 0) {
        double checksum = 0.0;
        for (double value : matrix_c) {
            checksum += value;
        }
        std::printf("N=%d  checksum=%.6f  tempo=%.6f s\n", dimension, checksum,
                    elapsed);
        std::printf(
            "tempos_max_s scatterv=%.9f bcast=%.9f computacao=%.9f "
            "gatherv=%.9f total=%.9f\n",
            maximum_stage_times[0], maximum_stage_times[1],
            maximum_stage_times[2], maximum_stage_times[3], elapsed);
        std::printf("carga_linhas min=%d max=%d diferenca=%d\n", minimum_rows,
                    maximum_rows, maximum_rows - minimum_rows);

        // MATMUL_DUMP=1 faz apenas o rank 0 exportar C global, ja ordenada por
        // Gatherv, em row-major e fora dos tempos medidos. Isso apoia testes sem
        // misturar linhas impressas por processos diferentes.
        const char* dump = std::getenv("MATMUL_DUMP");
        if (dump != nullptr && std::strcmp(dump, "1") == 0) {
            std::printf("C=");
            for (double value : matrix_c) std::printf(" %.17g", value);
            std::printf("\n");
        }

        // --verify recalcula C sequencialmente na raiz. O maior erro e comparado a
        // tolerancia numerica em verify_result; uma falha muda o codigo de saida.
        if (verify) {
            double maximum_error = 0.0;
            const bool result_is_correct =
                verify_result(matrix_c, dimension, maximum_error);
            std::printf("verificacao=%s  max_erro=%.3e\n",
                        result_is_correct ? "OK" : "FALHOU", maximum_error);
            if (!result_is_correct) {
                exit_code = EXIT_FAILURE;
            }
        }
    }

    // Fase 9 - a raiz transmite o status final para que todos os processos retornem
    // o mesmo codigo. Esta e a ultima coletiva: omiti-la faria ranks nao raiz
    // reportarem sucesso mesmo se a verificacao da raiz falhasse. MPI_Finalize
    // encerra o ambiente MPI e deve ser chamado por todos antes de sair normalmente.
    MPI_Bcast(&exit_code, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Finalize();
    return exit_code;
}
