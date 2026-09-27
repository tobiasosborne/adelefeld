#!/usr/bin/env python3
"""Brute-force check of the precision rules in docs/SPEC.md, section 4.

A profinite ball is the set a + N*Zhat with centre a in Q and radius N in Q, N >= 0
(N = 0: the exact rational a). The integers are dense in Zhat, so the ball is sampled by
a + N*k for integers k. For each rule we check (i) containment: every sampled result lies in
the predicted ball, and (ii) tightness: the predicted radius is the gcd of all sampled
differences, so no smaller ball contains the result set.
"""
from fractions import Fraction as F
from math import gcd
from functools import reduce
import itertools, random

def qgcd(*xs):
    """gcd of rationals: the positive generator of the group they generate (0 if all are 0)."""
    xs = [F(x) for x in xs if x != 0]
    if not xs:
        return F(0)
    den = reduce(lambda a, b: a * b // gcd(a, b), [x.denominator for x in xs])
    return F(reduce(gcd, [abs(int(x * den)) for x in xs]), den)

def add_rule(a, N, b, M):
    return a + b, qgcd(N, M)

def mul_rule(a, N, b, M):
    return a * b, qgcd(a * M, b * N, N * M)

def sample(a, N, K=6):
    return [a + N * k for k in range(-K, K + 1)] if N != 0 else [a]

def check(rule, op, a, N, b, M):
    c, R = rule(a, N, b, M)
    diffs = [op(x, y) - c for x in sample(a, N) for y in sample(b, M)]
    g = qgcd(*diffs)
    if R == 0:
        assert all(d == 0 for d in diffs)
    else:
        assert all((d / R).denominator == 1 for d in diffs), "containment fails"
        assert g == R, f"not tight: predicted {R}, sampled gcd {g}"
    return c, R

def show(name, a, N, b, M, rule, op):
    c, R = check(rule, op, F(a), F(N), F(b), F(M))
    print(f"{name}: ({a} mod {N}) , ({b} mod {M})  ->  {c} mod {R}   [centre reduced: {c % R if R else c}]")

if __name__ == "__main__":
    import operator as o
    show("add", 3, 12, 5, 18, add_rule, o.add)
    show("mul", 3, 12, 5, 18, mul_rule, o.mul)
    show("mul by exact 12", 12, 0, 5, 18, mul_rule, o.mul)
    show("mul by exact 1/3", F(1, 3), 0, 5, 18, mul_rule, o.mul)
    show("mul, rational centres", F(1, 2), 8, F(2, 3), 9, mul_rule, o.mul)
    random.seed(1)
    n = 0
    for _ in range(3000):
        a = F(random.randint(-40, 40), random.randint(1, 12))
        b = F(random.randint(-40, 40), random.randint(1, 12))
        N = F(random.randint(0, 60), random.randint(1, 6))
        M = F(random.randint(0, 60), random.randint(1, 6))
        check(add_rule, o.add, a, N, b, M); check(mul_rule, o.mul, a, N, b, M); n += 2
    print(f"random checks passed: {n}")
    # idempotents: solutions of x^2 = x modulo N number 2^(number of primes dividing N)
    for N in (30, 210, 2310):
        sols = [x for x in range(N) if (x * x - x) % N == 0]
        print(f"x^2 = x mod {N}: {len(sols)} solutions, e.g. {sols[:6]}")
    # unit classes: the idele class (t, c mod N) multiplies componentwise
    N = 36
    units = [c for c in range(N) if gcd(c, N) == 1]
    print(f"(Z/{N})^x has {len(units)} elements; 5*29 mod 36 = {5*29 % 36}")
