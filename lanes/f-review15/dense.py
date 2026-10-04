"""Dense independent checks near poles, far left, and on wide rectangles."""
import json
import time
import mpmath as mp
from oracle import Bridge, dy, val, factor, contained, LANE

mp.mp.dps = 420
bridge = Bridge()
cases = []
for n in [0, 1, 8, 31, 62]:
    for e in [4, 20, 100]:
        cases.append({'id': f'left-{n}-{e}', 'p': 0, 'prec': 256,
                      's': [dy(-2*n*2**e - 1, -e), dy(), dy(1, -e-4), dy()]})
    e = 20
    cases.append({'id': f'imag-{n}', 'p': 0, 'prec': 256,
                  's': [dy(-2*n), dy(1, -e), dy(1, -e-4), dy(1, -e-4)]})
cases += [
    {'id': 'wide-left', 'p': 0, 'prec': 256,
     's': [dy(-247, -1), dy(3, -17), dy(3, -1), dy(1, -17)]},
    {'id': 'wide-positive', 'p': 0, 'prec': 256,
     's': [dy(127, -1), dy(1), dy(125, -1), dy(1, -1)]},
    {'id': 'wide-between-poles', 'p': 0, 'prec': 256,
     's': [dy(-3), dy(1, -20), dy(2**29-1, -29), dy(1, -22)]},
    {'id': 'recurrence-design', 'p': 0, 'prec': 256,
     's': [dy(-2), dy(13, -3), dy(1, -3), dy(1, -3)]},
]
start = time.monotonic()
rows = []
for c in cases:
    r = bridge.call(c)
    row = {'input': c, 'returned': r, 'samples': 0, 'bound_samples': 0, 'failures': []}
    if r['status'] == 0:
        x, y, rx, ry = map(val, r['s'])
        maximum = mp.mpf(0)
        for a in range(9):
            for b in range(9):
                s = mp.mpc(x + rx * (mp.mpf(a)/4-1), y + ry * (mp.mpf(b)/4-1))
                v = factor(0, s)
                row['samples'] += 1
                inside, comp, excess = contained(v, r['y'], 420)
                if not inside:
                    row['failures'].append({'kind': 'containment', 'a': a, 'b': b,
                                            'component': comp, 'excess': excess})
                d = abs(v * (mp.digamma(s/2) - mp.log(mp.pi)) / 2)
                maximum = max(maximum, d)
                if r['B']:
                    row['bound_samples'] += 1
                    if d > val(r['B']) * (1 + mp.mpf('1e-330')):
                        row['failures'].append({'kind': 'B', 'a': a, 'b': b})
        row['sampled_sup'] = mp.nstr(maximum, 50)
        row['ratio'] = mp.nstr(maximum / val(r['B']), 50) if r['B'] else None
    rows.append(row)
    (LANE / 'dense.json').write_text(json.dumps(rows, indent=2) + '\n')
summary = {'cases': len(rows), 'ok': sum(r['returned']['status'] == 0 for r in rows),
           'samples': sum(r['samples'] for r in rows),
           'bound_samples': sum(r['bound_samples'] for r in rows),
           'failures': sum(len(r['failures']) for r in rows),
           'seconds': round(time.monotonic()-start, 3), 'bridge': bridge.close()}
(LANE / 'dense-summary.json').write_text(json.dumps(summary, indent=2) + '\n')
print(json.dumps(summary))
raise SystemExit(bool(summary['failures']))
