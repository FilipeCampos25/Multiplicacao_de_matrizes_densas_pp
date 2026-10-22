#!/usr/bin/env python3
"""Corretude dos executaveis reais; somente bibliotecas padrao do Python."""

import functools
import math
import os
from pathlib import Path
import re
import shlex
import shutil
import signal
import subprocess
import sys
import unittest


ROOT = Path(__file__).resolve().parents[1]
DIMENSIONS = (1, 2, 3, 5, 8, 16, 32, 64)
WORKERS = (1, 2, 3, 4)
MPI_REQUIRED = {(4, 2), (5, 2), (7, 3), (10, 4), (2, 4), (8, 4)}
MPI_CASES = sorted({(n, p) for n in DIMENSIONS for p in WORKERS} | MPI_REQUIRED)
SMALL_LIMIT = 10
CHECKSUM_ATOL = 1e-6
ELEMENT_ATOL = 1e-10
RTOL = 1e-9


def compare(expected, actual, label="checksum", atol=CHECKSUM_ATOL):
    difference = abs(expected - actual)
    tolerance = atol + RTOL * max(abs(expected), abs(actual))
    detail = (f"{label}: esperado={expected:.17g}; obtido={actual:.17g}; "
              f"diferenca={difference:.3g}; tolerancia={tolerance:.3g}")
    if not math.isfinite(expected) or not math.isfinite(actual):
        raise AssertionError(f"Resultado NaN ou infinito. {detail}")
    if difference > tolerance:
        raise AssertionError(f"Resultado fora da tolerancia. {detail}")
    return detail


@functools.lru_cache(maxsize=None)
def python_matrix(n):
    """Mesmas formulas, indices row-major e ordem i-k-j do baseline."""
    a = [(i % 100) * 0.01 for i in range(n*n)]
    b = [((i * 7) % 100) * 0.01 for i in range(n*n)]
    c = [0.0] * (n*n)
    for i in range(n):
        for k in range(n):
            value = a[i*n + k]
            for j in range(n):
                c[i*n + j] += value * b[k*n + j]
    return tuple(c)


def sequential_sum(values):
    # Evita depender das alteracoes de precisao de sum() entre versoes Python.
    total = 0.0
    for value in values:
        total += value
    return total


def compare_matrix(n, expected, actual):
    if len(actual) != n*n or len(expected) != n*n:
        raise AssertionError(f"C incompleta/duplicada: esperado={n*n} elementos; "
                             f"obtido={len(actual)}")
    for index, (wanted, got) in enumerate(zip(expected, actual)):
        compare(wanted, got, f"C[{index // n},{index % n}]", ELEMENT_ATOL)


def execute(command, env, timeout=30):
    """Em Linux, encerra o grupo local inteiro (incluindo ranks) no timeout."""
    command = [str(arg) for arg in command]
    try:
        with subprocess.Popen(command, cwd=ROOT, env=env, text=True,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              start_new_session=(os.name == "posix")) as process:
            try:
                stdout, stderr = process.communicate(timeout=timeout)
            except subprocess.TimeoutExpired:
                if os.name == "posix":
                    try:
                        os.killpg(process.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                else:
                    process.kill()
                try:
                    stdout, stderr = process.communicate(timeout=5)
                except subprocess.TimeoutExpired as error:
                    # Um descendente fora do grupo pode ainda manter um pipe aberto.
                    # Nao deixe a coleta de diagnosticos bloquear a propria suite.
                    stdout, stderr = error.output or "", error.stderr or ""
                    process.stdout.close()
                    process.stderr.close()
                raise AssertionError(f"Timeout de {timeout}s: {shlex.join(command)}\n"
                                     f"stdout: {stdout}\nstderr: {stderr}")
            if process.returncode != 0:
                raise AssertionError(f"Retorno {process.returncode}: {shlex.join(command)}\n"
                                     f"stdout: {stdout}\nstderr: {stderr}")
            return stdout, stderr
    except OSError as error:
        raise AssertionError(f"Nao foi possivel executar {shlex.join(command)}: {error}") from error


def parse_output(stdout, n, dump=False):
    lines = re.findall(r"^N=.*$", stdout, re.MULTILINE)
    if len(lines) != 1:
        raise AssertionError(f"Esperada uma unica linha de resultado; obtido={len(lines)}")
    pairs = re.findall(r"(\w+)=([^\s]+)", lines[0])
    fields = dict(pairs)
    if len(pairs) != len(fields):
        raise AssertionError("Campos duplicados na linha de resultado")
    try:
        dimension = int(fields["N"])
        checksum = float(fields["checksum"])
        elapsed = float(fields["tempo"])
    except (KeyError, ValueError) as error:
        raise AssertionError("Campos N, checksum ou tempo ausentes/invalidos") from error
    if dimension != n:
        raise AssertionError(f"Dimensao incorreta: esperado={n}; obtido={dimension}")
    if not math.isfinite(checksum):
        raise AssertionError(f"Checksum NaN ou infinito: obtido={checksum}")
    if not math.isfinite(elapsed) or elapsed < 0:
        raise AssertionError(f"Tempo invalido: {elapsed}")
    matrices = re.findall(r"^C=(.*)$", stdout, re.MULTILINE)
    if len(matrices) != int(dump):
        raise AssertionError(f"Linhas C= esperadas={int(dump)}; obtidas={len(matrices)}")
    values = None
    if dump:
        try:
            values = [float(value) for value in matrices[0].split()]
        except ValueError as error:
            raise AssertionError("Elemento nao numerico em C") from error
        if len(values) != n*n or not all(math.isfinite(value) for value in values):
            raise AssertionError(f"C deve conter {n*n} elementos finitos; obtidos={len(values)}")
    return checksum, fields, values


class Programs:
    def __init__(self):
        self.bin_dir = ROOT / os.environ.get("MATMUL_BIN_DIR", "bin")
        self.timeout = float(os.environ.get("MATMUL_TEST_TIMEOUT", "30"))
        if not math.isfinite(self.timeout) or self.timeout <= 0:
            raise ValueError("MATMUL_TEST_TIMEOUT deve ser positivo e finito")
        self.env = {key: value for key, value in os.environ.items()
                    if not key.startswith("OMP_") and key != "MATMUL_DUMP"}
        self.env.update(LC_ALL="C", OMP_DYNAMIC="FALSE", OMP_THREAD_LIMIT="4")

    def binary(self, kind, name):
        source_dir = {"base": "baselines_sequenciais_CCO085",
                      "openmp": "01_openmp", "mpi": "02_mpi"}[kind]
        source = ROOT / "codigos" / source_dir / f"{name}.cpp"
        if not source.is_file() and kind != "base":
            raise unittest.SkipTest(f"Implementacao ausente: {source.relative_to(ROOT)}")
        binary = self.bin_dir / kind / name
        if not binary.is_file():
            raise AssertionError(f"Executavel ausente: {binary}. Execute make {kind}.")
        return binary

    def result(self, command, n, threads=1, dump=False):
        env = dict(self.env, OMP_NUM_THREADS=str(threads))
        if dump:
            env["MATMUL_DUMP"] = "1"
        stdout, stderr = execute(command, env, self.timeout)
        try:
            return parse_output(stdout, n, dump), stdout
        except AssertionError as error:
            raise AssertionError(f"{error}\nstdout: {stdout}\nstderr: {stderr}") from error

    @functools.lru_cache(maxsize=None)
    def baseline(self, n):
        (checksum, _, _), _ = self.result([self.binary("base", "01_matmul"), n], n)
        if n <= SMALL_LIMIT:
            compare(sequential_sum(python_matrix(n)), checksum, "checksum Python x baseline")
        return checksum

    def check_parallel(self, n, result):
        checksum, _, matrix = result
        detail = compare(self.baseline(n), checksum)
        if n <= SMALL_LIMIT:
            compare_matrix(n, python_matrix(n), matrix)
            detail += f"; {n*n} elementos conferidos"
        return detail

    def openmp(self, name, n, threads, argument=False):
        binary = self.binary("openmp", name)
        self.baseline(n)
        command = [binary, n]
        # Ambiente divergente verifica a prioridade do argumento opcional.
        env_threads = threads
        if argument:
            command.append(threads)
            env_threads = 1 if threads != 1 else 2
        result, _ = self.result(command, n, env_threads, n <= SMALL_LIMIT)
        detail = self.check_parallel(n, result)
        field = "threads_configuradas" if name == "01_matmul_openmp" else "threads"
        if result[1].get(field) != str(threads):
            raise AssertionError(f"Configuracao incorreta: {field} esperado={threads}; "
                                 f"obtido={result[1].get(field)}. {detail}")
        return detail

    @functools.lru_cache(maxsize=1)
    def launcher(self):
        requested = os.environ.get("MPI_RUN")
        launcher = shutil.which(requested) if requested else (
            shutil.which("mpirun") or shutil.which("mpiexec"))
        if not launcher:
            raise unittest.SkipTest("Launcher MPI ausente: instale/use o ambiente MPI ou defina MPI_RUN")
        stdout, stderr = execute([launcher, "--version"], self.env, self.timeout)
        arguments = shlex.split(os.environ.get("MPI_RUN_ARGS", ""))
        if re.search(r"Open MPI|OpenRTE", stdout + stderr, re.IGNORECASE):
            if "--oversubscribe" not in arguments:
                arguments.append("--oversubscribe")
        return [launcher, *arguments]

    def mpi(self, n, processes):
        binary = self.binary("mpi", "matmul_mpi")
        launcher = self.launcher()
        self.baseline(n)
        result, stdout = self.result([*launcher, "-np", processes, binary, n],
                                     n, dump=n <= SMALL_LIMIT)
        detail = self.check_parallel(n, result)
        # Confere tambem a distribuicao relatada, incluindo ranks sem linhas.
        loads = re.findall(r"^carga_linhas min=(\d+) max=(\d+) diferenca=(\d+)$",
                           stdout, re.MULTILINE)
        minimum = n // processes
        maximum = minimum + int(n % processes != 0)
        expected = (minimum, maximum, maximum - minimum)
        if len(loads) != 1 or tuple(map(int, loads[0])) != expected:
            raise AssertionError(f"Distribuicao incorreta: esperado min/max/diferenca={expected}; "
                                 f"obtido={loads}. {detail}")
        return detail


class Case(unittest.TestCase):
    def __init__(self, label, action):
        super().__init__("runTest")
        self.label = label
        self.action = action
        self.detail = ""

    def __str__(self):
        return self.label

    def runTest(self):
        self.detail = self.action() or ""


class Result(unittest.TextTestResult):
    def addSuccess(self, test):
        super().addSuccess(test)
        self.stream.writeln(f"[PASS] {test} | {getattr(test, 'detail', '')}")

    def addFailure(self, test, err):
        super().addFailure(test, err)
        self.stream.writeln(f"[FAIL] {test}\n{err[1]}")

    def addError(self, test, err):
        super().addError(test, err)
        self.stream.writeln(f"[FAIL] {test}\n{err[1]}")

    def addSkip(self, test, reason):
        super().addSkip(test, reason)
        self.stream.writeln(f"[SKIP] {test} | {reason}")


def build_suite():
    programs = Programs()
    cases = []
    for n in sorted(set(DIMENSIONS) | {n for n, _ in MPI_REQUIRED}):
        def baseline(n=n):
            checksum = programs.baseline(n)
            return f"checksum={checksum:.6f}" + ("; referencia Python conferida" if n <= SMALL_LIMIT else "")
        cases.append(Case(f"Sequencial | N={n}", baseline))
    for name in ("01_matmul_openmp", "matmul_openmp"):
        for n in DIMENSIONS:
            for threads in WORKERS:
                modes = (False, True) if name == "01_matmul_openmp" else (False,)
                for argument in modes:
                    mode = "argumento" if argument else "OMP_NUM_THREADS"
                    cases.append(Case(f"OpenMP/{name} | N={n} | Threads={threads} | {mode}",
                                      functools.partial(programs.openmp, name, n, threads, argument)))
    for n, processes in MPI_CASES:
        cases.append(Case(f"MPI | N={n} | Processos={processes}",
                          functools.partial(programs.mpi, n, processes)))
    return unittest.TestSuite(cases)


def load_tests(loader, tests, pattern):
    # Permite tambem python3 -m unittest discover -s tests.
    return build_suite()


def main():
    try:
        suite = build_suite()
    except ValueError as error:
        print(f"[FAIL] Configuracao dos testes: {error}", file=sys.stderr)
        return 1
    result = unittest.TextTestRunner(stream=sys.stdout, verbosity=0,
                                    resultclass=Result).run(suite)
    failed = len(result.failures) + len(result.errors)
    skipped = len(result.skipped)
    passed = result.testsRun - failed - skipped
    print(f"Resumo: PASS={passed} FAIL={failed} SKIP={skipped}")
    if skipped:
        print("Validacao incompleta: consulte os motivos de SKIP; casos nao executados nao foram aprovados.")
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    sys.exit(main())
