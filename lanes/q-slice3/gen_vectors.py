#!/usr/bin/env python3
"""Exact stored dyadic real balls; finite centres/radii are arbitrary rationals.

Oracle: proto/quotient3_checks.py psi and nearest_distance, independent of C.
Sources: refs/src/tate-poonen/notes.txt:693-700; analysis.md Lemma 2; api-3.md Q4.
Run from repository root under timeout. No numerical hull values are rounded into fixtures.
"""
import json
from fractions import Fraction as F
from pathlib import Path
import random
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from proto.quotient3_checks import Adele, Ball, psi, nearest_distance

rng = random.Random(320301)
cases = []
for den in (1, 2, 3, 4, 6, 12, 360):
    for sign in (-1, 1):
        cases.append(Adele(F(sign * 7, 8), 0, Ball(F(sign * 17, den), 0)))
for radius in (F(0), F(1), F(6), F(1, 2), F(2, 3), F(3, 4), F(1, 360)):
    for mid, rad in ((F(0), F(0)), (F(1, 8), F(1, 32)), (F(-3, 8), F(1, 4))):
        cases.append(Adele(mid, rad, Ball(F(7, 12), radius)))
# Interior extrema, E3's line hull, full circle, and a denominator too large to enumerate.
cases += [Adele(0, F(1, 16), Ball(0, 1)), Adele(0, 0, Ball(0, F(1, 2))),
          Adele(0, F(1, 2), Ball(F(1, 3), 0)),
          Adele(F(1, 8), 0, Ball(F(-1, 3), F(1, (1 << 2000) + 1))),
          Adele(F((1 << 2000) + 7, 8), 0, Ball(F(-(1 << 2000) + 9, 360), 0))]
for _ in range(40):
    cases.append(Adele(F(rng.randrange(-500, 501), 64), F(rng.randrange(17), 128),
                       Ball(F(rng.randrange(-1000, 1001), rng.choice((1, 2, 3, 6, 12, 360))),
                            F(rng.randrange(10), rng.choice((1, 2, 3, 4, 6, 12))))))
out = Path('tests/ref/vectors/q-slice3/psi.jsonl')
with out.open('w') as f:
    for i, x in enumerate(cases):
        value = psi(x)
        points = []
        for j in range(40):
            t = x.mid + x.rad * F(2*j-39, 39)
            a = x.fin.a + j*x.fin.N
            points.append({'t': str(t), 'a': str(a), 'phase': str(psi(Adele(t, 0, Ball(a, 0))))})
        row = {'id': i, 'm': str(x.mid), 'r': str(x.rad), 'a': str(x.fin.a), 'N': str(x.fin.N),
               'phase': str(value) if isinstance(value, F) else None,
               'dist': [str(nearest_distance(value, F(t))) for t in (0, F(1, 2), F(1, 4), F(3, 4))],
               'points': points}
        f.write(json.dumps(row, separators=(',', ':')) + '\n')
assert out.stat().st_size <= 400*1024
print(f'{len(cases)} vectors, {40*len(cases)} sampled points, {out.stat().st_size} bytes')
