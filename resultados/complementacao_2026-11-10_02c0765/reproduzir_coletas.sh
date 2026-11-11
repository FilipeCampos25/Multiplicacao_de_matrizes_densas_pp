#!/usr/bin/env bash
set -euo pipefail
OUT=resultados/complementacao_2026-11-10_02c0765
exec > >(tee "$OUT/coletas.txt") 2>&1
set -x
date -u
uname -a
lscpu
cat /sys/fs/cgroup/cpu.max
cat /sys/fs/cgroup/memory.max
g++ --version
mpirun --version
python3 --version
python3 scripts/coletar_perfil_sequencial.py --sizes 256 512 1024 --repetitions 5 --output "$OUT/perfil_sequencial.csv"
DIMENSOES="256 512 1024" PROCESSOS="1 2 4 8" MPI_RUN_ARGS="--use-hwthread-cpus --bind-to hwthread" bash scripts/benchmark_mpi.sh 5 "$OUT/mpi_etapas.csv"
python3 scripts/analisar_complementacao.py --sequencial "$OUT/perfil_sequencial.csv" --mpi "$OUT/mpi_etapas.csv" --expected-sizes 256 512 1024 --expected-processes 1 2 4 8 --minimum-repetitions 5 --output-dir "$OUT/resumos"
date -u
