"""Search a stated small dyadic family; retain the first distinguishing input."""
import json
import mpmath as mp
from oracle import Bridge, dy, factor, contained, val, LANE

mp.mp.dps = 600
rows = []
for name in ['half-B', 'lower-E', 'open-integer']:
    mutant, baseline = Bridge('bridge-' + name), Bridge()
    candidates = []
    if name == 'half-B':
        for e in [1, 2, 3, 4, 5]:
            for re in range(e + 1, e + 15):
                for prec in [2, 4, 8, 16, 32, 64, 128, 256]:
                    candidates.append({'id': f'min-half-{e}-{re}-{prec}', 'p': 0, 'prec': prec,
                                       's': [dy(-1, -e), dy(), dy(1, -re), dy()]})
    elif name == 'lower-E':
        candidates = [{'id': 'min-lower-E', 'p': 2, 'prec': 2,
                       's': [dy(1), dy(), dy(1), dy()]}]
    else:
        candidates = [{'id': 'min-open', 'p': 0, 'prec': 2,
                       's': [dy(-255, -1), dy(), dy(1, -1), dy()]}]
    found = None
    for i, c in enumerate(candidates):
        r = mutant.call(c)
        bad = []
        if name == 'half-B' and r['status'] == 0:
            x, y, rx, ry = map(val, r['s'])
            for sign in [-1, 1]:
                s = mp.mpc(x + sign * rx, y)
                v = factor(0, s)
                ok, comp, excess = contained(v, r['y'], 600)
                if not ok:
                    bad.append({'sign': sign, 'value': mp.nstr(v.real, 90), 'excess': excess})
        b = baseline.call(c) if bad or name != 'half-B' else None
        if bad or (b and b['status'] != r['status']):
            found = {'fault': name, 'input': c, 'mutant': r, 'baseline': b, 'bad': bad,
                     'candidates_tried': i + 1}
            break
    mutant.close(); baseline.close()
    assert found, name
    rows.append(found)
    (LANE / 'minimized.json').write_text(json.dumps(rows, indent=2) + '\n')
    print(json.dumps({'fault': name, 'input': found['input'], 'tried': found['candidates_tried'],
                      'bad_endpoints': len(found['bad'])}), flush=True)
