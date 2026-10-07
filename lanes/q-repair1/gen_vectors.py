"""Lane q-repair1: vectors whose upper real end is an integer plus a tiny fraction (review q-review3, R1).

Oracle: proto/quotient3_checks.py (exact Fraction algorithm R, Q1, lift membership), as in
lanes/q-slice2/gen_vectors.py. Unlike that generator, the input midpoint is NOT rounded to 53 bits:
the stored real ball is exactly [hi - W, hi] with hi = t + K + eps, eps in {2^-200, 2^-101, 2^-99}
and t a dyadic fibre centre a_j = a + j A/B, so that h_j = hi - a_j = K + eps.
Run from the worktree root: timeout 60 python3 lanes/q-repair1/gen_vectors.py
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


def is_dyadic(q):
    d = F(q).denominator
    return d & (d-1) == 0


def vector(lo, hi, a, N, prec):
    m, r = (lo+hi)/2, (hi-lo)/2
    assert is_dyadic(m) and is_dyadic(r)
    assert r == o.round_binary(r, 30, True)  # the stored radius is an exact 30-bit mag
    a = a % N if N else a
    x = o.Adele(m, r, o.Ball(a, N))
    ps = o.reduce(m, r, x.fin)
    B = N.denominator if N else 1
    raw = 0
    for j in range(B):
        l, h = lo-a-j*N, hi-a-j*N
        raw += 1 if l == h else ceil(h)-floor(l)
    rounded = sorted({o.round_piece(p, prec) for p in ps}, key=o.piece_key)
    # Input end points of every fibre first, then the pieces' ends and midpoints, then a grid.
    points = [(s, a+j*N) for j in range(B) for s in (hi, lo)]
    for p in ps:
        for s in (p.lo, p.hi, (p.lo+p.hi)/2):
            points.append((s, F(p.m)))
    points += [(F(0), F(w)) for w in range(-3, 4)]
    points += [(F(j % 17, 16), F((j*7) % 13-6, (1, 2, 3, 4, 6)[j % 5])) for j in range(40)]
    points = list(dict.fromkeys(points))[:40]
    assert len(points) == 40
    labels = [o.lift_member(x, s, w) for s, w in points]
    assert all(labels[:2*B]) or len(points) < 2*B
    return dict(source=[str(lo), str(hi)], mid=str(m), rad=str(r), lo=str(lo), hi=str(hi),
                a=str(a), N=str(N), prec=prec, raw=raw,
                exact=[key(p) for p in ps], rounded=[key(p) for p in rounded],
                points=[[str(s), str(w), b] for (s, w), b in zip(points, labels)])


# (centre, finite radius); the dyadic fibre centre t is found below.
finite = [(F(0), F(0)), (F(-3), F(0)), (F(0), F(1)), (F(1, 2), F(3)), (F(-3), F(2)),
          (F(0), F(1, 2)), (F(1, 3), F(2, 3)), (F(-5, 4), F(1, 4)), (F(0), F(3, 2)), (F(7), F(12))]
short = [F(1, 2), F(3, 4), F(1, 2**20)]
long_ = [F(5), F(17), F(40), F(23, 2)]
tiny = [F(1, 2**200), F(1, 2**101), F(1, 2**99)]
rows = []
# Review q-review3 R1, smallest input: [-2^-212, 2^-210] x {0}, prec 53.
rows.append(vector(-F(1, 2**212), F(1, 2**210), F(0), F(0), 53))
for i, (a, N) in enumerate(finite):
    B = N.denominator if N else 1
    ac = a % N if N else a
    centres = [ac+j*N for j in range(B) if is_dyadic(ac+j*N)]
    t = centres[-1]  # the last dyadic fibre centre: a later fibre where B > 1
    for k, eps in enumerate(tiny):
        for kind, widths in (('short', short), ('long', long_)):
            W = widths[(i+k) % len(widths)]
            K = (0, 3, -2, 1)[(i+2*k) % 4]
            hi = t+K+eps
            prec = (53, 300, 20)[(i+k+len(kind)) % 3]
            rows.append(vector(hi-W, hi, a, N, prec))
path = root/'tests/ref/vectors/q-repair1/tiny_frac.jsonl'
path.write_text(''.join(json.dumps(v, separators=(',', ':'))+'\n' for v in rows))
print(len(rows), 'vectors,', sum(v['raw'] for v in rows), 'raw pieces,', path.stat().st_size, 'bytes')
assert path.stat().st_size < 500_000
