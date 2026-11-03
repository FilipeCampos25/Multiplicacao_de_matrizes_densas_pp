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
                writer = csv.DictWriter(stream, fieldnames=["implementacao", "n", "processos", "tempo_s", "scatterv_s", "bcast_s", "computacao_s", "gatherv_s", "status"])
                writer.writeheader()
                writer.writerow({"implementacao": "mpi", "n": 256, "processos": 2, "tempo_s": 10, "scatterv_s": 1, "bcast_s": 2, "computacao_s": 6, "gatherv_s": 1, "status": "ok"})
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


if __name__ == "__main__":
    unittest.main()
