#!/usr/bin/env python3
"""Resume CSVs complementares de perfil sequencial e etapas MPI."""
import argparse
import csv
import statistics
from pathlib import Path


def mediana(rows, field):
    return statistics.median(float(row[field]) for row in rows)


def escrever(path, fields, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("x", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def analisar_sequencial(path):
    with path.open(newline="", encoding="utf-8") as stream:
        rows = [r for r in csv.DictReader(stream) if r["status"] == "ok"]
    result = []
    for n in sorted({int(r["N"]) for r in rows}):
        group = [r for r in rows if int(r["N"]) == n]
        total = mediana(group, "total_s")
        for stage in ("alocacao", "inicializacao", "multiplicacao", "checksum"):
            value = mediana(group, stage + "_s")
            result.append({"N": n, "trecho": stage, "mediana_s": value,
                           "participacao_total_pct": 100 * value / total,
                           "repeticoes": len(group)})
    return result


def analisar_mpi(path):
    with path.open(newline="", encoding="utf-8") as stream:
        rows = [r for r in csv.DictReader(stream) if r["implementacao"] == "mpi" and r["status"] == "ok"]
    result = []
    keys = sorted({(int(r["n"]), int(r["processos"])) for r in rows})
    for n, processes in keys:
        group = [r for r in rows if int(r["n"]) == n and int(r["processos"]) == processes]
        total = mediana(group, "tempo_s")
        for stage, field in (("scatterv", "scatterv_s"), ("bcast", "bcast_s"),
                             ("computacao", "computacao_s"), ("gatherv", "gatherv_s")):
            value = mediana(group, field)
            result.append({"n": n, "processos": processes, "etapa": stage,
                           "mediana_s": value, "participacao_total_pct": 100 * value / total,
                           "tempo_total_mediano_s": total, "repeticoes": len(group)})
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sequencial", type=Path)
    parser.add_argument("--mpi", type=Path)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    if not args.sequencial and not args.mpi:
        parser.error("informe --sequencial e/ou --mpi")
    if args.sequencial:
        escrever(args.output_dir / "perfil_sequencial_resumo.csv",
                 ["N", "trecho", "mediana_s", "participacao_total_pct", "repeticoes"],
                 analisar_sequencial(args.sequencial))
    if args.mpi:
        escrever(args.output_dir / "etapas_mpi_resumo.csv",
                 ["n", "processos", "etapa", "mediana_s", "participacao_total_pct",
                  "tempo_total_mediano_s", "repeticoes"], analisar_mpi(args.mpi))


if __name__ == "__main__":
    main()
