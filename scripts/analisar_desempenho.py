#!/usr/bin/env python3
"""Consolida observacoes reais; SVG e estatisticas usam somente a biblioteca padrao."""
import argparse
import csv
import hashlib
import json
import math
import statistics as st
from collections import defaultdict
from html import escape
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RAW = ['implementacao', 'N', 'trabalhadores', 'schedule', 'repeticao', 'tempo_s',
       'checksum', 'ambiente', 'experimento', 'N_base', 'origem', 'linha', 'metadados', 'dados_originais']
KEY = ['ambiente', 'experimento', 'N_base', 'implementacao', 'N', 'trabalhadores', 'schedule']
STATS = KEY + ['repeticoes', 'tempo_medio_s', 'tempo_mediano_s', 'tempo_min_s',
              'tempo_max_s', 'desvio_padrao_s', 'cv_percentual', 'speedup', 'eficiencia',
              'speedup_relativo_1', 'eficiencia_relativa_1']
GENERATED = {'desempenho_consolidado.csv', 'metricas_desempenho.csv', 'escalabilidade_fraca.csv'}


def finite(value, positive=False):
    number = float(value)
    if not math.isfinite(number) or (positive and number <= 0):
        raise ValueError('numero nao finito ou nao positivo')
    return number


def compatible(a, b):
    return abs(a-b) <= 1e-6 + 1e-9 * max(abs(a), abs(b))


def load(directory):
    rows, notes, sources, seen_files = [], [], [], set()
    for path in sorted(directory.rglob('*.csv')):
        if path.name in GENERATED or 'graficos' in path.parts:
            continue
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        if digest in seen_files:
            notes.append(f'{path.name}: copia identica ignorada (SHA-256).')
            continue
        seen_files.add(digest)
        with path.open(encoding='utf-8-sig', newline='') as stream:
            reader = csv.DictReader(stream)
            headers = set(reader.fieldnames or [])
            if not {'tempo_s', 'repeticao', 'checksum'} <= headers or not ({'N', 'n'} & headers):
                notes.append(f'{path.name}: nao e CSV de observacoes individuais; ignorado.')
                continue
            if not ({'implementacao', 'versao', 'threads'} & headers) and path.name != 'baseline_sequencial.csv':
                notes.append(f'{path.name}: implementacao desconhecida; ignorado.')
                continue
            prefix = path.stem.removesuffix('_desempenho')
            meta_path = path.with_name(prefix + '_ambiente.txt')
            if not meta_path.exists():
                meta_path = path.with_name(path.stem + '_ambiente.txt')
            metadata = {}
            if meta_path.exists():
                try:
                    metadata = json.loads(meta_path.read_text(encoding='utf-8-sig'))
                except (ValueError, UnicodeError):
                    notes.append(f'{path.name}: metadados textuais; comparacoes restritas ao arquivo.')
            # Cada coleta e isolada por padrao. Somente o coletor conjunto associa implementacoes.
            environment = path.relative_to(directory).as_posix()
            sources.append({'arquivo': environment, 'sha256': digest,
                            'metadados': meta_path.name if meta_path.exists() else '',
                            'conteudo_metadados': metadata})
            identities = {}
            for line, item in enumerate(reader, 2):
                try:
                    if item.get('status', 'ok').lower() not in ('ok', 'pass'):
                        notes.append(f'{environment}:{line}: status invalido; ignorado.')
                        continue
                    impl = item.get('implementacao') or item.get('versao') or ('openmp' if 'threads' in headers else 'sequencial')
                    if impl not in ('sequencial', 'openmp', 'mpi'):
                        raise ValueError('implementacao desconhecida')
                    n = int(item.get('N') or item.get('n'))
                    p = int(item.get('trabalhadores') or item.get('threads') or item.get('processos') or item.get('paralelismo') or 1)
                    repetition = int(item['repeticao'])
                    t, checksum = finite(item['tempo_s'], True), finite(item['checksum'])
                    if min(n, p, repetition) <= 0 or (impl == 'sequencial' and p != 1):
                        raise ValueError('N, trabalhadores ou repeticao invalidos')
                    if item.get('threads_utilizadas') and int(item['threads_utilizadas']) != p:
                        raise ValueError('equipe efetiva difere da solicitada')
                    experiment = item.get('experimento') or 'forte'
                    base = int(item.get('N_base') or 0)
                    if experiment not in ('forte', 'fraca') or (experiment == 'fraca' and base <= 0):
                        raise ValueError('experimento/N_base invalido')
                    if experiment == 'fraca' and n != round(base * p**(1/3)):
                        raise ValueError('N nao corresponde ao arredondamento da serie fraca')
                    row_environment = environment + ('::'+item['ambiente'] if item.get('ambiente') else '')
                    row = dict(zip(RAW, [impl, n, p, item.get('schedule', ''), repetition,
                        t, checksum, row_environment, experiment, base, environment, line,
                        meta_path.name if meta_path.exists() else '', json.dumps(item, ensure_ascii=False, sort_keys=True)]))
                    identity = tuple(row[k] for k in KEY) + (repetition,)
                    if identity in identities:
                        raise ValueError('repeticao duplicada')
                    identities[identity] = row
                    rows.append(row)
                except (ValueError, TypeError) as error:
                    raise ValueError(f'{path}:{line}: {error}') from error
            # Verifica checksums dentro de N, incluindo entre implementacoes, sem depender de tempos.
            refs = {int(n): finite(c) for n, c in metadata.get('baseline_checksums', {}).items()}
            for row in identities.values():
                n = row['N']
                reference = refs.setdefault((row['ambiente'], n), refs.get(n, row['checksum']))
                if not compatible(reference, row['checksum']):
                    raise ValueError(f'{path}:{row["linha"]}: checksum divergente para N={n}')
            if not meta_path.exists():
                notes.append(f'{environment}: ambiente nao registrado; isolado, sem comparacao entre arquivos.')
    return rows, notes, sources


def summarize(rows):
    groups = defaultdict(list)
    for row in rows:
        groups[tuple(row[k] for k in KEY)].append(row['tempo_s'])
    result = []
    for key, times in sorted(groups.items()):
        row = dict(zip(KEY, key))
        mean, median = st.mean(times), st.median(times)
        sd = st.stdev(times) if len(times) > 1 else None
        row.update(repeticoes=len(times), tempo_medio_s=mean, tempo_mediano_s=median,
                   tempo_min_s=min(times), tempo_max_s=max(times),
                   desvio_padrao_s=sd, cv_percentual=100*sd/mean if sd is not None else None,
                   speedup=None, eficiencia=None, speedup_relativo_1=None, eficiencia_relativa_1=None)
        result.append(row)
    for row in result:
        common = lambda r: all(r[k] == row[k] for k in ('ambiente', 'experimento', 'N_base', 'N'))
        seq = [r for r in result if common(r) and r['implementacao'] == 'sequencial']
        if row['experimento'] == 'forte' and len(seq) == 1:
            row['speedup'] = seq[0]['tempo_mediano_s'] / row['tempo_mediano_s']
            row['eficiencia'] = row['speedup'] / row['trabalhadores']
        one = [r for r in result if common(r) and r['implementacao'] == row['implementacao']
               and r['schedule'] == row['schedule'] and r['trabalhadores'] == 1]
        if row['experimento'] == 'forte' and len(one) == 1:
            row['speedup_relativo_1'] = one[0]['tempo_mediano_s'] / row['tempo_mediano_s']
            row['eficiencia_relativa_1'] = row['speedup_relativo_1'] / row['trabalhadores']
    return result


def weak_metrics(metrics):
    result = []
    for row in metrics:
        if row['experimento'] != 'fraca' or row['implementacao'] == 'sequencial':
            continue
        refs = [r for r in metrics if all(r[k] == row[k] for k in
                ('ambiente', 'N_base', 'implementacao', 'schedule', 'experimento'))
                and r['trabalhadores'] == 1 and r['N'] == row['N_base']]
        ideal = row['N_base'] * row['trabalhadores']**(1/3)
        efficiency = refs[0]['tempo_mediano_s']/row['tempo_mediano_s'] if len(refs) == 1 else None
        result.append({**row, 'N_ideal': ideal, 'arredondamento': row['N']-ideal,
                       'trabalho_relativo': row['N']**3/(row['trabalhadores']*row['N_base']**3),
                       'eficiencia_fraca': efficiency,
                       'speedup_escalado': row['trabalhadores']*efficiency if efficiency is not None else None})
    return result


def write_csv(path, fields, rows):
    with path.open('w', encoding='utf-8', newline='') as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, extrasaction='ignore')
        writer.writeheader()
        writer.writerows(rows)


def plot(path, title, ylabel, series):
    """SVG independente: eixos lineares partindo de zero e legenda fora da area de dados."""
    colors = ['#1764ab', '#ce4c17', '#22824b', '#8e44ad', '#59636e', '#a07800', '#00838f', '#b52d65']
    points = [point for _, data, _ in series for point in data]
    if not points:
        return
    xmax, ymax = max(x for x, _ in points), max(y for _, y in points)*1.12
    ymax = ymax or 1
    width, height = 1350, max(530, 130+len(series)*25)
    x = lambda v: 90+v/max(xmax, 1)*610
    y = lambda v: 440-v/ymax*330
    parts = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}" role="img">',
             f'<title>{escape(title)}</title>', '<rect width="100%" height="100%" fill="white"/>',
             '<g font-family="sans-serif" font-size="14" fill="#17212b">',
             f'<text x="35" y="30" font-size="19">{escape(title)}</text>',
             f'<text x="35" y="65">{escape(ylabel)}</text>']
    for i in range(6):
        value = ymax*i/5
        parts += [f'<path d="M90 {y(value):.2f} H700" stroke="#dce2e8"/>',
                  f'<text x="80" y="{y(value)+5:.2f}" text-anchor="end">{value:.3g}</text>']
    for value in sorted({v for v, _ in points}):
        parts.append(f'<text x="{x(value):.2f}" y="465" text-anchor="middle">{value:g}</text>')
    parts += ['<path d="M90 110 V440 H700" stroke="#17212b" fill="none"/>',
              '<text x="390" y="500" text-anchor="middle">Trabalhadores (threads/processos)</text>']
    for i, (label, data, theoretical) in enumerate(series):
        color = colors[i % len(colors)]
        dash = ' stroke-dasharray="7 5"' if theoretical else ''
        coords = ' '.join(f'{x(a):.2f},{y(b):.2f}' for a, b in sorted(data))
        parts.append(f'<polyline points="{coords}" fill="none" stroke="{color}" stroke-width="2"{dash}/>')
        if not theoretical:
            for a, b in data:
                parts.append(f'<circle cx="{x(a):.2f}" cy="{y(b):.2f}" r="4" fill="{color}"/>')
        yy = 105+i*25
        parts += [f'<path d="M730 {yy} H760" stroke="{color}" stroke-width="2"{dash}/>',
                  f'<text x="770" y="{yy+5}">{escape(label)}</text>']
    parts.append('</g></svg>')
    path.write_text('\n'.join(parts), encoding='utf-8')


def graphics(metrics, weak, output):
    output.mkdir(parents=True, exist_ok=True)
    generated = []
    groups = defaultdict(list)
    for row in metrics:
        if row['experimento'] == 'forte':
            groups[(row['ambiente'], row['N'])].append(row)
    for (environment, n), rows in sorted(groups.items()):
        tag = hashlib.sha256(environment.encode()).hexdigest()[:8]
        for field, kind, label in [('tempo_mediano_s', 'tempo', 'Tempo mediano (s)'),
                                   ('speedup', 'speedup', 'Speedup Tseq/Tp'),
                                   ('eficiencia', 'eficiencia', 'Eficiencia (%)'),
                                   ('speedup', 'amdahl', 'Speedup Tseq/Tp')]:
            series = []
            for impl, schedule in sorted({(r['implementacao'], r['schedule']) for r in rows}):
                data = [(r['trabalhadores'], r[field]*(100 if field == 'eficiencia' else 1))
                        for r in rows if r['implementacao'] == impl and r['schedule'] == schedule and r[field] is not None]
                if data and impl != 'sequencial':
                    series.append((impl + (' / '+schedule if schedule else '') + ' (medido)', data, False))
            ps = sorted({r['trabalhadores'] for r in rows})
            if kind == 'tempo':
                seq = [r for r in rows if r['implementacao'] == 'sequencial']
                if len(seq) == 1:
                    series.append(('Sequencial medido', [(p, seq[0][field]) for p in ps], True))
            if not series:
                continue
            if kind == 'speedup':
                series.append(('Ideal teorico S=p', [(p, p) for p in ps], True))
            if kind == 'eficiencia':
                series.append(('Ideal teorico 100%', [(p, 100) for p in ps], True))
            if kind == 'amdahl':
                for f in (.05, .10, .20):
                    series.append((f'Amdahl hipotetico f={f:.2f}', [(p, 1/(f+(1-f)/p)) for p in ps], True))
            name = f'{kind}_{tag}_N{n}.svg'
            plot(output/name, f'{kind.capitalize()} | N={n} | {environment}', label, series)
            generated.append(name)
    wg = defaultdict(list)
    for row in weak:
        wg[(row['ambiente'], row['N_base'])].append(row)
    for (environment, base), rows in sorted(wg.items()):
        series = []
        for impl, schedule in sorted({(r['implementacao'], r['schedule']) for r in rows}):
            subset = [r for r in rows if r['implementacao'] == impl and r['schedule'] == schedule]
            dims = ', '.join(f'{r["trabalhadores"]}:{r["N"]}' for r in sorted(subset, key=lambda r:r['trabalhadores']))
            series.append((f'{impl} {schedule} p:N={dims}', [(r['trabalhadores'], r['tempo_mediano_s']) for r in subset], False))
        name = f'fraca_{hashlib.sha256(environment.encode()).hexdigest()[:8]}_base{base}.svg'
        plot(output/name, f'Escalabilidade fraca | N_base={base}', 'Tempo mediano (s)', series)
        generated.append(name)
    return generated


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input-dir', type=Path, default=ROOT/'resultados')
    parser.add_argument('--output-dir', type=Path, default=ROOT/'resultados')
    args = parser.parse_args(argv)
    rows, notes, sources = load(args.input_dir)
    if not rows:
        raise ValueError('Nenhuma observacao valida encontrada')
    metrics = summarize(rows)
    weak = weak_metrics(metrics)
    for row in metrics:
        if row['repeticoes'] < 5:
            notes.append('Grupo com menos de cinco repeticoes: '+str({k:row[k] for k in KEY}))
    if not weak:
        notes.append('Escalabilidade fraca: nenhuma coleta especifica disponivel; pendente.')
    for impl in ('openmp', 'mpi'):
        subset = [r for r in metrics if r['implementacao'] == impl]
        if not subset:
            notes.append(f'{impl}: sem medicoes disponiveis.')
        elif any(r['speedup'] is None for r in subset if r['experimento'] == 'forte'):
            notes.append(f'{impl}: grupos sem Tseq compativel; speedup e eficiencia principais vazios.')
    args.output_dir.mkdir(parents=True, exist_ok=True)
    write_csv(args.output_dir/'desempenho_consolidado.csv', RAW, rows)
    write_csv(args.output_dir/'metricas_desempenho.csv', STATS, metrics)
    write_csv(args.output_dir/'escalabilidade_fraca.csv', STATS + ['N_ideal', 'arredondamento', 'trabalho_relativo', 'eficiencia_fraca', 'speedup_escalado'], weak)
    graphs = graphics(metrics, weak, args.output_dir/'graficos')
    manifest = {'observacoes': len(rows), 'grupos': len(metrics), 'fontes': sources,
                'avisos': notes, 'graficos': graphs}
    (args.output_dir/'analise_manifesto.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(f'{len(rows)} observacoes, {len(metrics)} grupos, {len(graphs)} graficos SVG.')
    for note in notes:
        print('AVISO:', note)
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as error:
        raise SystemExit(f'Erro: {error}')
