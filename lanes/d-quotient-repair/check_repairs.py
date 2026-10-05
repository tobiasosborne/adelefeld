#!/usr/bin/env python3
"""Checks for the repair texts R1 and R3 of lane d-quotient-repair, in exact rationals.

R1: Q1 step 3, the two cases `u < 2^e` and `u = 2^e`, for a radius just below, at and just above a
power of two, with the kernel of `proto/quotient3_checks.py` (imported, not re-implemented).
R3: `dist(t,Z) = 1/2 - dist(t-1/2,Z)` and `cos(2 pi (1/2-x)) = -cos(2 pi x)` on rational samples.

Run: timeout 100 python3 lanes/d-quotient-repair/check_repairs.py
"""
from fractions import Fraction as F
from math import ceil, floor
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'proto'))
import quotient3_checks as O  # noqa: E402


def ru(d):
    """Least 30-bit number >= d, and the binade exponent e with 2^(e-1) <= d < 2^e."""
    e = d.numerator.bit_length()-d.denominator.bit_length()+1
    while d < F(2)**(e-1):
        e -= 1
    while d >= F(2)**e:
        e += 1
    return F(ceil(d/F(2)**(e-30)))*F(2)**(e-30), e


def case_R1():
    n = worst_case = 0
    rows = []
    for k in range(-40, 1):
        e = k
        for num, den in ((-3, 4), (-1, 2), (-1, 64), (0, 1), (1, 64), (1, 2), (3, 4),
                         (-3, 2**40), (-1, 2**40), (1, 2**40), (2**39-1, 2**40), (2**39+1, 2**40)):
            d = F(2)**e + F(num, den)
            if d <= 0:
                continue
            u, eb = ru(d)
            rho = O.q1_radius(d)
            bound = F(1, 2**28)*d
            assert rho >= d and rho == u + F(2)**((eb if u < F(2)**eb else eb+1)-30)
            assert rho-d <= bound, (e, num, den, rho-d, bound)
            s = F(2)**(eb-30)
            if u == F(2)**eb:                   # case u = 2^e: the successor is two spacings
                assert rho-d < 3*s and F(1, 2**28)*d > 4*s - F(1, 2**28)*s > 3*s
                worst_case = max(worst_case, (rho-d)/d)
                rows.append((e, num, den, 'u = 2^e', rho-d, bound, (rho-d)/d))
            else:                               # case u < 2^e: one spacing each way
                assert rho-d < 2*s and 2*s == F(1, 2**28)*F(2)**(eb-1) <= F(1, 2**28)*d
                rows.append((e, num, den, 'u < 2^e', rho-d, bound, (rho-d)/d))
            n += 1
    tight = max(r[6] for r in rows)
    two = [r for r in rows if r[3] == 'u = 2^e']
    one = [r for r in rows if r[3] == 'u < 2^e']
    print(f'RESULT R1: {n} radii, {len(one)} with u < 2^e and {len(two)} with u = 2^e; '
          f'worst (rho-d)/d = {tight} = {float(tight):.4e}; '
          f'bound 2^-28 = {2.0**-28:.4e}; ratio to bound {float(tight/2.0**-28):.6f}')
    for name, group in (('u < 2^e', one), ('u = 2^e', two)):
        if group:
            r = max(group, key=lambda g: g[6])
            print(f'RESULT R1 case {name}: worst at e={r[0]} offset {r[1]}/{r[2]}: '
                  f'rho-d = {r[4]}, 2^-28 d = {r[5]}, '
                  f'fraction of the bound {float(r[6]/(F(1, 2**28))):.6f}')
    for r in rows:
        if r[0] == 0:
            print(f'RESULT R1 e=0 offset={r[1]}/{r[2]} case {r[3]}: rho-d = {r[4]}, '
                  f'2^-28 d = {r[5]}, fraction of the bound {float(r[6]/(F(1,2**28))):.6f}')
    return n


def case_R3():
    n = 0
    for den in range(1, 25):
        for num in range(-2*den, 2*den):
            t = F(num, den)
            u = t % 1
            dist = min(u, 1-u)
            shifted = ((t-F(1, 2)) % 1)
            assert dist == F(1, 2) - min(shifted, 1-shifted)
            for x in (F(0), F(1, 4), F(1, 2), dist):
                import mpmath as mp
                mp.mp.dps = 60
                a = mp.cos(2*mp.pi*mp.mpf(x.numerator)/x.denominator)
                b = mp.cos(2*mp.pi*mp.mpf((F(1, 2)-x).numerator)/(F(1, 2)-x).denominator)
                assert abs(a+b) < mp.mpf('1e-55')
            n += 1
    print(f'RESULT R3: {n} rational t with dist(t,Z) = 1/2 - dist(t-1/2,Z); '
          f'cos(2 pi (1/2-x)) + cos(2 pi x) below 1e-55 in 4 cases each')
    return n


if __name__ == '__main__':
    a, b = case_R1(), case_R3()
    print(f'{a+b} repair checks')