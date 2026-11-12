#!/usr/bin/env python3
"""Compila a copia instrumentada e coleta tempos por trecho sem tocar no baseline."""
import argparse
import csv
import hashlib
import json
import os
import platform
import re
import shutil
import subprocess
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "codigos/instrumentacao/01_matmul_perfil.cpp"
BASELINE_SOURCE = ROOT / "codigos/00_sequencial/01_matmul.cpp"
PATTERN = re.compile(r"(N|checksum|total_s|alocacao_s|inicializacao_s|multiplicacao_s|checksum_s)=([^ ]+)")
BASELINE_PATTERN = re.compile(r"N=(\d+)\s+checksum=([^ ]+)\s+tempo=([^ ]+)\s+s")
FIELDS = ["N", "repeticao", "total_s", "alocacao_s", "inicializacao_s", "multiplicacao_s", "checksum_s", "checksum", "status"]


def executar(binary, n):
    result = subprocess.run([str(binary), str(n)], check=True, text=True, capture_output=True)
    values = dict(PATTERN.findall(result.stdout.strip()))
    missing = set(FIELDS[:1] + FIELDS[2:8]) - values.keys()
    if missing:
        raise RuntimeError(f"saida incompleta ({sorted(missing)}): {result.stdout!r}")
    return values


def executar_baseline(binary, n):
    result = subprocess.run([str(binary), str(n)], check=True, text=True, capture_output=True)
    match = BASELINE_PATTERN.search(result.stdout.strip())
    if not match:
        raise RuntimeError(f"saida incompleta do baseline: {result.stdout!r}")
    return float(match.group(2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sizes", nargs="+", type=int, default=[256, 512, 1024])
    parser.add_argument("--repetitions", type=int, default=5)
    parser.add_argument("--output", type=Path,
                        default=ROOT / "resultados/01_coletas/complementares/perfil_sequencial.csv")
    parser.add_argument("--compiler", default="g++")
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists() or output.with_name(output.stem + "_ambiente.txt").exists():
        parser.error("o destino e/ou seus metadados ja existem; escolha um nome novo")
    if args.repetitions < 1 or any(n < 1 for n in args.sizes):
        parser.error("dimensoes e repeticoes devem ser positivas")
    compiler = shutil.which(args.compiler)
    if not compiler:
        parser.error(f"compilador nao encontrado: {args.compiler}")

    binary = ROOT / "bin/perfil/01_matmul_perfil"
    baseline_binary = ROOT / "bin/perfil/01_matmul_baseline"
    binary.parent.mkdir(parents=True, exist_ok=True)
    flags = ["-O2", "-std=c++17", "-Wall", "-Wextra", "-Wpedantic"]
    subprocess.run([compiler, *flags, "-o", str(binary), str(SOURCE), "-lm"], check=True)
    subprocess.run([compiler, *flags, "-o", str(baseline_binary), str(BASELINE_SOURCE), "-lm"], check=True)
    output.parent.mkdir(parents=True, exist_ok=True)
    started = datetime.now(timezone.utc).isoformat()
    rows = []
    expected = {}
    for n in args.sizes:
        baseline_checksum = executar_baseline(baseline_binary, n)
        warmup = executar(binary, n)  # aquecimento, deliberadamente fora do CSV
        if abs(float(warmup["checksum"]) - baseline_checksum) > 1e-9 * (1 + abs(baseline_checksum)):
            raise RuntimeError(f"checksum do aquecimento diverge do baseline para N={n}")
        for repetition in range(1, args.repetitions + 1):
            values = executar(binary, n)
            checksum = float(values["checksum"])
            if abs(checksum - baseline_checksum) > 1e-9 * (1 + abs(baseline_checksum)):
                raise RuntimeError(f"checksum diverge do baseline para N={n}, repeticao={repetition}")
            expected.setdefault(n, baseline_checksum)
            rows.append({**values, "N": n, "repeticao": repetition, "status": "ok"})
    with output.open("x", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=FIELDS)
        writer.writeheader()
        writer.writerows(rows)

    digest = hashlib.sha256(SOURCE.read_bytes()).hexdigest()
    compiler_version = subprocess.run([compiler, "--version"], text=True, capture_output=True, check=True).stdout.splitlines()[0]
    metadata = {
        "started_utc": started,
        "finished_utc": datetime.now(timezone.utc).isoformat(),
        "platform": platform.platform(),
        "processor": platform.processor(),
        "cpu_logical": os.cpu_count(),
        "compiler": compiler_version,
        "flags": flags,
        "source": str(SOURCE.relative_to(ROOT)),
        "source_sha256": digest,
        "baseline_source": str(BASELINE_SOURCE.relative_to(ROOT)),
        "baseline_source_sha256": hashlib.sha256(BASELINE_SOURCE.read_bytes()).hexdigest(),
        "baseline_checksums": expected,
        "sizes": args.sizes,
        "repetitions": args.repetitions,
        "warmups_per_size": 1,
        "output": str(output),
        "observations": len(rows),
    }
    output.with_name(output.stem + "_ambiente.txt").write_text(
        json.dumps(metadata, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )
    print(f"Dados: {output}")
    print(f"Ambiente: {output.with_name(output.stem + '_ambiente.txt')}")


if __name__ == "__main__":
    main()
