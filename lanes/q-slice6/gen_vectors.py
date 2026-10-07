#!/usr/bin/env python3
"""Exact Q2 vectors, with direct membership and q-review3 containment cross-checks.

Run from the repository root under timeout. No C library is used.
E counts distinct endpoints of exact R pieces; artificial cell boundaries are not counted.
"""
import json
import random
import sys
from fractions import Fraction as F
from math import ceil, floor, lcm
from pathlib import Path

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path('proto').resolve()))
import quotient3_checks as q
sys.path.insert(0, str(Path('lanes/q-review3').resolve()))
import model


def piece(lo, hi, a=0, N=1):
    return tuple(map(F, (lo, hi, a, N)))


def family(ps):
    return [q.Piece(lo, hi, int(a), int(N)) for lo, hi, a, N in ps]


def exact(ps):
    return [p for lo, hi, a, N in ps
            for p in q.reduce((lo+hi)/2, (hi-lo)/2, q.Ball(a, N))]


def stored(ps, form):
    out = []
    for lo, hi, a, N in ps:
        A, H, d = model.canon_fin(a, N)
        a, N = F(A, d), F(H, d)
        mid, rad = (lo+hi)/2, (hi-lo)/2
        assert mid.denominator & (mid.denominator-1) == 0
        assert rad.denominator & (rad.denominator-1) == 0
        assert rad.numerator.bit_length() <= 30
        out.append(dict(mid=str(mid), rad=str(rad), lo=str(lo), hi=str(hi), a=str(a), N=str(N)))
    if form:
        out.sort(key=lambda p: (F(p['lo']), F(p['hi']), F(p['N']), F(p['a'])))
        assert all(0 <= F(p['mid']) <= 1 and F(p['N']).denominator == 1 for p in out)
    return dict(form=form, pieces=out)


def make(name, x, y, fx=0, fy=0):
    # Canonical finite centres are the exact same cosets as the original centres.
    ix, iy = stored(x, fx), stored(y, fy)
    x = [piece(p['lo'], p['hi'], p['a'], p['N']) for p in ix['pieces']]
    y = [piece(p['lo'], p['hi'], p['a'], p['N']) for p in iy['pieces']]
    nx, ny = exact(x), exact(y)
    X, Y = nx, ny
    xy, yx = q.compare(X, Y), q.compare(Y, X)
    # For integer finite parts the brute oracle also reads unreduced spill and huge centres.
    bx = family(x) if all(a.denominator == N.denominator == 1 for _, _, a, N in x) else X
    by = family(y) if all(a.denominator == N.denominator == 1 for _, _, a, N in y) else Y
    L = lcm(*(p.N for p in X+Y if p.N))
    wmax = 2*L+2+max((abs(p.m) for p in X+Y if not p.N), default=0)
    assert wmax < 3000
    b, br = q.brute_sets(bx, by, wmax), q.brute_sets(by, bx, wmax)
    assert xy == b[:3] and yx == br[:3], name
    # Independent interval covering; target pieces have integral finite radii.
    target_y = [(p.lo, p.hi, F(p.m), F(p.N)) for p in Y]
    target_x = [(p.lo, p.hi, F(p.m), F(p.N)) for p in X]
    cx = all(model.contains_slow(target_y, *p)[0] for p in x)
    cy = all(model.contains_slow(target_x, *p)[0] for p in y)
    assert (cx, cy) == (xy[1], yx[1]), name
    K = sum(model.R_count(*p) for p in x+y)
    E = len({t for p in X+Y for t in (p.lo, p.hi)})
    samples = []
    for s in q.cells(X+Y)[:5]:
        w = len(samples)-2
        samples.append([str(s), str(w), q.direct_member(bx, s, w), q.direct_member(by, s, w)])
    return dict(name=name, x=ix, y=iy, xy=list(xy), yx=list(yx), brute=list(b[:3]),
                K=K, L=L, E=E, budget=(2*E+1)*K*L, witnesses=b[3]+br[3], points=samples)


def main():
    out = []
    def add(name, x, y, fx=0, fy=0):
        out.append(make(name, x, y, fx, fy))
    for N in range(2, 9):
        for c in range(N):
            add(f'glue-{N}-{c}', [piece(F(1, 2), 1, c, N)],
                [piece(0, F(1, 2), c-1, N)])
            if N > 2:
                add(f'wrong-glue-{N}-{c}', [piece(F(1, 2), 1, c, N)],
                    [piece(0, F(1, 2), c+1, N)])
    for N in (2, 3, 4, 6, 12, 360):
        x = [piece(F(1, 4), F(3, 4), 0, N)]
        add(f'refine-{N}', x, [piece(F(1, 4), F(3, 4), 0, 2*N),
                             piece(F(1, 4), F(3, 4), N, 2*N)], 0, 1)
        add(f'one-way-{N}', [piece(F(3, 8), F(5, 8), 0, 2*N)], x)
        add(f'boundary-{N}', [piece(0, F(1, 2), 0, N)], [piece(F(1, 2), 1, 0, N)])
        add(f'point-in-{N}', [piece(F(1, 2), F(1, 2), -N, 0)], x)
        add(f'point-out-{N}', [piece(F(1, 2), F(1, 2), -1, 0)], x)
        add(f'samples-not-coset-{N}', [piece(F(1, 2), F(1, 2), 0, N)],
            [piece(F(1, 2), F(1, 2), k*N, 0) for k in range(-2, 3)], 0, 1)
    for j, x in enumerate(([piece(-F(1, 8), F(1, 8), 1, 3)],
                           [piece(F(7, 8), F(9, 8), -7, 0)],
                           [piece(-1, 2, -2, 4)], [piece(F(1, 2), F(1, 2), 0, 3)])):
        nx = exact(x)
        ps = [(p.lo, p.hi, F(p.m), F(p.N)) for p in nx]
        add(f'lift-exact-reduction-{j}', x, ps, 0, 1)
        add(f'piece-order-{j}', ps, list(reversed(ps)), 1, 1)
    for N in (F(1, 2), F(2, 3), F(7, 360)):
        x = [piece(-F(1, 8), F(1, 8), -F(5, 3), N)]
        add(f'fractional-{N}', x, [piece(0, 1, 0, 1)])
    huge = 2**2000+17
    for N in (0, 3, 12):
        add(f'huge-{N}', [piece(huge-F(1, 4), huge+F(1, 4), huge, N)],
            [piece(-F(1, 4), F(1, 4), 0, N)])
    # A real gap below 2^-100 is visible to exact queries, despite coarse real rounding.
    add('exact-gap', [piece(F(1, 2), F(1, 2), 0, 3)],
        [piece(F(1, 2)+F(1, 2**100), F(1, 2)+F(1, 2**100), 0, 3)])
    rng = random.Random(310600)
    for i in range(160):
        fs = []
        for side in range(2):
            ps = []
            for _ in range(rng.randrange(1, 4)):
                mid = F(rng.randrange(9), 8)
                rad = F(rng.randrange(9), 8)
                N = rng.choice((0, 1, 2, 3, 4, 6, 12))
                a = rng.randrange(N) if N else rng.randrange(-5, 6)
                ps.append(piece(mid-rad, mid+rad, a, N))
            ps = sorted(set(ps), key=lambda p: (p[0], p[1], p[3], p[2]))
            fs.append(ps)
        add(f'random-{i}', *fs, 1, 1)
    path = Path('tests/ref/vectors/q-slice6')
    path.mkdir(parents=True, exist_ok=True)
    data = ''.join(json.dumps(v, separators=(',', ':'))+'\n' for v in out)
    assert len(data.encode()) <= 400_000
    (path/'sets.jsonl').write_text(data)
    # Huge B is recorded without asking the unbounded oracle to enumerate it.
    (path/'limits.jsonl').write_text(json.dumps(dict(
        x=stored([piece(0, 0, 0, F(1, 10**100))], 0), B=str(10**100), limit=1000))+'\n')
    print(f'{len(out)} vectors, {len(data.encode())} bytes, '
          f'{sum(v["witnesses"] for v in out)} direct membership pairs, '
          f'{2*len(out)} independent containment checks; 1 huge-B refusal')


if __name__ == '__main__':
    main()
