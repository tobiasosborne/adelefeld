"""Exact oracle vectors for adf_qclass_neg and adf_qclass_add (slice 3.1-f).

Run from the worktree root: timeout 300 python3 lanes/q-slice7/gen_vectors.py
Oracle: proto/quotient3_checks.py (add, neg, reduce, round_piece, compare, direct_member).
Design: docs/api-3.md 2.4 and Q3; the finite sum (a+b) + gcd(N,M) Zhat is
docs/proofs/precision.md:25-30 (Proposition 1), with rational gcd as defined at :9-11.
Inputs are stored dyadic balls: midpoint any dyadic, radius at most 30 significant bits.
For every pair (add) or entry (neg) the exact real endpoints are added or negated BEFORE
rounding; R is the oracle's reduce; Q1 is round_piece. The raw count is R's construction
count before deduplication. Each record carries 40 sample points: points of the input sets
(add: one point of x and one of y), whose sum or negation the C test forms itself.
"""
import importlib.util
import json
import random
import sys
from fractions import Fraction as F
from math import ceil, floor, gcd
from pathlib import Path

root = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('quotient3', root/'proto/quotient3_checks.py')
o = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = o
spec.loader.exec_module(o)

rng = random.Random(310207)


def is_dyadic(q):
    q = F(q)
    return q.denominator & (q.denominator-1) == 0


def mag_ok(r):
    """A mag holds r exactly iff r is a dyadic with at most 30 significant bits."""
    r = F(r)
    if r == 0:
        return True
    if not is_dyadic(r):
        return False
    n = r.numerator
    while n % 2 == 0:
        n //= 2
    return n.bit_length() <= 30


def canon(a, N):
    a, N = F(a), F(N)
    return (a % N if N else a), N


class Entry:
    """One stored adele: real ball [mid-rad, mid+rad], finite a + N Zhat (canonical a)."""

    def __init__(self, mid, rad, a, N):
        self.mid, self.rad = F(mid), F(rad)
        assert is_dyadic(self.mid) and mag_ok(self.rad) and self.rad >= 0
        self.a, self.N = canon(a, N)

    def adele(self):
        return o.Adele(self.mid, self.rad, o.Ball(self.a, self.N))

    def lo(self):
        return self.mid-self.rad

    def hi(self):
        return self.mid+self.rad

    def json(self):
        return dict(mid=str(self.mid), rad=str(self.rad), a=str(self.a), N=str(self.N))


def lift(lo, hi, a, N):
    lo, hi = F(lo), F(hi)
    return ('lift', [Entry((lo+hi)/2, (hi-lo)/2, a, N)])


def pieces_of(cls, prec):
    """Canonical PIECES storage: the oracle's Q1 output of reducing a lift."""
    form, es = cls
    assert form == 'lift'
    e = es[0]
    exact = o.reduce(e.mid, e.rad, o.Ball(e.a, e.N))
    rounded = sorted({o.round_piece(p, prec) for p in exact}, key=o.piece_key)
    out = [Entry((p.lo+p.hi)/2, (p.hi-p.lo)/2, p.m, p.N) for p in rounded]
    for x in out:
        assert 0 <= x.mid <= 1 and x.N.denominator == 1
    keys = [(x.lo(), x.hi(), x.N, x.a) for x in out]
    assert keys == sorted(keys) and len(set(keys)) == len(keys)
    return ('pieces', out)


def raw_count(z):
    """R's construction count for one exact adele (docs/api-3.md 2.2 step 4)."""
    N, a = z.fin.N, z.fin.a
    total = 0
    for j in range(N.denominator if N else 1):
        lo, hi = z.mid-z.rad-a-j*N, z.mid+z.rad-a-j*N
        total += 1 if lo == hi else ceil(hi)-floor(lo)
    return total


def sources(op, x, y):
    if op == 'neg':
        return [o.neg(e.adele()) for e in x[1]]
    return [o.add(ex.adele(), ey.adele()) for ex in x[1] for ey in y[1]]


def sample(e):
    """A rational point (s, w) of the stored adele e: s in [lo, hi], w in a + N Z."""
    t = rng.choice([F(0), F(1), F(1, 2), F(1, 3), F(5, 7), F(rng.randrange(1, 64), 64)])
    s = e.lo()+t*(e.hi()-e.lo())
    w = e.a+rng.randrange(-3, 4)*e.N
    return s, w


def record(name, op, x, y=None, prec=53, self_alias=False):
    zs = sources(op, x, y)
    exact_all = [p for z in zs for p in o.reduce(z.mid, z.rad, z.fin)]
    raw = sum(raw_count(z) for z in zs)
    assert raw >= len(exact_all)  # reduce already removes duplicates per adele
    exact = sorted(set(exact_all), key=o.piece_key)
    rounded = sorted({o.round_piece(p, prec) for p in exact}, key=o.piece_key)
    # Independent check (oracle check_arithmetic): the operation distributes over the
    # exact integer-modulus pieces of the inputs (Q3 step 2).
    px = [p for e in x[1] for p in o.reduce(e.mid, e.rad, o.Ball(e.a, e.N))]
    if op == 'neg':
        other = [o.Piece(-p.hi, -p.lo, -p.m, p.N) for p in px]
    else:
        py = [p for e in y[1] for p in o.reduce(e.mid, e.rad, o.Ball(e.a, e.N))]
        other = [o.Piece(a.lo+b.lo, a.hi+b.hi, a.m+b.m, gcd(a.N, b.N)) for a in px for b in py]
    if len(other) <= 400:
        assert o.compare(exact, other)[0], name
    points = []
    while len(points) < 40:
        u = sample(rng.choice(x[1]))
        if op == 'neg':
            s, w = -u[0], -u[1]
            points.append([str(u[0]), str(u[1])])
        else:
            v = sample(rng.choice(y[1]))
            s, w = u[0]+v[0], u[1]+v[1]
            points.append([str(u[0]), str(u[1]), str(v[0]), str(v[1])])
        # Diagonal translation by -w (P10.1) gives an integral finite coordinate.
        assert o.direct_member(exact, s-w, 0), (name, s, w)
    for p in rounded:
        assert 0 <= (p.lo+p.hi)/2 <= 1
    v = dict(name=name, op=op, x=dict(form=x[0], pieces=[e.json() for e in x[1]]))
    if op == 'add':
        v['y'] = dict(form=y[0], pieces=[e.json() for e in y[1]])
        v['self'] = self_alias
    v.update(prec=prec, raw=raw, exact=[key(p) for p in exact],
             rounded=[key(p) for p in rounded], points=points)
    return v


def key(p):
    return [str(p.lo), str(p.hi), str(p.m), str(p.N)]


rows = []
P = (2, 20, 53, 128)
# Real crossings after the sum: 0, 1, several integers (moduli 3 and 0).
reals = [((F(-1, 4), F(1, 8)), (F(-1, 8), F(1, 16))),
         ((F(3, 8), F(5, 8)), (F(1, 4), F(1, 2))),
         ((F(-3, 2), F(9, 4)), (F(1, 2), F(7, 4))),
         ((F(0), F(0)), (F(1, 2), F(1, 2))),
         ((F(1, 2), F(1)), (F(0), F(1))),
         ((F(-5, 2), F(-1, 4)), (F(-1), F(3, 8)))]
for i, ((xl, xh), (yl, yh)) in enumerate(reals):
    for j, (N, M) in enumerate(((3, 3), (0, 0), (2, 3))):
        x, y = lift(xl, xh, F(j-1, 3), N), lift(yl, yh, F(-2*j, 5), M)
        rows.append(record(f'real{i}-{j}', 'add', x, y, P[(i+j) % 4]))
# Moduli 2, 3, 4, 6, 12, 360: gcd 1, proper divisor, equal; centres include negatives.
mods = [(2, 3), (3, 2), (4, 6), (6, 4), (12, 4), (4, 12), (360, 360), (12, 360), (360, 3),
        (6, 6), (2, 2), (12, 6), (360, 4)]
for i, (N, M) in enumerate(mods):
    x = lift(F(-1, 4)+F(i, 16), F(3, 4), F(-7, 3)+i, N)
    y = lift(F(1, 8), F(5, 8)+F(i, 32), -5+F(i, 2), M)
    rows.append(record(f'mod{N}-{M}', 'add', x, y, P[i % 4]))
# Finite radius zero on one or both sides.
for i, (N, M) in enumerate(((0, 3), (6, 0), (0, 0), (0, 360))):
    x = lift(F(1, 4), F(3, 4)+F(i, 4), F(-3, 7), N)
    y = lift(F(-1, 8), F(1, 2), F(5, 6), M)
    rows.append(record(f'zero{N}-{M}', 'add', x, y, P[(i+1) % 4]))
# Fractional radii 1/2, 2/3, 7/360 (rational gcd: precision.md:9-11, Proposition 1).
fracs = [(F(1, 2), F(1, 2)), (F(1, 2), F(2, 3)), (F(2, 3), F(3)), (F(7, 360), 0),
         (F(7, 360), F(1, 2)), (F(2, 3), 0), (0, F(1, 2)), (F(2, 3), F(2, 3))]
for i, (N, M) in enumerate(fracs):
    w = F(1, 8) if F(N).denominator*F(M).denominator > 100 or N == F(7, 360) else F(5, 4)
    x = lift(F(1, 4), F(1, 4)+w, F(-1, 3), N)
    y = lift(F(-1, 2), F(-1, 2)+F(i, 64), F(1, 5)+i, M)
    rows.append(record(f'frac{N}-{M}', 'add', x, y, 20 if N == F(7, 360) else P[i % 4]))
# 2000-bit finite centres with small moduli, and a 1000-bit real midpoint.
big = F(2)**2000+F(1, 3)
rows.append(record('big-point+mod3', 'add', lift(F(1, 4), F(3, 4), big, 0),
                   lift(F(0), F(1, 2), 1, 3), 53))
rows.append(record('big-point+point', 'add', lift(F(1, 4), F(3, 4), big, 0),
                   lift(F(1, 8), F(1, 2), -F(2)**1999+F(1, 6), 0), 128))
rows.append(record('200bit-point+mod1/2', 'add', lift(F(1, 4), F(3, 4), -F(2)**200-F(1, 3), 0),
                   lift(F(1, 8), F(1, 2), F(1, 4), F(1, 2)), 20))
rows.append(record('big-real', 'add', lift(F(2)**1000+F(5, 8), F(2)**1000+F(7, 8), 0, 0),
                   lift(F(-1, 4), F(1, 4), F(1, 3), 0), 53))
# x + x with the same object: independent variation, wider than 2x.
selfs = [lift(F(1, 4), F(3, 4), 1, 3), lift(F(0), F(1, 2), F(1, 3), F(1, 2)),
         lift(F(-1, 4), F(5, 4), F(-2, 3), 4), lift(F(3, 8), F(3, 8), 7, 0)]
for i, x in enumerate(selfs):
    rows.append(record(f'self{i}', 'add', x, x, P[(i+2) % 4], self_alias=True))
# The sum crosses an integer ONLY at an exact endpoint built from many-bit dyadics:
# an early arb addition at prec would widen it and add a crossing (docs/api-3.md 2.4).
e1 = F(round(F(1, 3)*2**80), 2**80)
t = F(1, 2**100)


def ball(mid, rad, a, N):
    return ('lift', [Entry(mid, rad, a, N)])


# hi = 1 exactly; the exact midpoint sum 1-2^-99 is not representable at 53 bits.
rows.append(record('exact-end-hi', 'add', ball(e1, t, 0, 3), ball(1-2*t-e1, t, 0, 3), 53))
# lo = 2 exactly; the midpoint sum 2+2^-99 is not representable at 2 bits.
rows.append(record('exact-end-lo', 'add', ball(e1, t, 1, 2), ball(2+2*t-e1, t, 0, 2), 2))
# Two exact points with sum 2^60+1/2: one piece; rounding at 53 bits would cross 2^60.
rows.append(record('exact-point', 'add', ball(F(1, 2**80), 0, 0, 0),
                   ball(F(2)**60+F(1, 2)-F(1, 2**80), 0, F(1, 2), 0), 53))
# Radii of 30 bits each whose exact sum needs 61 bits; hi = 1 exactly.
rx, ry = F(2**29+1, 2**31), F(2**29+1, 2**61)
rows.append(record('exact-radius', 'add', ball(F(1, 2), rx, 0, 5), ball(F(1, 2)-rx-ry, ry, 0, 5), 128))
# PIECES inputs (canonical Q1 outputs, with spill), with each other, lifts and themselves.
pa = pieces_of(lift(F(-3, 4), F(5, 4), F(1, 3), 3), 53)
pb = pieces_of(lift(F(1, 4), F(9, 4), F(-1, 2), F(1, 2)), 20)
pc = pieces_of(lift(F(1), F(9, 8), F(1, 10), 2), 128)
pd = pieces_of(lift(F(0), F(2), 0, 1), 53)
rows.append(record('pieces+lift', 'add', pa, lift(F(0), F(1, 2), 2, 4), 53))
rows.append(record('lift+pieces', 'add', lift(F(1, 8), F(1, 4), F(5, 3), 0), pb, 20))
rows.append(record('pieces+pieces', 'add', pa, pc, 128))
rows.append(record('pieces-self', 'add', pc, pc, 53, self_alias=True))
rows.append(record('dup+zero', 'add', lift(F(0), F(2), 0, 1), lift(0, 0, 0, 0), 53))
rows.append(record('pieces-dup', 'add', pd, lift(F(0), F(1), 0, 1), 53))
# Zero class on either side.
for i, x in enumerate((lift(F(1, 4), F(9, 8), F(-2, 3), 4), pa, pb)):
    rows.append(record(f'zero-class{i}', 'add', x, lift(0, 0, 0, 0), P[i % 4]))
# Negation: lifts of every family above, PIECES, points and spill.
negs = [lift(F(1, 4), F(3, 4), 1, 3), lift(F(-3, 2), F(9, 4), F(-7, 3), 360),
        lift(F(0), F(0), F(5, 6), 0), lift(F(1, 4), F(7, 4), F(-1, 3), F(1, 2)),
        lift(F(1, 4), F(3, 8), F(2, 7), F(7, 360)), lift(F(1, 4), F(3, 4), big, 0),
        ball(e1, t, F(1, 3), F(2, 3)), lift(F(1, 2), F(1), 0, 12),
        lift(F(0), F(2), 0, 1), lift(F(-1, 4), F(0), 0, 0), lift(F(1), F(1), 1, 3),
        pa, pb, pc, pd]
for i, x in enumerate(negs):
    rows.append(record(f'neg{i}', 'neg', x, prec=P[i % 4]))

path = root/'tests/ref/vectors/q-slice7/arith.jsonl'
path.write_text(''.join(json.dumps(v, separators=(',', ':'))+'\n' for v in rows))
size = path.stat().st_size
print(f'{len(rows)} records ({sum(r["op"] == "add" for r in rows)} add), {size} bytes, '
      f'raw counts {min(r["raw"] for r in rows)}..{max(r["raw"] for r in rows)}, '
      f'{sum(r["raw"] > len(r["exact"]) for r in rows)} with duplicates before dedup, '
      f'{sum(len(r["exact"]) > len(r["rounded"]) for r in rows)} with duplicates after Q1')
assert size < 400_000
