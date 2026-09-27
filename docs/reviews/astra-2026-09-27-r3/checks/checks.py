"""Round-3 finite checks; run from repository root with python3 -B.

Proofs and the limits of finite checks are in ../review.md.
No third-party Python packages, bytecode or compiled binaries are written.
"""
import ctypes
from fractions import Fraction as F
from functools import reduce
from math import gcd, factorial, lcm
from pathlib import Path
import runpy

root = Path(__file__).resolve().parents[4]
p = runpy.run_path(str(root / "proto/precision_rules.py"), run_name="precision_import")

flint = ctypes.CDLL("libflint.so")
flint.fmpz_kronecker.argtypes = [ctypes.POINTER(ctypes.c_long)] * 2
flint.fmpz_kronecker.restype = ctypes.c_int
def kronecker(a, b):
    # These small integers are inline fmpz values.
    return flint.fmpz_kronecker(ctypes.byref(ctypes.c_long(a)), ctypes.byref(ctypes.c_long(b)))
assert kronecker(1, 2) == 1 and kronecker(3, 2) == -1
assert kronecker(1, -1) == 1 and kronecker(-1, -1) == -1
print("FLINT Kronecker: (1/2)=1, (3/2)=-1 despite 1=3 mod 2; (1/-1)=1, (-1/-1)=-1")

# All Q_2 square classes. The review proves why modulus 16 is sufficient.
classes = (1, 3, 5, 7, 2, 6, 10, 14)
for a in classes:
    for b in classes:
        assert (p["hilbert"](a, b, 2) == 1) == p["hilbert_has_solution"](a, b, 2, 4)
assert p["hilbert_has_solution"](2, 6, 2, 3) and p["hilbert"](2, 6, 2) == -1
print("Hilbert: all 64 Q_2 square-class pairs agree with primitive solvability modulo 16")
print("Insufficient precision example: (2,6)_2=-1, but (x,y,z)=(1,1,0) solves modulo 8")
assert p["hilbert"](1, 1, 2) == 1 and p["hilbert"](3, 3, 2) == -1
print("Two arbitrary unit ideles: the 2-adic symbol can be +1 or -1")

# Largest divisor of the supplied base modulus, including negative exponents.
cases = 0
for N in range(1, 51):
    for c in range(N):
        if gcd(c, N) != 1:
            continue
        for M in range(1, 13):
            D = gcd(N, pow(c, M, N) - 1)
            for e in (-3, 0, 2):
                y = pow(c, e, N)
                # The first two exponent choices force the same gcd as the full orbit.
                actual = gcd(N, pow(c, e + M, N) - y)
                assert actual == D
                assert all((pow(c, e + M*t, N) - y) % D == 0 for t in range(-3, 4))
            cases += 1
print(f"Profinite power: largest divisor D=gcd(N,c^M-1), {cases} (N,c,M) cases with signed exponents")
for N in range(1, 40, 2):
    for c in range(N):
        if gcd(c, N) != 1:
            continue
        odd_lift = c if c % 2 else c + N
        for M in range(1, 9):
            d1 = gcd(N, pow(c, M, N) - 1)
            d2 = gcd(2*N, pow(odd_lift, M, 2*N) - 1)
            assert d2 == 2*d1
assert all(pow(b, 2, 24) == 1 for b in range(24) if gcd(b, 24) == 1)
print("Canonical U(2N)=U(N) is respected; all unit squares are 1 modulo 24 (canonical modulus 24)")

def vp_integer(a, prime):
    if a == 0:
        return float("inf")
    v = 0
    while a % prime == 0:
        a //= prime
        v += 1
    return v

# Check the unrestricted table against the factorisation-free finite algorithm.
# Trial factors are used ONLY by the independent small-case table oracle.
cases = 0
for N in (1, 3, 4, 5, 8, 9, 12):
    for c in range(N):
        if gcd(c, N) != 1:
            continue
        for e, M in ((0, 1), (0, 2), (1, 2), (2, 4), (3, 3), (-2, 2)):
            g = gcd(e, M)
            B = 8*N*g*factorial(g+1)
            bases = [b for b in range(B) if b % N == c and gcd(b, B) == 1]
            y0 = pow(bases[0], e, B)
            actual = B
            for b in bases:
                actual = gcd(actual, pow(b, e, B)-y0)
                actual = gcd(actual, pow(b, M, B)-1)
            expected = 1
            for prime in p["primes_of"](N) | {2, 3}:
                # g <= 3, so these include all possible primes outside N.
                if N % prime == 0:
                    k = min(vp_integer(N, prime)+vp_integer(g, prime), vp_integer(c**M-1, prime))
                elif prime == 2:
                    k = 1 if g % 2 else 2+vp_integer(g, 2)
                else:
                    k = 1+vp_integer(g, prime) if g % (prime-1) == 0 else 0
                expected *= prime**k
            assert actual == expected, (N, c, e, M, actual, expected)
            cases += 1
print(f"Unrestricted power precision: {cases} table comparisons with the exhaustive gcd algorithm")

def binom(x, k):
    return reduce(lambda a, b: a*b, (x-i for i in range(k)), 1) // factorial(k)

def tight_binom(a, N, k):
    return reduce(gcd, (binom(a+N*j, k)-binom(a, k) for j in range(1, k+1)), 0)

cases = 0
for N in range(1, 41):
    for a in range(N):
        for k in range(0, 11):
            R = tight_binom(a, N, k)
            vals = [binom(a+N*t, k)-binom(a, k) for t in range(-12, 18)]
            assert reduce(gcd, vals, 0) == R
            if k:
                conservative = N // gcd(N, factorial(k))
                uniform = N // gcd(N, lcm(*range(1, k+1)))
                assert R % uniform == 0 and uniform % conservative == 0
            else:
                assert R == 0
            cases += 1
print(f"Binomial fixed-divisor formula and both enclosures: {cases} cases, including negative lifts and k=0")
assert tight_binom(0, 8, 4) == 2
assert tight_binom(0, 8, 3) == 8 and tight_binom(1, 8, 3) == 4
print("Binomial examples: (a,N,k)=(0,8,4): tight R=2 versus factorial bound 1")
print("Centre dependence: (0,8,3) has R=8; (1,8,3) has R=4")

# Proposition 3 zero cases and Proposition 6, including a=0 and signed scalars.
cases = 0
for a in (F(-7, 3), F(0), F(5, 2)):
    for R in (F(1, 7), F(2), F(15, 4)):
        for K in (1, 2, 5, 12):
            s = p["qgcd"](a, R/K)
            u = int(a/s) % K
            assert p["contains"](a, R, s*u, s*K)
            for q in (F(-3, 5), F(0), F(7, 2)):
                c, radius = p["mul_rule"](q, F(0), s*u, s*K)
                assert p["equal_set"](c, radius, q*s*u, abs(q)*s*K)
            cases += 1
print(f"Proposition 6: {cases} scaled conversions and signed/zero scalar cases")

# Actual local moduli in Proposition 4's example.
a, N, d = F(101, 90), F(20, 3), 90
for prime, centre, exponent in ((2, F(1, 2), 2), (3, F(2, 9), -1), (5, F(7, 5), 1)):
    vdiff, _ = p["val_unit"](a-centre, prime)
    vn, _ = p["val_unit"](N, prime)
    vd, _ = p["val_unit"](d, prime)
    assert vdiff >= exponent and vn == exponent
    modulus = prime ** (vd+exponent)
    assert F((a-centre)*d/modulus).denominator == 1
    print(f"Proposition 4: p={prime}, CRT modulus={modulus}, v(a-c_p)={vdiff}, n_p={exponent}")

# Negative/fractional root valuations: compare the exact branch image mod p^T
# for unit roots, then scale by p^j. The review supplies the universal proof.
cases = 0
for prime, cdisc in ((2, 2), (3, 1), (5, 1)):
    for degree in (1, 2, 3, 4, 5, 6):
        vn, _ = p["val_unit"](degree, prime)
        relative = cdisc + vn
        T = relative + 2
        q = prime**T
        branch = range(1, q, prime**cdisc)
        image = {pow(b, degree, q) for b in branch}
        expected = set(range(1, q, prime**relative))
        assert image == expected
        for j in (-2, 0, 1):
            m = degree*j
            Nin = m+relative
            Nout = Nin-vn-(degree-1)*j
            assert Nout == j+cdisc
        cases += 1
print(f"Root guard boundary: {cases} exact finite branch-image checks; valuations scaled by -2, 0, 1")
