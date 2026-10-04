from pathlib import Path
import json
import os
from oracle import Bridge, dy

lane = Path(__file__).resolve().parent
bridge = Bridge(os.environ.get('REVIEW_BRIDGE', 'bridge'), modes=True)
cases = []
for p in [2, 0]:
    for part in [0, 1]:
        for special in ['nan', 'inf', 'ninf']:
            for prec, expected in [(64, 7), (2**21 + 1, 10)]:
                s = [dy(1), dy(), dy(), dy()]
                s[part] = [special, '0']
                cases.append({'p': p, 'prec': prec, 's': s, 'expected': expected})
    for part in [2, 3]:
        for prec, expected in [(64, 7), (2**21 + 1, 10)]:
            s = [dy(1), dy(), dy(), dy()]; s[part] = ['inf', '0']
            cases.append({'p': p, 'prec': prec, 's': s, 'expected': expected})
    for prec in [-2**63, -5, 0, 1, 2]:
        cases.append({'p': p, 'prec': prec, 's': [dy(1), dy(), dy(), dy()], 'expected': 0})
    cases.append({'p': p, 'prec': 2**63 - 1, 's': [dy(), dy(), dy(), dy()], 'expected': 10})
cases += [
    {'p': 2, 'prec': 64, 's': [dy(), dy(), dy(), dy()], 'expected': 7},
    {'p': 0, 'prec': 64, 's': [dy(-200), dy(), dy(), dy()], 'expected': 7},
    {'p': 0, 'prec': 64, 's': [dy(-200), dy(), dy(1, -20), dy()], 'expected': 1},
    {'p': 0, 'prec': 64, 's': [dy(-130), dy(1, -2), dy(1), dy(1, -4)], 'expected': 10},
]
rows = []
fail = 0
for c in cases:
    r = bridge.call(c)
    ok = r['status'] == c['expected'] and r['repr'] and r['where']
    rows.append({'input': c, 'returned': r, 'passes': bool(ok)})
    fail += not ok
(lane / 'statuses.json').write_text(json.dumps(rows, indent=2) + '\n')
print(json.dumps({'cases': len(rows), 'failures': fail, 'bridge': bridge.close()}))
raise SystemExit(bool(fail))
