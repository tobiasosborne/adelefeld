#!/usr/bin/env python3
"""Own exact checks for the repaired endpoint arguments. No library calls."""
from math import gcd
from fractions import Fraction

comparisons = 0
for order in (2, 4, 6, 10, 12, 16, 18, 30, 36, 64):
    for n in range(1, 33):
        d = gcd(n, order)
        roots_of_one = [k for k in range(order) if n*k % order == 0]
        assert len(roots_of_one) == d
        for e in range(-32, 33):
            image = {e*k % order for k in roots_of_one}
            assert (len(image) == d) == (gcd(e, d) == 1)
            assert len(image) == d//gcd(e, d)
            comparisons += 1
assert Fraction(2)**2 == Fraction(-2)**2 == 4 and 3**2 != 4
assert gcd(3, gcd(3, 4)) == 1  # unreduced 3/3 at 5 still injective
print(f'P1 cyclic-group comparisons={comparisons}, failures=0')

count = 0
for n in range(2, 20):
    for N in range(-10, 11):
        ell = max(1, (N+n-1)//n)
        assert n*ell >= N
        for e in (-1, -3, 1, 3):
            # Nonzero p^(n*ell) has a root p^ell and admits its negative powers.
            for p in (2, 3, 5, 7):
                assert Fraction(p)**(n*ell) == (Fraction(p)**ell)**n
                assert Fraction(p)**(ell*e) != 0
                k = N if N % n else N+1
                assert k >= N and k % n != 0
                count += 1
print(f'P1 zero-ball domain/complement witness comparisons={count}, failures=0')

count = 0
for u in range(1, 128, 2):
    for s in range(64):
        assert pow(u, s, 2) == 1
        count += 1
print(f'P7 H=1 at 2 integer power comparisons={count}, failures=0')

# R=1 contributes only the identity. Its inverse is omitted, including p=3.
count = 0
for p, generator in ((3, 2), (5, 2), (17, 3), (97, 5), (65537, 3)):
    P = p-1
    for d in range(2, min(P, 128)+1):
        if P % d:
            continue
        D, rem = 1, d
        for q in range(2, d+1):
            if rem % q:
                continue
            while rem % q == 0:
                rem //= q
            Q, rest = 1, P
            while rest % q == 0:
                Q *= q
                rest //= q
            D *= Q
        if P//D != 1:
            continue
        for y in range(1, min(p, 128)):
            A = pow(y, d, p)
            # Only q-components remain; their CRT projectors sum to 1 mod P.
            projectors = []
            rem = d
            for q in range(2, d+1):
                if rem % q:
                    continue
                while rem % q == 0:
                    rem //= q
                Q, rest = 1, P
                while rest % q == 0:
                    Q *= q
                    rest //= q
                projectors.append((P//Q)*pow(P//Q, -1, Q))
            assert sum(projectors) % P == 1
            product = 1
            for eps in projectors:
                product = product*pow(A, eps, p) % p
            assert product == A and pow(1, d, p) == 1
            count += 1
print(f'R8 R=1 CRT comparisons={count}, failures=0')

j, e, K, ve = -10, 2, 0, 0
rel = max(1, K-e*j-ve)
assert rel == 20 and j+rel == 10 and abs(-(2**63)) == 2**63
print('P3 precision endpoint: rel_r=20, Nr=10, K=0; P8 magnitude endpoint=9223372036854775808')

p, n = 2**64-59, 24068
w, residue = pow(3, n, p), pow(3, n, p*p)
assert residue != w
print(f'R9 test comment: w={w}, 3^n mod p^2={residue}, difference/p mod p={(residue-w)//p}')
