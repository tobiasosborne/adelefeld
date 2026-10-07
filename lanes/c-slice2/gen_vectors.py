#!/usr/bin/env python3
"""Slice b oracle vectors. Run under timeout 120; no adelefeld implementation imported."""
import json
import sys
from fractions import Fraction as F
from math import gcd
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'proto'))
import char_checks as ref
import flint

OUT = ROOT / 'tests/ref/vectors/c-slice2'
OUT.mkdir(parents=True, exist_ok=True)


def write(name, rows):
    p = OUT / (name + '.jsonl')
    p.write_text(''.join(json.dumps(r, separators=(',', ':')) + '\n' for r in rows))
    print(name, len(rows), p.stat().st_size)


write('conj', [{'C': C, 'n': n, 'inverse': 1 if C == 1 else pow(n, -1, C)}
               for C, n in ref.primitive_pairs(80)])
operands, images, cases = [], [], []
image_ids = {}


def operand(c):
    s = str(c)
    if s not in operands:
        operands.append(s)
    return operands.index(s)


def image(phases):
    key = tuple(sorted(phases))
    if key not in image_ids:
        # An extremum is the real/imaginary part of an explicit root of unity.
        # Certified 512-bit FLINT bounds are rounded outward to 60 decimal places.
        with flint.ctx.workprec(512):
            balls = [ref.phase_acb(t) for t in key]
            extrema, labels = [], []
            for coordinate in ('real', 'imag'):
                intervals = [ref.arb_interval(getattr(z, coordinate)) for z in balls]
                for maximum in (False, True):
                    pick = max if maximum else min
                    lo = pick(v[0] for v in intervals)
                    hi = pick(v[1] for v in intervals)
                    scale = 10**60
                    a = lo.numerator * scale // lo.denominator
                    b = -((-hi.numerator * scale) // hi.denominator)
                    extrema.append([str(F(a, scale)), str(F(b, scale))])
                    # Choose the attaining phase with an independent mpmath enumeration.
                    phase = pick(key, key=lambda t: getattr(ref.E(t), coordinate))
                    labels.append(('cos' if coordinate == 'real' else 'sin') + '(2*pi*' + str(phase) + ')')
        image_ids[key] = len(images)
        images.append({'phases': list(map(str, key)), 'extrema': extrema, 'algebraic': labels})
    return image_ids[key]


pairs = ref.primitive_pairs(24)
for C, n in pairs:
    for N in (0, 1, 2, 3, 4, 6, 8, 12, 24, 360):
        cs = [1, -1] if N == 0 else [1, -1]
        if N:
            # A nonunit modulo C is included whenever the unit-coset predicate permits it.
            nonunit = next((c for c in range(2, 2*C+2) if gcd(c, C) > 1 and gcd(c, N) == 1), None)
            if nonunit is not None:
                cs.append(nonunit)
            huge = 2**2000
            while gcd(huge, N) != 1:
                huge += 1
            cs += [huge, -huge]
        for c in dict.fromkeys(cs):
            phases = ref.coset_values(C, n, c, N)
            singleton = N == 0 or N % C == 0
            assert (len(phases) == 1) == singleton
            if abs(c) < 1000:
                assert phases == ref.brute_coset(C, n, c, N)
            # Compact schema: C,n,operand index,N,image index,singleton,raw chi(c) zero.
            cases.append([C, n, operand(c), N, image(phases), int(singleton), int(gcd(c, C) > 1)])
# Mutation survivor: the ambiguous image at C=27,N=9,c=2 is not conjugation-symmetric.
phases = ref.coset_values(27, 2, 2, 9)
assert phases == ref.brute_coset(27, 2, 2, 9) == {F(1, 18), F(7, 18), F(13, 18)}
cases.append([27, 2, operand(2), 9, image(phases), 0, 0])
write('operands', [{'c': operands}])
write('images', images)
write('cosets', cases)

dump = []
for j, (C, n) in enumerate(pairs[:30]):
    re = '0 0 0 0' if j % 2 == 0 else '-3 -2 1 -5'
    im = '1 -1 0 0' if j % 2 == 0 else '5 -3 3 -6'
    dump.append({'input': f'adf1 Q char {C:x} {n:x} {re} {im}', 'status': 'OK'})
for text, status in [
        ('8 1 0 0 0 0 0 0 0 0', 'DOMAIN'), ('10 9 0 0 0 0 0 0 0 0', 'DOMAIN'),
        ('0 1 0 0 0 0 0 0 0 0', 'DOMAIN'), ('5 z 0 0 0 0 0 0 0 0', 'PARSE'),
        ('5 2 0 0 0 0 0 0 0', 'PARSE'), ('5 2 0 0 0 0 0 1 0 0', 'DOMAIN'),
        ('10000 5 0 0 0 0 0 0 0 0', 'OK'), ('10001 3 0 0 0 0 0 0 0 0', 'LIMIT'),
        ('ffffffffffffffff 1 0 0 0 0 0 0 0 0', 'DOMAIN')]:
    dump.append({'input': 'adf1 Q char ' + text, 'status': status})
write('dump', dump)
total = sum(p.stat().st_size for p in OUT.glob('*.jsonl'))
assert total <= 400*1024
print('primitive_pairs_24', len(pairs), 'TOTAL_BYTES', total)
