#!/usr/bin/env python3
"""Independent slice c vectors; exact phases from proto/char_checks.py.

Point values use exp(s log(t)), not the C evaluator. Intervals are certified by
python-flint at 512 bits and rounded out to 60 decimal places. Family rows give
corner/center witnesses, whole-family enclosures per phase, and certified hull
extrema from the finite-candidate proof in family_hull.py and api-3d Slice c.
"""
import json
import sys
from fractions import Fraction as F
from itertools import product
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'proto'))
import char_checks as ref
import flint
from family_hull import certified_hull

OUT = ROOT / 'tests/ref/vectors/c-slice3'
OUT.mkdir(parents=True, exist_ok=True)
flint.ctx.prec = 512
flint.ctx.threads = 1


def ball(mid, rad=F(0)):
    mid, rad = F(mid), F(rad)
    return flint.arb(flint.fmpq(mid.numerator, mid.denominator),
                     flint.fmpq(rad.numerator, rad.denominator))


def interval(x):
    lo, hi = ref.arb_interval(x)
    scale = 10**60
    a = lo.numerator * scale // lo.denominator
    b = -((-hi.numerator * scale) // hi.denominator)
    return [str(a), str(b)]  # Each endpoint is this integer times 10^-60.


def rectangle(z):
    return interval(z.real) + interval(z.imag)


def phase(t):
    if t.denominator in (1, 2, 4):
        return (flint.acb(1), flint.acb(0, 1), flint.acb(-1), flint.acb(0, -1))[int(t*4)]
    return ref.phase_acb(t)


params = [[F(2), F(0), F(0), F(0), F(0), F(0)],
          [F(3, 2), F(0), F(1), F(0), F(0), F(0)],
          [F(1, 3), F(0), F(-1, 2), F(0), F(0), F(0)],
          [F(5, 4), F(0), F(1, 2), F(0), F(14), F(0)],
          [F(2), F(0), F(2), F(0), F(3), F(0)],
          [F(2), F(1, 4), F(1, 2), F(1, 16), F(14), F(1, 32)],
          [F(1), F(1, 4), F(2), F(1, 16), F(3), F(1, 32)],
          [F(2), F(1, 4), F(-1, 2), F(1, 16), F(0), F(0)],
          [F(1), F(0), F(2), F(1, 16), F(3), F(1, 32)]]
values, value_ids, images, image_ids = [], {}, [], {}


def value(t, a, b, theta):
    key = (t, a, b, theta)
    if key not in value_ids:
        z = (flint.acb(ball(a), ball(b)) * ball(t).log()).exp() * phase(theta)
        # Exact zero exponent / t=1, and small integer powers at cardinal phases.
        if (a == 0 and b == 0) or t == 1:
            z = phase(theta)
        elif b == 0 and a.denominator == 1:
            z = flint.acb(ball(t ** int(a))) * phase(theta)
        value_ids[key] = len(values)
        values.append(rectangle(z))
    return value_ids[key]


def image(k, phases):
    phases = tuple(sorted(phases))
    key = (k, phases)
    if key in image_ids:
        return image_ids[key]
    t, tr, a, ar, b, br = params[k]
    ids = sorted(set(value(tv, av, bv, theta)
                     for tv, av, bv, theta in product(sorted({t-tr, t, t+tr}),
                                                       sorted({a-ar, a, a+ar}),
                                                       sorted({b-br, b, b+br}), phases)))
    endpoints = [[int(values[j][i]) for j in ids] for i in range(4)]
    extrema = [[min(endpoints[0]), min(endpoints[1])], [max(endpoints[0]), max(endpoints[1])],
               [min(endpoints[2]), min(endpoints[3])], [max(endpoints[2]), max(endpoints[3])]]
    family = tr != 0 or ar != 0 or br != 0
    if family:
        extrema = certified_hull(params[k], phases)
    extrema = [list(map(str, interval)) for interval in extrema]
    hull = [extrema[0][0], extrema[1][1], extrema[2][0], extrema[3][1]]
    roots = [phase(theta) for theta in phases]
    re = [ref.arb_interval(z.real) for z in roots]
    im = [ref.arb_interval(z.imag) for z in roots]
    def hullball(xs):
        lo, hi = min(x[0] for x in xs), max(x[1] for x in xs)
        return ball((lo+hi)/2, (hi-lo)/2)
    unit = flint.acb(hullball(re), hullball(im))
    power = (flint.acb(ball(a, ar), ball(b, br)) * ball(t, tr).log()).exp()
    if (a == 0 and ar == 0 and b == 0 and br == 0) or (t == 1 and tr == 0):
        power = flint.acb(1)
    elif b == 0 and br == 0:
        power = flint.acb(ball(t, tr) ** ball(a, ar))
    box = rectangle(power * unit)
    enclosures = [rectangle(power * phase(theta)) for theta in phases] if family else []
    image_ids[key] = len(images)
    images.append({'values': ids, 'hull': hull, 'extrema': extrema, 'box': box,
                   'enclosures': enclosures, 'phases': list(map(str, phases)), 'family': int(family)})
    return image_ids[key]


cases = []
for C, n in ref.primitive_pairs(24):
    for c, N in [(1, 0), (-1, 0), (C-1 if C > 1 else 1, C), (1, 1)]:
        phases = ref.coset_values(C, n, c, N)
        assert phases == ref.brute_coset(C, n, c, N)
        for k in range(5):
            cases.append([C, n, c, N, k, image(k, phases), int(len(phases) == 1)])
for C, n, c, N in [(3, 2, 3, 4), (5, 2, 1, 1), (5, 2, 2, 5)]:
    for k in range(5, len(params)):
        phases = ref.coset_values(C, n, c, N)
        cases.append([C, n, c, N, k, image(k, phases), int(len(phases) == 1)])

ideles = []
for C, n in [(3, 2), (5, 2), (5, 4), (1, 1)]:
    for r, negative, radius, k in product([F(1), F(2), F(1, 3), F(6, 5)],
                                           [False, True], [F(0), F(1, 8)], [1, 3]):
        inf = F(-2 if negative else 2)
        t = abs(inf)/r
        p = [t, radius/r] + params[k][2:]
        if p not in params:
            params.append(p)
        j = params.index(p)
        c, N = (1, 0) if k == 1 else (2, 5)
        phases = ref.coset_values(C, n, -c if negative else c, N)
        ideles.append([C, n, str(inf), str(radius), str(r), c, N, j, image(j, phases)])

# Invalid raw t is a constructor status, not an evaluation input (api-3c section 3).
invalid = [['0', '0'], ['-1', '0'], ['1', '1'], ['0', '1'], ['-2', '1']]
for name, rows in [('params', [list(map(str, p)) for p in params]), ('values', values),
                   ('images', images), ('classes', cases), ('ideles', ideles), ('invalid', invalid)]:
    path = OUT / (name + '.jsonl')
    path.write_text(''.join(json.dumps(r, separators=(',', ':')) + '\n' for r in rows))
    print(name, len(rows), path.stat().st_size)
total = sum(p.stat().st_size for p in OUT.glob('*.jsonl'))
assert total <= 400*1024, total
print('TOTAL_BYTES', total)
