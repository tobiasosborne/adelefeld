#!/usr/bin/env python3
"""Independent exact-integer oracle. No import of the earlier catalogue checks.

For binomials, evaluate the falling polynomial. Let S=|h(1)-h(0)|>0, h(t)=binom(a+Nt,k).
T=k!*S/gcd(N,k!*S) is a period modulo S: shifting t by T changes x by a multiple of k!*S,
so each integer-coefficient numerator difference is divisible by k!*S. Division by k! gives S.
The gcd of S and all differences over one period is therefore the gcd over all integer t.
Continuity gives enclosure on Zhat, and these integer points force minimality. If h(1)=h(0),
choose the first nonzero difference among 1..k; a nonconstant polynomial cannot vanish there
and at 0. This argument does not use the k-sample formula as the oracle.
"""
import argparse
import json
from math import factorial, gcd, lcm, prod
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / "tests/ref/vectors/f-slice13"


def binomial(a, k):
    return prod(a-i for i in range(k)) // factorial(k)


def smallest_enum(a, N, k):
    if not k or not N:
        return 0, 1
    c = binomial(a, k)
    seed = next(abs(binomial(a+N*j, k)-c) for j in range(1, k+1)
                if binomial(a+N*j, k) != c)
    T = factorial(k)*seed // gcd(N, factorial(k)*seed)
    R = seed
    for j in range(T):
        R = gcd(R, binomial(a+N*j, k)-c)
    return R, T


def write(name, rows):
    DEST.mkdir(parents=True, exist_ok=True)
    with (DEST / (name + ".jsonl")).open("w") as f:
        for row in rows:
            f.write(json.dumps(row, separators=(",", ":")) + "\n")
    print(name, "rows", len(rows))


def binomials():
    rows = []
    samples = 0
    for N in range(9):
        for a in range(-4, 5):
            A = a % N if N else a
            for k in range(5):
                R, T = smallest_enum(A, N, k)
                samples += T
                c = binomial(A, k)
                for tight in (0, 1):
                    H = R if tight else (N//gcd(N, factorial(k)) if k and N else 0)
                    rows.append(dict(a=a, N=N, d=1, k=k, tight=tight, status=0,
                                     C=str(c % H if H else c), R=str(H)))
    for a, N, d, st in ((1, 2, 2, 7), (0, 1, 2, 1), (1, 0, 2, 7)):
        for k in (0, 2):
            for tight in (0, 1):
                rows.append(dict(a=a, N=N, d=d, k=k, tight=tight, status=st, C="99", R="0"))
    write("binomial", rows)
    print("binomial period samples", samples)


def canon(N):
    return N//2 if N % 4 == 2 else N


def unit_period(B):
    # Lagrange can be avoided: compute each order by multiplication, then take their lcm.
    period = 1
    for b in range(B):
        if gcd(b, B) == 1:
            r, order = b % B, 1
            while r != 1 % B:
                r, order = r*b % B, order+1
            period = lcm(period, order)
    return period


def power_enum(N, c, e, M, B):
    """Unit lifts modulo B and integer exponents over their full common period.

    Every unit mod B lifts to a profinite unit by choosing unit coordinates at all other primes.
    The admitted b satisfy b=c mod N, with N|B. Multiplication proves each computed finite order.
    e+M*t cycles after period/gcd(period,M) steps. Every exponent class is represented.
    Thus enumeration is the entire image modulo B, not sampling. The gcd of image differences
    and B gives its largest constant modulus dividing B; its representative is a power of a lift.
    """
    period = unit_period(B)
    vals = {pow(b, e+M*t, B) for b in range(B) if gcd(b, B) == 1 and b % N == c % N
            for t in range(period//gcd(period, M))}
    r = min(vals)
    F = B
    for v in vals:
        F = gcd(F, v-r)
    return r % F, canon(F), len(vals)


def powers():
    rows = []
    images = 0
    # B=8*N*g*(g+1)! contains the finest modulus by catalogue.md:282-283.
    # These cases have g<=2, so B<=672. No prime-exponent formula is used by this oracle.
    for rawN in range(1, 8):
        N = canon(rawN)
        for c in range(1, rawN+1):
            if gcd(c, rawN) != 1:
                continue
            c0 = c % N
            for e in range(-2, 3):
                for M in range(5):
                    E = e % M if M else e
                    if E == M == 0:
                        fineC, fineN, count = 1, 0, 1
                    else:
                        g = gcd(E, M)
                        B = 8*N*g*factorial(g+1)
                        fineC, fineN, count = power_enum(N, c0, E, M, B)
                        fineC = fineC % fineN if fineN > 1 else 1
                    images += count
                    # Enumerate base/exponent lifts at target N separately.
                    period = unit_period(N)
                    vals = {pow(c0, E+M*t, N) for t in range(period//gcd(period, M))}
                    first = min(vals)
                    D = N
                    for v in vals:
                        D = gcd(D, v-first)
                    D = canon(D)
                    for policy in range(3):
                        st = 1 if policy == 0 and len(vals) > 1 else 0
                        R = N if policy == 0 else D
                        C = first % R if R > 1 else 1
                        if policy == 2:
                            C, R = fineC, fineN
                        if E == M == 0:
                            C, R = 1, 0
                        rows.append(dict(c=c, N=rawN, e=e, M=M, d=1, policy=policy,
                                         status=st, C=str(C if st == 0 else -1), R=str(R if st == 0 else 0)))
    for c, e, M in ((1, 2, 1), (-1, 0, 1), (-1, 1, 2), (-1, -1, 0)):
        for policy in range(3):
            st = 1 if c == -1 and M % 2 and policy == 0 else 0
            C = -1 if st else (1 if c == 1 or M % 2 else (-1 if e % 2 else 1))
            R = 1 if c == -1 and M % 2 and policy else 0
            rows.append(dict(c=c, N=0, e=e, M=M, d=1, policy=policy,
                             status=st, C=str(C), R=str(R)))
    write("power", rows)
    print("power enumerated image residues", images)


def cyclotomic():
    rows = []
    for N in range(13):
        cs = (-1, 1) if N == 0 else range(1, N+1)
        for c in cs:
            if N and gcd(c, N) != 1:
                continue
            for n in range(1, 25):
                L = lcm(N, n) if N else n
                vals = ({c % n} if N == 0 else
                        {b % n for b in range(L) if gcd(b, L) == 1 and b % N == c % N})
                assert vals
                st = 0 if len(vals) == 1 else 1
                for inverse in (0, 1):
                    j = min(vals)
                    if inverse:
                        j = pow(j, -1, n)
                    rows.append(dict(c=c, N=N, n=n, inverse=inverse, status=st,
                                     j=str(j if st == 0 else 99)))
    for n in (-1, 0):
        for inverse in (0, 1):
            rows.append(dict(c=1, N=0, n=n, inverse=inverse, status=7, j="99"))
    write("cyclotomic", rows)


def volumes():
    # Exact quotient/index calculation, including points and rational radii.
    from fractions import Fraction
    rows = []
    for a in range(-2, 3):
        for H in range(6):
            for d in range(1, 5):
                v = Fraction(d, H) if H else Fraction(0)
                rows.append(dict(a=a, H=H, d=d, num=v.numerator, den=v.denominator))
    write("volume", rows)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("slice", choices=["A", "B", "C", "D"])
    args = parser.parse_args()
    if args.slice == "A":
        binomials()
    elif args.slice == "B":
        powers()
    elif args.slice == "C":
        volumes()
    else:
        cyclotomic()
