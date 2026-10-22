"""Fixtures negativas: verificam o detector sem alterar fontes ou binarios."""

import contextlib
import io
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

import test_correctness as correctness


class HarnessTests(unittest.TestCase):
    def test_incorrect_checksum_from_controlled_process(self):
        stdout, _ = correctness.execute(
            [sys.executable, "-c", "print('N=2 checksum=999 tempo=0.0 s')"],
            os.environ.copy())
        actual, _, _ = correctness.parse_output(stdout, 2)
        expected = correctness.sequential_sum(correctness.python_matrix(2))
        with self.assertRaisesRegex(AssertionError, "fora da tolerancia"):
            correctness.compare(expected, actual)

    def test_permutation_preserving_checksum_is_rejected(self):
        expected = correctness.python_matrix(2)
        actual = list(expected)
        actual[0], actual[1] = actual[1], actual[0]
        correctness.compare(correctness.sequential_sum(expected),
                            correctness.sequential_sum(actual))
        with self.assertRaisesRegex(AssertionError, r"C\[0,0\]"):
            correctness.compare_matrix(2, expected, actual)

    def test_nan_and_infinity_are_rejected(self):
        for value in ("nan", "inf", "-inf"):
            with self.subTest(value=value):
                with self.assertRaises(AssertionError):
                    correctness.parse_output(f"N=1 checksum={value} tempo=0 s", 1)
                with self.assertRaises(AssertionError):
                    correctness.parse_output(f"N=1 checksum=0 tempo=0 s\nC= {value}", 1, True)
                with self.assertRaises(AssertionError):
                    correctness.compare(float(value), float(value))

    def test_missing_duplicate_and_invalid_fields_are_rejected(self):
        outputs = ("", "N=2 tempo=0 s", "N=3 checksum=0 tempo=0 s",
                   "N=2 checksum=0 checksum=1 tempo=0 s",
                   "N=2 checksum=0 tempo=0 s\nN=2 checksum=0 tempo=0 s",
                   "N=2 checksum=abc tempo=0 s", "N=2 checksum=0 tempo=-1 s")
        for output in outputs:
            with self.subTest(output=output), self.assertRaises(AssertionError):
                correctness.parse_output(output, 2)

    def test_incomplete_or_duplicate_matrix_is_rejected(self):
        prefix = "N=2 checksum=0 tempo=0 s\n"
        for dump in ("", "C= 0 0", "C= 0 0 0 0 0", "C= 0 0 0 0\nC= 0 0 0 0"):
            with self.subTest(dump=dump), self.assertRaises(AssertionError):
                correctness.parse_output(prefix + dump, 2, True)

    def test_nonzero_exit_captures_stderr(self):
        with self.assertRaisesRegex(AssertionError, "erro controlado"):
            correctness.execute([sys.executable, "-c",
                                 "import sys; print('erro controlado', file=sys.stderr); sys.exit(7)"],
                                os.environ.copy())

    def test_timeout(self):
        code = ("import subprocess, sys, time; "
                "subprocess.Popen([sys.executable, '-c', 'import time; time.sleep(10)']); "
                "time.sleep(10)")
        with self.assertRaisesRegex(AssertionError, "Timeout"):
            correctness.execute([sys.executable, "-c", code],
                                os.environ.copy(), timeout=0.2)

    def test_missing_implementation_is_skip_but_missing_binary_is_failure(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with mock.patch.object(correctness, "ROOT", root):
                programs = correctness.Programs()
                with self.assertRaises(unittest.SkipTest):
                    programs.binary("mpi", "matmul_mpi")
                source = root / "codigos/02_mpi/matmul_mpi.cpp"
                source.parent.mkdir(parents=True)
                source.touch()
                with self.assertRaisesRegex(AssertionError, "Executavel ausente"):
                    programs.binary("mpi", "matmul_mpi")

    def test_missing_launcher_is_skip(self):
        with mock.patch.object(correctness.shutil, "which", return_value=None):
            with self.assertRaises(unittest.SkipTest):
                correctness.Programs().launcher()

    def test_main_returns_nonzero_and_reports_failure(self):
        case = correctness.Case("Fixture | checksum incorreto",
                                lambda: correctness.compare(1.0, 2.0))
        output = io.StringIO()
        with mock.patch.object(correctness, "build_suite", return_value=unittest.TestSuite([case])):
            with contextlib.redirect_stdout(output):
                self.assertEqual(correctness.main(), 1)
        self.assertIn("[FAIL] Fixture", output.getvalue())
        self.assertIn("Resumo: PASS=0 FAIL=1 SKIP=0", output.getvalue())


if __name__ == "__main__":
    unittest.main(verbosity=2)
