#!/usr/bin/env python3
"""Exact P10 membership vectors. Run from repository root with timeout 120."""
from fractions import Fraction as F
from pathlib import Path
import json
import random
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from proto.quotient3_checks import Adele, Ball, lift_member, reduce, direct_member
from proto.text_grammar import read_real, print_real, canonical

rng = random.Random(3101)
out = Path('tests/ref/vectors/q-slice1/translation.jsonl')
members = 0
with out.open('w') as stream:
    for i in range(1000):
        mid = F(rng.randrange(-64, 65), 8)
        rad = F(rng.randrange(0, 4), 16)
        a = F(rng.randrange(-23, 24), rng.randrange(1, 10))
        N = F(rng.randrange(2, 12), rng.randrange(1, 5)) if i % 5 else F(0)
        q = F(rng.randrange(-100000, 100001), rng.randrange(1, 1000))
        x = Adele(mid, rad, Ball(a, N))
        translated = Adele(mid + q, rad, Ball(a + q, N))
        points = []
        # Rational finite witnesses represent points of A/Q. Include both outcomes.
        for sign in (True, False):
            for attempt in range(20000):
                s = (mid-a) % 1 if sign else F(rng.randrange(16), 16)
                w = a + s - mid if sign else F(rng.randrange(-80, 81))
                before = lift_member(x, s, w)
                if before != sign:
                    continue
                after = lift_member(translated, s, w)
                assert before == after
                # P8/P6 give an independently expressed membership check.
                assert direct_member(reduce(mid, rad, x.fin), s, w) == before
                points.append({'s': str(s), 'w': str(w), 'before': before, 'after': after})
                members += 1
                break
            else:
                raise AssertionError('failed to find witness')
        # Dyadic inputs are exact at 128 bits, so these vectors test exact stored sets.
        text = f'({float(mid):g} +/- {float(rad):g} ; {a} mod {N})'
        stream.write(json.dumps({'lift': text, 'q': str(q), 'points': points}) + '\n')
print(f'{out}: 1000 translations, {members} membership witnesses')


def pow2(e):
    return F(2 ** e) if e >= 0 else F(1, 2 ** -e)


def stored_ball(mid, rad, prec=128):
    """conventions 9.5: truncated p-bit midpoint, outward 30-bit radius."""
    if mid:
        e = abs(mid.numerator).bit_length() - mid.denominator.bit_length() - prec + 1
        unit = pow2(e)
        if abs(mid) / unit < 2 ** (prec - 1):
            unit /= 2
        m = int(abs(mid) / unit) * unit * (1 if mid > 0 else -1)
    else:
        m = F(0)
    R = rad + abs(mid-m)
    if R:
        unit = pow2(R.numerator.bit_length() - R.denominator.bit_length() - 29)
        r = -((-R) // unit) * unit
    else:
        r = F(0)
    return m, r


out = out.with_name('golden.jsonl')
rows = 0
with out.open('w') as stream:
    for line, raw in enumerate(Path('tests/golden/qclass.tsv').read_text().splitlines(), 1):
        if not raw or raw.startswith('#'):
            continue
        text, expected = raw.split('\t')
        if text.startswith('union') or expected.startswith('!'):
            continue
        assert canonical('qclass', text) == expected
        real = text[1:].split(';')[0].strip()
        lo, hi = read_real(real)
        m, r = stored_ball((hi+lo)/2, (hi-lo)/2)
        printed_real = print_real(m, r, 6)
        fin = expected.split(';')[1].split(')')[0].strip()
        want = f'({printed_real} ; {fin}) + Q'
        lo2, hi2 = read_real(printed_real)
        m2, r2 = stored_ball((hi2+lo2)/2, (hi2-lo2)/2)
        second = f'({print_real(m2, r2, 6)} ; {fin}) + Q'
        stream.write(json.dumps({'line': line, 'lo': str(lo), 'hi': str(hi),
                                 'mid': str(m), 'rad': str(r), 'want': want,
                                 'second': second}) + '\n')
        rows += 1
print(f'{out}: {rows} golden lifts, exact enclosure and printer expectations')
