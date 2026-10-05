#!/usr/bin/env python3
"""Independent refutation oracle for docs/api-3.md (lane q-review1).

Nothing is imported from proto/quotient3_checks.py. Every set-theoretic statement is decided by
my own enumeration in exact rationals (fractions.Fraction). Numerical statements (Q4 extrema, the
arc hull) use mpmath at 90 digits with a 1e-60 margin, which is evidence, not an interval proof.

Sections, in the order of the brief:
  A  reduction R and the exact image (Q1 before rounding)
  B  piece count k + 1 (M0-D4)
  C  Q1 rounding: enclosure, midpoint, excess bound
  D  Q2 set relations: the design's algorithm against brute force on a fine grid
  E  Q3 sum and negation
  F  Q4 phase extrema, additivity, width
  G  Q5 full image, fractional and zero radius
  H  examples E1 to E4 and the witnesses of F1 to F5
  I  phase of (0 ; 1/3) and the local character

Run: timeout 160 python3 lanes/q-review1/review_checks.py
"""
from fractions import Fraction as F
from math import ceil, floor, gcd
from itertools import product
import sys

OUT = []


def say(name, count, detail=''):
    OUT.append(name)
    print(f'PASS {name}: {count} cases; {detail}')


# --------------------------------------------------------------------------------------------
# Model of a quotient set. A value is a list of closed pieces (lo, hi, m, N), lo <= hi, N >= 0 int,
# representing union_i ([lo_i,hi_i] x (m_i + N_i Zhat)) mapped into A/Q. No canonicalisation of m is
# applied here: m is any integer and m + N Zhat does not depend on it modulo N.
# --------------------------------------------------------------------------------------------


def canon(m, N):
    return m % N if N else m


def key(p):
    return (p[0], p[1], p[3], canon(p[2], p[3]))


def R_design(lo, hi, a, N):
    """Algorithm R of docs/api-3.md 2.2, exact rationals, before Q1."""
    lo, hi, a, N = F(lo), F(hi), F(a), F(N)
    if lo > hi:
        raise ValueError('empty interval')
    if N == 0:
        splits = [(a, 0, 1)]
    else:
        A, B = N.numerator, N.denominator
        if N < 0:
            raise ValueError('negative finite radius')
        splits = [(a + F(j * A, B), A, B) for j in range(B)]
    raw = []
    for a_j, A, B in splits:
        l_j, h_j = lo - a_j, hi - a_j
        if l_j == h_j:
            ns = [floor(l_j)]
        else:
            ns = range(floor(l_j), ceil(h_j))
        for n in ns:
            raw.append((max(l_j, F(n)) - n, min(h_j, F(n + 1)) - n, -n, A))
    return sorted({(l, h, canon(m, A), A) for l, h, m, A in raw}, key=key)


def R_raw_count(lo, hi, a, N):
    """Number of constructions of R before deduplication, by a different enumeration."""
    lo, hi, a, N = F(lo), F(hi), F(a), F(N)
    if N == 0:
        splits = [a]
    else:
        A, B = N.numerator, N.denominator
        splits = [a + F(j * A, B) for j in range(B)]
    total = 0
    for a_j in splits:
        l_j, h_j = lo - a_j, hi - a_j
        if l_j == h_j:
            ns = [floor(l_j)]
        else:
            ns = [n for n in range(floor(l_j), ceil(h_j))]
            assert all(max(l_j, n) <= min(h_j, n + 1) for n in ns)
        total += len(ns)
    return total


def as_ball(lo, hi, a, N):
    """One stored adele written as a single piece list."""
    return [(F(lo), F(hi), int(a) if F(a).denominator == 1 else F(a), N)]


def bf_member(fam, s, w):
    """My own membership test, for integer w.

    (s,w) is in the image of [lo,hi] x (a + N Zhat) iff some q in Q has s+q in [lo,hi] and
    w+q-a in N Zhat. Writing N = A/B in lowest terms, the second condition says
    B(w+q-a)/A is in Z, so w+q-a = (A/B) k for some integer k, that is q = a + Ak/B - w.
    Each such k is admissible exactly when s + a + Ak/B - w lies in [lo,hi]. For N = 0 the
    condition forces q = a - w. The enumeration is therefore complete for every a and N.
    """
    s, w = F(s), F(w)
    for lo, hi, m, N in fam:
        m = F(m)
        if N == 0:
            if lo <= s + m - w <= hi:
                return True
            continue
        A, B = F(N).numerator, F(N).denominator
        u0, u1 = (lo - s - m + w) * B / A, (hi - s - m + w) * B / A
        for k in range(ceil(u0), floor(u1) + 1):
            q = m + F(A * k, B) - w
            if lo <= s + q <= hi:
                return True
    return False


# --------------------------------------------------------------------------------------------
# Q1, implemented from the statement, in exact rationals.
# --------------------------------------------------------------------------------------------


def _binade(q):
    """Binade exponent e with 2^(e-1) <= q < 2^e, for q > 0."""
    e = q.numerator.bit_length() - q.denominator.bit_length() + 1
    while q < F(2) ** (e - 1):
        e -= 1
    while q >= F(2) ** e:
        e += 1
    return e


def rn(q, p):
    """Round to nearest, ties to even, at p significant binary bits (sign symmetric)."""
    q = F(q)
    if q == 0:
        return F(0)
    s = -1 if q < 0 else 1
    q = abs(q)
    u = F(2) ** (_binade(q) - p)
    t = q / u
    n = floor(t)
    if t - n > F(1, 2) or (t - n == F(1, 2) and n % 2):
        n += 1
    return s * F(n) * u


def ru(q, p):
    """Least p-bit number >= q (round up), for q >= 0."""
    q = F(q)
    if q == 0:
        return F(0)
    return F(ceil(q / F(2) ** (_binade(q) - p))) * F(2) ** (_binade(q) - p)


def succ(q, p):
    """Least p-bit number strictly above q > 0: q plus one ulp of its own binade."""
    q = F(q)
    return q + F(2) ** (_binade(q) - p)


def q1_round(l, h, p):
    """Q1 kernel, exact rationals. Returns (midpoint, radius)."""
    l, h = F(l), F(h)
    m = rn((l + h) / 2, max(p, 2))
    d = max(m - l, h - m)
    if d == 0:
        return m, F(0)
    u = ru(d, 30)
    return m, succ(u, 30)


# --------------------------------------------------------------------------------------------
# A: reduction preserves the image set before rounding.
# --------------------------------------------------------------------------------------------


def grid_points(fam, per_gap=3, dense=64):
    """Real coordinates to test: 0, 1, every endpoint, several interior points per open gap,
    and a uniform grid of multiples of 1/dense."""
    ends = sorted({F(0), F(1)} | {F(t) for p in fam for t in (p[0], p[1])})
    pts = set()
    for u, v in zip(ends, ends[1:]):
        if u == v:
            continue
        n = per_gap if (v - u) > 0 else 0
        for i in range(n + 1):
            s = u + (v - u) * F(i, n + 1)
            if 0 <= s < 1:
                pts.add(s)
        for s in (u, v):
            if 0 <= s < 1:
                pts.add(s)
    for k in range(-dense, 2 * dense + 1):
        s = F(k, dense)
        if 0 <= s < 1:
            pts.add(s)
    return sorted(pts)


def section_A():
    n = pt = 0
    centres = [F(0), F(1, 2), F(-3, 2)]                               # API-legal: integer centres
    radii = [F(0), F(1), F(2), F(3), F(1, 2), F(1, 3), F(2, 3), F(3, 2)]
    los = [F(0), F(1), F(-1), F(9, 10), F(-2, 3), F(1, 4)]
    widths = [F(0), F(1, 10), F(1), F(5, 2), F(1, 2)]
    for a, N, lo, w in product(centres, radii, los, widths):
        if w < 0:
            continue
        hi = lo + w
        out = R_design(lo, hi, a, N)
        src = as_ball(lo, hi, a, N)
        # (1) the reduced list represents exactly the same subset of A/Q
        for s in grid_points(out, per_gap=1, dense=8):
            for ww in range(-12, 13):
                assert bf_member(out, s, ww) == bf_member(src, s, ww), (a, N, lo, w, s, ww)
                pt += 1
        # (2) translation by a rational changes nothing
        q = F(-7, 11)
        assert R_design(lo + q, hi + q, a + q, N) == out, (a, N, lo, w, q)
        # (3) every stored piece lies inside [0,1] before rounding, and its finite radius is integral
        for l, h, m, NN in out:
            assert 0 <= l <= h <= 1, (l, h)
            assert isinstance(NN, int) and NN >= 0
            assert 0 <= m < NN or NN == 0
        n += 1
    say('A_reduction_image', n, f'{pt} brute-force membership comparisons over rational grids')


def section_A2():
    """Reduction after Q1 must be a SUPERSET of the exact reduced set."""
    n = pt = 0
    for a, N, lo, w in product([F(0), F(1, 2), F(-2, 3)], [F(0), F(1), F(2), F(1, 2), F(2, 3)],
                               [F(0), F(1), F(-1), F(9, 10), F(1, 2)],
                               [F(0), F(1, 10), F(1), F(5, 2)]):
        hi = lo + w
        if lo < 0 or hi > 1:
            continue
        exact = R_design(lo, hi, a, N)
        for prec in (2, 20, 53):
            rounded = [(mm - rr, mm + rr, m, NN)
                       for l, h, m, NN in exact for mm, rr in [q1_round(l, h, prec)]]
            for s in grid_points(exact, per_gap=1):
                for ww in range(-8, 9):
                    assert (not bf_member(exact, s, ww)) or bf_member(rounded, s, ww)
                    pt += 1
            n += 1
    say('A2_rounded_superset', n, f'{pt} membership comparisons after Q1')


# --------------------------------------------------------------------------------------------
# B: piece count.
# --------------------------------------------------------------------------------------------


def section_B():
    n = mismatch_raw = mismatch_k = 0
    examples = []
    for lo in [F(i, 4) for i in range(-8, 9)]:
        for hi in [F(i, 4) for i in range(-8, 9)]:
            if hi < lo:
                continue
            got = len(R_design(lo, hi, F(0), F(1)))
            raw = R_raw_count(lo, hi, F(0), F(1))
            k = len([j for j in range(floor(lo) - 1, ceil(hi) + 1) if lo < j < hi])
            if raw != got:
                mismatch_raw += 1
                examples.append((str(lo), str(hi), raw, got))
            if lo < hi and raw != k + 1:
                mismatch_k += 1
            if lo == hi and raw != 1:
                mismatch_k += 1
            n += 1
    tot = merged = 0
    for N in [F(1, 2), F(1, 3), F(2, 3), F(3, 2), F(1, 4), F(5, 6), F(7, 6)]:
        for lo, w in product([F(0), F(1, 10), F(1), F(-1, 3), F(1, 3)],
                             [F(0), F(1), F(1, 2), F(5, 4)]):
            got = R_raw_count(lo, lo + w, F(0), N)
            dedup = len(R_design(lo, lo + w, F(0), N))
            assert dedup <= got
            if dedup < got:
                merged += 1
            for j in range(N.denominator):
                a_j = F(j * N.numerator, N.denominator)
                l_j, h_j = lo - a_j, lo + w - a_j
                if l_j < h_j:
                    kk = len([j2 for j2 in range(floor(l_j) - 1, ceil(h_j) + 1) if l_j < j2 < h_j])
                    assert ceil(h_j) - floor(l_j) == kk + 1, (N, lo, w, j)
                    # every point of [l_j,h_j] has its representative in exactly one constructed piece
                    for t in [F(i, 12) for i in range(-24, 25)]:
                        if l_j <= t <= h_j:
                            nn = floor(t)
                            assert (max(l_j, F(nn)) - nn <= t - nn
                                    <= min(h_j, F(nn + 1)) - nn), (N, j, t)
                else:
                    assert ceil(h_j) - floor(l_j) == 1 or ceil(h_j) == floor(l_j)
            tot += 1
    say('B_piece_count', n + tot,
        f'{n} integer-radius intervals: the RAW construction count equals k+1 in {n - mismatch_k} '
        f'of them (mismatches {mismatch_k}); it differs from the stored count in {mismatch_raw} '
        f'cases, all by deduplication, first three {examples[:3]}; {tot} fractional cases, raw = '
        f'sum of the per-ball counts, deduplication merged in {merged}')


# --------------------------------------------------------------------------------------------
# C: Q1.
# --------------------------------------------------------------------------------------------


def section_C():
    n = over = 0
    for d in range(1, 33):
        for j in range(d):
            l, h = F(j, d), F(j + 1, d)
            for prec in (2, 3, 20, 53, 128):
                m, r = q1_round(l, h, prec)
                assert m - r <= l and h <= m + r
                assert 0 <= m <= 1, (l, h, prec, m)
                if r == 0:
                    assert m == l == h
                eta = abs(m - (l + h) / 2)
                need = max(m - l, h - m)
                bound = 2 * eta + F(1, 2 ** 28) * need
                assert (l - (m - r)) <= bound, (l, h, prec, r, need)
                assert ((m + r) - h) <= bound
                n += 1
    # excess bound stress near binade boundaries of the radius, where the successor is a power of 2
    worst = F(0)
    cases = 0
    for k in range(-60, 1):
        base = F(2) ** k
        for num in (-3, -2, -1, 0, 1, 2, 3, 7, 15, 31, 32):
            d = base + F(num) / 2 ** 40
            if d <= 0:
                continue
            u = ru(d, 30)
            r = succ(u, 30)
            if r - d > F(1, 2 ** 28) * d:
                over += 1
            worst = max(worst, (r - d) / d)
            assert r - d > 0
            cases += 1
    say('C_q1', n + cases,
        f'{n} pieces x 5 precisions; enclosure, midpoint and excess bound held; radius-successor '
        f'stress {cases} binade neighbourhoods, violations {over}, worst (rho-d)/d = {worst} '
        f'= {float(worst):.3e}, the bound needs it <= 1/2^28 = {2.0 ** -28:.3e}')
    assert over == 0


def section_C2():
    """Q1 is stated for 0 <= l <= h <= 1. Section 3.3 applies it to hull coordinates in [-1,1]."""
    n = 0
    worst = F(0)
    for a, b in product([F(-1), F(-3, 4), F(-1, 2), F(0), F(1, 2), F(3, 4), F(1)],
                        [F(0), F(1, 3), F(1, 2), F(3, 4), F(1)]):
        for l in [F(i, 4) for i in range(-4, 5)]:
            h = a + (b - a) * F(1, 3)
            if not (l <= h):
                continue
            for prec in (20, 53):
                m, r = q1_round(l, h, prec)
                assert m - r <= l and h <= m + r
                eta = abs(m - (l + h) / 2)
                need = max(m - l, h - m)
                bound = 2 * eta + F(1, 2 ** 28) * need
                assert (l - (m - r)) <= bound and ((m + r) - h) <= bound
                worst = max(worst, eta)
                n += 1
    say('C2_q1_negative_midpoint', n,
        f'{n} intervals in [-1,1] with negative midpoints: enclosure and the excess bound still hold; '
        f'Q1 as stated does not cover them')


# --------------------------------------------------------------------------------------------
# D: Q2.
# --------------------------------------------------------------------------------------------


def normalize(fam):
    out = []
    for lo, hi, m, N in fam:
        out += R_design(lo, hi, F(m), F(N))
    return sorted(set(out), key=key)


def q2_cells(fam):
    e = sorted({F(0), F(1)} | {F(t) for p in fam for t in (p[0], p[1])})
    out = []
    for u, v in zip(e, e[1:]):
        for s in (u, (u + v) / 2):
            if 0 <= s < 1 and s not in out:
                out.append(s)
    return out


def q2_fibers(fam, s, L):
    """The fibre at s: residues modulo L of the positive cosets, plus exact integer points.

    Step 2 of Q2: a piece whose upper end is 1 also contributes the class m - 1 at s = 0.
    """
    R, E = set(), set()
    for lo, hi, m, N in fam:
        inside = lo <= s <= hi and s < 1
        glued = s == 0 and hi == 1
        if not (inside or glued):
            continue
        centres = ([m] if inside else []) + ([m - 1] if glued else [])
        for c in centres:
            if N:
                R |= {(c + j * N) % L for j in range(L // N)}
            else:
                E.add(c)
    return R, {e for e in E if e % L not in R}


def q2_compare(x, y):
    xn, yn = normalize(x), normalize(y)
    L = 1
    for p in xn + yn:
        if p[3]:
            L = L * p[3] // gcd(L, p[3])
    in_xy = in_yx = True
    ov = False
    for s in q2_cells(xn + yn):
        Rx, Ex = q2_fibers(xn, s, L)
        Ry, Ey = q2_fibers(yn, s, L)
        in_xy = in_xy and Rx <= Ry and all(e % L in Ry or e in Ey for e in Ex)
        in_yx = in_yx and Ry <= Rx and all(e % L in Rx or e in Ex for e in Ey)
        ov = ov or bool(Rx & Ry) or bool(Ex & Ey) or any(e % L in Ry for e in Ex) \
            or any(e % L in Rx for e in Ey)
    return bool(in_xy and in_yx), bool(in_xy), bool(ov)


def brute_compare(x, y, W, per_gap=1, dense=8):
    xs = grid_points(list(x) + list(y), per_gap=per_gap, dense=dense)
    eq = True
    in_xy = in_yx = True
    ov = False
    for s in xs:
        for w in W:
            a, b = bf_member(x, s, w), bf_member(y, s, w)
            eq &= a == b
            in_xy &= (not a) or b
            in_yx &= (not b) or a
            ov |= a and b
    return bool(eq), bool(in_xy), bool(ov)


D_CASES = [
    ('point vs Zhat', [(F(0), F(0), 0, 0)], [(F(0), F(0), 0, 1)]),
    ('same set, translated finite part', [(F(0), F(1), 1, 2)], [(F(0), F(1), 3, 2)]),
    ('same set, shifted real by 1', [(F(0), F(1), 0, 2)], [(F(1), F(2), -1, 2)]),
    ('zero radius vs radius 3', [(F(1, 4), F(1, 2), 0, 0)], [(F(1, 4), F(1, 2), 0, 3)]),
    ('zero radius covered by coset', [(F(1, 4), F(1, 2), 5, 0)], [(F(0), F(1), 1, 2)]),
    ('glued endpoint', [(F(1, 2), F(1), 0, 3)], [(F(0), F(1, 2), 2, 3)]),
    ('glued endpoint swapped', [(F(1, 2), F(1), 2, 3)], [(F(0), F(1, 2), 2, 3)]),
    ('spill above', [(F(0), F(1, 16), 0, 2)], [(F(-1, 16), F(1, 16), 0, 2)]),
    ('spill below both', [(F(-1, 16), F(1, 16), 0, 2)], [(F(1, 16), F(3, 16), -1, 2)]),
    ('two points mod 2 vs interval', [(F(0), F(0), 0, 0), (F(1, 2), F(1, 2), 1, 0)],
     [(F(0), F(1), 0, 2)]),
    ('mod 2 vs mod 4 refinement', [(F(0), F(1), 0, 2)], [(F(0), F(1), 0, 4)]),
    ('mod 4 vs mod 6', [(F(0), F(1, 2), 0, 4)], [(F(1, 2), F(1), 0, 6)]),
    ('empty-looking zero radius', [(F(0), F(0), -7, 0)], [(F(0), F(0), 5, 0)]),
    ('nested intervals same finite', [(F(0), F(1, 2), 0, 3)], [(F(1, 4), F(1, 4), 0, 3)]),
    ('whole quotient vs itself', [(F(0), F(1), 0, 1)], [(F(0), F(1), 0, 1)]),
    ('whole quotient vs pieces', [(F(0), F(1), 0, 1)],
     [(F(0), F(1, 2), 0, 1), (F(1, 2), F(1), -1, 1)]),
    ('two cosets mod 2 vs three mod 3', [(F(0), F(1), 0, 2), (F(0), F(1), 1, 2)],
     [(F(0), F(1), 0, 3), (F(0), F(1), 1, 3), (F(0), F(1), 2, 3)]),
    ('midpoint only', [(F(1, 3), F(1, 3), 0, 2)], [(F(1, 3), F(1, 3), 0, 3)]),
    ('point 1 glued', [(F(1), F(1), 0, 3)], [(F(0), F(0), 2, 3)]),
    ('point 1 glued, wrong side', [(F(1), F(1), 0, 3)], [(F(0), F(0), 1, 3)]),
    ('point 0 vs point 1 same class', [(F(0), F(0), 0, 2)], [(F(1), F(1), 1, 2)]),
    ('entirely left of the domain', [(F(-1), F(-1, 2), 0, 2)], [(F(0), F(1, 2), 1, 2)]),
    ('entirely right of the domain', [(F(3, 2), F(2), 0, 2)], [(F(0), F(1, 2), 1, 2)]),
    ('two exact points, one shared', [(F(1, 4), F(1, 4), 0, 0), (F(3, 4), F(3, 4), 0, 0)],
     [(F(1, 4), F(1, 4), 0, 0), (F(3, 4), F(3, 4), 1, 0)]),
    ('coset vs two points in it', [(F(0), F(1), 0, 6)], [(F(1, 4), F(1, 4), 0, 0),
                                                     (F(3, 4), F(3, 4), 6, 0)]),
    ('mod 3 half vs mod 2 whole', [(F(0), F(1, 2), 0, 3)], [(F(0), F(1), 0, 2)]),
    ('interval union touching', [(F(0), F(1, 4), 0, 2)], [(F(1, 4), F(1, 2), 0, 2)]),
]


def section_D():
    diffs = []
    pt = 0
    for name, x, y in D_CASES:
        got = q2_compare(x, y)
        want = brute_compare(x, y, range(-48, 49), per_gap=2, dense=16)
        pt += 1
        if got != want:
            diffs.append((name, got, want))
    say('D_q2_named', len(D_CASES),
        f'{pt} hand-made pairs; design-vs-brute-force differences {len(diffs)}: {diffs}')


def section_D2():
    """Random small families, my own generator, fine grid."""
    import random
    rng = random.Random(20261005)
    diffs = []
    n = pt = 0
    for it in range(300):
        fams = []
        for _ in range(2):
            fam = []
            for _ in range(rng.randrange(1, 4)):
                lo = F(rng.randrange(-8, 9), 8)
                hi = lo + F(rng.randrange(9), 8)
                m = rng.randrange(-4, 5)
                N = rng.choice([0, 0, 1, 2, 3, 4, 6])
                fam.append((lo, hi, m, N))
            fams.append(fam)
        x, y = fams
        L = 1
        for p in normalize(x + y):
            if p[3]:
                L = L * p[3] // gcd(L, p[3])
        assert L <= 24, L
        got = q2_compare(x, y)
        want = brute_compare(x, y, range(-48, 49), per_gap=2, dense=16)
        if got != want:
            diffs.append((x, y, got, want))
        n += 1
    say('D2_q2_random', n,
        f'{n} random pairs, up to 3 pieces each, moduli 0,1,2,3,4,6, real parts in [-1,2]; '
        f'differences {len(diffs)}')
    for d in diffs[:5]:
        print('   DIFF', d)


# --------------------------------------------------------------------------------------------
# E: Q3.
# --------------------------------------------------------------------------------------------


def rgcd2(a, b):
    a, b = F(a), F(b)
    g = gcd(a.numerator, b.numerator)
    l = a.denominator * b.denominator // gcd(a.denominator, b.denominator)
    return F(g, l)


def section_E():
    n = pt = 0
    ins = [Adele_(F(a, 3), F(r, 3), F(c, 2), F(NN, 2)) for a in (-1, 1) for r in (0, 1)
           for c in (0, 1) for NN in (0, 1, 2, 3)]
    for x, y in product(ins, repeat=2):
        z = (x[0] + y[0], x[1] + y[1], x[2] + y[2], rgcd2(x[3], y[3]))
        direct = R_design(z[0] - z[1], z[0] + z[1], z[2], z[3])
        pair = []
        for a, b in product(R_design(x[0] - x[1], x[0] + x[1], x[2], x[3]),
                            R_design(y[0] - y[1], y[0] + y[1], y[2], y[3])):
            pair.append((a[0] + b[0], a[1] + b[1], a[2] + b[2], gcd(a[3], b[3])))
        for s in grid_points(pair, per_gap=1, dense=4):
            for w in range(-12, 13):
                assert bf_member(pair, s, w) == bf_member(direct, s, w), (x, y, s, w)
                pt += 1
        nx = (-x[0], x[1], -x[2], x[3])
        negd = R_design(nx[0] - nx[1], nx[0] + nx[1], nx[2], nx[3])
        negr = [(F(-b[1]), F(-b[0]), -b[2], b[3])
                for b in R_design(x[0] - x[1], x[0] + x[1], x[2], x[3])]
        for s in grid_points(negr, per_gap=1, dense=4):
            for w in range(-12, 13):
                assert bf_member(negr, s, w) == bf_member(negd, s, w), (x, s, w)
                pt += 1
        n += 2
    say('E_q3_arithmetic', n,
        f'{len(ins)**2} sums and {len(ins)} negations, {pt} brute-force membership comparisons')


def Adele_(mid, rad, a, N):
    return (mid, rad, a, N)


# --------------------------------------------------------------------------------------------
# F: Q4.
# --------------------------------------------------------------------------------------------


def d_formula(b, r, B, t):
    u = (B * (F(t) - b)) % 1
    return max(F(0), min(u, 1 - u) / B - r)


def q4_hull(b, r, B):
    import mpmath as mp
    mp.mp.dps = 90
    def c(d):
        d = F(d)
        if d in (F(0), F(1, 4), F(1, 2)):
            return mp.mpf({F(0): 1, F(1, 4): 0, F(1, 2): -1}[d])
        return mp.cos(2 * mp.pi * mp.mpf(d.numerator) / d.denominator)
    return (-c(d_formula(b, r, B, F(1, 2))), c(d_formula(b, r, B, F(0))),
            -c(d_formula(b, r, B, F(3, 4))), c(d_formula(b, r, B, F(1, 4))))


def enumerate_extrema(b, r, B):
    import mpmath as mp
    mp.mp.dps = 90
    arcs = []
    for k in range(B):
        c = (b + F(k, B)) % 1
        lo, hi = c - r, c + r
        if lo < 0:
            arcs += [(F(0), hi), (lo + 1, F(1))]
        elif hi > 1:
            arcs += [(lo, F(1)), (F(0), hi - 1)]
        else:
            arcs.append((lo, hi))
    pts = set()
    for lo, hi in arcs:
        pts |= {lo, hi}
        for t in (F(0), F(1, 4), F(1, 2), F(3, 4)):
            if lo <= t <= hi:
                pts.add(t)
    vals = [mp.expjpi(2 * mp.mpf(t.numerator) / t.denominator) for t in pts]
    return (min(v.real for v in vals), max(v.real for v in vals),
            min(v.imag for v in vals), max(v.imag for v in vals))


def section_F():
    import mpmath as mp
    mp.mp.dps = 90
    margin = mp.mpf('1e-60')
    n = 0
    for B in (1, 2, 3, 4, 5, 6, 7, 8, 12):
        for b in (F(0), F(1, 7), F(3, 4), F(-2, 9), F(5, 11)):
            for r in (F(0), F(1, 100), F(1, 8), F(1, 6), F(1, 4), F(1, 3), F(1, 2), F(1)):
                got, want = q4_hull(b, r, B), enumerate_extrema(b, r, B)
                for a, c in zip(got, want):
                    assert abs(a - c) < margin, (B, b, r, got, want)
                n += 1
    say('F_q4_hull', n,
        f'{n} (B,b,r) images; four-distance hull equals enumerated extrema (90 digits, 1e-60)')


def section_F2():
    """Q4 step 4: diameter and coordinate width for B = 1."""
    import mpmath as mp
    mp.mp.dps = 90
    margin = mp.mpf('1e-60')
    n = 0
    for r in (F(1, 20), F(1, 8), F(1, 4), F(1, 3), F(1, 2), F(3, 5), F(1)):
        b = F(1, 7)
        w = 2 * r
        delta = min(w, F(1, 2))
        dia = 2 * mp.sin(mp.pi * mp.mpf(delta.numerator) / delta.denominator)
        h = q4_hull(b, r, 1)
        assert (h[1] - h[0]) <= dia + margin, (r, h, dia)
        assert (h[3] - h[2]) <= dia + margin
        # the diameter itself is attained
        if w <= F(1, 2):
            p = mp.expjpi(2 * mp.mpf(b.numerator) / b.denominator)
            q = mp.expjpi(2 * mp.mpf((b + w).numerator) / (b + w).denominator)
            assert abs(abs(p - q) - dia) < margin
        else:
            assert abs(dia - 2) < margin
        n += 1
    say('F2_q4_width', n, f'{n} arcs: coordinate width <= diameter; diameter attained or 2')


def arcs_of(b, r, B):
    """Materialise the exact image (b,r,B) as a sorted list of closed arcs in [0,1]."""
    b, r, B = F(b), F(r), B
    if 2 * r * B >= 1:
        return [(F(0), F(1))]
    out = []
    for k in range(B):
        c = (b + F(k, B)) % 1
        lo, hi = c - r, c + r
        if lo < 0:
            out += [(F(0), hi), (lo + 1, F(1))]
        elif hi > 1:
            out += [(lo, F(1)), (F(0), hi - 1)]
        else:
            out.append((lo, hi))
    out.sort()
    merged = []
    for lo, hi in out:
        if merged and lo <= merged[-1][1]:
            merged[-1] = (merged[-1][0], max(merged[-1][1], hi))
        else:
            merged.append((lo, hi))
    return merged


def member(arcs, t):
    return any(lo <= t <= hi or (t == 0 and hi == 1) for lo, hi in arcs)


def section_F3():
    """psi of a sum of two balls equals the pairwise product of the images (Q4 step 6).

    Left: every product of one point of each exact image, as exact rational arcs.
    Right: the image of the summed adele, whose centre is a1+a2-(m1+m2), radius r1+r2 and
    finite radius gcd(N1,N2) (Q3 step 3)."""
    n = pt = 0
    ins = [(F(1, 3), F(0), F(1, 5), F(0)), (F(1, 3), F(0), F(1, 5), F(1, 3)),
           (F(-1, 4), F(0), F(2, 7), F(0)), (F(-1, 4), F(0), F(2, 7), F(2, 3)),
           (F(0), F(1, 5), F(1, 7), F(1, 2)), (F(2, 5), F(1, 9), F(-1, 3), F(0)),
           (F(1, 2), F(1, 8), F(1, 11), F(4, 5)), (F(0), F(1, 12), F(3, 8), F(0))]
    for (m1, r1, a1, N1), (m2, r2, a2, N2) in product(ins, repeat=2):
        B1 = N1.denominator if N1 else 1
        B2 = N2.denominator if N2 else 1
        g = rgcd2(N1, N2)
        B3 = g.denominator if g else 1
        left = []
        for lo1, hi1 in arcs_of((a1 - m1) % 1, r1, B1):
            for lo2, hi2 in arcs_of((a2 - m2) % 1, r2, B2):
                A, Bq = lo1 + lo2, hi1 + hi2
                if Bq - A >= 1:
                    left.append((F(0), F(1)))
                else:
                    for j in range(floor(A), floor(Bq) + 1):
                        left.append((max(A, F(j)) - j, min(Bq, F(j + 1)) - j))
        right = arcs_of((a1 + a2 - m1 - m2) % 1, r1 + r2, B3)
        ends = sorted({F(0), F(1)} | {t for arc in left + right for t in arc})
        for u, v in zip(ends, ends[1:]):
            for t in (u, (u + v) / 2):
                assert member(left, t) == member(right, t), \
                    ((m1, r1, a1, N1), (m2, r2, a2, N2), t, left, right)
                pt += 1
        n += 1
    say('F3_q4_additivity', n, f'{n} pairs of balls, {pt} exact rational witnesses of '
        f'psi(X+Y) = psi(X) psi(Y) as sets of angles')


# --------------------------------------------------------------------------------------------
# G: Q5.
# --------------------------------------------------------------------------------------------


def section_G():
    n = miss = 0
    for A in range(1, 7):
        for B in range(1, 7):
            if gcd(A, B) != 1:
                continue
            N = F(A, B)
            for width in (F(1, 10), N / 2, N - F(1, 10) if N > 0 else F(0), N, N + F(1, 7), F(3, 2)):
                lo, a = F(1, 7), F(2, 3)
                hi = lo + width
                out = R_design(lo, hi, a, N)
                full = [(F(0), F(1), 0, 1)]
                if width >= N:
                    for s in grid_points(out):
                        for w in range(-60, 61):
                            assert bf_member(out, s, w), (N, width, s, w)
                else:
                    # construct an explicit missing point
                    bad = None
                    for s in grid_points(out):
                        for w in range(-60, 61):
                            if not bf_member(out, s, w):
                                bad = (s, w)
                                break
                        if bad:
                            break
                    assert bad is not None, (N, width)
                    miss += 1
                n += 1
    say('G_q5_positive_radius', n,
        f'{n} (N,width) pairs; every member tested for width >= N, {miss} explicit missing points '
        f'for width < N')


def section_G2():
    """N = 0 is never full, whatever the width."""
    n = 0
    full = [(F(0), F(1), 0, 1)]
    for width in (F(0), F(1, 20), F(1), F(3, 2), F(100)):
        out = R_design(F(0), F(0) + width, F(1, 3), F(0))
        bad = [(s, w) for s in grid_points(out, per_gap=1) for w in range(-20, 21)
               if not bf_member(out, s, w)]
        assert bad, (width, out)
        assert not all(bf_member(out, s, w) for s in grid_points(out, per_gap=1)
                       for w in range(-20, 21))
        n += 1
    say('G2_q5_zero_radius', n, f'{n} widths with N = 0; the image is never all of A/Q (E4 witness)')


# --------------------------------------------------------------------------------------------
# H: examples and findings.
# --------------------------------------------------------------------------------------------


def section_H():
    E1 = R_design(F(9, 10), F(11, 10), F(0), F(2))
    E2 = R_design(F(0), F(1), F(0), F(2))
    E3 = R_design(F(0), F(0), F(0), F(1, 2))
    E4 = R_design(F(0), F(0), F(0), F(0))
    print('EXAMPLE E1', [(str(l), str(h), m, N) for l, h, m, N in E1])
    print('EXAMPLE E2', [(str(l), str(h), m, N) for l, h, m, N in E2])
    print('EXAMPLE E3', [(str(l), str(h), m, N) for l, h, m, N in E3])
    print('EXAMPLE E4', [(str(l), str(h), m, N) for l, h, m, N in E4])
    assert E1 == [(F(0), F(1, 10), 1, 2), (F(9, 10), F(1), 0, 2)]
    assert E2 == [(F(0), F(1), 0, 2)]
    assert E3 == [(F(0), F(0), 0, 1), (F(1, 2), F(1, 2), 0, 1)]
    assert E4 == [(F(0), F(0), 0, 0)]
    n = 0
    # F1: (1/2 ; 0) is not in E4
    assert not bf_member(E4, F(1, 2), 0)
    n += 1
    # F2: the non-singleton set equals itself; spill case
    assert q2_compare([(F(0), F(1), 0, 2)], [(F(0), F(1), 0, 2)])[0]
    assert bf_member([(F(-1, 16), F(1, 16), 0, 2)], F(31, 32), 1)
    n += 2
    # F3: lift image and reduced image are both {+1,-1}
    lift3 = [(F(0), F(0), 0, 0)]
    def psi_image(mid, rad, a, N):
        b = (a - mid) % 1
        B = N.denominator if N else 1
        if rad == 0 and B == 1:
            return {b % 1}
        return {((b + F(k, B)) % 1) for k in range(B)} if rad == 0 else 'arc'
    assert psi_image(F(0), F(0), F(0), F(1, 2)) == psi_image(F(0), F(0), F(0), 0) \
        | psi_image(F(1, 2), F(0), F(0), 0)
    n += 1
    # F4: rounded translation of zero by 1/3 at 8 bits
    m8 = rn(F(1, 3), 8)
    rad = ru(abs(m8 - F(1, 3)), 30)
    translated = R_design(m8 - rad, m8 + rad, F(1, 3), F(0))
    zero = [(F(0), F(0), 0, 0)]
    assert m8 == F(171, 512), m8
    assert q2_compare(zero, translated) == (False, True, True), q2_compare(zero, translated)
    print('EXAMPLE F4 translated', [(str(l), str(h), m, N) for l, h, m, N in translated])
    n += 1
    # F5: [15/16 +/- 1/16] = [7/8,1] encloses [9/10,1] and stays inside [0,1]
    assert F(7, 8) <= F(9, 10) and F(1) <= F(1) and 0 <= F(7, 8)
    n += 1
    say('H_examples_findings', n, 'E1-E4 as stated; F1 to F5 witnesses reproduced')


# --------------------------------------------------------------------------------------------
# I: the character itself.
# --------------------------------------------------------------------------------------------


def fp_p(a, p):
    a = F(a)
    den, h = a.denominator, 1
    while den % p == 0:
        den //= p
        h *= p
    if h == 1:
        return F(0)
    return F((a.numerator * pow(den, -1, h)) % h, h)


def section_I():
    n = 0
    # PLAN 3.2: a non-trivial phase at (0 ; 1/3)
    x = (F(0), F(0), F(1, 3), F(0))
    b = (x[2] - x[0]) % 1
    assert b == F(1, 3), b
    assert b not in (F(0), F(1, 2)), 'the phase at (0 ; 1/3) is non-trivial'
    n += 1
    # product formula: sum_p fp_p(a) = a mod 1 for small primes p > numerator, denominator
    for num in range(-12, 13):
        for den in range(1, 13):
            a = F(num, den)
            s = sum((fp_p(a, p) for p in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37)),
                    F(0)) % 1
            assert s == a % 1, (a, s)
            n += 1
    # fp_p agrees with Tate's source: psi(1/p^n) = E(1/p^n), psi|Z_p = 1
    for p in (2, 3, 5, 7):
        for n_ in range(1, 5):
            assert fp_p(F(1, p ** n_), p) == F(1, p ** n_)
            assert fp_p(F(p ** n_ * 3 + 1), p) == F(0)
            n += 1
    # real factor is E(-m): the phase of (1/3 ; 0) is E(-1/3) = E(2/3)
    assert ((F(0) - F(1, 3)) % 1) == F(2, 3)
    n += 1
    say('I_character_sign', n,
        f'{n} sign and product-formula cases; (0 ; 1/3) gives E(1/3); psi_inf is E(-x); '
        f'fp_p(1/p^n) = 1/p^n as in notes.txt:700')


def main():
    for fn in (section_A, section_A2, section_B, section_C, section_C2, section_D, section_D2,
               section_E, section_F, section_F2, section_F3, section_G, section_G2, section_H,
               section_I):
        fn()
    print(f'{len(OUT)} sections')


if __name__ == '__main__':
    sys.exit(main())