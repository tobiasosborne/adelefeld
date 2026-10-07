#!/usr/bin/env python3
"""Lane m3-review1, hunt 1: set queries by brute force.
Own decision (not algorithm R): the class of (s, z), s in [0,1), z in Zhat, is in pi([lo,hi] x (c + N Zhat))
iff, for N = A/B > 0, some s + c - rho + k N lies in [lo,hi] (rho = z mod A, an integer; N Zhat cap Q = N Z),
and for N = 0 iff z is the integer e with s + c - e in [lo,hi]. Membership in s is a finite union of closed
intervals per residue rho mod L (L = lcm of the A) and per integer point e; testing every interval endpoint in
[0,1) and every gap midpoint decides inclusion and intersection. Second check: random rational points through
common.member (a different formula path). Budget expectation from my own R count (lanes/q-review3/model.py
R_construct, copied: same model family) with the design 2.3 definitions."""
import random
import sys
from fractions import Fraction as F
from math import floor, ceil, gcd
from common import Piece, enc_class, run, member

seed = int(sys.argv[1]) if len(sys.argv) > 1 else 1
NP = int(sys.argv[2]) if len(sys.argv) > 2 else 500
EXE = sys.argv[3] if len(sys.argv) > 3 else 'h'
rng = random.Random(seed)


def lcm(a, b):
    return a * b // gcd(a, b)


def R_construct(lo, hi, a, N):
    if N == 0:
        fib = [(a, 0)]
    else:
        A, B = N.numerator, N.denominator
        fib = [(a + F(j * A, B), A) for j in range(B)]
    out = []
    for aj, A in fib:
        l, h = lo - aj, hi - aj
        ns = [floor(l)] if l == h else range(floor(l), ceil(h))
        for n in ns:
            out.append((max(l, F(n)) - n, min(h, F(n + 1)) - n, (-n) % A if A else -n, A))
    return out


def budget(X, Y):
    K, L, ends = 0, 1, set()
    for (lo, hi, c, N) in X + Y:
        if N:
            L = lcm(L, N.numerator)
        for (l, h, m, A) in R_construct(lo, hi, c, N):
            K += 1
            ends.add(l)
            ends.add(h)
    return K, L, len(ends)


def intervals_pos(pieces, rho):
    """closed s-intervals (within [0,1]) where some positive piece contains (s, z), z = rho mod L."""
    out = []
    for (lo, hi, c, N) in pieces:
        if N == 0:
            continue
        base = c - rho
        # s in [lo - base - kN, hi - base - kN] meeting [0,1)
        k0 = ceil((lo - base - 1) / N)
        k1 = floor((hi - base) / N)
        for k in range(k0, k1 + 1):
            a, b = lo - base - k * N, hi - base - k * N
            if b >= 0 and a < 1:
                out.append((a, b))
    return out


def intervals_pt(pieces, e):
    out = []
    for (lo, hi, c, N) in pieces:
        if N != 0:
            continue
        a, b = lo - c + e, hi - c + e
        if b >= 0 and a < 1:
            out.append((a, b))
    return out


def inside(s, ivs):
    return any(a <= s <= b for a, b in ivs)


def samples(ivs1, ivs2):
    pts = {F(0)}
    for a, b in ivs1 + ivs2:
        for t in (a, b):
            if 0 <= t < 1:
                pts.add(t)
    pts = sorted(pts)
    out = list(pts)
    for i in range(len(pts)):
        nxt = pts[i + 1] if i + 1 < len(pts) else F(1)
        out.append((pts[i] + nxt) / 2)
    return out


def point_cands(pieces):
    es = set()
    for (lo, hi, c, N) in pieces:
        if N == 0:
            for e in range(ceil(c - hi), floor(c + 1 - lo) + 1):
                es.add(e)
    return es


def decide(X, Y):
    """returns (X sub Y witness or None, Y sub X witness or None, overlap witness or None)."""
    L = 1
    for (lo, hi, c, N) in X + Y:
        if N:
            L = lcm(L, N.numerator)
    wxy = wyx = wov = None
    # generic z
    for rho in range(L):
        ix, iy = intervals_pos(X, rho), intervals_pos(Y, rho)
        if not ix and not iy:
            continue
        for s in samples(ix, iy):
            a, b = inside(s, ix), inside(s, iy)
            if a and not b and wxy is None:
                wxy = (s, ('g', rho))
            if b and not a and wyx is None:
                wyx = (s, ('g', rho))
            if a and b and wov is None:
                wov = (s, ('g', rho))
    for e in point_cands(X) | point_cands(Y):
        ix = intervals_pos(X, e % L) + intervals_pt(X, e)
        iy = intervals_pos(Y, e % L) + intervals_pt(Y, e)
        for s in samples(ix, iy):
            a, b = inside(s, ix), inside(s, iy)
            if a and not b and wxy is None:
                wxy = (s, ('e', e))
            if b and not a and wyx is None:
                wyx = (s, ('e', e))
            if a and b and wov is None:
                wov = (s, ('e', e))
    return wxy, wyx, wov, L


def zpoint(w, L):
    kind, v = w
    return F(v + L * 10 ** 12) if kind == 'g' else F(v)


def check_witness(w, X, Y, inX, inY, L):
    s, z = w[0], zpoint(w[1], L)
    return member(s, z, X) == inX and member(s, z, Y) == inY


# ------------------------------------------------------------------ generation

MODS = [2, 3, 4, 5, 6, 8, 9, 10, 12, 360, 7, 11]


def half(n):
    return F(n, 2)


def rand_lift():
    a = half(rng.randint(-6, 6))
    b = a + half(rng.randint(0, 5))
    if rng.random() < 0.2:
        a, b = a + F(1, 4), b + F(1, 4) if b > a else a + F(1, 4)
    mid, rad = (a + b) / 2, (b - a) / 2
    if rad < 0:
        rad = -rad
    c = rng.random()
    if c < 0.25:
        N = F(0)
    elif c < 0.6:
        N = F(rng.choice(MODS[:10] if rng.random() < 0.8 else MODS))
    else:
        N = rng.choice([F(1, 2), F(2, 3), F(3, 2), F(5, 6), F(4, 3)])
    if rng.random() < 0.1:
        cc = F(rng.randint(-2 ** 2000, 2 ** 2000))
    else:
        cc = F(rng.randint(-20, 20), rng.choice([1, 1, 2, 3]))
    return Piece(mid, rad, cc, N)


def rand_piece_pieces():
    a = F(rng.randint(0, 4), 4)
    b = a + F(rng.randint(0, 4), 4) if rng.random() < 0.7 else a
    if rng.random() < 0.15:
        b = b + F(rng.randint(1, 3), 2)   # spill
    mid, rad = (a + b) / 2, (b - a) / 2
    if mid > 1:
        mid, rad = F(1), rad
    H = rng.choice([0, 0, 1, 2, 3, 4, 6, 12, 360])
    if H == 0:
        c = F(rng.randint(-5, 5)) if rng.random() < 0.9 else F(rng.randint(-2 ** 2000, 2 ** 2000))
    else:
        c = F(rng.randint(0, H - 1))
    return Piece(mid, rad, c, F(H))


def rand_class():
    if rng.random() < 0.5:
        return 0, [rand_lift()]
    n = rng.randint(1, 4)
    return 1, [rand_piece_pieces() for _ in range(n)]


def exact_pieces_of(form, pcs):
    out = []
    for p in (pcs if form else pcs[:1]):
        k = (p.lo, p.hi, p.c, p.N)
        if k not in out:      # set_pieces removes equal keys; the budget counts stored entries
            out.append(k)
    return out


def related(form, pcs):
    """A class equal to, inside, or around the given one."""
    t = rng.random()
    if t < 0.3 and form == 0:
        # rational translation by a dyadic q: same set
        p = pcs[0]
        q = F(rng.randint(-8, 8), 4)
        return 0, [Piece(p.mid + q, p.rad, p.c + q, p.N)]
    if t < 0.55:
        # exact R pieces as PIECES (equal set) when dyadic and few
        out = []
        for (lo, hi, c, N) in exact_pieces_of(form, pcs):
            for (l, h, m, A) in R_construct(lo, hi, c, N):
                mid, rad = (l + h) / 2, (h - l) / 2
                if mid.denominator & (mid.denominator - 1) or rad.denominator & (rad.denominator - 1):
                    return rand_class()
                out.append(Piece(mid, rad, F(m), F(A)))
        if not out or len(out) > 40:
            return rand_class()
        rng.shuffle(out)
        if rng.random() < 0.3 and len(out) > 1:
            out.pop()            # a strict subset (usually)
        return 1, out
    if t < 0.75:
        # shrink / modulus multiple / add a piece
        f2, p2 = form, [Piece(p.mid, p.rad, p.c, p.N) for p in pcs]
        p = rng.choice(p2)
        if rng.random() < 0.5 and p.rad > 0:
            p.rad = p.rad / 2
        elif p.N != 0:
            p.N = p.N * rng.choice([2, 3])
            if f2 == 1:
                p.c = p.c % p.N
        if f2 == 1:
            if rng.random() < 0.5:
                p2.append(rand_piece_pieces())
        return f2, p2
    return rand_class()


def main():
    lines, cases = [], []
    i = 0
    while len(cases) < NP:
        fx, X = rand_class()
        fy, Y = related(fx, X) if rng.random() < 0.6 else rand_class()
        if rng.random() < 0.5:
            fx, X, fy, Y = fy, Y, fx, X
        ex, ey = exact_pieces_of(fx, X), exact_pieces_of(fy, Y)
        K, L, E = budget(ex, ey)
        if K > 3000 or L > 3000:
            continue
        need = (2 * E + 1) * K * L
        c = rng.random()
        if c < 0.7:
            W = 10 ** 12
        elif c < 0.8:
            W = need
        elif c < 0.9:
            W = need - 1
        else:
            W = rng.choice([K, K - 1, L, L - 1, max(K, L)])
        expect_limit = K > W or L > W or need > W
        for kind in (0, 1, 2):
            lines.append('Q %d %d %s %s' % (kind, W, enc_class(fx, X), enc_class(fy, Y)))
        cases.append((fx, X, fy, Y, ex, ey, W, expect_limit, (K, L, E)))
    out, err = run(lines, EXE)
    assert len(out) == 3 * len(cases), (len(out), len(cases))
    bad = stats = 0
    cnt = dict(eq=0, sub=0, ov=0, limit=0, skipped=0, truths=[0, 0, 0])
    for j, (fx, X, fy, Y, ex, ey, W, expect_limit, KLE) in enumerate(cases):
        res = [out[3 * j + k].split() for k in range(3)]
        if res[0][0] == 'X':
            cnt['skipped'] += 1
            continue
        sts = [int(r[1]) for r in res]
        if expect_limit:
            cnt['limit'] += 1
            for k, r in enumerate(res):
                if int(r[1]) != 10 or r[3] != '1':
                    print('FINDING budget expected LIMIT', k, r, KLE, W, enc_class(fx, X), enc_class(fy, Y))
                    bad += 1
            continue
        if any(s != 0 for s in sts):
            print('FINDING budget LIMIT within budget', res, KLE, W, enc_class(fx, X), enc_class(fy, Y))
            bad += 1
            continue
        wxy, wyx, wov, L = decide(ex, ey)
        want = [int(wxy is None and wyx is None), int(wxy is None), int(wov is not None)]
        got = [int(r[2]) for r in res]
        for k in range(3):
            cnt['truths'][k] += want[k]
        if want != got:
            print('FINDING truth want', want, 'got', got, enc_class(fx, X), '|', enc_class(fy, Y))
            bad += 1
        # witnesses through the other formula path
        if wxy is not None and not check_witness(wxy, ex, ey, True, False, L):
            print('MODEL witness xy fails', wxy)
        if wyx is not None and not check_witness(wyx, ex, ey, False, True, L):
            print('MODEL witness yx fails', wyx)
        if wov is not None and not check_witness(wov, ex, ey, True, True, L):
            print('MODEL witness ov fails', wov)
        # random point sampling
        for _ in range(30):
            s = F(rng.randint(0, 999), 1000)
            z = F(rng.randint(-30, 30) + (L * 10 ** 12 if rng.random() < 0.5 else 0))
            a, b = member(s, z, ex), member(s, z, ey)
            if (a and not b and wxy is None) or (b and not a and wyx is None) or (a and b and wov is None):
                print('MODEL sample contradicts decision', s, z, enc_class(fx, X), enc_class(fy, Y))
    print('pairs', len(cases), cnt, 'findings', bad)


main()
