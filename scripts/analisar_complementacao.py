#!/usr/bin/env python3
"""Valida e resume CSVs complementares de perfil sequencial e etapas MPI."""
import argparse
import csv
import math
import statistics
import sys
from pathlib import Path


def estatisticas(rows, field):
    values = [float(row[field]) for row in rows]
    return {
        "media_s": statistics.mean(values),
        "mediana_s": statistics.median(values),
        "desvio_padrao_s": statistics.stdev(values) if len(values) > 1 else 0.0,
        "min_s": min(values),
        "max_s": max(values),
    }


def escrever(path, fields, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("x", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def analisar_sequencial(path):
    with path.open(newline="", encoding="utf-8") as stream:
        reader = csv.DictReader(stream)
        required = {"N", "repeticao", "total_s", "alocacao_s", "inicializacao_s",
                    "multiplicacao_s", "checksum_s", "checksum", "status"}
        missing = required - set(reader.fieldnames or [])
        if missing:
            raise ValueError("colunas sequenciais ausentes: " + ", ".join(sorted(missing)))
        rows = [r for r in reader if r["status"] == "ok"]
    if not rows:
        raise ValueError("o CSV sequencial nao contem medicoes validas")
    seen, checksums = set(), {}
    for row in rows:
        n, repetition = int(row["N"]), int(row["repeticao"])
        if n <= 0 or repetition <= 0 or (n, repetition) in seen:
            raise ValueError(f"N={n}/repeticao={repetition}: dimensao ou repeticao invalida/duplicada")
        seen.add((n, repetition))
        for field in ("total_s", "alocacao_s", "inicializacao_s", "multiplicacao_s", "checksum_s"):
            value = float(row[field])
            if not math.isfinite(value) or value < 0 or (field == "total_s" and value == 0):
                raise ValueError(f"N={n}/repeticao={repetition}: {field} invalido")
        checksum = float(row["checksum"])
        if not math.isfinite(checksum):
            raise ValueError(f"N={n}/repeticao={repetition}: checksum invalido")
        reference = checksums.setdefault(n, checksum)
        if abs(reference-checksum) > 1e-6 + 1e-9*max(abs(reference), abs(checksum)):
            raise ValueError(f"N={n}: checksum divergente")
    result = []
    for n in sorted({int(r["N"]) for r in rows}):
        group = [r for r in rows if int(r["N"]) == n]
        total = estatisticas(group, "total_s")["mediana_s"]
        for stage in ("alocacao", "inicializacao", "multiplicacao", "checksum"):
            stats = estatisticas(group, stage + "_s")
            result.append({"N": n, "trecho": stage, **stats,
                           "participacao_total_pct": 100 * stats["mediana_s"] / total,
                           "tempo_total_mediano_s": total, "repeticoes": len(group)})
    return result


def analisar_mpi(path, expected_sizes=None, expected_processes=None,
                 minimum_repetitions=1):
    if minimum_repetitions < 1:
        raise ValueError("minimum-repetitions deve ser positivo")
    if (expected_sizes is None) != (expected_processes is None):
        raise ValueError("expected-sizes e expected-processes devem ser usados juntos")
    with path.open(newline="", encoding="utf-8") as stream:
        reader = csv.DictReader(stream)
        required = {"implementacao", "n", "processos", "repeticao", "tempo_s",
                    "scatterv_s", "bcast_s", "computacao_s", "gatherv_s",
                    "linhas_min", "linhas_max", "checksum", "status"}
        missing = required - set(reader.fieldnames or [])
        if missing:
            raise ValueError("colunas MPI ausentes: " + ", ".join(sorted(missing)))
        rows = [r for r in reader
                if r["implementacao"] == "mpi" and r["status"] == "ok"]
    if not rows:
        raise ValueError("o CSV MPI nao contem medicoes validas")

    result = []
    configurations = []
    keys = sorted({(int(r["n"]), int(r["processos"])) for r in rows})
    if expected_sizes is not None and expected_processes is not None:
        expected = {(n, p) for n in expected_sizes for p in expected_processes}
        absent = sorted(expected - set(keys))
        if absent:
            raise ValueError("configuracoes MPI ausentes: " +
                             ", ".join(f"N={n}/p={p}" for n, p in absent))

    baselines = {}
    checksums = {}
    for n, processes in keys:
        if processes == 1:
            group = [r for r in rows if int(r["n"]) == n and int(r["processos"]) == 1]
            baselines[n] = statistics.median(float(r["tempo_s"]) for r in group)

    for n, processes in keys:
        group = [r for r in rows if int(r["n"]) == n and int(r["processos"]) == processes]
        if len(group) < minimum_repetitions:
            raise ValueError(f"N={n}/p={processes}: {len(group)} repeticoes validas; "
                             f"minimo exigido={minimum_repetitions}")
        repetitions = [int(r["repeticao"]) for r in group]
        if len(repetitions) != len(set(repetitions)):
            raise ValueError(f"N={n}/p={processes}: repeticoes duplicadas")
        numeric_fields = ("tempo_s", "scatterv_s", "bcast_s", "computacao_s",
                          "gatherv_s")
        for row in group:
            for field in numeric_fields:
                value = float(row[field])
                if value < 0 or not (value < float("inf")):
                    raise ValueError(f"N={n}/p={processes}: {field} invalido")
            checksum = float(row["checksum"])
            if not math.isfinite(checksum):
                raise ValueError(f"N={n}/p={processes}: checksum invalido")
            reference = checksums.setdefault(n, checksum)
            if abs(reference-checksum) > 1e-6 + 1e-9*max(abs(reference), abs(checksum)):
                raise ValueError(f"N={n}/p={processes}: checksum divergente")
        total = estatisticas(group, "tempo_s")["mediana_s"]
        if total == 0:
            raise ValueError(f"N={n}/p={processes}: tempo total mediano igual a zero")
        stage_medians = {}
        for stage, field in (("scatterv", "scatterv_s"), ("bcast", "bcast_s"),
                             ("computacao", "computacao_s"), ("gatherv", "gatherv_s")):
            value = estatisticas(group, field)["mediana_s"]
            stage_medians[stage] = value
            result.append({"n": n, "processos": processes, "etapa": stage,
                           "mediana_s": value, "participacao_total_pct": 100 * value / total,
                           "tempo_total_mediano_s": total, "repeticoes": len(group)})
        minimum_rows = {int(r["linhas_min"]) for r in group}
        maximum_rows = {int(r["linhas_max"]) for r in group}
        expected_minimum = n // processes
        expected_maximum = expected_minimum + (1 if n % processes else 0)
        if minimum_rows != {expected_minimum} or maximum_rows != {expected_maximum}:
            raise ValueError(
                f"N={n}/p={processes}: carga observada {sorted(minimum_rows)}/"
                f"{sorted(maximum_rows)}, esperada {expected_minimum}/{expected_maximum}")
        communication_proxy = (stage_medians["scatterv"] + stage_medians["bcast"] +
                               stage_medians["gatherv"])
        baseline = baselines.get(n)
        configurations.append({
            "n": n, "processos": processes, "repeticoes": len(group),
            "tempo_total_mediano_s": total,
            "scatterv_mediana_s": stage_medians["scatterv"],
            "bcast_mediana_s": stage_medians["bcast"],
            "computacao_mediana_s": stage_medians["computacao"],
            "gatherv_mediana_s": stage_medians["gatherv"],
            # Proxy descritivo, nao tempo real: os maximos por etapa podem vir
            # de ranks distintos e nao podem reconstruir o caminho critico.
            "comunicacao_proxy_nao_aditiva_s": communication_proxy,
            "comunicacao_proxy_pct_total": 100 * communication_proxy / total,
            "speedup_relativo_p1": "" if baseline is None else baseline / total,
            "eficiencia_relativa_p1_pct": "" if baseline is None else
                100 * baseline / total / processes,
            "linhas_min": expected_minimum, "linhas_max": expected_maximum,
        })
    return result, configurations


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sequencial", type=Path)
    parser.add_argument("--mpi", type=Path)
    parser.add_argument("--output-dir", type=Path,
                        default=Path(__file__).resolve().parents[1] /
                        "resultados/02_analises/complementares")
    parser.add_argument("--expected-sizes", type=int, nargs="+")
    parser.add_argument("--expected-processes", type=int, nargs="+")
    parser.add_argument("--minimum-repetitions", type=int, default=1)
    args = parser.parse_args()
    if not args.sequencial and not args.mpi:
        parser.error("informe --sequencial e/ou --mpi")
    if args.sequencial:
        escrever(args.output_dir / "perfil_sequencial_resumo.csv",
                 ["N", "trecho", "media_s", "mediana_s", "desvio_padrao_s", "min_s", "max_s",
                  "participacao_total_pct", "tempo_total_mediano_s", "repeticoes"],
                 analisar_sequencial(args.sequencial))
    if args.mpi:
        stages, configurations = analisar_mpi(
            args.mpi, args.expected_sizes, args.expected_processes,
            args.minimum_repetitions)
        escrever(args.output_dir / "etapas_mpi_resumo.csv",
                 ["n", "processos", "etapa", "mediana_s", "participacao_total_pct",
                  "tempo_total_mediano_s", "repeticoes"], stages)
        escrever(args.output_dir / "configuracoes_mpi_resumo.csv",
                 ["n", "processos", "repeticoes", "tempo_total_mediano_s",
                  "scatterv_mediana_s", "bcast_mediana_s", "computacao_mediana_s",
                  "gatherv_mediana_s", "comunicacao_proxy_nao_aditiva_s",
                  "comunicacao_proxy_pct_total", "speedup_relativo_p1",
                  "eficiencia_relativa_p1_pct", "linhas_min", "linhas_max"],
                 configurations)


if __name__ == "__main__":
    try:
        main()
    except (KeyError, ValueError) as error:
        print(f"Erro: {error}", file=sys.stderr)
        raise SystemExit(2)
