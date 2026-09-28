#!/usr/bin/env python3
"""Independent exact arithmetic implementation of conventions 9.5; compare the reference.

No imported printer helpers are used by the oracle. Decimal output is read with Fraction.
All randomly generated radii have at most 30 bits, as required for stored dyadic balls.
"""
from fractions import Fraction as F
from pathlib import Path
import random
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "proto"))
import text_grammar as tg

MAX_LOOPS = MAX_LEVEL = MAX_PASSES = 0


def decade(x):
    x = abs(x)
    assert x
    e = len(str(x.numerator)) - len(str(x.denominator))
    return e if x >= F(10) ** e else e - 1


def decimal_parts(x):
    den, a, b = x.denominator, 0, 0
    while den % 2 == 0:
        den //= 2
        a += 1
    while den % 5 == 0:
        den //= 5
        b += 1
    assert den == 1
    exponent = -max(a, b)
    integer = x.numerator * 2 ** (-exponent - a) * 5 ** (-exponent - b)
    while integer and integer % 10 == 0:
        integer //= 10
        exponent += 1
    return integer, exponent


def fmt(x):
    if not x:
        return "0"
    integer, e = decimal_parts(x)
    digits = str(abs(integer))
    top = e + len(digits) - 1
    if -4 <= top <= 20:
        if e >= 0:
            body = digits + "0" * e
        elif top >= 0:
            body = digits[:top + 1] + "." + digits[top + 1:]
        else:
            body = "0." + "0" * (-top - 1) + digits
    else:
        body = digits[0] + ("." + digits[1:] if len(digits) > 1 else "") + "e" + str(top)
    return ("-" if x < 0 else "") + body


def ceiling(x, k):
    if not x:
        return F(0)
    unit = F(10) ** (decade(x) - k + 1)
    y = x / unit
    return (-(-y.numerator // y.denominator)) * unit


def level(mid, rad, n, k):
    global MAX_LOOPS, MAX_LEVEL
    MAX_LEVEL = max(MAX_LEVEL, k)
    nd = n + k - 2
    if not rad and (not mid or len(str(abs(decimal_parts(mid)[0]))) <= nd):
        return mid, F(0)
    if not mid:
        return F(0), ceiling(rad, k)
    q = decade(mid) - nd + 1
    if rad:
        q = max(q, decade(rad) - k + 1)
    for count in range(1, 100):
        unit = F(10) ** q
        y = mid / unit
        floor, remainder = divmod(y.numerator, y.denominator)
        up = 2 * remainder > y.denominator or (2 * remainder == y.denominator and floor % 2)
        M = (floor + int(up)) * unit
        R = ceiling(rad + abs(M - mid), k)
        assert R > 0
        qnext = decade(R) - k + 1
        if M:
            qnext = max(qnext, decade(M) - nd + 1)
        if (M / F(10) ** qnext).denominator == 1:
            MAX_LOOPS = max(MAX_LOOPS, count)
            return M, R
        assert qnext > q
        q = qnext
    raise AssertionError("rounding did not terminate")


def own_print(mid, rad, n, cond):
    global MAX_PASSES
    if cond is None:
        M, R = level(mid, rad, n, 2)
    else:
        previous = None
        for passes in range(1, 100):
            for k in range(2, 200):
                M, R = level(mid, rad, n, k)
                if (M > R if cond == "positive" else abs(M) > R):
                    break
            else:
                raise AssertionError("sign margin not reached")
            if previous == (M, R):
                MAX_PASSES = max(MAX_PASSES, passes)
                break
            previous = mid, rad = M, R
        else:
            raise AssertionError("fixed point not reached")
    return fmt(M) + (" +/- " + fmt(R) if R else "")


def check(mid, rad, n, cond=None):
    expected = own_print(mid, rad, n, cond)
    actual = tg.print_real(mid, rad, n, cond)
    assert actual == expected, (mid, rad, n, cond, expected, actual)
    parts = actual.split(" +/- ")
    M = F(parts[0])
    R = F(parts[1]) if len(parts) == 2 else F(0)
    assert R >= rad + abs(M - mid)
    assert own_print(M, R, n, cond) == actual
    assert tg.print_real(M, R, n, cond) == actual
    if cond:
        assert (M > R if cond == "positive" else abs(M) > R)
    return actual


def main():
    rng = random.Random(2026092803)
    for i in range(12000):
        mid = F(rng.getrandbits(rng.randint(1, 128))) * F(2) ** rng.randint(-500, 500)
        mid *= rng.choice((-1, 1))
        rad = F(rng.getrandbits(rng.randint(1, 30))) * F(2) ** rng.randint(-500, 500)
        if i % 7 == 0:
            rad = F(0)
        if i % 31 == 0:
            mid = F(0)
        check(mid, rad, rng.choice((1, 2, 3, 5, 20, 50)))
    print("random_dyadic_balls=12000 failures=0 seed=2026092803")
    for i in range(3000):
        rad = F(rng.randrange(1, 2 ** 30)) * F(2) ** rng.randint(-100, 100)
        mid = rad + F(2) ** (decade(rad) - rng.randint(1, 60))
        cond = "positive" if i % 2 == 0 else "nonzero"
        if cond == "nonzero":
            mid = -mid
        check(mid, rad, rng.choice((1, 2, 5, 20)), cond)
    print("random_sign_constrained_balls=3000 failures=0")
    count = 0
    for line in (ROOT / "tests/golden/realball_print.tsv").read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        inp, expected = line.split("\t")
        mid, rad, n = inp.split()
        assert check(F(mid), F(rad), int(n)) == expected
        count += 1
    print("golden_hard_cases=%d failures=0" % count)
    for sign in (-1, 1):
        for mid, rad, n in ((F(20396493, 16), F(1258000), 2),
                            (F(1), F(1) - F(2) ** -30, 20),
                            (F(1), F(1023, 1024), 20)):
            print("hard mid=%s rad=%s n=%s -> %s" %
                  (sign * mid, rad, n, check(sign * mid, rad, n, "nonzero")))
    print("hard_sign_cases=6 failures=0")
    print("max_rounding_loops=%d max_radius_digits=%d max_fixed_point_passes=%d" %
          (MAX_LOOPS, MAX_LEVEL, MAX_PASSES))


if __name__ == "__main__":
    main()
