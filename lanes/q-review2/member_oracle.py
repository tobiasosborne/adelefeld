#!/usr/bin/env python3
"""lanes/q-review2/member_oracle.py: the represented set of a LIFT class by exact membership of
rational points of A/Q, against set_adele, set_rat, add_rat, get_piece, set and swap.

A point (t ; w) with t, w rational lies in the class of the adele (I x (a + N Zhat)) exactly when
some rational translation moves it into the adele, that is when there is q in Q with t + q in I and
w + q in a + N Zhat. That decision is exact (a two-line argument, written out in the report).

Usage: timeout 170 python3 member_oracle.py <seed> <nlifts>
"""
import os
import random
import subprocess
import sys
from fractions import Fraction as F
from math import ceil, floor


class C:
    def __init__(self, exe):
        self.p = subprocess.Popen([exe], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  text=True, bufsize=1)

    def cmd(self, s):
        self.p.stdin.write(s + "\n")
        self.p.stdin.flush()
        return self.p.stdout.readline().rstrip("\n")

    def piece(self):
        f = self.cmd("PIECE").split()
        assert f[0] == 'P', f
        return dict(mid=F(int(f[1]), int(f[2])), rad=F(int(f[3]), int(f[4])),
                    A=int(f[5]), H=int(f[6]), d=int(f[7]), backend=int(f[8]), canon=int(f[9]))

    def close(self):
        self.p.stdin.close()
        self.p.wait()


def member(p, t, w):
    """Is (t ; w) in the class of the piece p? Exact."""
    mid, rad, A, H, d = p['mid'], p['rad'], p['A'], p['H'], p['d']
    lo, hi = mid - rad, mid + rad
    a0 = F(A, d)
    if H == 0:
        q = w - a0
        return lo <= t + q <= hi
    alpha = d * (lo - t)
    beta = d * (hi - t)
    c = d * (w - a0)
    return ceil(F(alpha - c, H)) <= floor(F(beta - c, H))


def rand_rat(rng, maxbits):
    n = rng.randrange(1, maxbits + 1)
    a = rng.randrange(-(2 ** n), 2 ** n)
    b = rng.randrange(1, 2 ** rng.randrange(1, min(n, 20) + 1))
    return F(a, b)


def main():
    seed = int(sys.argv[1]) if len(sys.argv) > 1 else 3
    nlifts = int(sys.argv[2]) if len(sys.argv) > 2 else 200
    exe = os.environ.get('MEMBER_EVAL', './lanes/q-review2/member_eval')
    c = C(exe)
    rng = random.Random(seed)
    bad = 0
    npoints = 200
    for k in range(nlifts):
        mid = F(rng.randrange(-2 ** 20, 2 ** 20), 2 ** rng.randrange(0, 12))
        rad = F(rng.randrange(0, 2 ** 12), 2 ** rng.randrange(0, 10))
        H = rng.choice([0, 0, 1, 2, 3, 5, 8, 13, 30])
        d = rng.choice([1, 1, 1, 2, 3, 4, 6])
        A = rng.randrange(0, H) if H > 0 else rng.randrange(-50, 50)
        # the canonical triple (A, H, d) must satisfy G
        from math import gcd
        if H == 0:
            g = gcd(abs(A), d)
            A, d = A // g, d // g
        else:
            g = gcd(gcd(A, H), d)
            A, H, d = A // g, H // g, d // g
        # a dyadic mid and a dyadic radius, as the harness writes them
        def split(x):
            num, den = x.numerator, x.denominator
            e = 0
            while den % 2 == 0:
                den //= 2
                e += 1
            return num, e
        mn, me = split(mid)
        rn, re_ = split(rad)
        while rn % 2 == 0 and rn != 0:
            rn //= 2
            re_ -= 1        # rn * 2^-re_ is unchanged
        c.cmd("NEW")
        c.cmd("AD %d %d %d %d %d %d %d 1" % (mn, -me, rn, -re_, A, H, d))
        got = c.piece()
        want = dict(mid=mid, rad=rad, A=A, H=H, d=d, backend=0)
        for key in ('mid', 'rad', 'A', 'H', 'd'):
            if got[key] != want[key]:
                bad += 1
                print("SET ADELE CHANGES %s: got %s want %s" % (key, got[key], want[key]))
        if got['canon'] != 1:
            bad += 1
            print("NOT CANONICAL after set_adele: %s" % got)
        points = [(rand_rat(rng, 24), rand_rat(rng, 24)) for _ in range(npoints)]
        # the class before the translation
        for (t, w) in points:
            if member(got, t, w) != member(want, t, w):
                bad += 1
                print("MEMBERSHIP before add_rat at %s: %s" % ((t, w), got))
                break
        # add_rat by a rational of 1 to 2000 bits
        c.cmd("COPY")
        q = rand_rat(rng, 2000)
        c.cmd("ADD %d %d" % (q.numerator, q.denominator))
        got2 = c.piece()
        idf = c.cmd("IDENT").split()
        if idf[1] != '1':
            bad += 1
            print("ADD_RAT CHANGED THE REPRESENTATION")
        for key in ('mid', 'rad', 'A', 'H', 'd'):
            if got2[key] != got[key]:
                bad += 1
                print("ADD_RAT CHANGES %s: %s vs %s" % (key, got2[key], got[key]))
        # the set must be the same, so the membership answers are the same
        for (t, w) in points:
            if member(got2, t, w) != member(want, t, w):
                bad += 1
                print("MEMBERSHIP after add_rat at %s" % ((t, w),))
                break
        # set_rat: the zero class
        c.cmd("RAT %d %d" % (q.numerator, q.denominator))
        z = c.piece()
        if (z['mid'], z['rad'], z['A'], z['H'], z['d']) != (F(0), F(0), 0, 0, 1):
            bad += 1
            print("SET_RAT NOT THE ZERO CLASS: %s" % z)
        zero = dict(mid=F(0), rad=F(0), A=0, H=0, d=1)
        for (t, w) in points[:50]:
            if member(z, t, w) != member(zero, t, w):
                bad += 1
                print("SET_RAT MEMBERSHIP at %s" % ((t, w),))
                break
        # aliasing: set, swap and add_rat with the same object
        c.cmd("AD %d %d %d %d %d %d %d 0" % (mn, -me, rn, -re_, A, H, d))
        before = c.piece()
        c.cmd("SELF set")
        if c.piece() != before:
            bad += 1
            print("SELF SET CHANGED THE VALUE")
        c.cmd("SELF swap")
        if c.piece() != before:
            bad += 1
            print("SELF SWAP CHANGED THE VALUE")
        c.cmd("SELF add")
        if c.piece() != before:
            bad += 1
            print("SELF ADD_RAT CHANGED THE VALUE")
        # get_piece out of range leaves the destination alone
        for i in (-1, 1, 2 ** 62, 9223372036854775807, -9223372036854775808):
            f = c.cmd("GET %d" % i).split()
            if f[1] != '7' or f[3] != '12345' or f[7] != '777':
                bad += 1
                print("GET %d NOT DOMAIN/UNTOUCHED: %s" % (i, f))
        f = c.cmd("GET 0").split()
        if f[1] != '0':
            bad += 1
            print("GET 0 NOT OK: %s" % f)
        elif (F(int(f[2]), int(f[3])) != before['mid'] or F(int(f[4]), int(f[5])) != before['rad']
              or int(f[6]) != before['A'] or int(f[7]) != before['H'] or int(f[8]) != before['d']):
            bad += 1
            print("GET 0 RETURNED A DIFFERENT PIECE: %s vs %s" % (f, before))
    print("lifts %d, points per lift %d, total membership decisions %d" % (nlifts, npoints,
                                                                         nlifts * npoints * 3))
    c.close()
    print("problems %d" % bad)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
