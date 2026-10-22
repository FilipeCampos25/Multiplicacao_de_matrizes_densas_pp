#!/usr/bin/env bash
# Coleta reproduzivel de escalabilidade forte da multiplicacao de matrizes.
# Uso: ./scripts/benchmark_mpi.sh [REPETICOES] [ARQUIVO_CSV]
set -uo pipefail

export LC_ALL=C

REPETICOES="${1:-5}"
SAIDA="${2:-resultados/mpi_escalabilidade.csv}"
DIMENSOES="${DIMENSOES:-255 256 512 1024}"
PROCESSOS="${PROCESSOS:-1 2 4 8}"
MPI_RUN="${MPI_RUN:-mpirun}"
MPI_RUN_ARGS="${MPI_RUN_ARGS:-}"
MAX_TENTATIVAS="${MAX_TENTATIVAS:-$((REPETICOES + 3))}"
BASE_BIN="${BASE_BIN:-./bin/base/01_matmul}"
MPI_BIN="${MPI_BIN:-./bin/mpi/matmul_mpi}"

if ! [[ "$REPETICOES" =~ ^[1-9][0-9]*$ ]] ||
   ! [[ "$MAX_TENTATIVAS" =~ ^[1-9][0-9]*$ ]] ||
   (( MAX_TENTATIVAS < REPETICOES )); then
    echo "Erro: REPETICOES e MAX_TENTATIVAS devem ser inteiros positivos; MAX_TENTATIVAS >= REPETICOES." >&2
    exit 2
fi

for ferramenta in make "$MPI_RUN"; do
    if ! command -v "$ferramenta" >/dev/null 2>&1; then
        echo "Erro: ferramenta nao encontrada: $ferramenta" >&2
        exit 2
    fi
done

make base mpi || { echo "Erro: compilacao falhou; nenhum benchmark foi iniciado." >&2; exit 1; }

mkdir -p "$(dirname "$SAIDA")"
AMBIENTE="${SAIDA%.csv}_ambiente.txt"
{
    echo "data_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "host=$(hostname)"
    echo "sistema=$(uname -a)"
    echo "cpu_logicos=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo desconhecido)"
    command -v lscpu >/dev/null 2>&1 && lscpu
    command -v free >/dev/null 2>&1 && free -h
    g++ --version 2>/dev/null | head -n 1
    mpic++ --version 2>/dev/null | head -n 1
    "$MPI_RUN" --version 2>/dev/null | head -n 2
    echo "dimensoes=$DIMENSOES"
    echo "processos=$PROCESSOS"
    echo "repeticoes_validas=$REPETICOES"
    echo "mpi_run_args=$MPI_RUN_ARGS"
} > "$AMBIENTE"

echo 'implementacao,n,processos,repeticao,tentativa,tempo_s,checksum,scatterv_s,bcast_s,computacao_s,gatherv_s,linhas_min,linhas_max,status,erro' > "$SAIDA"

csv_texto() {
    local valor=${1//$'\r'/ }
    valor=${valor//$'\n'/ ; }
    valor=${valor//\"/\"\"}
    printf '"%s"' "${valor:0:1000}"
}

registrar_erro() {
    local implementacao=$1 n=$2 processos=$3 tentativa=$4 status=$5 mensagem=$6
    printf '%s,%s,%s,,%s,,,,,,,,,%s,' "$implementacao" "$n" "$processos" "$tentativa" "$status" >> "$SAIDA"
    csv_texto "$mensagem" >> "$SAIDA"
    printf '\n' >> "$SAIDA"
}

campo_saida() {
    local linha=$1 campo=$2
    sed -n "s/.*${campo}=\\([^[:space:]]*\\).*/\\1/p" <<< "$linha" | head -n 1
}

checksum_compativel() {
    awk -v esperado="$1" -v obtido="$2" 'BEGIN {
        d = obtido - esperado; if (d < 0) d = -d;
        m = esperado; if (m < 0) m = -m;
        exit !(d <= 1.0e-9 * (1.0 + m));
    }'
}

executar_baseline() {
    local n=$1 esperado tentativa=0 validas=0 saida tempo checksum

    if ! saida=$("$BASE_BIN" "$n" 2>&1); then
        registrar_erro sequencial "$n" 1 0 erro_aquecimento "$saida"
        return 1
    fi
    esperado=$(campo_saida "$saida" checksum)
    if [[ -z "$esperado" ]]; then
        registrar_erro sequencial "$n" 1 0 erro_aquecimento "checksum ausente: $saida"
        return 1
    fi

    while (( validas < REPETICOES && tentativa < MAX_TENTATIVAS )); do
        ((tentativa++))
        if ! saida=$("$BASE_BIN" "$n" 2>&1); then
            registrar_erro sequencial "$n" 1 "$tentativa" erro_execucao "$saida"
            continue
        fi
        tempo=$(campo_saida "$saida" tempo)
        checksum=$(campo_saida "$saida" checksum)
        if [[ -z "$tempo" || -z "$checksum" ]] || ! checksum_compativel "$esperado" "$checksum"; then
            registrar_erro sequencial "$n" 1 "$tentativa" erro_saida "$saida"
            continue
        fi
        ((validas++))
        printf 'sequencial,%s,1,%s,%s,%s,%s,,,,,,,ok,\n' \
            "$n" "$validas" "$tentativa" "$tempo" "$checksum" >> "$SAIDA"
    done
    (( validas == REPETICOES ))
}

executar_mpi() {
    local n=$1 processos=$2 esperado=$3 tentativa=0 validas=0
    local saida principal etapas carga tempo checksum scatter bcast computacao gather minimo maximo
    local -a argumentos_mpi=()
    read -r -a argumentos_mpi <<< "$MPI_RUN_ARGS"

    if ! saida=$("$MPI_RUN" "${argumentos_mpi[@]}" -np "$processos" "$MPI_BIN" "$n" 2>&1); then
        registrar_erro mpi "$n" "$processos" 0 erro_aquecimento "$saida"
        return 1
    fi

    while (( validas < REPETICOES && tentativa < MAX_TENTATIVAS )); do
        ((tentativa++))
        if ! saida=$("$MPI_RUN" "${argumentos_mpi[@]}" -np "$processos" "$MPI_BIN" "$n" 2>&1); then
            registrar_erro mpi "$n" "$processos" "$tentativa" erro_execucao "$saida"
            continue
        fi
        principal=$(grep '^N=' <<< "$saida" | head -n 1)
        etapas=$(grep '^tempos_max_s ' <<< "$saida" | head -n 1)
        carga=$(grep '^carga_linhas ' <<< "$saida" | head -n 1)
        tempo=$(campo_saida "$principal" tempo)
        checksum=$(campo_saida "$principal" checksum)
        scatter=$(campo_saida "$etapas" scatterv)
        bcast=$(campo_saida "$etapas" bcast)
        computacao=$(campo_saida "$etapas" computacao)
        gather=$(campo_saida "$etapas" gatherv)
        minimo=$(campo_saida "$carga" min)
        maximo=$(campo_saida "$carga" max)
        if [[ -z "$tempo" || -z "$checksum" || -z "$scatter" || -z "$bcast" ||
              -z "$computacao" || -z "$gather" || -z "$minimo" || -z "$maximo" ]] ||
           ! checksum_compativel "$esperado" "$checksum"; then
            registrar_erro mpi "$n" "$processos" "$tentativa" erro_saida "$saida"
            continue
        fi
        ((validas++))
        printf 'mpi,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,ok,\n' \
            "$n" "$processos" "$validas" "$tentativa" "$tempo" "$checksum" \
            "$scatter" "$bcast" "$computacao" "$gather" "$minimo" "$maximo" >> "$SAIDA"
    done
    (( validas == REPETICOES ))
}

# A verificacao elemento a elemento inclui casos divisiveis, nao divisiveis e P > N.
MPI_RUN="$MPI_RUN" MPI_RUN_ARGS="$MPI_RUN_ARGS" bash ./scripts/test_matmul_mpi.sh || {
    echo "Erro: validacao de corretude falhou; nenhum benchmark foi iniciado." >&2
    exit 1
}

falhas=0
for n in $DIMENSOES; do
    echo "Coletando baseline N=$n"
    if ! executar_baseline "$n"; then
        echo "Falha: nao foram obtidas $REPETICOES medicoes sequenciais validas para N=$n." >&2
        ((falhas++))
        continue
    fi
    esperado=$(awk -F, -v n="$n" '$1 == "sequencial" && $2 == n && $14 == "ok" { print $7; exit }' "$SAIDA")
    for processos in $PROCESSOS; do
        echo "Coletando MPI N=$n p=$processos"
        if ! executar_mpi "$n" "$processos" "$esperado"; then
            echo "Falha: nao foram obtidas $REPETICOES medicoes MPI validas para N=$n p=$processos." >&2
            ((falhas++))
        fi
    done
done

echo "Dados: $SAIDA"
echo "Ambiente: $AMBIENTE"
(( falhas == 0 ))
