#!/usr/bin/env python3
"""lanes/i-slice3/ref_faults.py: do the checks of part 4 of proto/ideles_checks.py bite? Three faults, each made by
replacing one reference function in memory (the file is not changed), then the check that should see it is run.

  R1  ref_tight_parts: b_p = e_p instead of 1 + e_p for an odd p with (p - 1) | k       -> check_api_powers
  R2  ref_hull: the simple modulus N instead of lcm(N, 2)                                -> check_api_hulls
  R3  ref_div_fin: the coset c instead of its inverse                                    -> check_api_division

Run: timeout 120 python3 lanes/i-slice3/ref_faults.py
"""
import os
import sys
from fractions import Fraction as F

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "proto"))
import ideles_checks as R  # noqa: E402


def tight_parts_R1(Nb, k):
    fac = R.factor_word(abs(k))
    A = Nb
    for p, e in fac.items():
        if Nb % p == 0:
            A *= p ** e
    B = 1
    if Nb % 2 == 1 and k % 2 == 0:
        B *= 2 ** (2 + fac.get(2, 0))
    for d in R.divisors_of(fac):
        p = d + 1
        if p > 2 and Nb % p != 0 and R.is_prime_word(p):
            B *= p ** (0 + fac.get(p, 0))
    return A, B


def hull_R2(x):
    ball, r, (c, N) = x
    if N == 0:
        return ball, R.fb(r * c, 0)
    return ball, R.fb(r * R.odd_rep(c, N), r * N)


def div_fin_R3(fin, y):
    A, H, d = fin
    a, M = F(A, d), F(H, d)
    _, r, (c, N) = y
    if N == 0:
        return R.fb(a * c / r, M / r)
    return R.fb(a * R.odd_rep(c, N) / r, R.qgcd(abs(a) * R.lcm(N, 2), M) / r)


for name, attr, fake, check in (("R1", "ref_tight_parts", tight_parts_R1, R.check_api_powers),
                                ("R2", "ref_hull", hull_R2, R.check_api_hulls),
                                ("R3", "ref_div_fin", div_fin_R3, R.check_api_division)):
    good = getattr(R, attr)
    setattr(R, attr, fake)
    R.FAILURES.clear()
    try:
        check()
        caught = bool(R.FAILURES)
    except Exception as e:          # an assertion inside the reference also counts as caught, and is named
        print(f"   ({type(e).__name__}: {e})")
        caught = True
    setattr(R, attr, good)
    print(f"{name} ({attr}): {'caught' if caught else 'NOT CAUGHT'}")
