"""
Demonstracao do GIL — Programacao Paralela (CCO085) — IESB 2026/2
Prof. Rodrigo Goncalves Pinto

Roda a MESMA carga de trabalho limitada por CPU de tres formas:
  1. sequencial
  2. com threading      -> nao acelera, por causa do GIL
  3. com multiprocessing -> acelera, porque cada processo tem seu proprio GIL

Executar:  python3 gil_demo.py
"""

import time
from concurrent.futures import ThreadPoolExecutor, ProcessPoolExecutor
import multiprocessing as mp

N_TAREFAS = 4
TAMANHO = 8_000_000


def trabalho(n):
    """Carga puramente de CPU: soma de uma serie."""
    total = 0.0
    passo = 1.0 / n
    for i in range(n):
        x = (i + 0.5) * passo
        total += 4.0 / (1.0 + x * x)
    return total * passo


def cronometra(rotulo, funcao):
    t0 = time.perf_counter()
    resultado = funcao()
    dt = time.perf_counter() - t0
    print(f"{rotulo:<22} t = {dt:7.3f} s")
    return dt, resultado


def sequencial():
    return [trabalho(TAMANHO) for _ in range(N_TAREFAS)]


def com_threads():
    with ThreadPoolExecutor(max_workers=N_TAREFAS) as ex:
        return list(ex.map(trabalho, [TAMANHO] * N_TAREFAS))


def com_processos():
    with ProcessPoolExecutor(max_workers=N_TAREFAS) as ex:
        return list(ex.map(trabalho, [TAMANHO] * N_TAREFAS))


def main():
    print(f"Nucleos disponiveis: {mp.cpu_count()}")
    print(f"{N_TAREFAS} tarefas de {TAMANHO:,} iteracoes cada\n")

    t_seq, _ = cronometra("sequencial", sequencial)
    t_thr, _ = cronometra("threading", com_threads)
    t_prc, _ = cronometra("multiprocessing", com_processos)

    print()
    print(f"speedup com threads   : {t_seq / t_thr:.2f}x")
    print(f"speedup com processos : {t_seq / t_prc:.2f}x")
    print()
    print("Leitura do resultado:")
    print("  O speedup com threads fica proximo de 1 mesmo com varios nucleos.")
    print("  O GIL permite que apenas uma thread execute bytecode Python por vez,")
    print("  entao threads nao ajudam em carga limitada por CPU — so em carga")
    print("  limitada por E/S, onde a thread libera o GIL enquanto espera.")
    print("  Com processos o GIL deixa de ser gargalo, mas paga-se o custo de")
    print("  criar processos e serializar os dados entre eles.")


if __name__ == "__main__":
    main()
