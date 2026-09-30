#!/usr/bin/env python3
"""lanes/f-slice6/gen_vectors.py: vectors of exp, log, Log of an EXACT rational at a prime, at absolute precision N.

Independent of src/lfunc.c and of proto/lfunc_checks.py: exact Fractions only.
  exp(x)  = sum x^k/k!                 for v_p(x) >= c   (c = 1 for odd p, 2 for p = 2)   functions.md Definition 1
  log(x)  = sum (-1)^(k+1) (x-1)^k/k   for v_p(x-1) >= 1
  Log(x)  = log(u) where x = p^m w u, w the Teichmueller factor (odd p: w = a^(p^n) mod p^n, a the unit part) or
            the sign +-1 (p = 2: w such that a/w = 1 mod 4), u a principal unit.   Proposition 4.
The value is reduced modulo p^N: the centre in [0, p^N). Exact results (the requested N is then not used):
exp(0) = 1, log(1) = 0, log(-1) = 0 at p = 2, Log(+-p^m) = 0.
Output: one JSON line per case: f, p, xn, xd, N, status, and on OK: exact (0/1), K, c (decimal string of the centre,
or of the value of an exact result).
Usage: python3 lanes/f-slice6/gen_vectors.py > tests/ref/vectors/f-slice6/at_prime.jsonl
"""
import json
import sys
from fractions import Fraction


def vp(n, p):
    n = abs(n)
    assert n != 0
    e = 0
    while n % p == 0:
        n //= p
        e += 1
    return e


def vpq(x, p):
    return vp(x.numerator, p) - vp(x.denominator, p)


def red(x, p, K):
    """x a Fraction that is a p-adic integer (denominator prime to p): x mod p^K in [0, p^K)."""
    assert x.denominator % p != 0
    m = p ** K
    return (x.numerator * pow(x.denominator, -1, m)) % m


def exp_series(x, p, K):
    total = Fraction(0)
    term = Fraction(1)
    k = 0
    while k < 6 * K + 60:
        total += term
        k += 1
        term = term * x / k
    # every term is a p-adic integer on the domain; terms of valuation >= K vanish modulo p^K
    return red(total, p, K)


def log_series(x, p, K):
    y = x - 1
    assert vpq(y, p) >= 1
    total = Fraction(0)
    pw = Fraction(1)
    for k in range(1, 12 * K + 80):
        pw *= y
        total += Fraction((-1) ** (k + 1), k) * pw
    return red(total, p, K)


def Log_series(x, p, K):
    m = vpq(x, p)
    a = x / Fraction(p) ** m           # a unit at p
    n = K + 6
    mod = p ** n
    if p != 2:
        ai = red(a, p, n)
        w = pow(ai, p ** n, mod)
        assert pow(w, p - 1, mod) == 1
        u = Fraction(ai * pow(w, -1, mod) % mod)    # a principal unit, known modulo p^n
    else:
        w = 1 if red(a, 2, 2) == 1 else -1
        u = a / w
        assert red(u, 2, 2) == 1
    return log_series(u, p, K)


def case(f, p, x, N):
    c1 = 1 if p != 2 else 2
    rec = {"f": f, "p": p, "xn": x.numerator, "xd": x.denominator, "N": N}
    if f == "exp":
        if x == 0:
            return dict(rec, status="OK", exact=1, K=0, c="1")
        if vpq(x, p) < c1:
            return dict(rec, status="DOMAIN")
        return dict(rec, status="OK", exact=0, K=N, c=str(exp_series(x, p, N)))
    if f == "log":
        if x == 1 or (p == 2 and x == -1):
            return dict(rec, status="OK", exact=1, K=0, c="0")
        if x == 0 or vpq(x - 1, p) < 1:
            return dict(rec, status="DOMAIN")
        return dict(rec, status="OK", exact=0, K=N, c=str(log_series(x, p, N)))
    assert f == "Log"
    if x == 0:
        return dict(rec, status="DOMAIN")
    if abs(x) == Fraction(p) ** vpq(x, p):
        return dict(rec, status="OK", exact=1, K=0, c="0")
    return dict(rec, status="OK", exact=0, K=N, c=str(Log_series(x, p, N)))


XS = [Fraction(n, d) for n, d in [
    (0, 1), (1, 1), (-1, 1), (2, 1), (3, 1), (4, 1), (5, 1), (6, 1), (7, 1), (8, 1), (9, 1), (10, 1), (12, 1), (16, 1),
    (25, 1), (31, 1), (49, 1), (50, 1), (-3, 1), (-6, 1), (1, 2), (1, 3), (1, 5), (1, 7), (2, 3), (5, 3), (20, 3),
    (3, 4), (-3, 4), (7, 25), (25, 6), (-10, 7), (11, 5), (13, 9), (17, 2), (65, 4), (100, 3), (-1, 25),
]]


def main():
    for p in (2, 3, 5, 7):
        for f in ("exp", "log", "Log"):
            for x in XS:
                for N in (1, 4, 9, 16):
                    sys.stdout.write(json.dumps(case(f, p, x, N), sort_keys=True) + "\n")


if __name__ == "__main__":
    main()
