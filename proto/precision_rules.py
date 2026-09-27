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


# ---------------------------------------------------------------------------------------------
# Added for draft 2 of the specification (after the design review of 2026-09-27).
# ---------------------------------------------------------------------------------------------
def is_int(x):
    return F(x).denominator == 1

def equal_set(a, N, b, M):
    if N == 0 or M == 0:
        return N == M and a == b
    return N == M and is_int((a - b) / N)

def overlaps(a, N, b, M):
    g = qgcd(N, M)
    return a == b if g == 0 else is_int((a - b) / g)

def contains(a, N, b, M):       # is a + N Zhat inside b + M Zhat ?
    if M == 0:
        return N == 0 and a == b          # a ball of positive radius is never inside a single point
    if N == 0:
        return is_int((a - b) / M)
    return is_int(N / M) and is_int((a - b) / M)

def scaled_add(s, u, t, v, K):
    g = qgcd(s, t); A, B = s / g, t / g
    return g, int(A * u + B * v) % K

def scaled_mul(s, u, t, v, K):
    return s * t, (u * v) % K

def draft2_checks():
    random.seed(2)
    # scaled residue policy: result (scale g, residue w) is the ball g*w + g*K*Zhat; it must contain the tight result
    n = 0
    for _ in range(20000):
        K = random.randint(1, 60)
        s = F(random.randint(1, 30), random.randint(1, 12)); t = F(random.randint(1, 30), random.randint(1, 12))
        u = random.randrange(K); v = random.randrange(K)
        a, N, b, M = s * u, s * K, t * v, t * K
        g, w = scaled_add(s, u, t, v, K); c, R = add_rule(a, N, b, M)
        assert equal_set(c, R, g * w, g * K), "scaled sum is not the tight sum"
        g, w = scaled_mul(s, u, t, v, K); c, R = mul_rule(a, N, b, M)
        assert contains(c, R, g * w, g * K), "scaled product does not enclose"
        assert R == s * t * K * gcd(gcd(u, v), K), "tight radius formula"
        n += 2
    print(f"scaled residue policy: {n} checks passed")
    # a fixed radius is not a policy: (1 mod 2) * (1/2) = 1/2 mod 1, and 1/2 mod 2 does not contain it
    c, R = mul_rule(F(1), F(2), F(1, 2), F(0))
    print(f"(1 mod 2) * 1/2 = {c} mod {R}; contained in 1/2 mod 2: {contains(c, R, F(1,2), F(2))}")
    assert not contains(c, R, F(1, 2), F(2))
    # overlap is not transitive
    print("0 mod 2 ~ 0 mod 1:", overlaps(F(0), F(2), F(0), F(1)), "; 0 mod 1 ~ 1 mod 2:", overlaps(F(0), F(1), F(1), F(2)),
          "; 0 mod 2 ~ 1 mod 2:", overlaps(F(0), F(2), F(1), F(2)))
    # fractional radius splits into B integer-radius balls: a + (A/B) Zhat = union of a + k A/B + A Zhat
    a, N = F(1, 3), F(3, 2)
    A, B = N.numerator, N.denominator
    pieces = [(a + k * N, F(A)) for k in range(B)]
    assert all(contains(c, r, a, N) for c, r in pieces)
    assert all(not overlaps(pieces[i][0], pieces[i][1], pieces[j][0], pieces[j][1]) for i in range(B) for j in range(i))
    assert all(any(contains(a + N * k, F(0) + A * 10**6, c, r) for c, r in pieces) for k in range(-20, 21))
    print(f"{a} mod {N} splits into {B} disjoint balls of radius {A}")
    # the additive character on a fractional radius is not determined: 0 mod 1/2 contains 0 and 1/2
    import cmath
    vals = {complex(round(cmath.exp(2j * cmath.pi * float(x)).real, 12), 0) for x in (F(0), F(1, 2))}
    print("psi_f on 0 mod 1/2 takes the values", sorted(v.real for v in vals))
    # rationals in a full ball: (a + N Zhat) meet Q = a + N Z ; 1/5 = 5 mod 6 as a residue but is not in 5 + 6 Zhat
    print("1/5 in 5 + 6 Zhat:", is_int((F(1, 5) - 5) / 6), "; 5*5 mod 6 =", 5 * 5 % 6)
    # weighted finite Fourier transform, D = 2, M = 3
    D, M = 2, 3; L = D * M
    f = [complex(random.random(), random.random()) for _ in range(L)]
    w = lambda x: cmath.exp(-2j * cmath.pi * x)
    g = [sum(f[j] * w(F(j * k, L)) for j in range(L)) / M for k in range(L)]
    h = [sum(g[k] * w(F(j * k, L)) for k in range(L)) / D for j in range(L)]
    err = max(abs(h[j] - f[(-j) % L]) for j in range(L))
    pl = abs(sum(abs(x) ** 2 for x in f) / M - sum(abs(x) ** 2 for x in g) / D)
    print(f"weighted transform twice = reflection: error {err:.1e}; Plancherel error {pl:.1e} (floating point, not certified)")
    assert err < 1e-12 and pl < 1e-12

# ---------------------------------------------------------------------------------------------
# Added for draft 3 (after review round 2): zero radii in the predicates; catalogue functions of SPEC 9.3.7.
# ---------------------------------------------------------------------------------------------
from math import comb, factorial

def legendre(a, p):
    a %= p
    return 0 if a == 0 else (1 if pow(a, (p - 1) // 2, p) == 1 else -1)

def val_unit(a, p):             # a non-zero rational -> (valuation, unit part as a Fraction)
    a = F(a); m = 0
    while a.numerator % p == 0: a /= p; m += 1
    while a.denominator % p == 0: a *= p; m -= 1
    return m, a

def hilbert(a, b, p):           # p a prime, or 0 for the real place
    if p == 0:
        return -1 if (a < 0 and b < 0) else 1
    al, u = val_unit(a, p); be, w = val_unit(b, p)
    if p == 2:
        U = u.numerator * pow(u.denominator, -1, 8) % 8; W = w.numerator * pow(w.denominator, -1, 8) % 8
        eps = lambda x: ((x - 1) // 2) % 2
        om = lambda x: ((x * x - 1) // 8) % 2
        return (-1) ** ((eps(U) * eps(W) + al * om(W) + be * om(U)) % 2)
    U = u.numerator * pow(u.denominator, -1, p) % p; W = w.numerator * pow(w.denominator, -1, p) % p
    return (-1) ** ((al * be * ((p - 1) // 2)) % 2) * legendre(U, p) ** (be % 2) * legendre(W, p) ** (al % 2)

def primes_of(*xs):
    ps = set()
    for x in xs:
        for n in (abs(F(x).numerator), F(x).denominator):
            d = 2
            while d * d <= n:
                while n % d == 0: ps.add(d); n //= d
                d += 1
            if n > 1: ps.add(n)
    return ps

def hilbert_has_solution(a, b, p, k=3):
    """brute force: a, b p-adic unit or p times unit integers; is a x^2 + b y^2 = z^2 solvable primitively mod p^k?"""
    q = p ** k
    return any((a * x * x + b * y * y - z * z) % q == 0
               for x in range(q) for y in range(q) for z in range(q) if (x % p or y % p or z % p))

def draft3_checks():
    random.seed(3)
    # predicates with zero radii
    assert overlaps(F(1), F(0), F(1), F(0)) and contains(F(1), F(0), F(1), F(0)) and equal_set(F(1), F(0), F(1), F(0))
    assert contains(F(7), F(0), F(1), F(3)) and not contains(F(1), F(3), F(1), F(0)) and not overlaps(F(1), F(0), F(2), F(0))
    assert overlaps(F(7), F(0), F(1), F(3)) and not overlaps(F(8), F(0), F(1), F(3))
    print("predicates with zero radii: ok")
    # binomial coefficient on a ball a + N Zhat: result known modulo N / gcd(N, k!)
    n = 0
    for _ in range(3000):
        N = random.randint(1, 400); a = random.randrange(N); k = random.randint(0, 9)
        R = N // gcd(N, factorial(k))
        ref = comb(a, k) if a >= k else 0
        def binom(x):           # polynomial value, also for negative x
            num = 1
            for i in range(k): num *= (x - i)
            assert num % factorial(k) == 0
            return num // factorial(k)
        assert all((binom(a + N * t) - binom(a)) % R == 0 for t in range(-8, 9)); n += 1
    print(f"binomial enclosure modulo N/gcd(N,k!): {n} checks passed")
    # profinite power: c^x for x = e mod M is determined modulo N exactly when c^M = 1 mod N
    n = 0
    for _ in range(3000):
        N = random.randint(2, 300); c = random.randrange(1, N)
        if gcd(c, N) != 1: continue
        M = random.randint(1, 60); e = random.randrange(M)
        vals = {pow(c, e + M * t, N) for t in range(0, 40)}
        assert (len(vals) == 1) == (pow(c, M, N) == 1); n += 1
    print(f"profinite power criterion: {n} checks passed")
    # Hilbert symbol: product formula over all places, and the definition by brute force at small primes
    n = 0
    for _ in range(2000):
        a = F(random.choice([-1, 1]) * random.randint(1, 200), random.randint(1, 30))
        b = F(random.choice([-1, 1]) * random.randint(1, 200), random.randint(1, 30))
        prod = hilbert(a, b, 0)
        for p in primes_of(a, b) | {2}:
            prod *= hilbert(a, b, p)
        assert prod == 1; n += 1
    print(f"Hilbert symbol product formula: {n} rational pairs passed")
    m = 0
    for p, k in ((2, 4), (3, 2), (5, 2)):
        for a in range(1, 2 * p * 4):
            for b in range(1, 2 * p * 4):
                if val_unit(a, p)[0] > 1 or val_unit(b, p)[0] > 1: continue
                assert (hilbert(a, b, p) == 1) == hilbert_has_solution(a, b, p, k), (a, b, p); m += 1
    print(f"Hilbert symbol against the definition (solvability modulo p^k, k = 4, 2, 2): {m} pairs passed")

if __name__ == "__main__":
    draft2_checks()
    draft3_checks()
