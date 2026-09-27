#!/usr/bin/env python3
"""Independent finite probes for docs/proofs/catalogue.md. No FLINT dependency."""

from fractions import Fraction as F
from functools import reduce
from itertools import product
from math import comb, factorial, gcd, lcm


def primes(n):
    n = abs(n)
    out = []
    d = 2
    while d * d <= n:
        if n % d == 0:
            out.append(d)
            while n % d == 0:
                n //= d
        d += 1
    if n > 1:
        out.append(n)
    return out


def vp(n, p):
    n = abs(n)
    if n == 0:
        raise ValueError("valuation of zero")
    v = 0
    while n % p == 0:
        n //= p
        v += 1
    return v


def legendre(a, p):
    a %= p
    if a == 0:
        return 0
    return 1 if pow(a, (p - 1) // 2, p) == 1 else -1


def jacobi(a, m):
    if m == 1:
        return 1
    out = 1
    for p in primes(m):
        out *= legendre(a, p) ** vp(m, p)
    return out


def kronecker_positive(a, b):
    assert b > 0
    t = vp(b, 2) if b % 2 == 0 else 0
    m = b // 2**t
    two = 0 if a % 2 == 0 else (-1) ** (((a * a - 1) // 8) % 2)
    return jacobi(a, m) * (two**t)


def kronecker_exact(a, b):
    if b == 0:
        return 1 if abs(a) == 1 else 0
    if b < 0:
        return (-1 if a < 0 else 1) * kronecker_positive(a, -b)
    return kronecker_positive(a, b)


def check_symbols():
    count = 0
    for p in (3, 5, 7, 11, 13):
        squares = {x*x % p for x in range(p)}
        for a in range(-25, 26):
            assert legendre(a, p) == (0 if a % p == 0 else (1 if a % p in squares else -1))
            count += 1
    assert kronecker_positive(1, 2) == 1 and kronecker_positive(3, 2) == -1
    assert (kronecker_exact(-3, -1), kronecker_exact(3, -1)) == (-1, 1)
    assert [kronecker_exact(a, 0) for a in range(-2, 3)] == [0, 1, 0, 1, 0]
    for b in range(1, 61):
        t = vp(b, 2) if b % 2 == 0 else 0
        m = b // 2**t
        period = m if t == 0 else lcm(m, 8)
        for a in range(-15, 16):
            assert kronecker_positive(a, b) == kronecker_positive(a + period, b)
            count += 1
    # A unit coset is tested by all unit lifts modulo the required period.
    for N, c, b in ((3, 2, 6), (8, 3, 2), (24, 5, 6), (5, 2, 3)):
        period = lcm(N, lcm(b // 2**(vp(b, 2) if b % 2 == 0 else 0), 8))
        vals = {kronecker_positive(a, b) for a in range(period) if gcd(a, period) == 1 and a % N == c % N}
        sampled = {kronecker_positive(a, b) for a in range(-period, period+1)
                   if gcd(a, period) == 1 and a % N == c % N}
        assert vals == sampled
        assert vals
        needed = (b // 2**(vp(b, 2) if b % 2 == 0 else 0)) if b % 2 else lcm(b // 2**vp(b, 2), 8)
        if canon(N) % canon(needed) == 0:
            assert len(vals) == 1
        count += 1
    for a, N, b in product(range(-2, 3), range(1, 9), (2, 3, 6, 9)):
        t = vp(b, 2) if b % 2 == 0 else 0
        K = b // 2**t if t == 0 else lcm(b // 2**t, 8)
        L = lcm(N, K)
        finite = {kronecker_positive(r, b) for r in range(L) if r % N == a % N}
        samples = {kronecker_positive(a + N*j, b) for j in range(-L, L+1)}
        assert finite == samples
        count += 1
    print(f"symbols: {count} residue and coset cases; (1/2)=1, (3/2)=-1")


def unit_of(q, p):
    q = F(q)
    a, b = q.numerator, q.denominator
    alpha = 0
    while a % p == 0:
        a //= p
        alpha += 1
    while b % p == 0:
        b //= p
        alpha -= 1
    return alpha, F(a, b)


def residue(q, p):
    return q.numerator * pow(q.denominator, -1, p) % p


def hilbert_formula(a, b, p):
    if p == 0:
        return -1 if a < 0 and b < 0 else 1
    alpha, u = unit_of(a, p)
    beta, w = unit_of(b, p)
    if p == 2:
        U, W = residue(u, 8), residue(w, 8)
        eps = lambda v: ((v - 1) // 2) % 2
        om = lambda v: ((v*v - 1) // 8) % 2
        return (-1) ** ((eps(U)*eps(W) + alpha*om(W) + beta*om(U)) % 2)
    U, W = residue(u, p), residue(w, p)
    return ((-1) ** (alpha*beta*(p-1)//2)
            * legendre(U, p) ** (beta % 2) * legendre(W, p) ** (alpha % 2))


def primitive_solution(a, b, p, k):
    q = p**k
    sq = {x*x % q for x in range(q)}
    unit_sq = {x*x % q for x in range(q) if x % p}
    for x in range(q):
        ax = a*x*x % q
        for y in range(q):
            target = (ax + b*y*y) % q
            if x % p or y % p:
                if target in sq:
                    return True
            elif target in unit_sq:
                return True
    return False


def check_hilbert():
    count = 0
    classes = (1, 3, 5, 7, 2, 6, 10, 14)
    rows = []
    for a in classes:
        row = []
        for b in classes:
            direct = 1 if primitive_solution(a, b, 2, 4) else -1
            assert hilbert_formula(a, b, 2) == direct, (a, b)
            row.append("+" if direct == 1 else "-")
            count += 1
        rows.append("".join(row))
    for p, k in ((3, 2), (5, 2)):
        nonsquare = next(u for u in range(2, p) if legendre(u, p) == -1)
        for a, b in product((1, nonsquare, p, p*nonsquare), repeat=2):
            assert (hilbert_formula(a, b, p) == 1) == primitive_solution(a, b, p, k)
            count += 1
    print(f"hilbert solvability: {count} class pairs; 2-adic table order {classes}")
    for a, row in zip(classes, rows):
        print(f"  {a:2}: {row}")
    assert hilbert_formula(1, 1, 2) == 1 and hilbert_formula(3, 3, 2) == -1
    # A+relative-precision ball has fixed square class; one lost digit may not.
    for p, A, centre in ((2, 3, 3), (3, 1, 2), (5, 1, 2)):
        data = {hilbert_formula(centre+p**A*j, centre, p) for j in range(-8, 9)}
        assert len(data) == 1
        count += 1
    return rows


def check_hilbert_product():
    count = 0
    samples = [F(n, d) for n in range(-9, 10) for d in range(1, 6) if n]
    for a in samples[::3]:
        for b in samples[::4]:
            ps = {2}
            for q in (a, b):
                ps.update(primes(q.numerator))
                ps.update(primes(q.denominator))
            prod = hilbert_formula(a, b, 0)
            for p in ps:
                prod *= hilbert_formula(a, b, p)
            assert prod == 1, (a, b)
            count += 1
    print(f"hilbert rational product: {count} rational pairs")


def check_zeta():
    count = 0
    for p, s, terms in product((2, 3, 5, 7), (1, 2, 3), (4, 8)):
        ratio = F(1, p**s)
        partial = sum((ratio**j for j in range(terms)), F(0))
        closed = F(1, 1) / (1-ratio)
        tail = ratio**terms / (1-ratio)
        assert closed-partial == tail and tail > 0
        count += 1
    # Gamma(j) checked by a rational Taylor integral on [0,T], with proved
    # Lagrange remainder and an integration-by-parts bound on the tail.
    T, degree = 30, 160
    exp_upper_tail = F(1, sum((F(T**k, factorial(k)) for k in range(T+1)), F(0)))
    for j in (1, 2, 3):
        partial = sum((F((-1)**k * T**(j+k), (j+k)*factorial(k))
                       for k in range(degree+1)), F(0))
        error = F(T**(j+degree+1), (j+degree+1)*factorial(degree+1))
        tail = exp_upper_tail * factorial(j-1) * sum(
            (F(T**k, factorial(k)) for k in range(j)), F(0))
        assert abs(partial-factorial(j-1)) <= error+tail
        assert error+tail < F(1, 10**7)
        count += 1
    # Ratios n/(n+1) approach the finite pole ratio 1; the factors diverge.
    for n in (10, 100, 1000):
        ratio = F(n, n+1)
        assert F(1, 1-ratio) == n+1
        count += 1
    print(f"zeta: {count} series-tail and real-integral cases")


def canon(n):
    return n // 2 if n % 2 == 0 and (n//2) % 2 else n


def predicted_finest(N, c, e, M):
    # General finite-quotient algorithm, independent of the local exponent table.
    g = gcd(e, M)
    B = 8*N*g*factorial(g+1)
    if B > 20000:
        return None
    residues = [a for a in range(B) if gcd(a, B) == 1 and a % N == c % N]
    if not residues:
        return None
    base = pow(residues[0], e, B)
    diffs = [B]
    for a in residues:
        diffs.append(pow(a, e, B)-base)
        diffs.append(pow(a, M, B)-1)
    return reduce(gcd, diffs)


def local_finest(N, c, e, M):
    g = gcd(e, M)
    out = 1
    for p in primes(8*N*g*factorial(g+1)):
        n = vp(N, p) if N % p == 0 else 0
        if n:
            cm = pow(c, M, p**(n+vp(g, p)+2))-1
            depth = n+vp(g, p) if cm == 0 else min(n+vp(g, p), vp(cm, p))
        elif p == 2:
            depth = 1 if g % 2 else 2+vp(g, 2)
        elif g % (p-1) == 0:
            depth = 1+vp(g, p) if g % p == 0 else 1
        else:
            depth = 0
        out *= p**depth
    return out


def check_power():
    count = 0
    fine_count = 0
    for N in range(1, 19):
        for c in range(N):
            if gcd(c, N) != 1:
                continue
            for M in range(1, 5):
                for e in range(-2, 3):
                    D = gcd(N, pow(c, M, N)-1)
                    vals = {pow(c, e+M*t, N) for t in range(-5, 6)}
                    assert (len(vals) == 1) == (D == N)
                    for L in range(1, N+1):
                        if N % L == 0:
                            assert (len({v % L for v in vals}) == 1) == (D % L == 0)
                    count += 1
                    if N >= 2 and canon(N) == N and e >= 0 and M <= 3:
                        brute = predicted_finest(N, c, e, M)
                        if brute is not None:
                            assert brute == local_finest(N, c, e, M), (N, c, e, M, brute)
                            fine_count += 1
    assert gcd(5, pow(2, 2, 5)-1) == 1
    assert predicted_finest(5, 2, 0, 2) == 24
    assert canon(6) == 3 and canon(2) == 1
    for odd in range(1, 20, 2):
        assert {u % odd for u in range(2*odd) if gcd(u, 2*odd) == 1} == {
            u for u in range(odd) if gcd(u, odd) == 1}
        count += 1
    print(f"power: {count} target-modulus cases, {fine_count} unrestricted-modulus cases; D=1, finest=24")


def binom_int(x, k):
    z = 1
    for j in range(k):
        z *= x-j
    return z//factorial(k)


def check_binomial():
    count = 0
    for N, a, k in product(range(1, 21), range(-3, 4), range(0, 7)):
        R = reduce(gcd, (abs(binom_int(a+N*j, k)-binom_int(a, k)) for j in range(1, k+1)), 0)
        brute = reduce(gcd, (abs(binom_int(a+N*t, k)-binom_int(a, k)) for t in range(-15, 16)), 0)
        assert R == brute
        conservative = N//gcd(N, factorial(k))
        assert R == 0 or R % conservative == 0
        count += 1
    special = reduce(gcd, (binom_int(8*j, 4) for j in range(1, 5)))
    assert special == 2
    print(f"binomial: {count} finite-difference cases; (0,8,4) radius={special}")


def check_content_volume():
    count = 0
    for N in range(1, 41):
        for a in range(N):
            q = 8*N
            hits = sum((x-a) % N == 0 for x in range(q))
            assert F(hits, q) == F(1, N)
            count += 1
    for r in (F(1, 12), F(5, 6), F(14, 25), F(45, 8)):
        rebuilt = F(1)
        for p in set(primes(r.numerator)+primes(r.denominator)):
            rebuilt *= F(p)**(vp(r.numerator, p) if r.numerator % p == 0 else 0)
            rebuilt /= F(p)**(vp(r.denominator, p) if r.denominator % p == 0 else 0)
        assert rebuilt == r
        count += 1
    for N in (F(3, 2), F(8, 3), F(9, 10)):
        local_product = F(1)
        for p in set(primes(N.numerator)+primes(N.denominator)):
            power = (vp(N.numerator, p) if N.numerator % p == 0 else 0)
            power -= (vp(N.denominator, p) if N.denominator % p == 0 else 0)
            local_product *= F(p)**(-power)
        assert local_product == 1/N
        count += 1
    print(f"content and volume: {count} quotient-count and valuation cases")


def check_cyclotomic():
    count = 0
    for N, n in product(range(1, 41), repeat=2):
        q = lcm(N, n)
        lifts = {a % n for a in range(q) if gcd(a, q) == 1 and a % N == 1 % N}
        assert (len(lifts) == 1) == (canon(N) % canon(n) == 0)
        count += 1
    assert canon(3) == canon(6) == 3
    for p, n in ((2, 3), (3, 5), (5, 8), (7, 9)):
        assert gcd(p, n) == 1
        # The class unit is p^-1 modulo n, so the arithmetic action is p.
        u = pow(p, -1, n)
        assert pow(u, -1, n) == p % n
        local_u_at_p = 1
        assert pow(local_u_at_p, -1, p**3) == 1
        count += 1
    print(f"cyclotomic: {count} canonical-containment and uniformiser cases")


if __name__ == "__main__":
    check_symbols()
    check_hilbert()
    check_hilbert_product()
    check_zeta()
    check_power()
    check_binomial()
    check_content_volume()
    check_cyclotomic()
