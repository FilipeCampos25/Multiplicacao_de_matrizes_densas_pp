#!/usr/bin/env bash
set -euo pipefail

bash ./scripts/test_matmul_openmp.sh
bash ./scripts/test_matmul_mpi.sh
printf 'OK: baseline, OpenMP e MPI produziram resultados consistentes.\n'
