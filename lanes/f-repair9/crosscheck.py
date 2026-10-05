"""Numerical cross-check of twenty selected certified cells at 400 decimal digits."""
import json
import sys
from pathlib import Path

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from fixtures import ROOT, dyad, points
import mpmath as mp

mp.mp.dps = 400
rows = []
for line in (ROOT / 'tests/ref/vectors/f-repair9/near-poles.jsonl').read_text().splitlines():
    r = json.loads(line)
    if 's' in r:
        rows.append(dict(s=r['s'], v=[], n=r['n']))
    else:
        rows[-1]['v'].append(r['v'])
assert all(len(r['v']) == r['n'] for r in rows)
selected = [(0, 2), (1, 2), (2, 0), (3, 4), (28, 2), (29, 0), (114, 0), (115, 3),
            (338, 2), (339, 0), (450, 0), (451, 4), (674, 2), (675, 0), (898, 0), (899, 4),
            (1010, 0), (1011, 4), (1344, 2), (1345, 0)]
records = []
for i, j in selected:
    pt = points(rows[i]['s'])[j]
    z = mp.mpc(*(mp.mpf(q.numerator)/q.denominator for q in pt))
    value = mp.exp(-z*mp.log(mp.pi)/2)*mp.gamma(z/2)
    for part, cell in zip((value.real, value.imag), rows[i]['v'][j]):
        if cell is None:
            assert part == 0
        else:
            lo, hi = dyad(cell[0], cell[1]), dyad(cell[0]+1, cell[1])
            assert mp.mpf(lo.numerator)/lo.denominator < part < mp.mpf(hi.numerator)/hi.denominator
    records.append(dict(row=i, sample=j, s=list(map(str, pt)), value=[mp.nstr(value.real, 80),
                                                                  mp.nstr(value.imag, 80)]))
out = dict(digits=400, samples=len(records), failures=0, selected=records)
(Path(__file__).resolve().parent / 'crosscheck.json').write_text(json.dumps(out, indent=2)+'\n')
print('400-digit mpmath cross-check: samples', len(records), 'failures', 0)
