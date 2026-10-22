#!/usr/bin/env bash
set -euo pipefail

# Testes basicos da Issue #5. MPI_RUN e MPI_RUN_ARGS permitem adaptar o
# launcher sem alterar o script (por exemplo: MPI_RUN=mpiexec).
MPI_RUN="${MPI_RUN:-mpirun}"
MPI_RUN_ARGS="${MPI_RUN_ARGS:-}"

make base mpi

cases=(
    "1 1"
    "2 4"
    "7 2"
    "7 4"
    "10 3"
    "64 1"
    "64 2"
    "64 4"
    "128 4"
)

for test_case in "${cases[@]}"; do
    read -r dimension processes <<< "${test_case}"

    baseline_output="$(./bin/base/01_matmul "${dimension}")"
    # A expansao sem aspas de MPI_RUN_ARGS e intencional: ela admite varios
    # argumentos opcionais do launcher, como "--oversubscribe".
    # shellcheck disable=SC2086
    mpi_output="$(${MPI_RUN} ${MPI_RUN_ARGS} -np "${processes}" \
        ./bin/mpi/matmul_mpi "${dimension}" --verify)"

    baseline_checksum="$(sed -n 's/.*checksum=\([^[:space:]]*\).*/\1/p' \
        <<< "${baseline_output}")"
    mpi_checksum="$(sed -n 's/.*checksum=\([^[:space:]]*\).*/\1/p' \
        <<< "${mpi_output}" | head -n 1)"

    if [[ -z "${baseline_checksum}" || -z "${mpi_checksum}" ]]; then
        printf 'FALHOU N=%s p=%s: checksum ausente\n' \
            "${dimension}" "${processes}" >&2
        exit 1
    fi

    if ! grep -q '^verificacao=OK' <<< "${mpi_output}"; then
        printf 'FALHOU N=%s p=%s: verificacao elemento a elemento\n%s\n' \
            "${dimension}" "${processes}" "${mpi_output}" >&2
        exit 1
    fi

    if ! awk -v expected="${baseline_checksum}" -v actual="${mpi_checksum}" '
        BEGIN {
            difference = actual - expected;
            if (difference < 0) difference = -difference;
            magnitude = expected;
            if (magnitude < 0) magnitude = -magnitude;
            tolerance = 1.0e-9 * (1.0 + magnitude);
            exit !(difference <= tolerance);
        }
    '; then
        printf 'FALHOU N=%s p=%s: baseline=%s mpi=%s\n' \
            "${dimension}" "${processes}" \
            "${baseline_checksum}" "${mpi_checksum}" >&2
        exit 1
    fi

    printf 'OK N=%s p=%s checksum=%s\n' \
        "${dimension}" "${processes}" "${mpi_checksum}"
done
