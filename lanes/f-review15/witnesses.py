from pathlib import Path
import json
import subprocess
import mpmath as mp
from oracle import Bridge, dy, samples, factor, contained, derivative, val
import random

lane = Path(__file__).resolve().parent
mp.mp.dps = 600
faults = ['half-B', 'lower-E', 'open-integer', 'no-logpi-B', 'no-M0-B']
rows = []
for name in faults:
    src = (lane / f'build/mutant-{name}.c').read_text()
    src = src.replace('if (!acb_is_finite(g))\n    {',
                      'if (!acb_is_finite(g))\n    {\n        review_fallback++;')
    src = src.replace('    arf_clear(t);\n    arb_clear(c);',
                      '    mag_set(review_B, B); review_has_B = 1;\n    arf_clear(t);\n    arb_clear(c);')
    path = lane / f'build/trace-{name}.c'
    path.write_text(src)
    cmd = ['timeout', '30', 'cc', '-std=c11', '-O2', '-g', '-Iinclude', '-Isrc', '-DREVIEW_TRACE',
           f'-DREVIEW_SOURCE="build/trace-{name}.c"', str(lane / 'bridge.c'),
           str(lane / 'build/libadelefeld.a'), '-lflint', '-lgmp', '-lm', '-o',
           str(lane / f'build/bridge-{name}')]
    subprocess.run(cmd, check=True)
    baseline = Bridge()
    mutant = Bridge(f'bridge-{name}')
    cases = []
    if name == 'lower-E':
        cases = [{'id': 'zero-at-closed-left', 'p': 2, 'prec': 128,
                  's': [dy(1), dy(), dy(1), dy()]}]
    elif name == 'open-integer':
        cases = [{'id': 'far-left-closed-left', 'p': 0, 'prec': 64,
                  's': [dy(-399, -1), dy(), dy(1, -1), dy()]}]
    else:
        for e in [2, 4, 8, 10, 20, 40]:
            for re in [e + 10, e + 20, e + 40]:
                cases.append({'id': f'thin-minus-near-zero-{e}-{re}', 'p': 0, 'prec': 256,
                              's': [dy(-1, -e), dy(), dy(1, -re), dy()]})
    found = None
    for c in cases:
        b = baseline.call(c); r = mutant.call(c)
        bad_samples = []
        if r['status'] == 0 and name != 'lower-E':
            for pt in samples(r['s'], random.Random(1), 20):
                value = factor(c['p'], pt)
                ok, component, excess = contained(value, r['y'], 600)
                if not ok:
                    bad_samples.append({'s': [mp.nstr(pt.real, 90), mp.nstr(pt.imag, 90)],
                                        'value': [mp.nstr(value.real, 90), mp.nstr(value.imag, 90)],
                                        'component': component, 'excess': excess})
        ratio = None
        if r['B']:
            ratio = max(abs(derivative(pt)) / val(r['B']) for pt in samples(r['s'], random.Random(1), 20))
        row = {'fault': name, 'input': c, 'baseline': b, 'mutant': r,
               'bad_samples': bad_samples, 'derivative_ratio': mp.nstr(ratio, 60) if ratio else None}
        rows.append(row)
        if bad_samples or b['status'] != r['status']:
            found = row; break
    baseline.close(); mutant.close()
    (lane / f'mutants/{name}-witness.json').write_text(json.dumps(rows[-1], indent=2) + '\n')
    print(json.dumps({'fault': name, 'distinguished': found is not None,
                      'cases': len([r for r in rows if r['fault'] == name]),
                      'input': found['input'] if found else None,
                      'bad_samples': len(found['bad_samples']) if found else 0,
                      'derivative_ratio': rows[-1]['derivative_ratio']}), flush=True)
(lane / 'witnesses.json').write_text(json.dumps(rows, indent=2) + '\n')
