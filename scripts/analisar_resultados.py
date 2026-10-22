#!/usr/bin/env python3
"""Valida o CSV e calcula medianas, speedup e eficiencia sem inventar dados."""

import csv
import statistics
import sys
from collections import defaultdict
from pathlib import Path


def main() -> int:
    if len(sys.argv) not in (2, 3):
        print("Uso: python3 scripts/analisar_resultados.py ENTRADA.csv [RESUMO.csv]", file=sys.stderr)
        return 2
    entrada = Path(sys.argv[1])
    saida = Path(sys.argv[2]) if len(sys.argv) == 3 else entrada.with_name(entrada.stem + "_resumo.csv")
    grupos = defaultdict(list)
    with entrada.open(newline="", encoding="utf-8") as arquivo:
        leitor = csv.DictReader(arquivo)
        obrigatorias = {"versao", "N", "paralelismo", "repeticao", "tempo_s", "checksum"}
        presentes = set(leitor.fieldnames or [])
        if not obrigatorias.issubset(presentes):
            print(f"Erro: colunas ausentes: {sorted(obrigatorias - presentes)}", file=sys.stderr)
            return 1
        for numero, linha in enumerate(leitor, 2):
            try:
                chave = (linha["versao"], int(linha["N"]), int(linha["paralelismo"]))
                grupos[chave].append((float(linha["tempo_s"]), float(linha["checksum"])))
            except (TypeError, ValueError) as erro:
                print(f"Erro na linha {numero}: {erro}", file=sys.stderr)
                return 1
    baselines = {
        n: (statistics.median(tempo for tempo, _ in valores), valores[0][1])
        for (versao, n, _), valores in grupos.items() if versao == "sequencial"
    }
    if not baselines:
        print("Erro: nenhuma medicao sequencial encontrada.", file=sys.stderr)
        return 1
    saida.parent.mkdir(parents=True, exist_ok=True)
    with saida.open("w", newline="", encoding="utf-8") as arquivo:
        campos = ["versao", "N", "paralelismo", "repeticoes", "tempo_mediano_s", "tempo_min_s", "tempo_max_s", "desvio_padrao_s", "speedup", "eficiencia"]
        escritor = csv.DictWriter(arquivo, fieldnames=campos)
        escritor.writeheader()
        for (versao, n, paralelismo), valores in sorted(grupos.items()):
            tempos = [tempo for tempo, _ in valores]
            checksums = [checksum for _, checksum in valores]
            referencia = checksums[0]
            tolerancia = 1.0e-9 * (1.0 + abs(referencia))
            if any(abs(valor - referencia) > tolerancia for valor in checksums):
                print(f"Erro: checksums inconsistentes em {versao}, N={n}, p={paralelismo}.", file=sys.stderr)
                return 1
            if n not in baselines:
                print(f"Erro: baseline ausente para N={n}.", file=sys.stderr)
                return 1
            tempo_baseline, checksum_baseline = baselines[n]
            tolerancia_baseline = 1.0e-9 * (1.0 + abs(checksum_baseline))
            if abs(referencia - checksum_baseline) > tolerancia_baseline:
                print(f"Erro: checksum de {versao}, N={n}, p={paralelismo} difere do baseline.", file=sys.stderr)
                return 1
            mediana = statistics.median(tempos)
            speedup = tempo_baseline / mediana
            escritor.writerow({
                "versao": versao, "N": n, "paralelismo": paralelismo,
                "repeticoes": len(tempos), "tempo_mediano_s": f"{mediana:.9f}",
                "tempo_min_s": f"{min(tempos):.9f}", "tempo_max_s": f"{max(tempos):.9f}",
                "desvio_padrao_s": f"{statistics.stdev(tempos) if len(tempos) > 1 else 0.0:.9f}",
                "speedup": f"{speedup:.6f}", "eficiencia": f"{speedup / paralelismo:.6f}",
            })
    print(f"Resumo: {saida}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
