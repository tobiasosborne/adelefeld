#!/usr/bin/env python3
"""Select the committed subset of the local zeta fixtures (lane f-slice14, slice C).

Input: the full fixture file of proto/zeta_checks.py (11 MB, not committed), regenerated with
    timeout 170 python3 proto/zeta_checks.py --fixtures lanes/f-slice14/zeta-fixtures.jsonl
Output: tests/ref/vectors/f-slice14/zeta.jsonl, at most 600 KB.

Selection rule (every row is copied unchanged, except that the samples of every row of positive radius
(rules 2, 5, 6 and 7) are restricted to the centre and the four corners; a subset of independently certified
samples is still a set of certified samples, so the containment test stays sound, it only tests fewer points):
 1. every row with status DOMAIN or LIMIT that has an input (the exact poles; the recurrence limit);
 2. every row of the kinds closed_boundary, wide_free, huge_real, huge_imaginary, imaginary_point, regular
    (both places, every prime, the extreme arguments +-2^1000 and i 2^1000), and regular_ball at the real place
    and at p = 2, 7, 2^64-59;
 3. around and segment at a prime, every prime: radius 10^-1 at every k = -3..3, radius 10^-60 at k = -3, 1, 3;
 4. around and segment at the real place: radius 10^-1 and 10^-60, the poles 0, -2, -40;
 5. near and near_negative at a prime: one k and one radius per prime (2: k=0, 10^-1; 3: k=1, 10^-60;
    5: k=-1, 10^-1; 7: k=2, 10^-60; 65537: k=3, 10^-1; 2^64-59: k=-3, 10^-60); near and near_imaginary at
    the real place: the poles 0 (radius 10^-1), -2 (10^-60), and near_imaginary at -40 (10^-1);
 6. the precision pairs of check_precision (not written by --fixtures): the same pole-free box at prec 16
    (NOT_DETERMINED) and 256 (OK), every prime, computed here with the oracle's own functions
    (expected, certified_reference, sample_points, width_witness, serialize), kind 'precision';
 7. near_imaginary at the real poles -4, -6, -8, -10, radius 10^-1: the four rows of the full file whose width
    target (factor 64) is missed when the midpoint refinement of Z4 step 5 is removed (mutation survivor of
    lanes/f-slice14/mutate.log, found with plant_faults.py R1 on the full file).
The radius of a row is recognised from its s_rad, k from s_mid, the real pole from s_mid.
"""
import json
import os
import sys
from fractions import Fraction as F

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, 'proto'))
import zeta_checks as Z  # noqa: E402
import mpmath as mp  # noqa: E402

K_OF = {2: (0, 1), 3: (1, 60), 5: (-1, 1), 7: (2, 60), 65537: (3, 1), 2**64 - 59: (-3, 60)}
REAL_POLES_POLE = {0, -2, -40}
REAL_POLES_NEAR = {0: 1, -2: 60, -40: 1}


def radius_j(row):
    """Return j when the nominal radius 10^-j is recognisable from the effective radius, else None."""
    r = max(F(row['s_rad'][0]), F(row['s_rad'][1]))
    for j in Z.JS:
        nominal = F(1, 10**j)
        if nominal <= r <= nominal * (1 + F(1, 2**20)):
            return j
    return None


def k_of(row):
    p = row['place']
    y = Z.mm(F(row['s_mid'][1]))
    return int(mp.nint(y * mp.log(p) / (2 * mp.pi)))


def real_pole(row, kind):
    x = F(row['s_mid'][0])
    if kind == 'near':
        x -= 16 * F(1, 10**radius_j(row))
    return int(round(x))


def keep(row):
    kind, status = row['kind'], row['status']
    if 's_mid' not in row:
        return False
    if status in ('DOMAIN', 'LIMIT'):
        return True
    if kind in ('closed_boundary', 'wide_free', 'huge_real', 'huge_imaginary', 'imaginary_point', 'regular'):
        return True
    if kind == 'regular_ball':
        return row['place'] in ('real', 2, 7, 2**64 - 59)
    j = radius_j(row)
    if j not in (1, 60):
        return False
    real = row['place'] == 'real'
    if kind in ('around', 'segment'):
        if real:
            return real_pole(row, kind) in REAL_POLES_POLE
        return j == 1 or k_of(row) in (-3, 1, 3)
    if kind in ('near', 'near_negative', 'near_imaginary'):
        if real:
            if kind == 'near_imaginary' and j == 1 and real_pole(row, kind) in (-4, -6, -8, -10):
                return True
            if kind == 'near' and real_pole(row, kind) == -40:
                return False
            return REAL_POLES_NEAR.get(real_pole(row, kind)) == j
        return (k_of(row), j) == K_OF[row['place']]
    return False


def trim_samples(row):
    mid = [F(t) for t in row['s_mid']]
    rad = [F(t) for t in row['s_rad']]
    wanted = {(mid[0], mid[1])}
    for i in (-1, 1):
        for j in (-1, 1):
            wanted.add((mid[0] + i * rad[0], mid[1] + j * rad[1]))
    row['samples'] = [s for s in row['samples'] if (F(s['s'][0]), F(s['s'][1])) in wanted]
    return row


def precision_rows():
    out = []
    for p in Z.PRIMES:
        y = Z.rounded_dyadic(2 * mp.pi / mp.log(p))
        r = F(1, 2**180)
        for prec in (16, 256):
            res = Z.expected(p, (F(0), y + 16 * r), (r, r), prec)
            width, sampled = Z.arb(0), []
            if res['status'] == 'OK':
                with Z.ctx.workprec(Z.CERT_BITS):
                    for pt in Z.sample_points(res['s_mid'], res['s_rad']):
                        v, err, box = Z.certified_reference(p, pt)
                        sampled.append((pt, v, err))
                    width = Z.width_witness(p, res['s_mid'], res['s_rad'])
            out.append(dict(Z.serialize(res, width, sampled), kind='precision'))
    return out


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, 'zeta-fixtures.jsonl')
    dst = sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, 'tests/ref/vectors/f-slice14/zeta.jsonl')
    rows = []
    with open(src, encoding='ascii') as f:
        for line in f:
            row = json.loads(line)
            if keep(row):
                rows.append(row)
    rows += precision_rows()
    rows = [trim_samples(r) if r.get('samples') and any(F(t) for t in r['s_rad']) else r for r in rows]
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    with open(dst, 'w', encoding='ascii') as f:
        for row in rows:
            f.write(json.dumps(row, sort_keys=True) + '\n')
    counts = {}
    for row in rows:
        key = (row['status'], 'real' if row['place'] == 'real' else 'prime')
        counts[key] = counts.get(key, 0) + 1
    print('rows', len(rows), 'bytes', os.path.getsize(dst))
    print('by status and place', sorted(counts.items()))
    print('samples', sum(len(r.get('samples', [])) for r in rows))


if __name__ == '__main__':
    main()
