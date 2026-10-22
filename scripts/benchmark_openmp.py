#!/usr/bin/env python3
"""Mede os kernels reais, valida resultados e nunca sobrescreve uma coleta."""

import argparse
from collections import defaultdict
from contextlib import ExitStack
import csv
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import random
import re
import shutil
import statistics
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tests"))
from test_correctness import (  # noqa: E402 - reutiliza as tolerancias da Issue #6
    compare, compare_matrix, execute, parse_output, python_matrix,
)

RAW_FIELDS = ["N", "schedule", "threads", "repeticao", "tempo_s", "checksum",
              "threads_utilizadas", "ordem"]
SUMMARY_FIELDS = ["N", "schedule", "threads", "tempo_medio_s", "tempo_mediano_s",
                  "tempo_min_s", "tempo_max_s", "desvio_padrao_s", "cv_percentual",
                  "speedup", "eficiencia", "repeticoes_validas"]


def read_text(path):
    try:
        return Path(path).read_text().strip()
    except (OSError, UnicodeError):
        return None


def cpu_set(text):
    result = set()
    for part in text.split(","):
        bounds = part.split("-")
        first, last = int(bounds[0]), int(bounds[-1])
        result.update(range(first, last + 1))
    return result


def cgroup_paths():
    """Localiza cgroup v1/v2 e seus ancestrais visiveis no namespace atual."""
    memberships = {}
    for line in (read_text("/proc/self/cgroup") or "").splitlines():
        _, controllers, member = line.split(":", 2)
        for controller in controllers.split(","):
            memberships[controller] = member
    paths = []
    for line in (read_text("/proc/self/mountinfo") or "").splitlines():
        before, after = line.split(" - ", 1)
        info, filesystem = before.split(), after.split()
        if filesystem[0] not in ("cgroup", "cgroup2"):
            continue
        controller = "" if filesystem[0] == "cgroup2" else next(
            (item for item in filesystem[2].split(",") if item in memberships), None)
        if controller not in memberships:
            continue
        mount_root, mount = Path(info[3]), Path(info[4])
        member = Path(memberships[controller])
        try:
            relative = member.relative_to(mount_root)
        except ValueError:
            relative = member.relative_to("/")
        current = mount / relative
        while current == mount or mount in current.parents:
            paths.append(current)
            if current == mount:
                break
            current = current.parent
    return sorted(set(paths))


def resources():
    if not hasattr(os, "sched_getaffinity"):
        raise RuntimeError("Execute em Linux/Docker: sched_getaffinity indisponivel.")
    affinity = set(os.sched_getaffinity(0))
    usable = set(affinity)
    quotas, memory_headrooms, inspected = [], [], {}
    for directory in cgroup_paths():
        values = {}
        for name in ("cpu.max", "cpu.cfs_quota_us", "cpu.cfs_period_us",
                     "cpuset.cpus.effective", "cpuset.cpus",
                     "memory.max", "memory.current", "memory.limit_in_bytes",
                     "memory.usage_in_bytes"):
            value = read_text(directory / name)
            if value is not None:
                values[name] = value
        inspected[str(directory)] = values
        if "cpu.max" in values:
            quota, period = values["cpu.max"].split()
            if quota != "max":
                quotas.append(int(quota) / int(period))
        elif int(values.get("cpu.cfs_quota_us", "-1")) > 0:
            quotas.append(int(values["cpu.cfs_quota_us"]) / int(values["cpu.cfs_period_us"]))
        cpus = values.get("cpuset.cpus.effective") or values.get("cpuset.cpus")
        if cpus:
            usable &= cpu_set(cpus)
        limit = values.get("memory.max", values.get("memory.limit_in_bytes", "max"))
        current = values.get("memory.current", values.get("memory.usage_in_bytes", "0"))
        if limit != "max":
            memory_headrooms.append(max(0, int(limit) - int(current)))
    meminfo = dict(re.findall(r"^(\w+):\s+(\d+) kB", read_text("/proc/meminfo") or "", re.M))
    available = int(meminfo.get("MemAvailable", "0")) * 1024
    if available <= 0:
        raise RuntimeError("Nao foi possivel determinar MemAvailable.")
    cpu_capacity = min([len(usable), *quotas])
    return {"affinity": sorted(affinity), "cpus_after_cpuset": sorted(usable),
            "quota_cpu_equivalents": quotas, "cpu_capacity": cpu_capacity,
            "memory_available_bytes": min([available, *memory_headrooms]),
            "meminfo_available_bytes": available, "cgroups": inspected}


def summarize(rows, repetitions):
    groups = defaultdict(list)
    seen = set()
    for row in rows:
        key = (int(row["N"]), row["schedule"], int(row["threads"]))
        identity = (*key, int(row["repeticao"]))
        if identity in seen:
            raise ValueError(f"Repeticao duplicada: {identity}")
        seen.add(identity)
        value = float(row["tempo_s"])
        if not math.isfinite(value) or value <= 0:
            raise ValueError("Tempo deve ser positivo e finito")
        groups[key].append(value)
    # Grupos incompletos permanecem no bruto, mas nao originam metricas finais.
    complete = {key: times for key, times in groups.items() if len(times) == repetitions}
    result = []
    for (n, schedule, threads), times in sorted(complete.items()):
        mean, median = statistics.mean(times), statistics.median(times)
        deviation = statistics.stdev(times) if len(times) > 1 else None
        reference = complete.get((n, schedule, 1))
        speedup = statistics.median(reference) / median if reference else None
        result.append(dict(zip(SUMMARY_FIELDS, [n, schedule, threads, mean, median,
            min(times), max(times), deviation if deviation is not None else "",
            100 * deviation / mean if deviation is not None else "",
            speedup if speedup is not None else "",
            speedup / threads if speedup is not None else "", len(times)])))
    return result


def output_paths(output):
    prefix = output.stem.removesuffix("_desempenho")
    return {"raw": output, "summary": output.with_name(prefix + "_resumo.csv"),
            "status": output.with_name(prefix + "_status.csv"),
            "environment": output.with_name(prefix + "_ambiente.txt")}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sizes", nargs="+", type=int, default=[256, 512, 1024])
    parser.add_argument("--threads", nargs="+", type=int, default=[1, 2, 4, 8])
    parser.add_argument("--repetitions", type=int, default=5)
    parser.add_argument("--schedules", nargs="+", choices=["static", "dynamic", "guided"],
                        default=["static", "dynamic", "guided"])
    parser.add_argument("--output", type=Path, default=ROOT / "resultados/openmp_desempenho.csv")
    parser.add_argument("--timeout", type=float, default=120)
    parser.add_argument("--seed", type=int, default=8506)
    args = parser.parse_args(argv)
    if (args.repetitions <= 0 or not math.isfinite(args.timeout) or args.timeout <= 0
            or any(value <= 0 for value in args.sizes + args.threads)):
        parser.error("Dimensoes, threads, repeticoes e timeout devem ser positivos e finitos.")
    if any(n*n > 2147483647 or 7*(n*n - 1) > 2147483647 for n in args.sizes):
        parser.error("Dimensao excede os indices/int da inicializacao do baseline.")
    if any(len(values) != len(set(values)) for values in (args.sizes, args.threads, args.schedules)):
        parser.error("Listas nao podem conter valores duplicados.")
    paths = output_paths(args.output.resolve())
    if len(set(paths.values())) != len(paths):
        parser.error("Escolha um nome de CSV bruto diferente dos nomes auxiliares.")
    existing = [str(path) for path in paths.values() if path.exists()]
    if existing:
        parser.error("Coleta existente; use outro --output. Nao sobrescrito: " + ", ".join(existing))

    bin_dir = ROOT / os.environ.get("MATMUL_BIN_DIR", "bin")
    binaries = {schedule: bin_dir / "benchmark_openmp" / f"matmul_{schedule}"
                for schedule in args.schedules}
    baseline = bin_dir / "base/01_matmul"
    for binary in [baseline, *binaries.values()]:
        if not binary.is_file():
            parser.error(f"Executavel ausente: {binary}. Execute make benchmark-openmp-build.")
    limits = resources()
    configurations = [(s, p) for s in args.schedules for p in args.threads
                      if p <= limits["cpu_capacity"]]
    env = {key: value for key, value in os.environ.items()
           if not key.startswith(("OMP_", "GOMP_", "KMP_")) and key != "MATMUL_DUMP"}
    controlled = {"LC_ALL": "C", "OMP_DYNAMIC": "FALSE", "OMP_PROC_BIND": "close",
                  "OMP_PLACES": "threads", "OMP_WAIT_POLICY": "PASSIVE",
                  "OMP_THREAD_LIMIT": str(max(args.threads))}
    env.update(controlled)
    print(f"Recursos: capacidade CPU={limits['cpu_capacity']}; "
          f"memoria disponivel={limits['memory_available_bytes']} bytes", flush=True)

    metadata = {"started_utc": datetime.now(timezone.utc).isoformat(),
                "arguments": vars(args) | {"output": str(args.output)},
                "resources_before": limits, "platform": platform.platform(),
                "os_release": read_text("/etc/os-release"), "openmp_environment": controlled,
                "baseline_checksums": {}, "status": "RUNNING", "variant_preflight_cases": 0}
    rows, statuses = [], []
    rng = random.Random(args.seed)
    paths["raw"].parent.mkdir(parents=True, exist_ok=True)
    with ExitStack() as stack:
        # Modo x: inclusive em caso de concorrencia, nunca trunca uma coleta.
        files = {name: stack.enter_context(path.open("x", newline="", encoding="utf-8"))
                 for name, path in paths.items()}
        raw_writer = csv.DictWriter(files["raw"], fieldnames=RAW_FIELDS)
        raw_writer.writeheader()
        status_writer = csv.DictWriter(files["status"], fieldnames=[
            "status", "N", "schedule", "threads", "etapa", "repeticao", "motivo"])
        status_writer.writeheader()

        def status(code, n="", schedule="", threads="", stage="", repetition="", reason=""):
            item = dict(zip(status_writer.fieldnames, [code, n, schedule, threads, stage, repetition, reason]))
            statuses.append(item)
            status_writer.writerow(item)
            files["status"].flush()
            print(f"[{code}] N={n} schedule={schedule} threads={threads} {stage} {reason}", flush=True)

        def measure(binary, n, threads=None, expected=None, dump=False):
            run_env = dict(env, OMP_NUM_THREADS=str(threads or 1))
            if dump:
                run_env["MATMUL_DUMP"] = "1"
            stdout, _ = execute([binary, n], run_env, args.timeout)
            checksum, fields, matrix = parse_output(stdout, n, dump)
            elapsed = float(fields["tempo"])
            if expected is not None:
                compare(expected, checksum)
            if threads is not None and fields.get("threads") != str(threads):
                raise AssertionError(f"Threads solicitadas={threads}; utilizadas={fields.get('threads')}")
            if dump:
                compare_matrix(n, python_matrix(n), matrix)
            return elapsed, checksum

        stage, n, schedule, threads, repetition = "preparacao", "", "", "", ""
        failed = False
        try:
            metadata["git_revision"] = (execute(["git", "rev-parse", "HEAD"], env)[0].strip()
                                        if shutil.which("git") else "git indisponivel; ver hashes dos artefatos")
            metadata["lscpu"] = execute(["lscpu"], env)[0]
            metadata["compiler"] = execute(["g++", "--version"], env)[0]
            metadata["build_commands"] = {}
            for name, binary in binaries.items():
                command = read_text(str(binary) + ".build.txt")
                if not command:
                    raise RuntimeError(f"Manifesto de compilacao ausente: {binary}.build.txt")
                metadata["build_commands"][name] = command
            artifacts = [baseline, *binaries.values(), ROOT / "codigos/01_openmp/matmul_openmp.cpp",
                         ROOT / "scripts/benchmark_openmp.py", ROOT / "Makefile"]
            metadata["sha256"] = {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                                  for path in artifacts}
            stage = "corretude existente"
            test_output, _ = execute(["make", "test-correctness", f"BIN_DIR={bin_dir}"], env, 300)
            summaries = re.findall(r"Resumo: PASS=\d+ FAIL=\d+ SKIP=\d+", test_output)
            if not summaries:
                raise AssertionError("Suite existente nao produziu resumo de corretude")
            metadata["correctness"] = summaries[-1]
            print(metadata["correctness"], flush=True)
            stage = "corretude variantes"
            _, small_reference = measure(baseline, 5)
            for schedule, threads in configurations:
                measure(binaries[schedule], 5, threads, small_reference, dump=True)
                metadata["variant_preflight_cases"] += 1
            for n in args.sizes:
                for schedule in args.schedules:
                    for threads in args.threads:
                        if threads > limits["cpu_capacity"]:
                            status("SKIP", n, schedule, threads, "recursos", reason="Excede CPU efetiva visivel; sem oversubscription")
                if 24*n*n > limits["memory_available_bytes"] * 0.5:
                    status("SKIP", n, stage="recursos", reason="Matrizes excedem 50% da memoria disponivel inicial")
                    continue
                if not configurations:
                    continue
                stage, schedule, threads, repetition = "baseline", "", "", ""
                _, reference = measure(baseline, n)
                metadata["baseline_checksums"][n] = reference
                stage = "aquecimento"
                order = list(configurations)
                rng.shuffle(order)
                for schedule, threads in order:
                    measure(binaries[schedule], n, threads, reference)
                for repetition in range(1, args.repetitions + 1):
                    stage = "medicao"
                    rng.shuffle(order)
                    for schedule, threads in order:
                        elapsed, checksum = measure(binaries[schedule], n, threads, reference)
                        if elapsed <= 0:
                            raise AssertionError(f"Tempo sem resolucao suficiente: {elapsed}")
                        row = dict(zip(RAW_FIELDS, [n, schedule, threads, repetition,
                            elapsed, checksum, threads, len(rows) + 1]))
                        rows.append(row)
                        raw_writer.writerow(row)
                        files["raw"].flush()
                    print(f"N={n}: rodada {repetition}/{args.repetitions} concluida", flush=True)
                for schedule, threads in configurations:
                    status("PASS", n, schedule, threads, "coleta", reason=f"{args.repetitions} repeticoes validas")
            if not rows:
                raise RuntimeError("Nenhuma medicao valida; consulte os SKIP e os recursos")
        except (AssertionError, OSError, RuntimeError, ValueError, KeyboardInterrupt) as error:
            failed = True
            status("FAIL", n, schedule, threads, stage, repetition, str(error))
        finally:
            summary_writer = csv.DictWriter(files["summary"], fieldnames=SUMMARY_FIELDS)
            summary_writer.writeheader()
            summary_writer.writerows(summarize(rows, args.repetitions))
            metadata.update(finished_utc=datetime.now(timezone.utc).isoformat(),
                            valid_measurements=len(rows),
                            status="FAIL" if failed else ("PARTIAL" if any(s["status"] == "SKIP" for s in statuses) else "PASS"))
            metadata["resources_after"] = resources()
            json.dump(metadata, files["environment"], indent=2, ensure_ascii=False)
            files["environment"].write("\n")
    print(f"Final: {metadata['status']}; medicoes validas={len(rows)}; dados={paths['raw']}", flush=True)
    return 1 if failed else 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, RuntimeError, ValueError) as error:
        print(f"[FAIL] {error}", file=sys.stderr)
        sys.exit(1)
