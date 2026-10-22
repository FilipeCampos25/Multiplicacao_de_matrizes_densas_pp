import csv
import importlib.util
from pathlib import Path
import tempfile
import unittest
import xml.etree.ElementTree as ET

SPEC = importlib.util.spec_from_file_location('analysis', Path(__file__).resolve().parents[1]/'scripts/analisar_desempenho.py')
a = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(a)


def row(impl='openmp', n=256, p=1, t=2., rep=1, env='coleta', exp='forte', base=0):
    return dict(implementacao=impl, N=n, trabalhadores=p, schedule='static' if impl == 'openmp' else '',
                repeticao=rep, tempo_s=t, checksum=10., ambiente=env, experimento=exp,
                N_base=base, origem=env, linha=rep+1, metadados='coleta_ambiente.txt')


class AnalysisTests(unittest.TestCase):
    def test_statistics_and_reference(self):
        rows = [row('sequencial', t=t, rep=i+1) for i,t in enumerate([8.,10.,12.])]
        rows += [row(p=2, t=t, rep=i+1) for i,t in enumerate([2.,4.,6.])]
        omp = next(r for r in a.summarize(rows) if r['implementacao'] == 'openmp')
        self.assertEqual((omp['tempo_medio_s'],omp['tempo_mediano_s'],omp['desvio_padrao_s']), (4.,4.,2.))
        self.assertEqual((omp['speedup'],omp['eficiencia']), (2.5,1.25))
        self.assertEqual(omp['cv_percentual'],50.)

    def test_environment_and_relative_reference(self):
        metrics = a.summarize([row('sequencial',t=100.,env='outro'),row(t=10.),row(p=2,t=4.)])
        omp = next(r for r in metrics if r['trabalhadores'] == 2)
        self.assertIsNone(omp['speedup'])
        self.assertEqual(omp['speedup_relativo_1'],2.5)
        self.assertIsNone(omp['desvio_padrao_s'])

    def test_weak_is_separate(self):
        metrics = a.summarize([row(n=256,p=1,t=2.,exp='fraca',base=256),
                               row(n=512,p=8,t=4.,exp='fraca',base=256)])
        weak = a.weak_metrics(metrics)
        eight = next(r for r in weak if r['trabalhadores'] == 8)
        self.assertEqual(eight['eficiencia_fraca'],.5)
        self.assertEqual(eight['speedup_escalado'],4.)
        self.assertIsNone(eight['speedup'])
        self.assertEqual(eight['trabalho_relativo'],1.)
        self.assertEqual(a.weak_metrics(a.summarize([row()])),[])

    def load_fixture(self, rows):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        path = Path(directory.name)/'sample.csv'
        with path.open('w',newline='',encoding='utf-8') as stream:
            writer = csv.DictWriter(stream,fieldnames=['implementacao','N','trabalhadores','repeticao','tempo_s','checksum'])
            writer.writeheader()
            writer.writerows(rows)
        return Path(directory.name)

    def test_bad_numeric_and_duplicate(self):
        base = dict(implementacao='mpi', N=256, trabalhadores=2, repeticao=1, tempo_s=1., checksum=10.)
        for invalid in ('NaN','inf','0','-1'):
            with self.subTest(invalid=invalid):
                directory = self.load_fixture([base|{'tempo_s':invalid}])
                with self.assertRaises(ValueError):
                    a.load(directory)
        with self.assertRaisesRegex(ValueError,'duplicada'):
            a.load(self.load_fixture([base,base]))

    def test_checksum_mismatch(self):
        base = dict(implementacao='mpi',N=256,trabalhadores=2,repeticao=1,tempo_s=1.,checksum=10.)
        with self.assertRaisesRegex(ValueError,'checksum'):
            a.load(self.load_fixture([base,base|{'repeticao':2,'checksum':20.}]))

    def test_summary_not_individual_and_copy_not_duplicated(self):
        base = dict(implementacao='mpi',N=256,trabalhadores=2,repeticao=1,tempo_s=1.,checksum=10.)
        directory = self.load_fixture([base])
        (directory/'copy.csv').write_bytes((directory/'sample.csv').read_bytes())
        (directory/'resumo.csv').write_text('N,tempo_mediano_s\n256,1.0\n')
        rows, notes, _ = a.load(directory)
        self.assertEqual(len(rows),1)
        self.assertTrue(any('copia identica' in s for s in notes))
        self.assertTrue(any('observacoes individuais' in s for s in notes))

    def test_svg_and_no_unfounded_speedup(self):
        with tempfile.TemporaryDirectory() as directory:
            names = a.graphics(a.summarize([row(),row(p=2,t=1.)]), [], Path(directory))
            self.assertEqual(len(names),1)
            self.assertTrue(names[0].startswith('tempo_'))
            ET.parse(Path(directory)/names[0])
            a.plot(Path(directory)/'amdahl.svg','Amdahl','S',[
                ('Amdahl hipotetico f=0.1',[(1,1.),(2,1/(.1+.9/2))],True)])
            ET.parse(Path(directory)/'amdahl.svg')


if __name__ == '__main__':
    unittest.main()

class RepositoryArtifactsTests(unittest.TestCase):
    """Auditoria independente dos artefatos reais, sem executar benchmarks."""
    def test_real_statistics_provenance_and_graphs(self):
        import hashlib
        import json
        import math
        import re
        import statistics
        from collections import defaultdict
        root = Path(__file__).resolve().parents[1]
        directory = root/'resultados'
        if not (directory/'analise_manifesto.json').exists():
            self.skipTest('Execute make analyze-performance para auditar os artefatos')
        def read(name):
            with (directory/name).open(encoding='utf-8-sig',newline='') as stream:
                return list(csv.DictReader(stream))
        manifest = json.loads((directory/'analise_manifesto.json').read_text(encoding='utf-8'))
        raw, metrics = read('desempenho_consolidado.csv'), read('metricas_desempenho.csv')
        self.assertEqual(manifest['observacoes'],len(raw))
        groups = defaultdict(list)
        for r in raw:
            groups[tuple(r[k] for k in a.KEY)].append(r)
            original = json.loads(r['dados_originais'])
            self.assertEqual(float(original['tempo_s']), float(r['tempo_s']))
            self.assertTrue((directory/r['origem']).is_file())
        for source in manifest['fontes']:
            self.assertEqual(source['sha256'], hashlib.sha256((directory/source['arquivo']).read_bytes()).hexdigest())
        for m in metrics:
            times = [float(r['tempo_s']) for r in groups[tuple(m[k] for k in a.KEY)]]
            expected = dict(tempo_medio_s=statistics.mean(times),tempo_mediano_s=statistics.median(times),
                            tempo_min_s=min(times),tempo_max_s=max(times))
            if len(times) > 1:
                expected.update(desvio_padrao_s=statistics.stdev(times),
                                cv_percentual=100*statistics.stdev(times)/statistics.mean(times))
            for field,value in expected.items():
                self.assertTrue(math.isclose(float(m[field]),value,rel_tol=1e-12))
            if m['implementacao'] != 'sequencial' and m['speedup']:
                seq = next(r for r in metrics if r['implementacao']=='sequencial' and
                    all(r[k]==m[k] for k in ('ambiente','N','experimento','N_base')))
                self.assertTrue(math.isclose(float(m['speedup']),float(seq['tempo_mediano_s'])/float(m['tempo_mediano_s']),rel_tol=1e-12))
                self.assertTrue(math.isclose(float(m['eficiencia']),float(m['speedup'])/int(m['trabalhadores']),rel_tol=1e-12))
        weak = read('escalabilidade_fraca.csv')
        for w in weak:
            self.assertFalse(w['speedup'])
            ref = next(r for r in weak if r['trabalhadores']=='1' and all(r[k]==w[k] for k in ('ambiente','implementacao','schedule','N_base')))
            self.assertTrue(math.isclose(float(w['eficiencia_fraca']),float(ref['tempo_mediano_s'])/float(w['tempo_mediano_s']),rel_tol=1e-12))
        for name in manifest['graficos']:
            svg = directory/'graficos'/name
            ET.parse(svg)
            if name.startswith('amdahl_'):
                self.assertIn('hipotetico',svg.read_text())
        document = root/'docs/analise_desempenho.md'
        for target in re.findall(r'\]\(([^)]+)\)',document.read_text(encoding='utf-8-sig')):
            self.assertTrue((document.parent/target).exists(),target)
        # Confere as celulas de tempo documentadas com o CSV, respeitando a procedencia.
        text = document.read_text(encoding='utf-8-sig')
        for m in metrics:
            if m['ambiente']=='issue10_desempenho.csv':
                formatted = f"{float(m['tempo_mediano_s']):.6f}".replace('.',',')
                self.assertIn(formatted,text)
