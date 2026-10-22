#!/usr/bin/env bash
# Coleta comparavel de baseline, OpenMP e MPI.
# Uso: ./scripts/medir_speedup.sh [N] [REPETICOES] [THREADS] [PROCESSOS] [CSV]
# THREADS e PROCESSOS aceitam listas entre aspas, por exemplo "1 2 4 8".
set -euo pipefail

export LC_ALL=C
export OMP_DYNAMIC=FALSE
N="${1:-512}"
REPETICOES="${2:-5}"
THREADS="${3:-1 2 4}"
PROCESSOS="${4:-1 2 4}"
ARQUIVO_CSV="${5:-resultados/benchmarks_automatizados.csv}"
MPI_RUN="${MPI_RUN:-mpirun}"
MPI_RUN_ARGS="${MPI_RUN_ARGS:-}"

if ! [[ "$N" =~ ^[1-9][0-9]*$ && "$REPETICOES" =~ ^[1-9][0-9]*$ ]]; then
    echo "Erro: N e REPETICOES devem ser inteiros positivos." >&2
    exit 2
fi
for ferramenta in make "$MPI_RUN"; do
    command -v "$ferramenta" >/dev/null 2>&1 || {
        echo "Erro: ferramenta nao encontrada: $ferramenta" >&2
        exit 2
    }
done

make all
MPI_RUN="$MPI_RUN" MPI_RUN_ARGS="$MPI_RUN_ARGS" bash ./scripts/test_matmul.sh
mkdir -p "$(dirname "$ARQUIVO_CSV")"
AMBIENTE="${ARQUIVO_CSV%.csv}_ambiente.txt"
{
    echo "data_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "host=$(hostname)"
    echo "sistema=$(uname -a)"
    echo "cpu_logicos=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo desconhecido)"
    g++ --version | head -n 1
    mpic++ --version | head -n 1
    "$MPI_RUN" --version 2>/dev/null | head -n 2
    echo "N=$N"
    echo "repeticoes=$REPETICOES"
    echo "threads=$THREADS"
    echo "processos=$PROCESSOS"
    echo "mpi_run_args=$MPI_RUN_ARGS"
} > "$AMBIENTE"

echo 'versao,N,paralelismo,repeticao,tempo_s,checksum,scatterv_s,bcast_s,computacao_s,gatherv_s,linhas_min,linhas_max' > "$ARQUIVO_CSV"

campo() {
    local texto=$1 nome=$2
    sed -n "s/.*${nome}=\([^[:space:]]*\).*/\1/p" <<< "$texto" | head -n 1
}

registrar_saida() {
    local versao=$1 paralelismo=$2 repeticao=$3 saida=$4
    local principal tempo checksum etapas carga threads_reais
    principal="$(grep '^N=' <<< "$saida" | head -n 1)"
    tempo="$(campo "$principal" tempo)"
    checksum="$(campo "$principal" checksum)"
    [[ -n "$tempo" && -n "$checksum" ]] || {
        echo "Erro: saida invalida de $versao: $saida" >&2
        return 1
    }
    if [[ "$versao" == openmp ]]; then
        threads_reais="$(campo "$principal" threads)"
        [[ "$threads_reais" == "$paralelismo" ]] || {
            echo "Erro: solicitadas $paralelismo threads, usadas $threads_reais." >&2
            return 1
        }
    fi
    if [[ "$versao" == mpi ]]; then
        etapas="$(grep '^tempos_max_s ' <<< "$saida" | head -n 1)"
        carga="$(grep '^carga_linhas ' <<< "$saida" | head -n 1)"
        printf 'mpi,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
            "$N" "$paralelismo" "$repeticao" "$tempo" "$checksum" \
            "$(campo "$etapas" scatterv)" "$(campo "$etapas" bcast)" \
            "$(campo "$etapas" computacao)" "$(campo "$etapas" gatherv)" \
            "$(campo "$carga" min)" "$(campo "$carga" max)" >> "$ARQUIVO_CSV"
    else
        printf '%s,%s,%s,%s,%s,%s,,,,,,\n' "$versao" "$N" "$paralelismo" \
            "$repeticao" "$tempo" "$checksum" >> "$ARQUIVO_CSV"
    fi
}

./bin/base/01_matmul "$N" >/dev/null
for repeticao in $(seq 1 "$REPETICOES"); do
    registrar_saida sequencial 1 "$repeticao" "$(./bin/base/01_matmul "$N")"
done

for threads in $THREADS; do
    OMP_NUM_THREADS="$threads" ./bin/openmp/matmul_openmp "$N" >/dev/null
    for repeticao in $(seq 1 "$REPETICOES"); do
        registrar_saida openmp "$threads" "$repeticao" \
            "$(OMP_NUM_THREADS="$threads" ./bin/openmp/matmul_openmp "$N")"
    done
done

read -r -a argumentos_mpi <<< "$MPI_RUN_ARGS"
for processos in $PROCESSOS; do
    "$MPI_RUN" "${argumentos_mpi[@]}" -np "$processos" ./bin/mpi/matmul_mpi "$N" >/dev/null
    for repeticao in $(seq 1 "$REPETICOES"); do
        registrar_saida mpi "$processos" "$repeticao" \
            "$("$MPI_RUN" "${argumentos_mpi[@]}" -np "$processos" ./bin/mpi/matmul_mpi "$N")"
    done
done

echo "Dados: $ARQUIVO_CSV"
echo "Ambiente: $AMBIENTE"
echo "Analise: python3 scripts/analisar_resultados.py $ARQUIVO_CSV"
