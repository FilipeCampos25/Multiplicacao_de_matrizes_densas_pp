import csv
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts/analisar_complementacao.py"


class ComplementacaoTests(unittest.TestCase):
    def test_resume_perfil_e_etapas_mpi(self):
        with tempfile.TemporaryDirectory() as directory_name:
            directory = Path(directory_name)
            sequential = directory / "sequencial.csv"
            mpi = directory / "mpi.csv"
            output = directory / "resumos"
            with sequential.open("w", newline="", encoding="utf-8") as stream:
                writer = csv.DictWriter(stream, fieldnames=["N", "repeticao", "total_s", "alocacao_s", "inicializacao_s", "multiplicacao_s", "checksum_s", "checksum", "status"])
                writer.writeheader()
                writer.writerows([
                    {"N": 256, "repeticao": 1, "total_s": 10, "alocacao_s": 1, "inicializacao_s": 2, "multiplicacao_s": 6, "checksum_s": 1, "checksum": 42, "status": "ok"},
                    {"N": 256, "repeticao": 2, "total_s": 12, "alocacao_s": 1, "inicializacao_s": 2, "multiplicacao_s": 8, "checksum_s": 1, "checksum": 42, "status": "ok"},
                ])
            with mpi.open("w", newline="", encoding="utf-8") as stream:
                writer = csv.DictWriter(stream, fieldnames=["implementacao", "n", "processos", "repeticao", "tempo_s", "scatterv_s", "bcast_s", "computacao_s", "gatherv_s", "linhas_min", "linhas_max", "checksum", "status"])
                writer.writeheader()
                writer.writerow({"implementacao": "mpi", "n": 256, "processos": 2, "repeticao": 1, "tempo_s": 10, "scatterv_s": 1, "bcast_s": 2, "computacao_s": 6, "gatherv_s": 1, "linhas_min": 128, "linhas_max": 128, "checksum": 42, "status": "ok"})
            subprocess.run([sys.executable, str(SCRIPT), "--sequencial", str(sequential), "--mpi", str(mpi), "--output-dir", str(output)], check=True)
            with (output / "perfil_sequencial_resumo.csv").open(newline="", encoding="utf-8") as stream:
                profile = list(csv.DictReader(stream))
            with (output / "etapas_mpi_resumo.csv").open(newline="", encoding="utf-8") as stream:
                stages = list(csv.DictReader(stream))
            multiplication = next(row for row in profile if row["trecho"] == "multiplicacao")
            self.assertEqual(float(multiplication["mediana_s"]), 7.0)
            self.assertAlmostEqual(float(multiplication["participacao_total_pct"]), 700 / 11)
            self.assertEqual(len(stages), 4)
            self.assertEqual(float(stages[0]["tempo_total_mediano_s"]), 10.0)
            with (output / "configuracoes_mpi_resumo.csv").open(newline="", encoding="utf-8") as stream:
                configurations = list(csv.DictReader(stream))
            self.assertEqual(float(configurations[0]["comunicacao_proxy_nao_aditiva_s"]), 4.0)
            self.assertEqual(configurations[0]["speedup_relativo_p1"], "")

    def test_rejeita_coleta_incompleta(self):
        with tempfile.TemporaryDirectory() as directory_name:
            directory = Path(directory_name)
            mpi = directory / "mpi.csv"
            output = directory / "resumos"
            with mpi.open("w", newline="", encoding="utf-8") as stream:
                fields = ["implementacao", "n", "processos", "repeticao", "tempo_s",
                          "scatterv_s", "bcast_s", "computacao_s", "gatherv_s",
                          "linhas_min", "linhas_max", "checksum", "status"]
                writer = csv.DictWriter(stream, fieldnames=fields)
                writer.writeheader()
                writer.writerow({"implementacao": "mpi", "n": 256, "processos": 1,
                                 "repeticao": 1, "tempo_s": 10, "scatterv_s": 1,
                                 "bcast_s": 2, "computacao_s": 6, "gatherv_s": 1,
                                 "linhas_min": 256, "linhas_max": 256, "checksum": 42, "status": "ok"})
            result = subprocess.run(
                [sys.executable, str(SCRIPT), "--mpi", str(mpi),
                 "--output-dir", str(output), "--expected-sizes", "256",
                 "--expected-processes", "1", "2", "--minimum-repetitions", "5"],
                text=True, capture_output=True)
            self.assertEqual(result.returncode, 2)
            self.assertIn("configuracoes MPI ausentes: N=256/p=2", result.stderr)


if __name__ == "__main__":
    unittest.main()
