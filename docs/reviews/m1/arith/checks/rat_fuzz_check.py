#!/usr/bin/env python3
"""Oracle for rat_fuzz.c with fractions.Fraction."""
import sys
from fractions import Fraction

fails = []
n = 0
for line in sys.stdin:
    p = [t.strip() for t in line.split("|")]
    n1, d1, n2, d2 = (int(t) for t in p[0].split())
    s1, xs = p[1].split()
    s2, ys = p[2].split()
    s1, s2 = int(s1), int(s2)
    x, y = Fraction(xs), Fraction(ys)
    bad = []
    wx = Fraction(5, 7) if d1 == 0 else Fraction(n1, d1)
    wy = Fraction(5, 7) if d2 == 0 else Fraction(n2, d2)
    if (s1 != (7 if d1 == 0 else 0)) or x != wx:
        bad.append("set_fmpq")
    if (s2 != (7 if d2 == 0 else 0)) or y != wy:
        bad.append("set_fmpz2")
    if Fraction(p[3]) != x + y: bad.append("add")
    if Fraction(p[4]) != x - y: bad.append("sub")
    if Fraction(p[5]) != x * y: bad.append("mul")
    if Fraction(p[6]) != -x: bad.append("neg")
    sd, vd = p[7].split()
    if y == 0:
        if int(sd) != 6 or Fraction(vd) != Fraction(-99, 7): bad.append("div by 0")
    elif int(sd) != 0 or Fraction(vd) != x / y: bad.append("div")
    si, vi = p[8].split()
    if y == 0:
        if int(si) != 6 or Fraction(vi) != Fraction(-99, 7): bad.append("inv of 0")
    elif int(si) != 0 or Fraction(vi) != 1 / y: bad.append("inv")
    sg = (x > 0) - (x < 0)
    if int(p[9]) != sg: bad.append("sgn")
    if int(p[10]) != int(x == y): bad.append("equal")
    if int(p[11]) != int(y == 0): bad.append("is_zero")
    if int(p[12]) != 1: bad.append("not canonical")
    if int(p[13]) != 1: bad.append("aliasing")
    n += 1
    if bad:
        fails.append((line.strip()[:200], bad))
print("cases:", n, "failures:", len(fails))
for l, b in fails[:10]:
    print("  ", b, l)
