#!/usr/bin/env python3
"""Vectors for slices 3.2-b and 3.2-c (lane q-slice5). Run from the repository root under timeout.

Oracle: proto/quotient3_checks.py (psi, phase_arcs, nearest_distance, local_phase, local_image,
reduce, round_piece), independent of C. Sources: refs/src/tate-poonen/notes.txt:693-700;
docs/proofs/analysis.md Lemma 2:76-94; docs/api-3.md 3.1-3.3, Q4, Q5, D3-3; conventions 6.1:844,876-887.

class.jsonl: kind "lift" (one stored adele), "reduce" (a lift that the C test reduces with adf_qclass_reduce;
'entries' are the oracle's rounded pieces, which the test compares with the stored C pieces before use), or
"pieces" (a canonical PIECES class built by hand in the test from 'entries'). Every record has the exact union
of the arcs of all stored entries (or null when it is too large), the four union distances of Q4 (the minimum
over the entries of each d(t)), the strict status of D3-3, and the exact class phase or null.
local.jsonl: a local ball a + p^e Z_p (e null: the exact a) with its exact phase fp_p(a) or null, the order
p^(-e) of the root family, its arcs and Q4 distances.
place.jsonl: an adele with, per place, the local image of its projection (Lemma 2 steps 1-3, conventions 6.1),
and the global phase when it is a singleton.
"""
import json
from fractions import Fraction as F
from pathlib import Path
import random
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from proto.quotient3_checks import (Adele, Ball, Piece, PhaseImage, psi, phase_arcs, nearest_distance,
                                    local_phase, local_image, reduce, round_piece)

TARGETS = (F(0), F(1, 2), F(1, 4), F(3, 4))
OUT = Path('tests/ref/vectors/q-slice5')
MAXARC = 64


def s(q):
    return str(F(q))


def image_of(value):
    return value if isinstance(value, PhaseImage) else PhaseImage(value, F(0), 1)


def merge(arcs):
    arcs = sorted(arcs)
    out = []
    for lo, hi in arcs:
        if out and lo <= out[-1][1]:
            out[-1] = out[-1][0], max(hi, out[-1][1])
        else:
            out.append((lo, hi))
    return out


def union(values):
    """Exact union of the arcs (small cases), the four union distances, the singleton phase."""
    imgs = [image_of(v) for v in values]
    dist = [min(nearest_distance(im, t) for im in imgs) for t in TARGETS]
    small = all(im.order <= MAXARC for im in imgs)
    arcs = merge([a for v in values for a in phase_arcs(v)]) if small else None
    singles = [v for v in values if isinstance(v, F)]
    phase = singles[0] if len(singles) == len(values) and len(set(singles)) == 1 else None
    if arcs is not None:
        # Independent extrema of the union from the materialized arcs: an endpoint or a quarter turn
        # inside an arc. The exact distance of each target to the union equals the oracle's minimum.
        for t, d in zip(TARGETS, dist):
            best = None
            for lo, hi in arcs:
                if lo <= t <= hi or (t == 0 and hi == 1):
                    e = F(0)
                else:
                    e = min(min(abs(t-x), 1-abs(t-x)) for x in (lo, hi))
                best = e if best is None else min(best, e)
            assert best == d, (values, t, best, d)
    return arcs, dist, phase


def points(x, n=40):
    out = []
    for j in range(n):
        t = x.mid + x.rad*F(2*j-n+1, n-1)
        a = x.fin.a + j*x.fin.N
        out.append([s(t), s(a), s(psi(Adele(t, 0, Ball(a, 0))))])
    return out


def ent(x):
    return [s(x.mid), s(x.rad), s(x.fin.a), s(x.fin.N)]


def class_record(kind, entries, lift=None, limit=None, prec=None):
    values = [psi(x) for x in entries]
    arcs, dist, phase = union(values)
    strict = 'NOT_DETERMINED' if any(x.fin.N.denominator != 1 for x in entries) else 'OK'
    row = {'kind': kind, 'entries': [ent(x) for x in entries],
           'points': [points(x) for x in entries],
           'arcs': None if arcs is None else [[s(a), s(b)] for a, b in arcs],
           'dist': [s(d) for d in dist], 'strict': strict,
           'phase': None if phase is None else s(phase)}
    if lift is not None:
        row.update({'lift': ent(lift), 'limit': limit, 'prec': prec})
    return row


def piece_adele(p):
    return Adele((p.lo+p.hi)/2, (p.hi-p.lo)/2, Ball(p.m, p.N))


def classes():
    rows = []
    rng = random.Random(320501)
    # Lifts: E1-E4 of design section 1, the arc about angle 0, fractional radii, large orders.
    lifts = [Adele(F(1), F(1, 8), Ball(0, 2)), Adele(F(1, 2), F(1, 2), Ball(0, 2)), Adele(0, 0, Ball(0, F(1, 2))),
             Adele(0, 0, Ball(0, 0)), Adele(0, F(1, 16), Ball(0, 1)), Adele(0, 0, Ball(F(1, 3), 0)),
             Adele(F(1, 4), 0, Ball(F(-1, 6), 6)), Adele(0, 0, Ball(F(1, 5), F(2, 3))),
             Adele(F(3, 8), F(1, 64), Ball(F(7, 12), F(3, 4))),
             Adele(F(1, 8), 0, Ball(F(-1, 3), F(1, (1 << 2000)+1))),
             Adele(F((1 << 2000)+7, 8), 0, Ball(F(-(1 << 2000)+9, 360), 0))]
    for x in lifts:
        rows.append(class_record('lift', [x]))
    # Reductions in C (piece limit and prec named); the lift of E3 is the D3-3 witness.
    reds = [(Adele(0, 0, Ball(0, F(1, 2))), 4, 53), (Adele(F(1, 2), 0, Ball(0, F(1, 2))), 4, 53),
            (Adele(F(1, 2), F(1, 2), Ball(0, 2)), 8, 64), (Adele(F(1), F(1, 8), Ball(0, 2)), 8, 64),
            (Adele(0, 0, Ball(F(1, 3), F(2, 3))), 8, 53),
            (Adele(F(1, 4), F(1, 32), Ball(F(1, 5), F(3, 4))), 16, 128),
            (Adele(F(5, 2), F(5, 2), Ball(0, 1)), 8, 64), (Adele(F(3, 8), 0, Ball(F(1, 7), F(1, 6))), 12, 20)]
    for x, limit, prec in reds:
        pieces = reduce(x.mid, x.rad, x.fin, limit)
        rounded = sorted({round_piece(p, prec) for p in pieces}, key=lambda q: (q.lo, q.hi, q.N, q.m))
        rounded = [piece_adele(q) for q in rounded]
        rows.append(class_record('reduce', rounded, lift=x, limit=limit, prec=prec))
    # Hand-built PIECES (canonical keys: lower end, upper end, H, A; midpoints in [0,1], d=1, 0<=A<H).
    hand = [
        [Piece(0, 0, 0, 1), Piece(0, 0, 1, 2)],                 # two singletons, same phase 0
        [Piece(0, 0, 0, 1), Piece(F(1, 2), F(1, 2), 0, 1)],     # E3's reduction: phases 0 and 1/2
        [Piece(F(1, 4), F(1, 4), 0, 0), Piece(F(1, 4), F(1, 4), 0, 3)],   # same phase 3/4
        [Piece(0, F(1, 8), 0, 0), Piece(F(3, 8), F(1, 2), 1, 2)],
        [Piece(F(1, 8), F(1, 8), 0, 1), Piece(F(1, 8), F(1, 8), 1, 2), Piece(F(5, 8), F(5, 8), 0, 1)],
    ]
    for _ in range(8):
        ps = set()
        while len(ps) < rng.randrange(2, 5):
            lo = F(rng.randrange(0, 64), 64)
            hi = lo + F(rng.randrange(0, 9), 64)
            N = rng.choice((0, 1, 2, 3, 6))
            m = rng.randrange(0, N) if N else rng.randrange(-5, 6)
            ps.add(Piece(lo, min(hi, F(1)) if (lo+min(hi, F(1)))/2 <= 1 else lo, m, N))
        hand.append(sorted(ps, key=lambda p: (p.lo, p.hi, p.N, p.m)))
    for ps in hand:
        ps = sorted(ps, key=lambda p: (p.lo, p.hi, p.N, p.m))
        rows.append(class_record('pieces', [piece_adele(p) for p in ps]))
    return rows


BIGP = 18446744073709551557   # the largest prime below 2^64; lball primes are one word (lball.h)


def local_record(p, a, e):
    value = local_image(a, e, p)
    img = image_of(value)
    arcs = phase_arcs(value) if img.order <= MAXARC else None
    if arcs is not None and e is not None and e < 0:
        size = p**(-e)
        want = sorted({local_phase(F(a)+F(p)**e*k, p) for k in range(size)})
        assert arcs == [(t, t) for t in want]
    return {'p': str(p), 'a': s(a), 'e': e, 'phase': s(value) if isinstance(value, F) else None,
            'base': s(img.base), 'order': str(img.order),
            'arcs': None if arcs is None else [[s(x), s(y)] for x, y in arcs],
            'dist': [s(nearest_distance(img, t)) for t in TARGETS],
            'strict': 'OK' if isinstance(value, F) else 'NOT_DETERMINED'}


def locals_():
    rows = []
    for p in (2, 3, 5, 7, 65537, BIGP):
        for a in (F(-1, 6), F(1, 6), F(1, 3), F(0), F(2, 5), F(7, p), F(5, p*p*3), F(p*p+1, 1),
                  F(-11, 2**40 * 3**5)):
            for e in (None, -3, -2, -1, 0, 1, 2, 3):
                if e is not None and p**max(0, -e) > 10**60:
                    continue
                rows.append(local_record(p, a, e))
    return rows


def place_record(x, extra):
    ps = set()
    for q in (x.fin.a.denominator, x.fin.N.denominator):
        n, d = q, 2
        while d*d <= n:
            while n % d == 0:
                ps.add(d)
                n //= d
            d += 1
        if n > 1:
            ps.add(n)
    places = []
    inf = psi(Adele(x.mid, x.rad, Ball(0, 0)))   # E(-I): base (-m) mod 1, radius r
    arcs, dist, phase = union([inf])
    places.append({'v': 'inf', 'phase': None if phase is None else s(phase), 'base': s((-x.mid) % 1),
                   'rad': s(x.rad), 'order': '1', 'arcs': None if arcs is None else [[s(a), s(b)] for a, b in arcs],
                   'dist': [s(d) for d in dist]})
    for p in sorted(ps | set(extra)):
        e = None
        if x.fin.N:
            e, n = 0, x.fin.N
            while n.numerator % p == 0:
                n /= p
                e += 1
            while n.denominator % p == 0:
                n *= p
                e -= 1
        r = local_record(p, x.fin.a, e)
        places.append({'v': str(p), 'phase': r['phase'], 'base': r['base'], 'rad': '0', 'order': r['order'],
                       'arcs': r['arcs'], 'dist': r['dist']})
    g = psi(x)
    if isinstance(g, F):
        total = sum((F(pl['phase']) for pl in places), F(0)) % 1
        assert total == g, (x, total, g)
    return {'x': ent(x), 'global': s(g) if isinstance(g, F) else None, 'places': places}


def places_():
    rows = []
    xs = [Adele(0, 0, Ball(F(1, 3), 0)), Adele(F(1, 4), 0, Ball(F(-1, 6), 6)), Adele(F(3, 8), 0, Ball(F(5, 12), 1)),
          Adele(F(-7, 16), 0, Ball(F(11, 360), 0)), Adele(0, F(1, 8), Ball(F(1, 6), 0)),
          Adele(F(1, 2), 0, Ball(F(1, 10), F(2, 3))), Adele(0, 0, Ball(F(1, 2), F(1, 2))),
          Adele(F(5, 4), F(1, 32), Ball(F(7, 30), F(4, 9))), Adele(0, 0, Ball(F(1, 2**30 * 3), F(5, 7))),
          Adele(F(1, 8), 0, Ball(F(-1, 65537*4), 65537))]
    rng = random.Random(320502)
    for _ in range(20):
        xs.append(Adele(F(rng.randrange(-64, 65), 16), rng.choice((F(0), F(0), F(1, 64))),
                        Ball(F(rng.randrange(-60, 61), rng.choice((1, 2, 3, 4, 6, 12, 30, 35))),
                             rng.choice((F(0), F(1), F(6), F(1, 2), F(2, 3), F(5, 6), F(3, 4))))))
    for x in xs:
        rows.append(place_record(x, (11, 13)))
    return rows


def write(name, rows):
    path = OUT/name
    with path.open('w') as f:
        for r in rows:
            f.write(json.dumps(r, separators=(',', ':')) + '\n')
    return path.stat().st_size


OUT.mkdir(parents=True, exist_ok=True)
sizes = {n: write(n, r) for n, r in (('class.jsonl', classes()), ('local.jsonl', locals_()),
                                       ('place.jsonl', places_()))}
total = sum(sizes.values())
assert total <= 400*1024, total
print(' '.join(f'{n}={sz}' for n, sz in sizes.items()), f'total={total}')
