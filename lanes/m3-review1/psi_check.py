#!/usr/bin/env python3
"""Lane m3-review1: the character, hunts 1-4 of lanes/q-review4/brief.md extended to classes, lballs, places.
Own model: fp_p from its definition (conventions 6.1:840-842; notes.txt:695-700), theta = -x_inf + sum_p fp_p(z)
mod 1 (notes.txt:693-694 for the real sign). Enclosure of exact point phases decided with mpmath at 700 digits;
equality within 1e-650 only occurs at rational cosines (0, +-1/2, +-1) and counts as contained (closed box).
Hull of B <= 400 images by enumeration of arcs (not by the Q4 formula)."""
import random
import sys
from fractions import Fraction as F
from math import floor, ceil, gcd
import mpmath
from common import Piece, enc_class, run

mpmath.mp.dps = 130
TOL = mpmath.mpf(10) ** -115
rng = random.Random(int(sys.argv[1]) if len(sys.argv) > 1 else 1)
NCASES = int(sys.argv[2]) if len(sys.argv) > 2 else 2000
EXE = sys.argv[3] if len(sys.argv) > 3 else 'h'


def factor_small(n):
    out, p = [], 2
    while p * p <= n:
        if n % p == 0:
            out.append(p)
            while n % p == 0:
                n //= p
        p += 1
    if n > 1:
        out.append(n)
    return out


def fp(q, p):
    """fp_p(q): unique element of Z[1/p] in [0,1) with q - fp in Z_p. Definition, solved by search for
    small p^h, else by Bezout (the q - fp in Z_p condition is checked afterwards in both cases)."""
    q = F(q)
    d = q.denominator
    h = 0
    while d % p == 0:
        d //= p
        h += 1
    if h == 0:
        return F(0)
    P = p ** h
    if P <= 2000:
        sols = [k for k in range(P) if (q - F(k, P)).denominator % p != 0]
        assert len(sols) == 1
        r = F(sols[0], P)
    else:
        k = (q.numerator * pow(d, -1, P)) % P
        r = F(k, P)
    assert (q - r).denominator % p != 0 and 0 <= r < 1
    return r


def fsum(q, primes):
    """sum over p of fp_p(q) mod 1, primes the primes of den(q) (given)."""
    t = sum((fp(q, p) for p in primes), F(0))
    return t - floor(t)


def frac(t):
    return t - floor(t)


def inbox(t, box):
    """E(t) in the closed box (re_lo, re_hi, im_lo, im_hi) (mp values)."""
    c = mpmath.cos(2 * mpmath.pi * mpmath.mpf(t.numerator) / t.denominator)
    s = mpmath.sin(2 * mpmath.pi * mpmath.mpf(t.numerator) / t.denominator)
    rl, rh, il, ih = box
    return (c >= rl - TOL and c <= rh + TOL and s >= il - TOL and s <= ih + TOL), (c, s)


def box_of(tok, off):
    rm = F(int(tok[off]), int(tok[off + 1]))
    rr = F(int(tok[off + 2]), int(tok[off + 3]))
    im = F(int(tok[off + 4]), int(tok[off + 5]))
    ir = F(int(tok[off + 6]), int(tok[off + 7]))
    mp = lambda v: mpmath.mpf(v.numerator) / v.denominator
    return (mp(rm - rr), mp(rm + rr), mp(im - ir), mp(im + ir)), (rr, ir)


def true_hull(arcs):
    """arcs: list of (centre angle b, half width r), small number. Exact extrema by enumeration."""
    best = None
    vals = []
    for b, r in arcs:
        if 2 * r >= 1:
            return (mpmath.mpf(-1), mpmath.mpf(1), mpmath.mpf(-1), mpmath.mpf(1))
        lo, hi = b - r, b + r
        cand = [lo, hi]
        for q in (0, 1, 2, 3):
            t = F(q, 4)
            k = ceil(lo - t)
            if t + k <= hi:
                cand.append(t + k)
        for t in cand:
            a = 2 * mpmath.pi * mpmath.mpf(t.numerator) / t.denominator
            vals.append((mpmath.cos(a), mpmath.sin(a)))
    return (min(v[0] for v in vals), max(v[0] for v in vals), min(v[1] for v in vals), max(v[1] for v in vals))


def rand_den():
    c = rng.random()
    if c < 0.5:
        return rng.choice([1, 2, 3, 4, 6, 8, 12, 360, 5, 7, 9, 25, 27, 1024])
    if c < 0.8:
        p = rng.choice([2, 3, 5, 7, 11, 13])
        return p ** rng.randint(1, {2: 20, 3: 12, 5: 8, 7: 7, 11: 5, 13: 5}[p])
    return rng.choice([1, 2, 6, 360]) * rng.choice([1, 3 ** 40, 2 ** 64 + 13, 10 ** 30])


def rand_piece(lift=True):
    # real part
    c = rng.random()
    if c < 0.2:
        mid = F(0)
    elif c < 0.4:
        mid = F(rng.randint(-50, 50))
    elif c < 0.7:
        e = rng.randint(-200, 200)
        mid = F(rng.randint(-2 ** 40, 2 ** 40)) * F(2) ** e
    else:
        mid = F(rng.randint(-2 ** 10, 2 ** 10), 2 ** rng.randint(0, 12))
    c = rng.random()
    if c < 0.4:
        rad = F(0)
    elif c < 0.7:
        rad = F(rng.randint(1, 2 ** 29), 2 ** rng.randint(20, 80))
    elif c < 0.9:
        rad = F(rng.randint(1, 2 ** 10), 2 ** rng.randint(0, 14))
    else:
        rad = F(rng.choice([1, 3, 5]), 2 ** rng.randint(0, 3)) * 2 ** rng.randint(0, 4)
    # finite part
    d = rand_den()
    if rng.random() < 0.1:
        num = rng.randint(-2 ** 2000, 2 ** 2000)
    else:
        num = rng.randint(-10 ** 6, 10 ** 6)
    cc = F(num, d)
    c = rng.random()
    if c < 0.35:
        N = F(0)
    elif c < 0.65:
        N = F(rng.choice([1, 2, 3, 4, 6, 12, 360, 7]))
    else:
        B = rng.choice([2, 3, 4, 6, 12, 360, 5, 8]) if rng.random() < 0.9 else rng.choice([2 ** 64 + 13, 3 ** 50])
        A = rng.choice([1, 2, 5, 7, 1, 1])
        while gcd(A, B) != 1:
            A += 1
        N = F(A, B)
    bk = 0
    if rng.random() < 0.2:
        d = rng.choice([1, 2, 7, 12, 49, 11, 3])
        cc, N, bk = F(rng.randint(-10 ** 6, 10 ** 6), d), F(360, d), 1
    return Piece(mid, rad, cc, N, bk)


def phases_of_piece(pc, nreal=6, nfin=8):
    """Exact sample phases of points of pi(piece). Returns list of rational angles t (E(t))."""
    lo, hi = pc.lo, pc.hi
    pts = [lo, hi, (lo + hi) / 2]
    for _ in range(nreal):
        pts.append(lo + (hi - lo) * F(rng.randint(0, 1000), 1000))
    N = pc.N
    ks = [0, 1, -1]
    if N != 0:
        B = N.denominator
        ks += [rng.randint(-10 ** 6, 10 ** 6) for _ in range(nfin)]
        if B <= 400:
            ks += list(range(B))
    out = []
    for k in ks:
        z = pc.c + N * k
        prs = factor_small(z.denominator) if z.denominator < 10 ** 12 else None
        if prs is None:
            fz = frac(z)  # triviality on Q (checked on small denominators below)
        else:
            fz = fsum(z, prs)
            assert fz == frac(z), (z, fz)   # analysis Lemma 2 triviality, own fp
        # extremal real points: theta in {0,1/4,1/2,3/4}
        extra = []
        for q in range(4):
            # s with fz - s = q/4 mod 1, s in [lo,hi]
            s0 = fz - F(q, 4)
            j = ceil(lo - s0)
            if s0 + j <= hi:
                extra.append(s0 + j)
        for s in pts + extra:
            out.append(frac(fz - s))
    return out


def main():
    lines, cases = [], []
    for i in range(NCASES):
        prec = rng.choice([2, 20, 53, 128, 300])
        if rng.random() < 0.7:
            pcs = [rand_piece()]
            form = 0
        else:
            # PIECES: mid in [0,1], integer finite centre in [0,H), H integer
            pcs = []
            for _ in range(rng.randint(1, 4)):
                mid = F(rng.randint(0, 2 ** 12), 2 ** 12)
                rad = rng.choice([F(0), F(1, 2 ** rng.randint(1, 40)), F(rng.randint(1, 2 ** 20), 2 ** 20)])
                H = rng.choice([0, 1, 2, 3, 12, 360])
                c = F(rng.randint(-50, 50)) if H == 0 else F(rng.randint(0, H - 1))
                pcs.append(Piece(mid, rad, c, H))
            form = 1
        lines.append('P %d %s' % (prec, enc_class(form, pcs)))
        cases.append((prec, form, pcs))
    out, err = run(lines, EXE)
    # group output by case
    it = iter(out)
    bad = 0
    stats = dict(ok=0, nd=0, other=0, pts=0, hull=0)
    cur = []
    groups = []
    for ln in out:
        if ln.startswith(('C', 'X')) and cur:
            groups.append(cur)
            cur = []
        cur.append(ln)
    groups.append(cur)
    assert len(groups) == len(cases), (len(groups), len(cases))
    for (prec, form, pcs), g in zip(cases, groups):
        if g[0].startswith('X'):
            stats['other'] += 1
            continue
        rec = {l.split()[0]: l.split() for l in g}
        # canonical finite of each piece (as stored), exactness
        frac_rad = any(p.N.denominator > 1 for p in pcs)
        exact_all = all(p.rad == 0 and p.N.denominator == 1 for p in pcs)
        phases = []
        for p in pcs:
            phases += phases_of_piece(p)
        # sets of distinct singleton phases
        sing = set()
        for p in pcs:
            if p.rad == 0 and p.N.denominator == 1:
                sing.add(frac(p.c - p.mid))
        for tag, strict in (('C', 0), ('K', 1), ('D', 0), ('E', 1)):
            if tag not in rec:
                continue
            t = rec[tag]
            st, untouched = int(t[1]), int(t[2])
            expect_nd = strict and frac_rad
            if expect_nd:
                if st != 1 or not untouched:
                    print('FINDING strict', tag, st, untouched, g[0][:0], [p.enc() for p in pcs], prec)
                    bad += 1
                continue
            if st != 0:
                print('FINDING status', tag, st, [p.enc() for p in pcs], prec)
                bad += 1
                continue
            box, (rr, ir) = box_of(t, 3)
            for th in phases:
                ok, cs = inbox(th, box)
                stats['pts'] += 1
                if not ok:
                    print('FINDING enclosure', tag, th, [p.enc() for p in pcs], prec, cs, box)
                    bad += 1
                    break
            # hull tightness when enumerable
            arcs = []
            enum = True
            for p in pcs:
                B = p.N.denominator
                if B > 400:
                    enum = False
                    break
                b = frac(p.c - p.mid)
                for k in range(B):
                    arcs.append((b + F(k, B), p.rad))
            if enum:
                T = true_hull(arcs)
                eps = mpmath.mpf(2) ** (-max(prec, 2))
                for coord in (0, 1):
                    tl, th_ = T[2 * coord], T[2 * coord + 1]
                    W = th_ - tl
                    allow = 4 * eps + mpmath.mpf(2) ** -28 * (W / 2 + 2 * eps)
                    sl, sh = box[2 * coord], box[2 * coord + 1]
                    if sh - th_ > allow + TOL or tl - sl > allow + TOL:
                        print('FINDING hull', tag, coord, [p.enc() for p in pcs], prec,
                              mpmath.nstr(sh - th_, 5), mpmath.nstr(tl - sl, 5), mpmath.nstr(allow, 5))
                        bad += 1
                    if sh < th_ - TOL or sl > tl + TOL:
                        print('FINDING hull-enclosure', tag, coord, [p.enc() for p in pcs], prec)
                        bad += 1
                stats['hull'] += 1
            stats['ok'] += 1
        # exact phase getters
        for tag in ('H', 'G', 'F'):
            if tag not in rec:
                continue
            t = rec[tag]
            st, untouched = int(t[1]), int(t[2])
            if tag == 'F':
                p = pcs[0]
                want = frac(p.c) if p.N.denominator == 1 else None
            elif tag == 'G':
                p = pcs[0]
                want = frac(p.c - p.mid) if (p.rad == 0 and p.N.denominator == 1) else None
            else:
                want = sing.pop() if (exact_all and len(sing) == 1) else None
            if want is None:
                if st != 1 or not untouched:
                    print('FINDING phase-nd', tag, st, untouched, [p.enc() for p in pcs])
                    bad += 1
            else:
                if st != 0 or F(int(t[3]), int(t[4])) != want:
                    print('FINDING phase', tag, t, want, [p.enc() for p in pcs])
                    bad += 1
    print('cases', len(cases), stats, 'findings', bad)


main()
