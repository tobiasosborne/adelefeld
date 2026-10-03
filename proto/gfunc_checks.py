#!/usr/bin/env python3
"""Oracle of WP 1F.8 (lane f-slice10): functions at all places. Writes tests/ref/vectors/f-slice10/*.jsonl.

Run from the repository root:  python3 -B proto/gfunc_checks.py

No C code, no FLINT, no arb. The finite parts are decided with exact integers only; the real parts are enclosed by
the mpmath intervals at 800 bits and the exact integer roots of proto/functions_checks.py (rf_image, rf_status,
rf_tight, section "f-slice2", lines 1371-1627), which do not use arb either. The seeds are fixed; a second run
reproduces the files byte for byte.

Slice A, the root (docs/api-1f8.md G1 to G4; SPEC 9.3.3 lines 636-646; functions.md Proposition 16, line 538):

  rat_root.jsonl   {"num", "den", "n", "status", "where"} and, on OK, "rnum", "rden": the root of num/den on the
                   branch sign = +1 (the non-negative root for even n, the only root for odd n). status "OK" or
                   "DOMAIN"; where "real" (even n, num < 0) or "none" (untouched). OUTPUT PRECISION: exact.
                   The root is found by two independent integer roots: Newton (iroot_newton, below) and the
                   bisection of proto/lpow_checks.py:70 (iroot); they must agree, and root^n must equal num/den.
                   For |num|, den <= 2^20 the existence is also decided by the criterion of Proposition 16 step 3
                   and check_global_roots (functions_checks.py:585): every valuation divisible by n (trial
                   division, a complete factorisation for these sizes) and the sign; it must agree.
  real_root.jsonl  rows of rf_vector("root", n, prec, ball) (functions_checks.py:1622): the status of adf_real_root
                   on the ball and, on OK, "lo", "hi": dyadic bounds (400 bits, outward) of the image of the real
                   root over the ball, and "tight". OUTPUT PRECISION: [lo, hi] contains the image; the C result
                   must contain [lo, hi] (widened by 2^-350 of its size) and, if tight, have a radius at most 4
                   times hi - lo plus the rounding of prec bits.

Slice B, exp, sin, sinh, cos, cosh at all places (docs/api-1f8.md G5, G6; SPEC 9.3.2 lines 588-596):

  series_real.jsonl  {"f", "x", "prec", "status": "OK", "lo", "hi", "lip"}: [lo, hi] (dyadics of 400 bits, outward)
                   contains f over the real ball x: rf_image for exp, sin, cos; for sinh and cosh mpmath intervals of
                   (exp(t) -+ exp(-t))/2 at 800 bits at the end points (sinh increasing; cosh with the minimum 1 at
                   0). lip >= sup |f'| over x (e^hi; 1; cosh(max |t|)). OUTPUT PRECISION: the real coordinate must
                   contain [lo, hi] and, from 24 bits on, have a radius at most 2 lip rad(x) + 2^(6 - prec) max(|lo|,
                   |hi|, 1). Arguments of magnitude above 2^11 occur for sin and cos only.
  series_place.jsonl {"num", "den", "where"}: the first prime p with v_p(q) < c_p for q = num/den (0 for q = 0), by
                   exact valuations (functions_checks.vp) over the primes up to 2000, trying 2, 3, 5, ...; the bound
                   of G6 (c), 3^(k-1) <= |num| for the k-th odd prime, is asserted on every row. OUTPUT PRECISION:
                   exact.
"""

import json
import random
import sys
from fractions import Fraction as F
from math import gcd
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "proto"))
import functions_checks as R  # noqa: E402
import lpow_checks as LP  # noqa: E402

OUT = ROOT / "tests" / "ref" / "vectors" / "f-slice10"
WORD_MAX = 2 ** 63 - 1
UWORD_MAX = 2 ** 64 - 1


def write(name, rows):
    OUT.mkdir(parents=True, exist_ok=True)
    with open(OUT / name, "w") as fh:
        for row in rows:
            fh.write(json.dumps(row, sort_keys=True, separators=(",", ":")) + "\n")
    return len(rows)


# ------------------------------------------------------------------------------------------- integer roots

def iroot_newton(a, n):
    """The exact n-th root of the integer a >= 0, or None. Newton on integers, then a correction; no FLINT. A degree
    n >= bits(a) has no root >= 2 (2^n has n + 1 bits), so the iteration runs only for n < bits(a)."""
    assert a >= 0 and n >= 1
    if a < 2:
        return a
    if n >= a.bit_length():
        return None
    r = 1 << ((a.bit_length() + n - 1) // n)       # r >= a^(1/n)
    while True:
        s = ((n - 1) * r + a // r ** (n - 1)) // n
        if s >= r:
            break
        r = s
    while r ** n > a:
        r -= 1
    while (r + 1) ** n <= a:
        r += 1
    return r if r ** n == a else None


def int_root(a, n):
    """iroot_newton and the bisection iroot of lpow_checks.py:70 must agree (two algorithms, no shared code)."""
    x = iroot_newton(a, n)
    y = LP.iroot(a, n) if n < 4096 or a < 2 else x   # the bisection is quadratic in n; large n: Newton alone
    assert x == y, (a, n, x, y)
    if x is not None:
        assert x ** n == a
    return x


def factor_small(a):
    """The prime factorisation {p: e} of 1 <= a <= 2^20 by trial division (complete)."""
    out, p = {}, 2
    while p * p <= a:
        while a % p == 0:
            out[p] = out.get(p, 0) + 1
            a //= p
        p += 1
    if a > 1:
        out[a] = out.get(a, 0) + 1
    return out


def rat_root_ref(num, den, n):
    """(status, where, root) of the rational num/den (lowest terms, den > 0) on the branch +1; n >= 1."""
    a = F(num, den)
    assert a.numerator == num and a.denominator == den
    if n == 1:
        return ("OK", None, a)
    if num == 0:
        return ("OK", None, F(0))
    if n % 2 == 0 and num < 0:
        return ("DOMAIN", "real", None)
    rn, rd = int_root(abs(num), n), int_root(den, n)
    if rn is None or rd is None:
        return ("DOMAIN", None, None)
    root = F(rn if num > 0 else -rn, rd)
    assert root ** n == a
    return ("OK", None, root)


def criterion_small(num, den, n):
    """Existence of a rational n-th root by valuations (Proposition 16 step 3, functions.md:562) and the sign
    (step 4), for |num|, den <= 2^20 (complete factorisation)."""
    if num == 0:
        return True
    vals = dict(factor_small(abs(num)))
    for p, e in factor_small(den).items():
        vals[p] = vals.get(p, 0) - e
    return all(e % n == 0 for e in vals.values()) and (num > 0 or n % 2 == 1)


def rat_row(num, den, n):
    st, where, root = rat_root_ref(num, den, n)
    if abs(num) <= 2 ** 20 and den <= 2 ** 20 and n >= 1:
        assert (st == "OK") == criterion_small(num, den, n), (num, den, n)
    row = {"num": str(num), "den": str(den), "n": n, "status": st, "where": where or "none"}
    if st == "OK":
        row["rnum"], row["rden"] = str(root.numerator), str(root.denominator)
    return row


def rat_rows():
    rng = random.Random(10)
    rows, seen = [], set()

    def add(q, n):
        q = F(q)
        key = (q.numerator, q.denominator, n)
        if key not in seen:
            seen.add(key)
            rows.append(rat_row(q.numerator, q.denominator, n))

    # 1. a grid: num/den with |num| <= 16, den <= 12, n = 1 .. 12
    for num in range(-16, 17):
        for den in range(1, 13):
            if gcd(num, den) == 1:
                for n in range(1, 13):
                    add(F(num, den), n)
    # 2. perfect powers, and perfect powers plus or minus 1, of small and big bases (thousands of bits)
    small = [1, 2, 3, 5, 6, 7, 10, 12, 2 ** 20 + 7]
    big = [3 ** 40, 10 ** 30 + 1, rng.getrandbits(300) | 1, rng.getrandbits(1000) | (1 << 999)]
    dens = [1, 3, 4, 2 ** 31 - 1, 7 ** 20, rng.getrandbits(500) | (1 << 499) | 1]
    for b in small + big:
        for c in dens:
            g = gcd(b, c)
            bb, cc = b // g, c // g
            for n in list(range(1, 13)) + [15, 31, 64]:
                if c not in (1, 3, 4) and n not in (1, 2, 3, 5, 6, 12):
                    continue
                size = (bb.bit_length() + cc.bit_length()) * n
                if size > 3600 or (b in big and n not in (1, 2, 3, 5, 12)):
                    continue
                if size > 1200 and not (b == big[-1] and c in (1, 3, dens[-1])):
                    continue   # rows of thousands of bits: the 1000-bit base only (the file size)
                if size > 600 and b in small:
                    continue
                for sgn in (1, -1):
                    add(F(sgn * bb ** n, cc ** n), n)
                add(F(bb ** n + 1, cc ** n), n)
                if bb ** n > 1:
                    add(F(bb ** n - 1, cc ** n), n)
                add(F(bb ** n, cc ** n + 1), n)
                add(F(bb ** n, cc ** n * 2 + 1) if cc > 1 else F(bb ** n, 2 ** n + 1), n)
    # 3. degrees near and above the bit length of numerator and denominator, and above WORD_MAX
    for q in (F(1), F(-1), F(2), F(-2), F(1, 2), F(-1, 2), F(3 ** 5), F(2 ** 64), F(2 ** 63), F(-(2 ** 63)),
              F(1, 2 ** 63), F(3 ** 63, 2 ** 63), F(-(3 ** 63), 2 ** 63), F(5, 7), F(2 ** 127), F(2 ** 128 - 1)):
        bits = {abs(q.numerator).bit_length(), q.denominator.bit_length()}
        ns = {WORD_MAX, WORD_MAX + 1, UWORD_MAX, UWORD_MAX - 1}
        for b in bits:
            ns |= {b - 1, b, b + 1}
        for n in sorted(x for x in ns if x >= 1):
            add(q, n)
    return rows


# ------------------------------------------------------------------------------------------- real roots

def real_root_rows():
    """Real balls for the real coordinate of the root: positive, negative, crossing 0, exact 0, huge and tiny, with
    even and odd n and the precisions 2 to 300."""
    rng = random.Random(11)
    RB = R.RB
    balls = [RB(0, 0), RB(1, 0), RB(-1, 0), RB(4, 0), RB(-8, 0), RB(9, 0, 1, -40), RB(-27, 0, 1, -40),
             RB(1, 0, 1, 0), RB(0, 0, 1, -3), RB(1, -1, 1, -1), RB(-1, -1, 1, -1), RB(3, -2, 1, -1),
             RB(-3, -2, 1, -1), RB(1, 1000, 1, 960), RB(1, -1000, 1, -1040), RB(-5, 300, 3, 280),
             RB(2, 0, 3, 0), RB(-2, 0, 3, 0), RB(7, 0, 1, 0), RB(-7, 0, 1, 0), RB(12345, -10, 7, -20)]
    for _ in range(10):
        m = rng.randrange(-2 ** 20, 2 ** 20)
        e = rng.randrange(-40, 40)
        rm = rng.randrange(0, 2 ** 20)
        re = e + rng.randrange(-40, 4)
        balls.append(RB(m, e, rm, re))
    rows = []
    for rb in balls:
        for n in (1, 2, 3, 4, 7, 12):
            for prec in (2, 64, 300):
                rows.append(R.rf_vector("root", n, prec, rb))
    return rows


# ------------------------------------------------------------------------------------------- slice B: the series

SERIES = ("exp", "sin", "sinh", "cos", "cosh")
CONST = {"exp": 1, "sin": 0, "sinh": 0, "cos": 1, "cosh": 1}   # Proposition 12 step 1, functions.md:394


def _iv_hyp(name, q):
    """An enclosure (lower, upper) of sinh(q) or cosh(q), q a Fraction: (exp(q) -+ exp(-q)) / 2 in mpmath interval
    arithmetic at 800 bits (mpmath's iv has no sinh, cosh); rigorous, as every iv operation rounds outward."""
    from mpmath import iv
    iv.prec = R.IV_PREC
    x = R._ivq(q)
    e, d = iv.exp(x), iv.exp(-x)
    return R._iv_bounds((e - d) / 2 if name == "sinh" else (e + d) / 2)


def series_image(f, lo, hi):
    """(minlo, maxhi) enclosing f over [lo, hi]: rf_image of functions_checks.py for exp, sin, cos; mpmath intervals
    at 800 bits for sinh (increasing) and cosh (decreasing on t <= 0, increasing on t >= 0, minimum 1 at 0)."""
    if f in ("exp", "sin", "cos"):
        return R.rf_image(f, 0, lo, hi)
    a, b = _iv_hyp(f, lo), _iv_hyp(f, hi)
    if f == "sinh":
        return (a[0], b[1])
    lows = [a[0], b[0]] + ([F(1)] if lo <= 0 <= hi else [])
    return (min(lows), max(a[1], b[1]))


def series_lip(f, lo, hi):
    """An upper bound of sup |f'| over [lo, hi], a dyadic of 400 bits rounded up: e^hi (exp), 1 (sin, cos),
    cosh(max |t|) (sinh; and cosh, whose derivative sinh is smaller)."""
    if f == "exp":
        return R.dy_json_bound(R._iv_fn("exp", hi)[1], True)
    if f in ("sin", "cos"):
        return {"m": 1, "e": 0}
    return R.dy_json_bound(_iv_hyp("cosh", max(abs(lo), abs(hi)))[1], True)


def series_real_rows():
    """Real balls for the real coordinate of the five series: positive, negative, around 0, large (2^40), exact."""
    rng = random.Random(12)
    RB = R.RB
    balls = [RB(0, 0), RB(1, -1), RB(-1, -1), RB(0, 0, 1, -3), RB(0, 0, 1, 0), RB(3, 0, 1, -2), RB(-3, 0, 1, -2),
             RB(100, 0), RB(-100, 0, 1, -10), RB(1, 40), RB(-1, 40, 1, 0), RB(1000, 0, 1, -30), RB(-1000, 0),
             RB(7, 0, 4, 0), RB(355, -7, 1, -40), RB(1, -60), RB(-5, 0, 3, 0)]
    for _ in range(6):
        m = rng.randrange(-2 ** 20, 2 ** 20)
        e = rng.randrange(-30, -10)
        balls.append(RB(m, e, rng.randrange(0, 2 ** 20), e + rng.randrange(-30, 2)))
    rows = []
    for rb in balls:
        for f in SERIES:
            if f not in ("sin", "cos") and max(abs(rb.lo), abs(rb.hi)) > 2 ** 11:
                continue   # exp(2^40) has an exponent of 2^40 bits: not written as an exact dyadic (sin, cos only)
            a, b = series_image(f, rb.lo, rb.hi)
            for prec in (2, 64, 200):
                rows.append({"f": f, "x": rb.json(), "prec": prec, "status": "OK",
                             "lo": R.dy_json_bound(a, False), "hi": R.dy_json_bound(b, True),
                             "lip": series_lip(f, rb.lo, rb.hi)})
    return rows


_PRIMES = {}


def first_failing_prime(q, limit=2000):
    """The first prime p in increasing order with v_p(q) < c_p (c_2 = 2, c_p = 1), by exact valuations of the
    Fraction q (functions_checks.vp), trying 2, 3, 5, ... (G6); 0 for q = 0 (no prime fails)."""
    if q == 0:
        return 0
    if limit not in _PRIMES:
        _PRIMES[limit] = R.small_primes(limit)
    for p in _PRIMES[limit]:
        if R.vp(q, p) < R.cdisc(p):
            return p
    raise AssertionError("no failing prime below the limit")


def series_place_rows():
    """{"num", "den", "where"}: the place of DOMAIN of an exact finite part q (0 for q = 0, which is OK)."""
    rows, seen = [], set()

    def add(q):
        q = F(q)
        if q in seen:
            return
        seen.add(q)
        p = first_failing_prime(q, 2000)
        rows.append({"num": str(q.numerator), "den": str(q.denominator), "where": p})

    for num in range(-60, 61):
        for den in (1, 2, 3, 4, 5, 9, 12, 25):
            add(F(num, den))
    odd = R.small_primes(400)[1:]
    for k in range(0, 60):   # 4 * (3 * 5 * ... * p_k): the first failing prime is the next odd prime
        prod = 4
        for p in odd[:k]:
            prod *= p
        for e in (1, 2, 3):
            add(F(prod ** e if e > 1 else prod, 7 ** e))
            add(F(-prod, 1))
            add(F(prod, prod + 1))
    rng = random.Random(13)
    for _ in range(40):
        add(F(rng.getrandbits(rng.randrange(1, 400)) * rng.choice((1, 4, 36, 900)), rng.randrange(1, 10 ** 6)))
    for q in rows:
        n = abs(int(q["num"]))
        if n:
            # the bound of G6: at most floor(log_3 |num|) + 1 odd primes are tried
            k = odd.index(q["where"]) + 1 if q["where"] != 2 else 0
            assert 3 ** (k - 1) <= n if k else True
    return rows


def main():
    counts = {}
    counts["rat_root"] = write("rat_root.jsonl", rat_rows())
    counts["real_root"] = write("real_root.jsonl", real_root_rows())
    counts["series_real"] = write("series_real.jsonl", series_real_rows())
    counts["series_place"] = write("series_place.jsonl", series_place_rows())
    names = ("rat_root.jsonl", "real_root.jsonl", "series_real.jsonl", "series_place.jsonl")
    sizes = sum((OUT / f).stat().st_size for f in names)
    print(" ".join(f"{k}={v}" for k, v in counts.items()), f"bytes={sizes}")


if __name__ == "__main__":
    main()
