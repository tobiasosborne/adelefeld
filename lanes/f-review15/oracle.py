"""Independent point oracle and exact dyadic bridge. Never imports proto/zeta_checks.

Factor definitions: refs/src/tate-poonen/notes.txt:1014-1016,1733.
Gamma poles and recurrence: that file:58,62-64. Derivative formula below is obtained
by differentiating pi^(-s/2) Gamma(s/2), with the product and chain rules.
The numerical margin is 10^(-dps+80) times each component's scale, with floor
10^(-dps+80) times |L|. Borderline values are recalculated with 100 extra digits.
This is a stated numerical margin, not a proof of correct rounding by mpmath.
"""
from pathlib import Path
from fractions import Fraction
import argparse
import json
import math
import random
import subprocess
import time
import os
import mpmath as mp

LANE = Path(__file__).resolve().parent
PRIMES = [2, 3, 5, 7, 65537, 18446744073709551557]


def dy(n=0, e=0):
    return [str(n), str(e)]


def dy_mp(x, bits=2000):
    if not x:
        return dy()
    e = int(mp.floor(mp.log(abs(x), 2))) - bits + 1
    return dy(int(mp.nint(mp.ldexp(x, -e))), e)


def val(d):
    return mp.ldexp(mp.mpf(d[0]), int(d[1]))


def frac(d):
    n, e = map(int, d)
    return Fraction(n << e, 1) if e >= 0 else Fraction(n, 1 << -e)


class Bridge:
    def __init__(self, exe='bridge', modes=False):
        self.proc = subprocess.Popen(['timeout', '165', str(LANE / 'build' / exe)]
                                     + (['modes'] if modes else []), stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    def call(self, c):
        fields = [str(c['p']), str(c['prec'])]
        for d in c['s']:
            fields += d
        self.proc.stdin.write(' '.join(fields) + '\n')
        self.proc.stdin.flush()
        line = self.proc.stdout.readline()
        if not line:
            raise RuntimeError('bridge stopped: ' + self.proc.stderr.read())
        return json.loads(line)

    def close(self):
        self.proc.stdin.close()
        st = self.proc.wait(timeout=3)
        msg = self.proc.stderr.read().strip()
        assert st == 0, (st, msg)
        return msg


def factor(p, s):
    if p:
        if s.real < 0:
            t = mp.power(p, s)
            return -t / (1 - t)
        return 1 / (1 - mp.power(p, -s))
    return mp.power(mp.pi, -s / 2) * mp.gamma(s / 2)


def derivative(s):
    return factor(0, s) * (mp.digamma(s / 2) - mp.log(mp.pi)) / 2


def samples(s, rng, count=20):
    x, y, rx, ry = map(val, s)
    grid = [(-1, -1), (-1, 1), (1, -1), (1, 1), (-1, 0), (1, 0),
            (0, -1), (0, 1), (0, 0)]
    # Include exact dyadic interior points, avoiding binary64 conversion.
    grid += [(mp.mpf(rng.randrange(-2**20, 2**20 + 1)) / 2**20,
              mp.mpf(rng.randrange(-2**20, 2**20 + 1)) / 2**20)
             for _ in range(max(0, count - len(grid)))]
    return [mp.mpc(x + a * rx, y + b * ry) for a, b in grid[:count]]


def contained(point, out, dps):
    ym = list(map(val, out))
    v = [point.real, point.imag]
    for j in range(2):
        # Preserve tiny components: allowance scales with the component, then |L|.
        scale = max(abs(v[j]), abs(point) * mp.power(10, -dps + 80), mp.power(10, -dps))
        margin = mp.power(10, -dps + 80) * scale
        if abs(v[j] - ym[j]) > ym[j + 2] + margin:
            return False, j, mp.nstr(abs(v[j] - ym[j]) - ym[j + 2], 30)
    return True, None, None


def real_pole_member(s):
    x, y, rx, ry = map(frac, s)
    if abs(y) > ry or x - rx > 0:
        return False
    lo = (x - rx) / 2
    hi = min((x + rx) / 2, Fraction(0))
    return math.ceil(lo) <= math.floor(hi)


def random_cases(p, start, count):
    rng = random.Random(150000 + p + start // 250)
    ps = [2, 8, 16, 32, 64, 128, 256, 512, 1024]
    for k in range(start, start + count):
        # Cycle all exponent sizes rather than mostly choosing enormous points.
        if k % 3 == 0:
            ex = rng.randrange(-8, 7)
            ey = rng.randrange(-8, 7)
        else:
            ex = rng.randrange(-200, 201)
            ey = rng.randrange(-200, 201)
        x = dy(rng.choice([-1, 1]) * rng.randrange(1, 2**20), ex - 20)
        y = dy(rng.choice([-1, 1]) * rng.randrange(1, 2**20), ey - 20)
        r = []
        for _ in range(2):
            r.append(dy() if rng.randrange(8) == 0 else
                     dy(rng.randrange(1, 2**29), rng.randrange(-230, -28)))
        yield {'id': f'random-{p}-{k}', 'p': p,
               'prec': 4096 if k % 1000 == 0 else ps[k % len(ps)], 's': [x, y] + r}


def exact_cases():
    for p in PRIMES:
        for n in [-257, -65, -16, -3, -2, -1, 1, 2, 3, 16, 65, 257]:
            for prec in [2, 64, 512, 4096]:
                yield {'id': f'exact-{p}-{n}-{prec}', 'p': p, 'prec': prec,
                       's': [dy(n), dy(), dy(), dy()], 'integer': n}


def finite_pole_cases():
    mp.mp.dps = 720  # More than 2000 bits, including the phase for k near 2^1000.
    rng = random.Random(15002)
    for p in PRIMES:
        a = mp.log(p)
        ks = [0, 1, -1, 2, 999999, 1000000, -1000000, 2**100, 2**1000]
        ks += [rng.randrange(-1000000, 1000001) for _ in range(23)]
        for k in ks:
            pole = 2 * mp.pi * k / a
            pm = dy_mp(pole, 2200) if k else dy(0, -2200)
            # A few ulps off the rounded 2200-bit midpoint.
            ulp = int(pm[1]) if k else -2200
            pm[0] = str(int(pm[0]) + 5)
            dist = abs(val(pm) - pole)
            rad = dy_mp(dist, 29)
            for delta, expected in [(2, 'pole'), (-2, 'regular')]:
                rd = dy(max(1, int(rad[0]) + delta), int(rad[1]))
                for thin in [True, False]:
                    yield {'id': f'prime-touch-{p}-{k}-{delta}-{thin}', 'p': p, 'prec': 4096,
                           's': [dy(), pm, dy() if thin else rd, rd], 'geometry': expected,
                           'pole': mp.nstr(pole, 710), 'pole_index': str(k)}
            # Off the imaginary axis, with the real radius reaching zero exactly.
            yield {'id': f'prime-reach-{p}-{k}', 'p': p, 'prec': 256,
                   's': [dy(1, -80), pm, dy(1, -80), dy(1, -70)], 'geometry': 'pole',
                   'pole_index': str(k)}
        for r in [2**15, 2**80, 2**1000]:
            yield {'id': f'prime-wide-{p}-{r.bit_length()}', 'p': p, 'prec': 64,
                   's': [dy(1, -20), dy(), dy(1), dy(r)], 'geometry': 'pole', 'pole_index': '0'}


def real_pole_cases():
    rng = random.Random(15003)
    ns = [0, 1, 2, 63, 64, 65, 100, 999999, 1000000, 2**200]
    ns += [rng.randrange(1000001) for _ in range(160)]
    for n in ns:
        for mode in range(9):
            s = [dy(-2 * n), dy(), dy(), dy()]
            expected = 7
            if mode == 1:
                s[2] = dy(1, -200); expected = 1
            elif mode == 2:
                s[3] = dy(1, -200); expected = 1
            elif mode == 3:
                s[0] = dy(-2 * n - 1); s[2] = dy(1); expected = 1
            elif mode == 4:
                s[0] = dy(-2 * n + 1); s[2] = dy(1); expected = 1
            elif mode == 5:
                s[0] = dy(-2 * n - 1); s[2] = dy(2**29 - 1, -29); expected = None
            elif mode == 6:
                s[1] = dy(2**29 + 1, -229); s[3] = dy(1, -200); expected = None
            elif mode == 7:
                s[0] = dy(-2 * n * 2**5000 + 1, -5000); expected = None
            elif mode == 8:
                s[0] = dy(-2 * n * 2**200 + 1, -200); s[2] = dy(1, -200); expected = 1
            yield {'id': f'real-pole-{n}-{mode}', 'p': 0,
                   'prec': 4096 if mode == 7 else 256, 's': s, 'expected': expected}


def near_cases(p, start, count):
    mp.mp.dps = 720
    rng = random.Random(15004 + p)
    for k in range(start + count):
        if p:
            pole = mp.mpc(0, 2 * mp.pi * rng.randrange(-1000000, 1000001) / mp.log(p))
        else:
            pole = mp.mpc(-2 * rng.randrange(64), 0)
        e = rng.randrange(-500, -2)
        d = mp.ldexp(mp.mpf(1), e)
        axis = k % 4
        m = pole + [d, -d, 1j * d, -1j * d][axis]
        div = [2, 16, 1000][k % 3]
        rad = dy_mp(d / div, 29)
        if k >= start:
            yield {'id': f'near-{p}-{k}', 'p': p, 'prec': [64, 256, 1024, 4096][k % 4],
                   's': [dy_mp(m.real, 2200), dy_mp(m.imag, 2200), rad, rad]}


def bound_cases(start, count):
    rng = random.Random(15005)
    for k in range(start + count):
        x = rng.randrange(-126 * 2**20, 128 * 2**20) / 2**20
        y = rng.choice([-1, 1]) * 2**rng.randrange(-20, 9) * rng.randrange(1, 2**20) / 2**20
        rx = rng.randrange(1, 2**29) * 2**rng.randrange(-60, -26)
        ry = rng.randrange(1, 2**29) * 2**rng.randrange(-60, -26)
        if k >= start:
            yield {'id': f'bound-{k}', 'p': 0, 'prec': 128,
                   's': [dy_mp(mp.mpf(x)), dy_mp(mp.mpf(y)), dy_mp(mp.mpf(rx), 29),
                         dy_mp(mp.mpf(ry), 29)]}


def run(args):
    start_time = time.monotonic()
    rng = random.Random(15100 + args.start)
    bridge = Bridge(args.exe, args.kind in ['exact', 'finite-poles', 'real-poles', 'statuses'])
    choices = {'exact': exact_cases, 'finite-poles': finite_pole_cases, 'real-poles': real_pole_cases}
    if args.kind in choices:
        cases = choices[args.kind]()
    elif args.kind == 'random':
        cases = random_cases(args.p, args.start, args.count)
    elif args.kind == 'near':
        cases = near_cases(args.p, args.start, args.count)
    else:
        cases = bound_cases(args.start, args.count)
    counters = dict(cases=0, ok=0, nd=0, domain=0, limit=0, samples=0, failures=0,
                    bounds=0, derivative_samples=0, fallback=0, exact_rationals=0)
    findings = []
    max_ratio = mp.mpf(0)
    dest = LANE / ('runs/' + (args.tag or f'{args.kind}-{args.p}-{args.start}') + '.json')
    dest.parent.mkdir(exist_ok=True)
    for c in cases:
        r = bridge.call(c)
        counters['cases'] += 1
        counters[{0: 'ok', 1: 'nd', 7: 'domain', 10: 'limit'}.get(r['status'], 'failures')] += 1
        counters['fallback'] += r['fallback'] > 0
        why = None
        if not r['repr'] or not r['where']:
            why = 'output representation or where'
        if c.get('expected') is not None and r['status'] != c['expected']:
            why = f"status expected {c['expected']} got {r['status']}"
        if c.get('geometry') == 'pole' and r['status'] == 0:
            why = 'OK ball contains finite pole'
        if c['p'] == 0 and real_pole_member(c['s']) and r['status'] == 0:
            why = 'OK ball contains real pole'
        if r['status'] == 0:
            dps = max(480, math.ceil((c['prec'] + 256) * math.log10(2)) + 100)
            if args.kind == 'real-poles':
                dps = max(dps, 1700)
            mp.mp.dps = dps
            points = samples(r['s'], rng, 81 if args.kind == 'bounds' else 20)
            if args.kind == 'bounds':
                x, y, rx, ry = map(val, r['s'])
                points = [mp.mpc(x + rx * (mp.mpf(a) / 4 - 1), y + ry * (mp.mpf(b) / 4 - 1))
                          for a in range(9) for b in range(9)]
            if 'integer' in c:
                n = c['integer']
                q = Fraction(c['p']**n, c['p']**n - 1) if n > 0 else Fraction(1, 1 - c['p']**(-n))
                ym, _, yr, _ = map(frac, r['y'])
                if not ym - yr <= q <= ym + yr:
                    why = 'exact rational outside result'
                counters['exact_rationals'] += 1
                points = points[:1]
            for point in points:
                value = factor(c['p'], point)
                counters['samples'] += 1
                inside, component, excess = contained(value, r['y'], dps)
                if not inside:
                    with mp.workdps(dps + 100):
                        value2 = factor(c['p'], point)
                        inside2, _, _ = contained(value2, r['y'], dps + 100)
                    if not inside2:
                        why = f'component {component} outside by {excess}'
                if r['B'] is not None and (args.kind == 'bounds' or args.derivatives):
                    d = abs(value * (mp.digamma(point / 2) - mp.log(mp.pi)) / 2)
                    b = val(r['B'])
                    counters['derivative_samples'] += 1
                    ratio = d / b
                    max_ratio = max(max_ratio, ratio)
                    if ratio > 1 + mp.mpf('1e-350'):
                        why = 'derivative exceeds B: ratio=' + mp.nstr(ratio, 35)
            if r['B'] is not None:
                counters['bounds'] += 1
        if why:
            counters['failures'] += 1
            findings.append({'why': why, 'input': c, 'returned': r})
            if len(findings) >= 8:
                break
        if counters['cases'] % 10 == 0:
            dest.with_suffix('.partial.json').write_text(json.dumps({**counters, 'last_id': c['id'],
                'seconds': round(time.monotonic() - start_time, 3), 'findings': findings}, indent=2) + '\n')
    stderr = bridge.close()
    result = {'kind': args.kind, 'p': args.p, 'start': args.start, 'count_requested': args.count,
              **counters, 'max_derivative_ratio': mp.nstr(max_ratio, 30),
              'seconds': round(time.monotonic() - start_time, 3), 'bridge': stderr, 'findings': findings}
    dest.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: v for k, v in result.items() if k != 'findings'}), flush=True)
    if findings:
        print(json.dumps(findings[0]), flush=True)
    return bool(findings)


if __name__ == '__main__':
    # All later worker processes share the same two allowed CPUs.
    os.sched_setaffinity(0, sorted(os.sched_getaffinity(0))[:2])
    parser = argparse.ArgumentParser()
    parser.add_argument('kind', choices=['exact', 'finite-poles', 'real-poles', 'random', 'near', 'bounds'])
    parser.add_argument('--p', type=int, default=0)
    parser.add_argument('--start', type=int, default=0)
    parser.add_argument('--count', type=int, default=250)
    parser.add_argument('--exe', default='bridge')
    parser.add_argument('--tag')
    parser.add_argument('--derivatives', action='store_true')
    raise SystemExit(run(parser.parse_args()))
