#!/usr/bin/env python3
"""Integer-only oracle, independent of FLINT and proto/catalogue_checks.py.

Legendre uses Euler's criterion and counts squares independently for p <= 65537.
The criterion's proof is catalogue.md:24-27. Jacobi uses the defining prime product.
Kronecker uses Definition 1, catalogue.md:14-20. No reciprocity algorithm is used here.
"""
import json
from math import gcd, lcm
from pathlib import Path
from random import Random

ROOT = Path(__file__).resolve().parents[1]
VECTORS = ROOT / 'tests/ref/vectors/f-slice12'


def legendre(a, p):
    r = pow(a % p, (p - 1) // 2, p)
    return 0 if a % p == 0 else (1 if r == 1 else -1)


def factors(n):
    out = []
    q = 2
    while q*q <= n:
        h = 0
        while n % q == 0:
            n //= q
            h += 1
        if h:
            out.append((q, h))
        q += 1
    if n > 1:
        out.append((n, 1))
    return out


def jacobi(a, b, fac=None):
    out = 1
    for p, h in (factors(b) if fac is None else fac):
        out *= legendre(a, p)**h
    return out


def kronecker(a, b, fac=None):
    if b == 0:
        return int(abs(a) == 1)
    s = -1 if b < 0 and a < 0 else 1
    b = abs(b)
    h = 0
    while b % 2 == 0:
        b //= 2
        h += 1
    # Direct arithmetic on (a*a-1)//8, independent of a residue lookup table.
    two = 0 if a % 2 == 0 else 1 - 2*(((a*a-1)//8) % 2)
    return s * two**h * jacobi(a, b, fac)


def modulus(kind, b):
    if kind != 2 or b % 2:
        return b
    while b % 2 == 0:
        b //= 2
    return lcm(b, 8)


def residue_vectors():
    rng = Random(1)
    rows = []
    counts = {'square_count': 0, 'exact': 0, 'finite': 0}
    def row(kind, a, b, inp=0, N=0, d=1, fac=None):
        fun = (legendre, jacobi, kronecker)[kind]
        st = 0
        if kind == 1 and (b <= 0 or b % 2 == 0):
            st = 7
        elif inp == 1 and d != 1:
            st = 7 if a % gcd(N, d) else 1
        elif inp and N and (b <= 0 or N % modulus(kind, b)):
            st = 1
        val = 99
        if st == 0:
            val = fun(a, b) if kind == 0 else fun(a, b, fac)
        rows.append(dict(kind=kind, input=inp, a=str(a), b=str(b), N=str(N),
                         d=str(d), status=st, value=val))
        counts['finite' if inp else 'exact'] += 1
    for p in (3, 5, 7, 11, 13, 17, 65537):
        squares = {x*x % p for x in range(p)}
        aa = range(-2*p, 2*p+1) if p < 20 else [rng.randrange(-p*p, p*p) for _ in range(160)]
        for a in aa:
            count = 0 if a % p == 0 else (1 if a % p in squares else -1)
            assert legendre(a, p) == count
            counts['square_count'] += 1
            row(0, a, p)
    for _ in range(1500):
        a, b = rng.randrange(-100000, 100000), rng.randrange(-100, 101)
        row(2, a, b)
        if b > 0 and b % 2:
            row(1, a, b)
    for a in range(-8, 9):
        for b in (0, -1, -2, -4, -9, 1, 2, 4, 8, 9, 15, 24):
            row(2, a, b)
    for kind, b in ((0, 3), (0, 7), (1, 9), (1, 15), (2, 2), (2, 4), (2, 24), (2, 3), (2, 15)):
        K = modulus(kind, b)
        for N in (1, 2, 3, 4, 8, K, 2*K, 3*K):
            for a in range(N):
                row(kind, a, b, 1, N)
                if gcd(a, N) == 1:
                    row(kind, a, b, 2, N)
        for a in (-1, 1):
            row(kind, a, b, 2)
    for kind in (1, 2):
        for b in (-2, 0, 2, 3, 9):
            row(kind, 1, b, 1, 2)
            row(kind, 1, b, 2, 2)
    for kind, b in ((0, 3), (1, 3), (2, 2)):
        for a, N, d in ((1, 2, 2), (0, 1, 2), (1, 3, 2), (1, 0, 2)):
            row(kind, a, b, 1, N, d)
    p64 = 2**64-59
    for _ in range(80):
        a = rng.getrandbits(4096) * rng.choice((-1, 1))
        row(0, a, p64)
        row(2, a, 2**80*3**7*5**3, fac=[(3, 7), (5, 3)])
    for _ in range(10):
        a = rng.getrandbits(4096)
        row(1, a, 3**1001*5**1000, fac=[(3, 1001), (5, 1000)])
    path = VECTORS / 'residue.jsonl'
    with path.open('w') as f:
        for r in rows:
            f.write(json.dumps(r, separators=(',', ':'))+'\n')
    assert path.stat().st_size < 1000000
    print(f'residue: {len(rows)} rows, {counts}, {path.stat().st_size} bytes')


if __name__ == '__main__':
    residue_vectors()

# Hilbert oracle: search, not a Hilbert formula. Coefficients are reduced by square scaling.
# For valuation 0 or 1, p^2 at odd p and 16 at 2 suffice. See docs/api-1f9.md Y6 for
# the stepwise proof and the on-disk Hensel statement closing catalogue.md's pending source.
def primitive_solution(a, b, p):
    M = 16 if p == 2 else p*p
    # Enumerate square residues together with whether their coordinate is a unit.
    # This is exactly the exhaustive triple search with equal square values grouped.
    squares = {(x*x % M, x % p != 0) for x in range(M)}
    for x2, xu in squares:
        for y2, yu in squares:
            for z2, zu in squares:
                if (xu or yu or zu) and (a*x2+b*y2-z2) % M == 0:
                    return 1
    return -1


def unit_class(q, p):
    from fractions import Fraction
    q = Fraction(q)
    a, b, va, vb = q.numerator, q.denominator, 0, 0
    while a % p == 0:
        a //= p
        va += 1
    while b % p == 0:
        b //= p
        vb += 1
    M = 8 if p == 2 else p
    u = a * pow(b, -1, M) % M
    return ((va-vb) % 2, u)


def hilbert_search(a, b, p):
    if p == 0:
        return -1 if a < 0 and b < 0 else 1
    va, u = unit_class(a, p)
    vb, w = unit_class(b, p)
    return primitive_solution(p**va*u, p**vb*w, p)


def hilbert_vectors():
    from fractions import Fraction as F
    rng = Random(2)
    rows = []
    for p in (2, 3, 5, 7):
        classes = [p**v*u for v in (0, 1) for u in range(1,8 if p==2 else p) if u%p]
        for a in classes:
            for b in classes:
                rows.append(dict(p=str(p), a=str(a), b=str(b), value=hilbert_search(a,b,p)))
    for _ in range(320):
        a=F(rng.choice([-1,1])*rng.randrange(1,200),rng.randrange(1,80))
        b=F(rng.choice([-1,1])*rng.randrange(1,200),rng.randrange(1,80))
        p=rng.choice((0,2,3,5,7))
        rows.append(dict(p=str(p), a=str(a), b=str(b), value=hilbert_search(a,b,p)))
    with (VECTORS/'hilbert.jsonl').open('w') as f:
        for row in rows:
            f.write(json.dumps(row,separators=(',',':'))+'\n')
    assert sum(row['p']=='2' for row in rows[:288]) >= 64
    print(f'hilbert: {len(rows)} primitive-solution rows (64 complete square-class pairs at 2)')


# Keep Slice A generation callable independently; B fixtures are requested explicitly.
if __name__ == '__main__':
    import sys
    if '--hilbert' in sys.argv:
        hilbert_vectors()


def hilbert_finite_vectors():
    rows=[]
    cache={}
    for p in (2,3,5,7):
        units=[u for u in range(1,8 if p==2 else p) if u%p]
        digits=range(4) if p==2 else range(2)
        for av in (0,1):
            for bv in (0,1):
                for au in units:
                    for bu in units:
                        for ak in digits:
                            for bk in digits:
                                allowed_a=[u for u in units if (u-au) % p**ak == 0]
                                allowed_b=[u for u in units if (u-bu) % p**bk == 0]
                                signs=set()
                                for u in allowed_a:
                                    for w in allowed_b:
                                        key=(p,p**av*u,p**bv*w)
                                        if key not in cache:
                                            cache[key]=primitive_solution(key[1],key[2],p)
                                        signs.add(cache[key])
                                rows.append(dict(p=p,av=av,bv=bv,au=au,bu=bu,ak=ak,bk=bk,
                                                 status=0 if len(signs)==1 else 1,
                                                 value=next(iter(signs)) if len(signs)==1 else 99))
    with (VECTORS/'hilbert_finite.jsonl').open('w') as f:
        for r in rows:
            f.write(json.dumps(r,separators=(',',':'))+'\n')
    total=sum(p.stat().st_size for p in VECTORS.glob('*.jsonl'))
    assert total<1000000,total
    print(f'hilbert finite: {len(rows)} rows, {len(cache)} searched class pairs, {total} total fixture bytes')


if __name__ == '__main__' and '--hilbert' in sys.argv:
    hilbert_finite_vectors()
