"""Own integer-bisection and 1200-bit interval oracles. No repository oracle imports."""
from pathlib import Path
from fractions import Fraction
import random
import sys
import mpmath as mp

sys.set_int_max_str_digits(0)
lane = Path(__file__).resolve().parent
rng = random.Random(202610039)
mp.iv.prec = 1200

def iroot(a, n):
    if a <= 1 or n == 1:
        return a, True
    if n >= a.bit_length():
        return 1, False
    lo, hi = 1, 1 << ((a.bit_length() + n - 1) // n)
    while lo + 1 < hi:
        m = (lo + hi) // 2
        if m ** n <= a:
            lo = m
        else:
            hi = m
    if hi ** n == a:
        return hi, True
    return lo, lo ** n == a

def root(q, n, s):
    if n == 0:
        return 7, 0, Fraction(0)
    if n == 1:
        return 0, 0, q
    if s not in (-1, 1):
        return 7, 0, Fraction(0)
    if not q:
        return 0, 0, q
    if s == -1 and n % 2:
        return 7, 0, Fraction(0)
    if q < 0 and n % 2 == 0:
        return 7, 1, Fraction(0)
    a, ae = iroot(abs(q.numerator), n)
    b, be = iroot(q.denominator, n)
    if not (ae and be):
        return 7, 0, Fraction(0)
    sign = (-1 if q < 0 else 1) if n % 2 else s
    return 0, 0, Fraction(sign * a, b)

rows = []
degrees = [0, 1, 2, 3, 4, 7, 31, 63, 64, (1 << 63) - 1, 1 << 63, (1 << 64) - 1]
for a in [0, 1, -1, 2, -2, 4, -4, 8, -8, 17, -17]:
    for b in [1, 9, 27, 1 << 257]:
        for n in degrees:
            for s in [-2147483648, -1, 0, 1, 2, 2147483647]:
                rows.append((Fraction(a, b), n, s))
for j in range(180):
    bits = rng.randrange(512, 3001)
    a = rng.getrandbits(bits) | (1 << (bits - 1)) | 1
    b = rng.getrandbits(bits - 7) | 1
    n = rng.randrange(2, 8)
    for delta in [-1, 0, 1]:
        q = Fraction(a ** n + delta, b ** n)
        for qsign in [-1, 1]:
            for s in [-1, 1]:
                rows.append((qsign * q, n, s))
    q = Fraction(a, b)
    for n2 in [bits - 1, bits, bits + 1]:
        rows.append((q, n2, 1))
for j in range(300):
    a = rng.randrange(-1000000, 1000001)
    b = rng.randrange(1, 1000000)
    rows.append((Fraction(a, b), rng.randrange(1, 16), rng.choice([-1, 1])))
if "--real-only" not in sys.argv:
    with (lane / "rat.tsv").open("w") as f:
        for q, n, s in rows:
            st, place, r = root(q, n, s)
            f.write(f"{q.numerator} {q.denominator} {n} {s} {st} {place} {r.numerator} {r.denominator}\n")

def dy(v):
    s, m, e, bits = v
    assert bits >= 0
    return -m if s else m, e

def endpoint_root(x, n):
    if x == 0:
        return mp.iv.mpf(0)
    if n % 2 or x > 0:
        a, ae = iroot(abs(x.numerator), n)
        b, be = iroot(x.denominator, n)
        if ae and be:
            return mp.iv.mpf(-a if x < 0 else a) / b
    if x < 0:
        return -mp.iv.exp(mp.iv.log(mp.iv.mpf(-x.numerator) / x.denominator) / n)
    return mp.iv.exp(mp.iv.log(mp.iv.mpf(x.numerator) / x.denominator) / n)

realrows = []
balls = [(0, 0, 0, 0), (0, 0, 1, 0), (1, 0, 1, -20), (-1, 0, 1, -20),
         (1, 0, 1, 0), (3, -1, 1, -1)]
for e in [-4096, -1000, -64, 64, 1000, 4096]:
    balls.extend([(1, e, 0, 0), (-1, e, 0, 0), (1, e, 1, e - 40)])
for j in range(60):
    e = rng.randrange(-512, 513)
    balls.append((rng.randrange(-15, 16), e, 1, e - rng.randrange(0, 60)))
for m, e, rm, re in balls:
    mp.iv.prec = 1200
    center = Fraction(m) * (Fraction(2) ** e)
    radius = Fraction(rm) * (Fraction(2) ** re)
    a, b = center - radius, center + radius
    for n in [2, 3, 4, 7, 64, (1 << 63) - 1, 1 << 63, (1 << 64) - 1]:
        st = 7 if n % 2 == 0 and b < 0 else 1 if n % 2 == 0 and a < 0 else 0
        for p in [-7, 0, 1, 2, 3, 24, 64, 257]:
            for s in [-1, 1]:
                status = st
                if s == -1 and n % 2 and (m or rm):
                    status = 7
                lo = hi = mp.iv.mpf(0)
                if status == 0:
                    lo = endpoint_root(a, n)
                    hi = endpoint_root(b, n)
                    if n % 2 == 0 and s == -1:
                        lo, hi = -hi, -lo
                lm, le = dy(lo._mpi_[0])
                hm, he = dy(hi._mpi_[1])
                realrows.append((0, m, e, rm, re, n, s, p, status, lm, le, hm, he))
    # Sample endpoints, center, and two interior points for the non-monotone series.
    if abs(center) < 10000 and radius < 10000:
        # Resolves cancellation at tiny sinh arguments and tiny cos/cosh deviations from 1.
        mp.iv.prec = 1200 + 2 * max(0, -e, -re)
        sinh = lambda x: (mp.iv.exp(x) - mp.iv.exp(-x)) / 2
        cosh = lambda x: (mp.iv.exp(x) + mp.iv.exp(-x)) / 2
        for op, fn in enumerate([mp.iv.exp, mp.iv.sin, sinh, mp.iv.cos, cosh], 1):
            for p in [2, 24, 64, 257]:
                for x in [a, (3*a+b)/4, center, (a+3*b)/4, b]:
                    val = fn(mp.iv.mpf(x.numerator) / x.denominator)
                    lm, le = dy(val._mpi_[0])
                    hm, he = dy(val._mpi_[1])
                    realrows.append((op, m, e, rm, re, 0, 1, p, 0, lm, le, hm, he))
with (lane / "real.tsv").open("w") as f:
    for row in realrows:
        f.write(" ".join(map(str, row)) + "\n")
print(f"rat_rows={len(rows)} real_rows={len(realrows)} root_interval_bits=1200 series_interval_bits=1200..9392")
