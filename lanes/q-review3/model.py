#!/usr/bin/env python3
"""Lane q-review3, part 1: enclosure, tightness, count and status of adf_qclass_reduce.

The model is my own: algorithm R of docs/api-3.md 2.2 and the kernel Q1 of section 4, in exact
rationals (fractions.Fraction).  Nothing is imported from proto/quotient3_checks.py or from the
vectors of lane q-slice2.  The membership test bf_member is copied from lanes/q-review1/review_checks.py,
the independent model of lane q-review1.

Usage:
  python3 lanes/q-review3/check.py --cases 20000 --seed 1 [--harness PATH] [--keep DIR]
It writes the case lines to DIR/cases.txt, runs the harness, and checks every answer.
"""
import argparse, os, random, subprocess, sys
from fractions import Fraction as F
from math import ceil, floor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))

# ---------------------------------------------------------------- exact kernel Q1

def binade(v):
    """e with 2^(e-1) <= v < 2^e for a positive dyadic rational v."""
    assert v > 0
    num, den = v.numerator, v.denominator
    assert den & (den - 1) == 0, den
    return num.bit_length() - (den.bit_length() - 1)

def ceil_to(v, p):
    """Least p-bit binary number >= v (Q1: u).  Exact."""
    e = binade(v)
    s = F(2) ** (e - p)
    n = -((-v) // s)          # ceil(v/s) as an integer
    return F(n) * s, e

def rn(v, p):
    """Round v to p significant bits, nearest, ties to even (arf RND_NEAR)."""
    if v == 0:
        return F(0)
    e = binade(v)
    s = F(2) ** (e - p)
    q = v / s
    fl = q.numerator // q.denominator
    rem = q - fl
    if rem * 2 < 1:
        r = fl
    elif rem * 2 > 1:
        r = fl + 1
    else:
        r = fl if fl % 2 == 0 else fl + 1
    out = F(r) * s
    return out

def round_piece(lo, hi, prec):
    """Q1.  Returns (mid, rad) as exact rationals."""
    p = max(prec, 2)
    m = rn((lo + hi) / 2, p)
    d = max(m - lo, hi - m)
    if d == 0:
        return m, F(0)
    u, e = ceil_to(d, 30)
    rho = u + F(2) ** (e - 30)          # the least 30-bit successor
    return m, rho

# ---------------------------------------------------------------- algorithm R

def R_raw(lo, hi, a, N):
    """The constructions of algorithm R, in order, with duplicates.  A list of (l, h, m, A)."""
    lo, hi, a, N = F(lo), F(hi), F(a), F(N)
    assert lo <= hi and N >= 0
    if N == 0:
        splits = [(a, 0)]
    else:
        A, B = N.numerator, N.denominator
        splits = [(a + F(j * A, B), A) for j in range(B)]
    out = []
    for a_j, A in splits:
        l_j, h_j = lo - a_j, hi - a_j
        if l_j == h_j:
            ns = [floor(l_j)]
        else:
            ns = range(floor(l_j), ceil(h_j))
        for n in ns:
            left = max(l_j, F(n)) - n
            right = min(h_j, F(n + 1)) - n
            if left < 0: left = F(0)
            if right > 1: right = F(1)
            out.append((left, right, -n, A))
    return out

def canon(m, N):
    return m % N if N else m

def expected(lo, hi, a, N, prec):
    """The expected stored output: the multiset of (mid, rad, A, H, d) after Q1, sort and dedup."""
    raw = R_raw(lo, hi, a, N, )
    got = []
    for (l, h, m, A) in raw:
        mid, rad = round_piece(l, h, prec)
        c = canon(m, A)
        got.append((mid - rad, mid + rad, F(A), c, F(A)))
    got.sort()
    ded = []
    for g in got:
        if not ded or ded[-1] != g:
            ded.append(g)
    stored = []
    for (lo2, hi2, H, c, A) in ded:
        mid, rad = (lo2 + hi2) / 2, (hi2 - lo2) / 2
        stored.append((mid, rad, c, H, F(1)))
    return len(raw), stored, raw

# ---------------------------------------------------------------- membership (q-review1)

def bf_member(fam, s, w):
    """From lanes/q-review1/review_checks.py: is the quotient point (s,w) in the family?"""
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

# ---------------------------------------------------------------- generators

def dyadic(rng, expo, width):
    """A random dyadic midpoint as (mantissa, exponent) and a dyadic radius."""
    mbits = rng.choice([0, 1, 2, 3, 5, 8, 17, 30, 31, 40, 64])
    mant = rng.randrange(-(1 << mbits), 1 << mbits) if mbits else 0
    e = expo
    rbits = rng.randrange(0, width + 1)
    rmant = rng.randrange(0, 1 << rbits) if rbits else 0
    rexp = rng.randrange(-expo, expo + 1) if expo else 0
    return mant, e, rmant, rexp

def gen_case(rng, kind):
    """Returns (mid_mant, mid_exp, rad_mant, rad_exp, cnum, cden, nnum, nden, backend, blocks)."""
    if kind == 'real':
        expo = rng.choice([1, 2, 3, 8, 30, 60, 200, 200])
        width = rng.choice([0, 1, 2, 3, 4, 8, 20, 40, 40])
        mant, e, rmant, rexp = dyadic(rng, expo, width)
        # end points exactly at integers or half-integers sometimes
        r = rng.random()
        if r < 0.25:
            rmant, rexp = rng.choice([(0, 0), (1, 0), (1, -1), (0, -30), (1, -31)])
        if r > 0.9:
            mant, e = rng.choice([(0, 0), (1, 0), (2, 0), (1, -1)])
    elif kind == 'wide':
        expo = rng.choice([1, 2, 5, 40, 200])
        mant, e = rng.randrange(-3, 4), expo
        rmant, rexp = rng.randrange(1, 1 << rng.randrange(0, 8)), 0
        r = rng.random()
        if r < 0.3:
            rmant, rexp = rng.choice([(1, 0), (2, 0), (3, 0)])
    elif kind == 'bigexp':
        expo = rng.choice([100, 150, 200, 200])
        mant, e, rmant, rexp = dyadic(rng, expo, rng.choice([0, 1, 5, 30, 40]))
    else:
        raise ValueError(kind)
    cnum, cden, nnum, nden = finite(rng)
    return mant, e, rmant, rexp, cnum, cden, nnum, nden, 0, ()

FRACS = [(0, 1), (1, 1), (2, 1), (3, 1), (12, 1), (360, 1), (1, 2), (1, 3), (2, 3),
         (3, 2), (5, 4), (7, 360), (4, 5), (5, 3), (11, 7), (1, 360), (359, 360), (6, 7)]

def finite(rng):
    n, d = rng.choice(FRACS)
    if rng.random() < 0.5:
        q = F(rng.randrange(-400, 401), rng.choice([1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 60, 360]))
    else:
        q = F(rng.randrange(-400, 401))
    return q.numerator, q.denominator, n, d

def line(limit, prec, pieces):
    s = "L %d %d %d" % (limit, prec, len(pieces))
    for (mm, me, rm, re, cn, cd, nn, nd, be, blocks) in pieces:
        s += " P %d %d %d %d %d %d %d %d %d 0" % (mm, me, rm, re, cn, cd, nn, nd, be)
    return s

# ---------------------------------------------------------------- reading the answer

def parse(out):
    """out: list of lines.  Returns a list of records."""
    recs = []
    i = 0
    while i < len(out):
        t = out[i].split()
        if t[0] == 'I':
            recs.append({'I': [int(x) for x in t[1:]]})
            i += 1
            continue
        rec = {'status': int(t[0]), 'form': int(t[1]), 'len': int(t[2]),
               'canon': int(t[3]), 'untouched': int(t[4]), 'S': []}
        i += 1
        while i < len(out) and out[i].startswith('S '):
            rec['S'].append([int(x) for x in out[i].split()[1:]])
            i += 1
        recs.append(rec)
    return recs

def fam_of(recs):
    """The input family of a case, as (lo, hi, centre, radius) with exact rationals."""
    out = []
    for r in recs:
        if 'I' not in r:
            continue
        I = r['I']
        mid = F(I[0], I[1]); rad = F(I[2], I[3])
        A, H, d = I[4], I[5], I[6]
        out.append((mid - rad, mid + rad, F(A, d), F(H, d)))
    return out