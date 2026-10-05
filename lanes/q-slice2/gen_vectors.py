"""Exact oracle vectors. Input midpoint/radius are dyadic, stored without Arb parsing.

The source endpoints document intent. The stored endpoints, not those source endpoints,
are the input to reduce. Input radius is the least RU30 bound, without a successor.
Exact pieces deduplicate by (lo,hi,N,m). Stored pieces deduplicate AFTER Q1 by that key.
"""
import importlib.util
import json
import sys
from fractions import Fraction as F
from math import floor, ceil
from pathlib import Path

root = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('quotient3', root/'proto/quotient3_checks.py')
o = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = o
spec.loader.exec_module(o)


def key(p):
    return [str(p.lo), str(p.hi), str(p.m), str(p.N)]


def vector(lo, hi, a, N, prec):
    m = o.round_binary(abs((lo+hi)/2), 53)
    if lo+hi < 0:
        m = -m
    r = o.round_binary(max(m-lo, hi-m), 30, True)
    a = a % N if N else a
    x = o.Adele(m, r, o.Ball(a, N))
    ps = o.reduce(m, r, x.fin)
    raw = 0
    for j in range(N.denominator if N else 1):
        l, h = m-r-a-j*N, m+r-a-j*N
        raw += 1 if l == h else ceil(h)-floor(l)
    rounded = sorted({o.round_piece(p, prec) for p in ps}, key=o.piece_key)
    points = [(F(0), F(w)) for w in range(-3, 4)]
    for p in ps:
        for s in (p.lo, p.hi, (p.lo+p.hi)/2):
            points.append((s, F(p.m)))
            if s == 1:
                points.append((F(0), F(p.m-1)))
    points += [(F(j % 17, 16), F((j*7) % 13-6, (1, 2, 3, 4, 6)[j % 5]))
               for j in range(40)]
    points = list(dict.fromkeys(points))[:40]
    assert len(points) == 40 and len(set(points)) == 40
    return dict(source=[str(lo), str(hi)], mid=str(m), rad=str(r),
                lo=str(m-r), hi=str(m+r), a=str(a), N=str(N), prec=prec, raw=raw,
                exact=[key(p) for p in ps], rounded=[key(p) for p in rounded],
                points=[[str(s), str(w), o.lift_member(x, s, w)] for s, w in points])


intervals = [(F(0), F(0)), (F(0), F(1)), (F(-1, 4), F(1, 4)),
             (F(9, 10), F(11, 10)), (F(-9, 4), F(13, 4)), (F(1, 4), F(9, 4)),
             (F(-2), F(3)), (F(9, 10), F(1)), (F(-7, 10), F(-1, 10))]
centres = [F(0), F(-3, 2), F(2, 3), F(-5, 4), F(7, 6), F(-7)]
counts = []
for name, radii in [('integer', [F(n) for n in (0, 1, 2, 3, 12)]),
                    ('fractional', [F(1, 2), F(1, 3), F(2, 3), F(3, 2), F(5, 4)])]:
    rows = []
    for i, N in enumerate(radii):
        for j, a in enumerate(centres):
            for k in range(2):
                lo, hi = intervals[(i*3+j+k*4) % len(intervals)]
                rows.append(vector(lo, hi, a, N, (2, 20, 53, 128)[(i+j+k) % 4]))
    if name == 'integer':
        # Shift by 1/10 creates EXACT [9/10,1] from a stored dyadic [1,11/10]
        # only approximately; use [1,2] with centre 1/10 for the exact boundary piece.
        for prec in (20, 53, 128):
            rows.append(vector(F(1), F(2), F(1, 10), F(3), prec))
        # An upper endpoint beyond the input radius's 30-bit grid.
        rows.append(vector(F(0), F(1, 2)+F(1, 2**33), F(0), F(3), 53))
        # A non-dyadic translated endpoint close to an integer.
        rows.append(vector(F(0), F(1)-F(1, 2**40), F(1, 3), F(3), 53))
        # The stored input crosses 0. Its main exact piece has d just below 1/2;
        # RU30 lands on 1/2, whose successor uses the NEXT binade's spacing (R1).
        rows.append(vector(F(0), F(1)-F(1, 2**40), F(0), F(3), 53))
    path = root/'tests/ref/vectors/q-slice2'/f'{name}.jsonl'
    path.write_text(''.join(json.dumps(v, separators=(',', ':'))+'\n' for v in rows))
    counts.append((name, len(rows), path.stat().st_size))
print(counts)
assert sum(n for _, _, n in counts) < 500_000

# Re-reduce the actual rounded PIECES, including all stored spill. No [0,1] clipping.
spill = []
for index in range(26):
    if index >= 24:
        # Legal single-piece spill with NO adjacent piece that could hide clipping loss.
        family = ([o.Piece(F(-1, 16), F(1, 16), 0, 2)] if index == 24 else
                  [o.Piece(F(15, 16), F(17, 16), -7, 0)])
        prec = 53
    else:
        N = [F(0), F(2), F(3), F(1, 2), F(2, 3), F(5, 4)][index % 6]
        a = [F(0), F(1, 3), F(-7, 6)][index % 3]
        lo, hi = [(F(0), F(1)), (F(9, 10), F(11, 10)),
                  (F(-1, 4), F(1, 4)), (F(1, 4), F(9, 4))][index // 6]
        prec = (20, 53, 128)[index % 3]
        first = vector(lo, hi, a, N, prec)
        family = [o.Piece(F(l), F(h), int(m), int(A)) for l, h, m, A in first['rounded']]
    exact = o.normalize(family)
    rounded = sorted({o.round_piece(p, prec) for p in exact}, key=o.piece_key)
    raw = sum(1 if p.lo == p.hi else ceil(p.hi-p.m)-floor(p.lo-p.m) for p in family)
    inputs = [dict(mid=str((p.lo+p.hi)/2), rad=str((p.hi-p.lo)/2),
                   lo=str(p.lo), hi=str(p.hi), a=str(p.m), N=str(p.N)) for p in family]
    points = []
    for p in family:
        points += [(p.lo, F(p.m)), (p.hi, F(p.m)), (F(0), F(p.m-1)), (F(1), F(p.m))]
        # Explicit spill witnesses, shifted into the half-open section.
        for s in (p.lo, p.hi):
            n = floor(s)
            points.append((s-n, F(p.m-n)))
    points += [(F(j % 33, 32), F(j % 19-9)) for j in range(40)]
    points = list(dict.fromkeys(points))[:40]
    assert len(points) == 40 and len(set(points)) == 40
    spill.append(dict(input_pieces=inputs, prec=prec, raw=raw,
                      exact=[key(p) for p in exact], rounded=[key(p) for p in rounded],
                      points=[[str(s), str(w), o.direct_member(family, s, w)] for s, w in points]))
path = root/'tests/ref/vectors/q-slice2/spill.jsonl'
path.write_text(''.join(json.dumps(v, separators=(',', ':'))+'\n' for v in spill))
print('spill', len(spill), path.stat().st_size)
assert sum(p.stat().st_size for p in path.parent.glob('*.jsonl')) < 500_000
