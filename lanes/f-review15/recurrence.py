import json
import random
import mpmath as mp
from oracle import Bridge, dy, val, factor, contained, samples, LANE

bridge = Bridge(modes=True)
rows = []
for n in range(1, 66):
    for prec in [16, 64, 256] + ([4096] if n in [2, 4, 8, 32, 64, 65] else []):
        mp.mp.dps = max(480, int(prec * .30103) + 200)
        c = {'id': f'recurrence-{n}-{prec}', 'p': 0, 'prec': prec,
             's': [dy(4-2*n), dy(1, -1), dy(1), dy(3, -3)]}
        r = bridge.call(c)
        row = {'input': c, 'returned': r, 'samples': 0, 'failures': []}
        if n <= 64 and r['status'] == 10 or n == 65 and r['status'] != 10:
            row['failures'].append('wrong recurrence limit')
        if r['status'] == 0:
            for point in samples(r['s'], random.Random(n), 20):
                v = factor(0, point)
                row['samples'] += 1
                ok, comp, excess = contained(v, r['y'], mp.mp.dps)
                if not ok:
                    row['failures'].append({'component': comp, 'excess': excess})
        rows.append(row)
        (LANE / 'recurrence.json').write_text(json.dumps(rows, indent=2) + '\n')
result = {'cases': len(rows), 'ok': sum(r['returned']['status'] == 0 for r in rows),
          'fallback_calls': sum(r['returned']['fallback'] > 0 for r in rows),
          'samples': sum(r['samples'] for r in rows),
          'failures': sum(len(r['failures']) for r in rows), 'bridge': bridge.close()}
(LANE / 'recurrence-summary.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result))
raise SystemExit(bool(result['failures']))
