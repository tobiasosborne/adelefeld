#!/usr/bin/env python3
"""Lane m3-review1, hunt 2: arithmetic by membership and by exact construction.
Own construction of x + y: pair every stored entry, [lo1+lo2, hi1+hi2] x ((c1+c2) + gcd(N1,N2) Zhat) with the
gcd of rationals gcd(a/b, c/d) = gcd(ad, cb)/(bd) (precision.md P1:27 cites it; here computed directly); -x:
[-hi,-lo] x (-c + N Zhat). Then R and Q1 from qr3_model.py (copied from lanes/q-review3/model.py).
Membership: rational points u of x and v of y (stored balls, exact), u+v and -u must lie in pi of a stored piece of
the output (common.member: exact A/Q membership)."""
import random
import sys
from fractions import Fraction as F
from math import gcd, floor, ceil
from common import Piece, enc_class, run, member, parse_S
from qr3_model import expected_store, R_count

seed = int(sys.argv[1]) if len(sys.argv) > 1 else 1
NP = int(sys.argv[2]) if len(sys.argv) > 2 else 300
EXE = sys.argv[3] if len(sys.argv) > 3 else 'h'
NPTS = int(sys.argv[4]) if len(sys.argv) > 4 else 60
rng = random.Random(seed)


def qgcd(a, b):
    a, b = F(a), F(b)
    if a == 0:
        return abs(b)
    if b == 0:
        return abs(a)
    return F(gcd(a.numerator * b.denominator, b.numerator * a.denominator), a.denominator * b.denominator)


def rand_entry(pieces_form):
    if pieces_form:
        a = F(rng.randint(0, 8), 8)
        b = a + (F(rng.randint(0, 8), 8) if rng.random() < 0.7 else F(rng.randint(0, 2 ** 20), 2 ** 20))
        if rng.random() < 0.1:
            b += 1
        mid, rad = (a + b) / 2, (b - a) / 2
        mid = min(mid, F(1))
        H = rng.choice([0, 0, 1, 2, 3, 4, 6, 12, 360])
        c = F(rng.randint(-9, 9)) if H == 0 else F(rng.randint(0, H - 1))
        if H == 0 and rng.random() < 0.05:
            c = F(rng.randint(-2 ** 2000, 2 ** 2000))
        return Piece(mid, rad, c, F(H))
    e = rng.choice([1, 2, 4, 8, 2 ** 20, 2 ** 53])
    mid = F(rng.randint(-8 * e, 8 * e), e)
    rad = F(rng.randint(0, 3 * 2 ** 10), 2 ** rng.choice([0, 1, 3, 10])) if rng.random() < 0.8 else F(0)
    t = rng.random()
    if t < 0.25:
        N = F(0)
    elif t < 0.6:
        N = F(rng.choice([1, 2, 3, 4, 5, 6, 8, 12, 360]))
    else:
        N = rng.choice([F(1, 2), F(2, 3), F(3, 4), F(5, 6), F(7, 12), F(1, 360), F(3, 2)])
    c = F(rng.randint(-40, 40), rng.choice([1, 1, 2, 3, 4, 6, 7]))
    return Piece(mid, rad, c, N)


def rand_class():
    if rng.random() < 0.5:
        return 0, [rand_entry(False)]
    return 1, [rand_entry(True) for _ in range(rng.randint(1, 3))]


def stored(form, pcs):
    out = []
    for p in (pcs if form else pcs[:1]):
        k = (p.lo, p.hi, p.c, p.N)
        if k not in out:
            out.append(k)
    return out


def sample_point(ent):
    lo, hi, c, N = ent
    s = lo + (hi - lo) * F(rng.randint(0, 64), 64)
    if rng.random() < 0.3:
        s = rng.choice([lo, hi])
    z = c + N * rng.randint(-50, 50)
    return s, z


def main():
    lines, cases = [], []
    while len(cases) < NP:
        fx, X = rand_class()
        fy, Y = rand_class()
        if rng.random() < 0.15:
            fy, Y = fx, X               # independent variation x + x
        ex, ey = stored(fx, X), stored(fy, Y)
        srcs = [(l1 + l2, h1 + h2, c1 + c2, qgcd(N1, N2)) for (l1, h1, c1, N1) in ex for (l2, h2, c2, N2) in ey]
        nsrcs = [(-h, -l, -c, N) for (l, h, c, N) in ex]
        K = sum(R_count(*s) for s in srcs)
        Kn = sum(R_count(*s) for s in nsrcs)
        if K > 4000:
            continue
        prec = rng.choice([2, 20, 53, 128])
        t = rng.random()
        lim = 10 ** 6 if t < 0.8 else rng.choice([K, K - 1, Kn, Kn - 1, len(ex) * len(ey), 1, 0])
        lines.append('A %d %d %s %s' % (lim, prec, enc_class(fx, X), enc_class(fy, Y)))
        lines.append('N %d %d %s' % (lim, prec, enc_class(fx, X)))
        cases.append((fx, X, fy, Y, ex, ey, srcs, nsrcs, K, Kn, lim, prec))
    out, err = run(lines, EXE)
    # split
    groups, cur = [], []
    for ln in out:
        if ln[0] in 'RX' and cur and cur[0][0] in 'RX' and not (ln[0] == 'R' and False):
            pass
        cur.append(ln)
    # re-parse sequentially
    i = 0
    bad = 0
    st_counts = {}
    npts = 0
    for (fx, X, fy, Y, ex, ey, srcs, nsrcs, K, Kn, lim, prec) in cases:
        for op in ('A', 'N'):
            head = out[i].split(); i += 1
            if head[0] == 'X':
                print('skip', head)
                if op == 'A':
                    pass
                continue
            st, ln_, canon, untouched, al = [int(v) for v in head[1:6]]
            S = []
            if st == 0:
                for _ in range(ln_):
                    S.append(out[i].split()); i += 1
            W = None
            if op == 'A':
                W = out[i].split(); i += 1
                Wst = int(W[1])
                WS = []
                if Wst == 0:
                    lenw = None
                    while i < len(out) and out[i].startswith('S'):
                        WS.append(out[i].split()); i += 1
            KK = K if op == 'A' else Kn
            sources = srcs if op == 'A' else nsrcs
            st_counts[(op, st)] = st_counts.get((op, st), 0) + 1
            exp_limit = lim < 1 or KK > lim
            if exp_limit:
                if st != 10 or not untouched:
                    print('FINDING expected LIMIT', op, st, untouched, KK, lim, enc_class(fx, X), enc_class(fy, Y))
                    bad += 1
                continue
            if st != 0:
                print('FINDING unexpected status', op, st, KK, lim, enc_class(fx, X), enc_class(fy, Y))
                bad += 1
                continue
            if not canon or not al:
                print('FINDING canon/alias', op, canon, al, enc_class(fx, X), enc_class(fy, Y))
                bad += 1
            cnt, exp, _ = expected_store(sources, prec)
            got = [(F(int(s[1]), int(s[2])), F(int(s[3]), int(s[4])), F(int(s[5])), F(int(s[6])), int(s[7])) for s in S]
            want = [(m, rho, F(c), F(A), 1) for (m, rho, c, A, d) in exp]
            if got != want:
                print('FINDING construction', op, prec, enc_class(fx, X), '|', enc_class(fy, Y))
                print('   got ', got[:6])
                print('   want', want[:6])
                bad += 1
            outp = [parse_S(s)[:4] for s in S]
            # membership
            for _ in range(NPTS):
                su, zu = sample_point(rng.choice(ex))
                if op == 'A':
                    sv, zv = sample_point(rng.choice(ey))
                    s, z = su + sv, zu + zv
                else:
                    s, z = -su, -zu
                npts += 1
                if not member(s, z, outp):
                    print('FINDING membership', op, s, z, enc_class(fx, X), '|', enc_class(fy, Y), prec)
                    bad += 1
                    break
            if op == 'A' and int(W[1]) == 0:
                if W[3] != '1':
                    print('FINDING alias z=x=y', W, enc_class(fx, X))
                    bad += 1
    print('cases', len(cases), 'statuses', sorted(st_counts.items()), 'points', npts, 'findings', bad)


main()
