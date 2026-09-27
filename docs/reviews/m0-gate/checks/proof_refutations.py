#!/usr/bin/env python3
"""Independent finite/exact refutation attempts for the repaired algebraic statements.

All mathematics here is derived in this file or in the accompanying review. No prototype formulas
are imported. Fractions are exact. A finite search is not presented as a proof.
"""
from fractions import Fraction as F
from math import gcd, lcm
from functools import reduce
import random
from math import prod

counts = {}


def check(group, condition):
    counts[group] = counts.get(group, 0) + 1
    assert condition, (group, counts[group])


def val(x, p):
    x = F(x)
    if not x:
        return 1000000
    n, d, v = abs(x.numerator), x.denominator, 0
    while n % p == 0:
        n //= p
        v += 1
    while d % p == 0:
        d //= p
        v -= 1
    return v


def qgcd(*xs):
    xs = list(map(F, xs))
    den = lcm(*(x.denominator for x in xs))
    return F(reduce(gcd, (int(x * den) for x in xs), 0), den)


def triple(c, radius):
    c, radius = F(c), F(radius)
    if radius:
        c %= radius
    d = lcm(c.denominator, radius.denominator)
    return int(c * d), int(radius * d), d


def ep(k, p):
    e = 0
    while p ** (e + 1) <= k:
        e += 1
    return e


for p in [2, 3, 5, 7, 13, 17]:
    for v in range(1, 6):
        for n in range(-3, 81):
            J = next(k for k in range(1, 200) if k * v - ep(k, p) >= n)
            safe = max(1, -((-2 * n) // (2 * v - 1)))
            check("functions_7b", J <= safe)
            for k in range(J, 300):
                check("functions_7b", k * v - val(k, p) >= n)

# Odd-power maps: finite images of each unit ball, scaled valuations j are handled algebraically.
for n in [1, 3, 5, 7, 9, 15]:
    for r in range(1, 6):
        for depth in [r + 2, r + 4]:
            modulus = 2 ** depth
            for b in [1, 3, 5, 7]:
                roots = {(b + 2 ** r * t) % modulus for t in range(2 ** (depth - r))}
                image = {pow(z, n, modulus) for z in roots}
                expected = {(pow(b, n, modulus) + 2 ** r * t) % modulus
                            for t in range(2 ** (depth - r))}
                check("functions_15r", image == expected and len(image) == len(roots))

rng = random.Random(20260928)
for _ in range(1600):
    H = rng.randrange(1, 61)
    A, B = rng.randrange(H), rng.randrange(H)
    d, e = rng.randrange(1, 31), rng.randrange(1, 31)
    c, R = F(A, d), F(H, d)
    g = gcd(A, H, d)
    check("policies_P24", triple(c, R) == (A // g, H // g, d // g))
    check("policies_P24", F(H, 1) / R == d and c * H / R == A)
    L, h = lcm(d, e), gcd(A, B, H)
    sums = triple(F(A, d) + F(B, e), qgcd(F(H, d), F(H, e)))
    products = triple(F(A * B, d * e), qgcd(F(A * H, d * e), F(B * H, d * e), F(H * H, d * e)))
    check("policies_S26", sums[1] == H // gcd(A * (L // d) + B * (L // e), H, L))
    check("policies_S26", products[1] == H * h // gcd(A * B, H * h, d * e))
    scale = F(rng.randrange(1, 21), rng.randrange(1, 21))
    scaled = triple(c * scale, R * scale)
    m, n = scale.numerator, scale.denominator
    check("policies_S26", scaled[1] == m * H // gcd(m * A, m * H, n * d))
    K = rng.randrange(1, 31)
    s = R / K
    can = (c / s).denominator == 1
    if can:
        check("policies_L6", triple(s * (int(c / s) % K), s * K) == triple(c, R))
    else:
        check("policies_L6", all(triple(s * u, s * K) != triple(c, R) for u in range(K)))
    u, w = rng.randrange(K), rng.randrange(K)
    hh = gcd(u, w, K)
    check("policies_P10", triple(s * s * u * w, s * s * K * hh)
          == triple(s * s * hh * ((u * w // hh) % K), s * s * hh * K))
    cap = F(rng.randrange(1, 31), rng.randrange(1, 31))
    rc = qgcd(R, cap)
    check("policies_P14", (R / rc).denominator == 1 and (cap / rc).denominator == 1)
    for q in [F(0), F(1, 3)]:
        check("policies_P14_exact_exception", triple(q, 0)[1] == 0 and qgcd(0, cap) == cap)

for q in range(2, 31):
    for d in range(1, 21):
        for A in range(q):
            solutions = sum((d * x - A) % q == 0 for x in range(q))
            g = gcd(d, q)
            check("policies_P25", solutions == (g if A % g == 0 else 0))
            determined = all(val(F(q, d), p) >= val(q, p) for p in [2, 3, 5, 7, 11, 13, 17, 19, 23, 29]
                             if q % p == 0)
            check("policies_P25", determined == (g == 1))

for blocks in [(4, 9), (8, 25, 7), (6, 35), (1,), (2, 3, 5)]:
    H = prod(blocks)
    for _ in range(200):
        A, d = rng.randrange(H), rng.randrange(1, H + 1)
        g = gcd(A, H, d)
        gs = [gcd(g, q) for q in blocks]
        check("policies_P24_blocks", prod(gs) == g)
        for q, gi in zip(blocks, gs):
            if q // gi == 1:
                continue
            derived = ((A % q) // gi) * pow(g // gi, -1, q // gi) % (q // gi)
            check("policies_P24_blocks", derived == (A // g) % (q // gi))

# S26's numerator-modulus column is correct, but its full-triple sentence omits reduction.
A, H, d = 9, 4, 1
g = gcd(A, H, d)
assert (A // g, H // g, d // g) != triple(F(A, d), F(H, d))
print("S26_full_triple_counterexample: raw=(9,4,1) displayed=(9,4,1) canonical=(1,4,1)")

# Local image hull for division: enumerate independent additive and inverse-unit residues.
for p in [2, 3, 5, 7]:
    Q = p ** 4
    for n in range(4):
        for m in range(4):
            for a in [0, 1, p, p + 1]:
                for c in [1, p + 1]:
                    units = [w for w in range(Q) if w % p and (w * c - 1) % (p ** n) == 0]
                    # Differences from every w at z=0 and z=1 already generate the image hull.
                    images = [(a + p ** m * z) * w for w in units for z in [0, 1]]
                    hull = reduce(gcd, (x - images[0] for x in images), Q)
                    ell = max(n, 1) if p == 2 else n
                    predicted = min(val(a, p) + ell, m)
                    check("ideles_P19", hull == p ** predicted)

# Proposition 13: enumerate one finite group deeper than every predicted precision.
for p in [2, 3, 5, 7]:
    Q = p ** 5
    for n in [0, 2, 3] if p == 2 else [0, 1, 2]:
        for c in [1, 2, 3, 5]:
            if n and c % p == 0:
                continue
            bases = [b for b in range(Q) if b % p and (n == 0 or (b - c) % p ** n == 0)]
            b0 = bases[0]
            for e, M in [(-2, 4), (0, 2), (1, 3), (2, 4), (3, 0), (4, 6)]:
                g = gcd(e, M)
                if n:
                    predicted = min(n + val(g, p), val(pow(c, M, Q) - 1, p))
                    centre = pow(c, e, Q)
                elif p == 2:
                    predicted = 1 if g % 2 else 2 + val(g, 2)
                    centre = 1
                else:
                    predicted = 1 + val(g, p) if g % (p - 1) == 0 else 0
                    centre = 1
                actual = reduce(gcd, (pow(b, e, Q) - pow(b0, e, Q) for b in bases), Q)
                actual = reduce(gcd, (pow(b, M, Q) - 1 for b in bases), actual)
                check("catalogue_P13", actual == p ** predicted)
                check("catalogue_P13", all((pow(b, e, Q) - centre) % actual == 0 for b in bases))

canon = lambda n: n // 2 if n % 4 == 2 else n
for N in range(1, 41):
    for n in range(1, 41):
        mod = lcm(N, n)
        actions = {u % n for u in range(1, mod + 1) if gcd(u, mod) == 1 and (u - 1) % N == 0}
        check("catalogue_P15", (len(actions) == 1) == (canon(N) % canon(n) == 0))

# Independent Hilbert oracle: ternary-conic solvability at the proved testing precisions.
for p, k in [(2, 4), (3, 2), (5, 2)]:
    Q = p ** k
    residues = [a for a in range(1, Q) if val(a, p) <= 1]
    for a in residues:
        for b in residues:
            va, vb = val(a, p), val(b, p)
            ua, ub = a // p ** va, b // p ** vb
            if p == 2:
                exponent = ((ua - 1) // 2) * ((ub - 1) // 2)
                exponent += va * ((ub * ub - 1) // 8) + vb * ((ua * ua - 1) // 8)
                sign = (-1) ** exponent
            else:
                la = 1 if pow(ua, (p - 1) // 2, p) == 1 else -1
                lb = 1 if pow(ub, (p - 1) // 2, p) == 1 else -1
                sign = (-1) ** (va * vb * ((p - 1) // 2)) * la ** vb * lb ** va
            # Require at least one primitive coordinate. Squares retain primitivity metadata.
            squares = [(z * z % Q, z % p != 0) for z in range(Q)]
            zflags = {}
            for z2, prim in squares:
                zflags.setdefault(z2, set()).add(prim)
            solvable = any((a * x2 + b * y2) % Q in zflags
                           and (xp or yp or True in zflags[(a * x2 + b * y2) % Q])
                           for x2, xp in squares for y2, yp in squares)
            check("catalogue_P7", solvable == (sign == 1))

for name, n in counts.items():
    print(f"{name}: checks={n} failures=0")
print(f"total={sum(counts.values())} failures=0")
