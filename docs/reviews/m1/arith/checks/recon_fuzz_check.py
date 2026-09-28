#!/usr/bin/env python3
"""Oracle for recon_fuzz.c: enumerate the rationals of the finite ball a + N Z that lie in the
closed interval [lo, hi], by walking k outwards from a point near lo (no floor/ceil formula of the
code; a walk with a stop when the point leaves the interval). Decide: 1 -> OK and the value,
0 -> NO_SOLUTION (5), >= 2 -> NOT_UNIQUE (4). On a failure the output must be the marker -99/7.
"""
import sys
from fractions import Fraction

OK, NOT_UNIQUE, NO_SOLUTION = 0, 4, 5


def candidates(a, N, lo, hi, cap=3):
    if lo > hi:
        return []
    if N == 0:
        return [a] if lo <= a <= hi else []
    # start from a lattice point at or below lo: k0 = an integer with a + N k0 <= lo
    k = int((lo - a) / N) - 2
    while a + N * k > lo:
        k -= 1
    out = []
    while a + N * k <= hi and len(out) < cap:
        p = a + N * k
        if p >= lo:
            out.append(p)
        k += 1
    return out


def main():
    counts = {}
    fails = []
    n = 0
    for line in sys.stdin:
        parts = [p.strip() for p in line.split("|")]
        kind, A, H, d = parts[0].split()
        A, H, d = int(A), int(H), int(d)
        a, N = Fraction(A, d), Fraction(H, d)
        if kind == "F":
            lo, hi = Fraction(parts[1]), Fraction(parts[2])
        else:
            m, r = Fraction(parts[1]), Fraction(parts[2])
            lo, hi = m - r, m + r
        st = int(parts[3])
        q = Fraction(parts[4])
        c = candidates(a, N, lo, hi)
        want = OK if len(c) == 1 else (NO_SOLUTION if not c else NOT_UNIQUE)
        n += 1
        counts[(kind, want)] = counts.get((kind, want), 0) + 1
        bad = []
        if st != want:
            bad.append("status %d expected %d (candidates %s)" % (st, want, c[:3]))
        elif want == OK and q != c[0]:
            bad.append("value %s expected %s" % (q, c[0]))
        elif want != OK and q != Fraction(-99, 7):
            bad.append("output written on failure")
        if kind == "F" and parts[5] != "1":
            bad.append("aliased call differs")
        if bad:
            fails.append((line.strip()[:300], bad))
    print("cases:", n, "by (call, expected status):", dict(sorted(counts.items())))
    print("failures:", len(fails))
    for l, b in fails[:10]:
        print("  ", b, "\n     ", l)


main()
