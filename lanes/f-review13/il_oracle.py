#!/usr/bin/env python3
"""f-review13: own exact-integer oracle for IL1-IL7, independent of proto/idlog_checks.py.

Log(a) mod p^H is computed WITHOUT the log series: torsion is removed by a^(p^(H-1)) (odd p,
Teichmueller limit, a^(p^n) = w mod p^(n+1)) or by the sign at 2, and the principal unit u is
inverted through a table of exp(y), y in p^d Z / p^H Z (Lemma 9: exp and log are inverse
bijections p^d Z_p <-> 1 + p^d Z_p). exp is summed in integers: S = sum_j y^j J!/j!, divided
by J! = p^e q exactly. Degrees j > J are dropped; the bound used is
v(y^j/j!) >= j d - (j-1)/(p-1) (Legendre), increasing in j, and J is the first j where it is >= H.
Every comparison is modulo p^H and states H. Python integers only.
"""
import sys, itertools, random, subprocess, json
from math import gcd, factorial

def v(n, p):
    if n == 0:
        return 10**9
    e = 0
    while n % p == 0:
        n //= p; e += 1
    return e

def dd(p):
    return 2 if p == 2 else 1

def exp_mod(y, p, H):
    """exp(y) mod p^H for y in p^d Z (integer)."""
    d = dd(p)
    assert v(y, p) >= d or y == 0
    # first J with J d - (J-1)/(p-1) >= H (lower bound of v of the degree-J term, Legendre)
    J = 1
    while J * d - (J - 1) / (p - 1) < H:
        J += 1
    # all degrees >= J have valuation >= H (bound increasing); keep 0..J-1
    F = factorial(J - 1)
    e = v(F, p); q = F // p**e
    mod = p**(H + e)
    S = 0
    yj = 1
    for j in range(J):
        S = (S + yj * (F // factorial(j))) % mod
        yj = yj * y % mod
    assert S % p**e == 0, "truncated exp not integral"
    return (S // p**e) * pow(q, -1, p**H) % p**H

_tables = {}
def log_table(p, H):
    key = (p, H)
    if key in _tables:
        return _tables[key]
    d = dd(p); mod = p**H
    inv = {}
    for t in range(p**(H - d)):
        y = t * p**d
        u = exp_mod(y, p, H)
        assert u % p**d == 1 % p**d
        assert u not in inv, "exp not injective mod p^H"
        inv[u] = y
    assert len(inv) == p**(H - d)
    tab = {}
    for a in range(1, mod):
        if a % p == 0:
            continue
        if p == 2:
            u = a if a % 4 == 1 else (-a) % mod
        else:
            w = pow(a, p**(H - 1), mod)
            assert pow(w, p - 1, mod) == 1
            u = a * pow(w, -1, mod) % mod
        tab[a] = inv[u]
    _tables[key] = tab
    return tab

def Log(a_num, a_den, p, H):
    """Log of the p-unit rational a_num/a_den modulo p^H (H >= 1)."""
    mod = p**H
    a = a_num * pow(a_den, -1, mod) % mod
    return log_table(p, H)[a]

def unit_part(num, den, p):
    m = v(num, p) - v(den, p)
    return m, num // p**v(num, p), den // p**v(den, p)

def image(p, num, den, c, M, H):
    """The exact local image mod p^H, enumerated from IL1's input set (own enumeration)."""
    mod = p**H
    m, a, b = unit_part(num, den, p)
    if M == 0:
        return {Log(a * c, b, p, H)}
    k = v(M, p)
    assert k < H
    units = [u for u in range(1, mod) if u % p and (k == 0 or (u - c) % p**k == 0)]
    return {Log(a * u, b, p, H) for u in units}

def predicted(p, num, den, c, M, H):
    """IL2-IL4 as stated: Log(r'c) + p^E Z_p (M>0, p|M, E=max(k,d)), p^d Z_p otherwise, singleton M=0."""
    mod = p**H
    m, a, b = unit_part(num, den, p)
    d = dd(p)
    if M == 0:
        return {Log(a * c, b, p, H)}
    k = v(M, p)
    if k == 0 or (p == 2 and k == 1):
        return set(range(0, mod, p**d))
    E = max(k, d)
    centre = Log(a * c, b, p, H)
    return set(range(centre % p**E, mod, p**E))
