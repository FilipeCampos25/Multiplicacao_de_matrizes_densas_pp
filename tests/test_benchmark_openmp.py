"""Fixtures de estatistica, recursos e protecao dos dados; nao sao medicoes."""

import contextlib
import io
import math
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
import benchmark_openmp as benchmark


class BenchmarkTests(unittest.TestCase):
    @staticmethod
    def rows(threads, times, schedule="static"):
        return [{"N": 8, "schedule": schedule, "threads": threads,
                 "repeticao": i, "tempo_s": value} for i, value in enumerate(times, 1)]

    def test_statistics_and_same_schedule_reference(self):
        rows = self.rows(1, [2, 4, 6]) + self.rows(2, [1, 2, 3])
        rows += self.rows(1, [10, 20, 30], "dynamic") + self.rows(2, [2, 4, 6], "dynamic")
        result = {(row["schedule"], row["threads"]): row for row in benchmark.summarize(rows, 3)}
        static = result["static", 1]
        self.assertEqual(static["tempo_medio_s"], 4)
        self.assertEqual(static["tempo_mediano_s"], 4)
        self.assertEqual(static["tempo_min_s"], 2)
        self.assertEqual(static["tempo_max_s"], 6)
        self.assertEqual(static["desvio_padrao_s"], 2)
        self.assertEqual(static["cv_percentual"], 50)
        self.assertEqual(result["static", 2]["speedup"], 2)
        self.assertEqual(result["static", 2]["eficiencia"], 1)
        self.assertEqual(result["dynamic", 2]["speedup"], 5)

    def test_missing_reference_and_incomplete_groups(self):
        rows = self.rows(1, [1, 2]) + self.rows(2, [2, 3, 4])
        result = benchmark.summarize(rows, 3)
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]["speedup"], "")
        self.assertEqual(result[0]["eficiencia"], "")

    def test_duplicate_invalid_times_and_single_repetition(self):
        row = self.rows(1, [1])
        with self.assertRaises(ValueError):
            benchmark.summarize(row + row, 2)
        for value in (0, -1, math.nan, math.inf):
            with self.subTest(value=value), self.assertRaises(ValueError):
                benchmark.summarize(self.rows(1, [value]), 1)
        result = benchmark.summarize(row, 1)[0]
        self.assertEqual(result["desvio_padrao_s"], "")
        self.assertEqual(result["cv_percentual"], "")

    def test_affinity_cpuset_parent_quota_and_memory(self):
        child, parent = Path("/test/child"), Path("/test")
        data = {str(child / "cpu.max"): "400000 100000",
                str(parent / "cpu.max"): "250000 100000",
                str(child / "cpuset.cpus.effective"): "0-3,8",
                str(parent / "memory.max"): "1000000",
                str(parent / "memory.current"): "250000",
                "/proc/meminfo": "MemAvailable: 2000 kB"}
        with mock.patch.object(benchmark, "cgroup_paths", return_value=[child, parent]), \
             mock.patch.object(benchmark.os, "sched_getaffinity", return_value=set(range(12))), \
             mock.patch.object(benchmark, "read_text", side_effect=lambda path: data.get(str(path))):
            resources = benchmark.resources()
        self.assertEqual(resources["cpu_capacity"], 2.5)
        self.assertEqual(resources["cpus_after_cpuset"], [0, 1, 2, 3, 8])
        self.assertEqual(resources["memory_available_bytes"], 750000)

    def test_cgroup_mount_and_ancestors(self):
        data = {"/proc/self/cgroup": "0::/group/child",
                "/proc/self/mountinfo": "29 23 0:26 / /sys/fs/cgroup rw - cgroup2 cgroup rw"}
        with mock.patch.object(benchmark, "read_text", side_effect=lambda path: data.get(str(path))):
            paths = benchmark.cgroup_paths()
        self.assertEqual(paths, [Path("/sys/fs/cgroup"), Path("/sys/fs/cgroup/group"),
                                 Path("/sys/fs/cgroup/group/child")])

    def test_existing_output_is_preserved(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "coleta.csv"
            output.write_text("dados anteriores\n")
            with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as error:
                benchmark.main(["--output", str(output)])
            self.assertEqual(error.exception.code, 2)
            self.assertEqual(output.read_text(), "dados anteriores\n")


if __name__ == "__main__":
    unittest.main(verbosity=2)
