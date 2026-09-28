#!/usr/bin/env python3
"""Oracle for adele_prec.c: exact rational arithmetic with fractions.Fraction only.

Reads the lines of adele_prec on stdin and checks, per line:
- the real output interval [r_lo, r_hi] contains the exact image of [x_lo, x_hi] under
  x -> x + q, x * q, x / q (for div only when the status is OK), and is finite;
- set: the real ball contains q; if q is dyadic with an odd mantissa of at most prec bits, the
  ball is the single point q (the claim of adele.h:102);
- the finite part: add: (a + q) + N Zhat; mul: q a + |q| N Zhat (the exact 0 for q = 0);
  div: a/q + N/|q| Zhat; set: the exact q. The canonical triple is computed here from the centre
  and the radius by Lemma 5.2 of conventions (d0 = lcm(den a, den N), H0 = N d0, A0 = a d0 mod H0),
  which is not the route of fball.c (gcd of the raw triple);
- the output is canonical (adf_adele_is_canonical = 1).
Prints counts and the first failures.
"""
import sys
from fractions import Fraction
from math import lcm


def dy(tok):
    m, e = tok.split("*2^")
    m, e = int(m), int(e)
    return Fraction(m) * (Fraction(2) ** e)


def canon(a, N):
    if N == 0:
        return (a.numerator, 0, a.denominator)
    d0 = lcm(a.denominator, N.denominator)
    H0 = int(N * d0)
    A0 = int(a * d0) % H0
    return (A0, H0, d0)


def centre_radius(A, H, d):
    return Fraction(A, d), Fraction(H, d)


def dyadic_exact_claim(q, prec):
    if q.denominator & (q.denominator - 1):
        return False
    m = abs(q.numerator)
    if m == 0:
        return True
    while m % 2 == 0:
        m //= 2
    return m.bit_length() <= prec


def main():
    n = 0
    fails = []
    counts = {}
    for line in sys.stdin:
        parts = [p.strip() for p in line.split("|")]
        head = parts[0].split()
        op, prec, alias, st = head[0], int(head[1]), int(head[2]), int(head[3])
        xi = parts[1].split()
        q = Fraction(parts[2])
        ri = parts[3].split()
        fin_in = tuple(int(t) for t in parts[4].split())
        fin_out = tuple(int(t) for t in parts[5].split())
        canon_flag = int(parts[6])
        n += 1
        key = (op, prec)
        counts[key] = counts.get(key, 0) + 1
        bad = []
        if op == "div" and q == 0:
            if st != 6:
                bad.append("div by 0 without NOT_UNIT")
            continue_real = False
        else:
            continue_real = True
        if continue_real:
            if ri[2] != "1":
                bad.append("real part not finite")
            else:
                rlo, rhi = dy(ri[0]), dy(ri[1])
                if op == "set":
                    lo = hi = q
                else:
                    xlo, xhi = dy(xi[0]), dy(xi[1])
                    if op == "add":
                        lo, hi = xlo + q, xhi + q
                    elif op == "mul":
                        lo, hi = sorted((xlo * q, xhi * q))
                    else:
                        lo, hi = sorted((xlo / q, xhi / q))
                if not (rlo <= lo and hi <= rhi):
                    bad.append("real enclosure fails: [%s, %s] not in [%s, %s]" % (lo, hi, rlo, rhi))
                if op == "set" and dyadic_exact_claim(q, prec) and not (rlo == rhi == q):
                    bad.append("set_rat: dyadic q with <= prec-bit odd mantissa not exact")
            a, N = centre_radius(*fin_in)
            if op == "add":
                exp = canon(a + q, N)
            elif op == "mul":
                exp = canon(q * a, abs(q) * N) if q != 0 else (0, 0, 1)
            elif op == "div":
                exp = canon(a / q, N / abs(q))
            else:
                exp = canon(q, Fraction(0))
            if fin_out != exp:
                bad.append("finite part %s, expected %s" % (fin_out, exp))
            if canon_flag != 1:
                bad.append("output not canonical")
        if bad:
            fails.append((line.strip()[:300], bad))
    print("cases:", n)
    for k in sorted(counts):
        print("  %s prec=%d: %d" % (k[0], k[1], counts[k]))
    print("failures:", len(fails))
    for l, b in fails[:10]:
        print("  ", b, "\n     ", l)


main()
