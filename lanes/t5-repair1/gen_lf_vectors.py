#!/usr/bin/env python3
"""Vectors of lane t5-repair1 for the Gamma certificate of src/localfactor.c:
timeout 300 python3 -B lanes/t5-repair1/gen_lf_vectors.py

Writes tests/ref/vectors/t5-repair1/gamma_bound.jsonl. mpmath at 60 digits; no adelefeld code and no proto/
oracle is imported. One record per exact dyadic box s = (x + i y) +/- (rx, ry) at the real place:
  samples: the 5 x 5 grid x + rx (i - 2)/2 + i (y + ry (j - 2)/2), corners and edges included, with
           L_inf(t) = pi^(-t/2) Gamma(t/2) (docs/api-1f9.md:412-414) as decimal strings [re, im];
  ym:      L_inf at the centre;
  rb:      R Bsmall, the enlargement of the midpoint value in docs/api-1f9.md Y16 (step 7 of the code's steps,
           :442-444, and the bound Bsmall of :479-483), evaluated from its statement with exact inputs:
           z = s/2, lo = min Re(z), hi = max Re(z), n = 0 if lo >= 1 else 1 - floor(lo) (:432-433), A = lo + n,
           c = ceil(hi + n), M0 = 1/A + (c - 1)!, M1 = 1/A^2 + c! (:476-478, src/localfactor.c:192-197),
           delta_j = sqrt(dx_j^2 + dy^2) with dx_j the distance of [lo, hi] from -j and dy that of the imaginary
           interval of z from 0 (:437-440), P = prod_(j<n) delta_j, Bsmall = pi^(-lo) [M1 + M0 sum 1/delta_j]/(2P),
           R = hypot(rx, ry) (src/localfactor.c:338);
  width:   1 when the record is also a width record: the test then asks the result to lie in ym +/- rb (plus
           rounding) in each component. Those boxes are chosen so that FLINT's candidate is wider than ym +/- rb:
           the refinement is the binding constraint there (lanes/t5-repair1/probe2.log).
"""
from fractions import Fraction as F
import json
import math
import os

import mpmath as mp

mp.mp.dps = 60
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
OUT = os.path.join(ROOT, 'tests', 'ref', 'vectors', 't5-repair1', 'gamma_bound.jsonl')


def mpq(q):
    return mp.mpf(q.numerator) / q.denominator


def L(t):
    return mp.power(mp.pi, -t / 2) * mp.gamma(t / 2)


def qstr(q):
    return str(q.numerator) if q.denominator == 1 else '%d/%d' % (q.numerator, q.denominator)


def dist(a, b, p):
    """distance of the interval [a, b] from the point p"""
    return 0 if a <= p <= b else min(abs(a - p), abs(b - p))


def rb(x, y, rx, ry):
    lo, hi = (x - rx) / 2, (x + rx) / 2
    n = 0 if lo >= 1 else 1 - math.floor(lo)
    A = lo + n
    c = math.ceil(hi + n)
    M0 = 1 / mpq(A) + mp.factorial(c - 1)
    M1 = 1 / mpq(A) ** 2 + mp.factorial(c)
    dy = mpq(F(dist((y - ry) / 2, (y + ry) / 2, 0)))
    P, S = mp.mpf(1), mp.mpf(0)
    for j in range(n):
        d = mp.sqrt(mpq(F(dist(lo, hi, -j))) ** 2 + dy ** 2)
        P *= d
        S += 1 / d
    B = mp.power(mp.pi, -mpq(lo)) * (M1 + M0 * S) / (2 * P)
    return mp.sqrt(mpq(rx) ** 2 + mpq(ry) ** 2) * B, n, c


def s3(v):
    return [mp.nstr(mp.re(v), 60), mp.nstr(mp.im(v), 60)]


# (x, y, rx, ry, width): exact dyadics
BOXES = [
    (F(2), F(1, 2), F(0), F(1, 8), 0),          # c = 1, n = 0: M1 = 1/A^2 + 1! (mutant 217 drops 1/A^2)
    (F(2), F(0), F(0), F(1, 8), 1),             # the same on the real axis; width (223: M1 = 2/A + c!; 255)
    (F(2), F(3), F(0), F(1, 64), 1),            # c = 1 higher up; width
    (F(-2), F(3), F(0), F(1, 8), 0),            # c = 1 with n = 2 (two factors of the recurrence)
    (F(9), F(6), F(1, 8), F(1, 8), 0),          # c = 5: M1 = c! (225 drops it), the guard (248), 253
    (F(9), F(3), F(1, 1024), F(1, 1024), 0),
    (F(-2), F(1, 8), F(1, 1024), F(1, 1024), 1),  # n = 2 near the pole -2: width (221: M0 = 1/A + c!)
    (F(-2), F(1, 2), F(1, 8), F(1, 8), 1),
]


def main():
    rows = []
    for x, y, rx, ry, width in BOXES:
        samples = []
        for i in range(5):
            for j in range(5):
                tx, ty = x + rx * (i - 2) / 2, y + ry * (j - 2) / 2
                samples.append({'t': [qstr(tx), qstr(ty)], 'v': s3(L(mp.mpc(mpq(tx), mpq(ty))))})
        r, n, c = rb(x, y, rx, ry)
        rows.append({'kind': 'gamma_box', 's': [qstr(x), qstr(y)], 'rad': [qstr(rx), qstr(ry)], 'n': n, 'c': c,
                     'precs': [24, 53, 120], 'ym': s3(L(mp.mpc(mpq(x), mpq(y)))), 'rb': mp.nstr(r, 30),
                     'width': width, 'samples': samples})
        print(qstr(x), qstr(y), qstr(rx), qstr(ry), 'n', n, 'c', c, 'rb', mp.nstr(r, 8), 'width', width)
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, 'w') as f:
        for r in rows:
            f.write(json.dumps(r, separators=(',', ':')) + '\n')
    print(len(rows), 'records ->', OUT)


if __name__ == '__main__':
    main()
