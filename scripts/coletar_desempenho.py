#!/usr/bin/env python3
"""Coleta conjunta forte/fraca; nao altera algoritmos e nunca sobrescreve coletas."""
import argparse
import csv
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import random
import subprocess
import sys

from benchmark_openmp import resources
from analisar_desempenho import compatible, finite

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tests'))
from test_correctness import execute, parse_output


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sizes', nargs='+', type=int, default=[256, 512, 1024])
    parser.add_argument('--workers', nargs='+', type=int, default=[1, 2, 4, 8])
    parser.add_argument('--repetitions', type=int, default=5)
    parser.add_argument('--weak-base', type=int, default=256)
    parser.add_argument('--seed', type=int, default=8510)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args(argv)
    if min(args.sizes+args.workers+[args.repetitions, args.weak_base]) <= 0:
        parser.error('Parametros devem ser positivos')
    if any(len(v) != len(set(v)) for v in (args.sizes, args.workers)) or 1 not in args.workers:
        parser.error('Listas sem duplicatas; workers deve incluir 1')
    output = args.output.resolve()
    meta_path = output.with_name(output.stem+'_ambiente.txt')
    if output.exists() or meta_path.exists():
        parser.error('Coleta existente; escolha outro --output')
    limits = resources()
    workers = [p for p in args.workers if p <= limits['cpu_capacity']]
    skipped = [p for p in args.workers if p not in workers]
    if not workers:
        parser.error('Sem capacidade para um trabalhador')
    cases = []
    for n in args.sizes:
        cases.append(('forte', 0, 'sequencial', n, 1))
        cases.extend(('forte', 0, impl, n, p) for impl in ('openmp', 'mpi') for p in workers)
    cases.extend(('fraca', args.weak_base, impl, round(args.weak_base*p**(1/3)), p)
                 for impl in ('openmp', 'mpi') for p in workers)
    dimensions = sorted({c[3] for c in cases})
    for n in dimensions:
        if 7*(n*n-1) > 2147483647:
            parser.error('Dimensao excede indices/int do baseline')
        # Limite conservador: B replicada, buffers locais e matrizes globais.
        if 8*n*n*(max(workers)+6) > limits['memory_available_bytes']*.5:
            parser.error('Memoria estimada excede 50% da memoria disponivel')
    bins = {'sequencial': ROOT/'bin/base/01_matmul',
            'openmp': ROOT/'bin/benchmark_openmp/matmul_static',
            'mpi': ROOT/'bin/mpi/matmul_mpi'}
    if any(not p.is_file() for p in bins.values()):
        parser.error('Execute make benchmark-openmp-build mpi antes da coleta')
    env = {k:v for k,v in os.environ.items() if not k.startswith(('OMP_', 'GOMP_', 'KMP_')) and k != 'MATMUL_DUMP'}
    controlled = {'LC_ALL':'C', 'OMP_DYNAMIC':'FALSE', 'OMP_PROC_BIND':'close',
                  'OMP_PLACES':'threads', 'OMP_WAIT_POLICY':'PASSIVE',
                  'OMP_THREAD_LIMIT':str(max(workers))}
    env.update(controlled)
    launcher = ['mpirun', '--use-hwthread-cpus', '--bind-to', 'hwthread']
    def command(impl, n, p):
        return (launcher+['-np', str(p)] if impl == 'mpi' else [])+[str(bins[impl]), str(n)]
    def measure(impl, n, p, reference=None):
        stdout, _ = execute(command(impl, n, p), dict(env, OMP_NUM_THREADS=str(p)), 120)
        checksum, fields, _ = parse_output(stdout, n, False)
        elapsed = finite(fields['tempo'], True)
        checksum = finite(checksum)
        if reference is not None and not compatible(reference, checksum):
            raise ValueError(f'Checksum divergente: {impl}, N={n}, p={p}')
        if impl == 'openmp' and int(fields.get('threads', 0)) != p:
            raise ValueError('Equipe efetiva incorreta')
        return elapsed, checksum
    # Reutiliza a suite anterior antes de aceitar qualquer medicao.
    check = subprocess.run(['make', 'test-correctness'], cwd=ROOT, env=env,
                           capture_output=True, text=True, timeout=300)
    if check.returncode or 'FAIL=0' not in check.stdout or 'SKIP=0' not in check.stdout:
        raise RuntimeError('Suite de corretude nao passou integralmente:\n'+check.stdout+check.stderr)
    artifacts = [*bins.values(), ROOT/'Makefile', Path(__file__),
                 ROOT/'codigos/baselines_sequenciais_CCO085/01_matmul.cpp',
                 ROOT/'codigos/01_openmp/matmul_openmp.cpp', ROOT/'codigos/02_mpi/matmul_mpi.cpp']
    meta = {'started_utc':datetime.now(timezone.utc).isoformat(), 'platform':platform.platform(),
            'resources_before':limits, 'arguments':vars(args)|{'output':str(output)},
            'openmp_environment':controlled, 'mpi_launcher':launcher,
            'compiler':execute(['g++','--version'], env)[0],
            'mpi_version':execute(['mpirun','--version'], env)[0],
            'lscpu':execute(['lscpu'], env)[0], 'correctness':check.stdout,
            'sha256':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in artifacts},
            'build_flags':'-O2 -std=c++17 -Wall -Wextra -Wpedantic; OpenMP: -fopenmp -DMATMUL_BENCH_SCHEDULE=static; -lm',
            'skipped_workers':skipped, 'baseline_checksums':{}, 'status':'RUNNING'}
    output.parent.mkdir(parents=True, exist_ok=True)
    rng = random.Random(args.seed)
    count = 0
    with meta_path.open('x', encoding='utf-8') as metadata, output.open('x', encoding='utf-8', newline='') as stream:
        writer = csv.DictWriter(stream, fieldnames=['implementacao','N','trabalhadores','schedule',
            'repeticao','tempo_s','checksum','experimento','N_base','ordem','status'])
        writer.writeheader()
        try:
            refs = {n:measure('sequencial', n, 1)[1] for n in dimensions}
            meta['baseline_checksums'] = refs
            order = list(cases)
            rng.shuffle(order)
            for _, _, impl, n, p in order:
                measure(impl, n, p, refs[n])
            for repetition in range(1, args.repetitions+1):
                rng.shuffle(order)
                for experiment, base, impl, n, p in order:
                    elapsed, checksum = measure(impl, n, p, refs[n])
                    count += 1
                    writer.writerow(dict(implementacao=impl, N=n, trabalhadores=p,
                        schedule='static' if impl == 'openmp' else '', repeticao=repetition,
                        tempo_s=elapsed, checksum=checksum, experimento=experiment,
                        N_base=base, ordem=count, status='ok'))
                    stream.flush()
                print(f'Rodada {repetition}/{args.repetitions}: {count} medicoes validas', flush=True)
            meta['status'] = 'PARTIAL' if skipped else 'PASS'
        except BaseException as error:
            meta.update(status='FAIL', erro=str(error))
            raise
        finally:
            meta.update(finished_utc=datetime.now(timezone.utc).isoformat(), valid_measurements=count,
                        resources_after=resources())
            json.dump(meta, metadata, ensure_ascii=False, indent=2)
            metadata.write('\n')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
