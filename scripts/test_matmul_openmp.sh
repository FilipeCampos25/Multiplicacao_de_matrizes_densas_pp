#!/usr/bin/env bash
set -euo pipefail

export LC_ALL=C
export OMP_DYNAMIC=FALSE
make base openmp

checksum() {
    sed -n 's/.*checksum=\([^[:space:]]*\).*/\1/p' <<< "$1" | head -n 1
}

compativel() {
    awk -v expected="$1" -v actual="$2" 'BEGIN {
        difference = actual - expected; if (difference < 0) difference = -difference;
        magnitude = expected; if (magnitude < 0) magnitude = -magnitude;
        exit !(difference <= 1.0e-9 * (1.0 + magnitude));
    }'
}

for dimension in 1 2 7 10 64 128; do
    baseline_output="$(./bin/base/01_matmul "$dimension")"
    baseline_checksum="$(checksum "$baseline_output")"
    for threads in 1 2 4 8; do
        openmp_output="$(OMP_NUM_THREADS="$threads" ./bin/openmp/matmul_openmp \
            "$dimension" --verify)"
        openmp_checksum="$(checksum "$openmp_output")"
        if [[ -z "$baseline_checksum" || -z "$openmp_checksum" ]] ||
           ! grep -q '^verificacao=OK' <<< "$openmp_output" ||
           ! grep -q "threads=${threads}$" <<< "$openmp_output" ||
           ! compativel "$baseline_checksum" "$openmp_checksum"; then
            printf 'FALHOU N=%s threads=%s\nbaseline: %s\nopenmp: %s\n' \
                "$dimension" "$threads" "$baseline_output" "$openmp_output" >&2
            exit 1
        fi
        printf 'OK N=%s threads=%s checksum=%s\n' \
            "$dimension" "$threads" "$openmp_checksum"
    done
done
