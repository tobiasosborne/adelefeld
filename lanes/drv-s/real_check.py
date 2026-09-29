# lanes/drv-s/real_check.py: the check of the one line of tests/driver/s-realroots.cmd that depends
# on which isolating ball FLINT returns (the line at the setting digits 20).
#
#   make all && cc -Iinclude -std=c11 -O2 lanes/drv-s/real_probe.c build/libadelefeld.a \
#       -lflint -lgmp -lm -o build/real_probe
#   python3 lanes/drv-s/real_check.py
#
# The script reads the exact dyadic midpoint and radius of every ball of the list from the probe
# and then makes four checks, all of them in exact rational arithmetic (fractions.Fraction), none
# of them in floating point:
#
#   1. the text that the algorithm of conventions 9.5 gives for (mid, rad) with n = 20 is the text
#      the driver printed; the algorithm is implemented here from conventions 9.5 and not read off
#      the program;
#   2. the printed interval [M - R, M + R] contains the exact ball [mid - rad, mid + rad], which
#      is the property of 9.5 that a printer must have;
#   3. the exact end points of the ball give opposite signs of g = X^2 - 2, so the ball holds a
#      real root of g and only one (solvers P3.8, the test the library makes), and the square of
#      the root lies in [lo^2, hi^2] with 2 strictly inside and lo * hi > 0, so the root is the
#      one of +sqrt(2) or of -sqrt(2) that the sign of mid names;
#   4. the accuracy of the ball is at least prec = 64 bits, as S-D19 requires, computed from the
#      position of the top bit of the radius and of the midpoint as arb.rst:500 to 504 defines it.
#
# So the only thing this script takes from the program is which ball FLINT chose.  Every property
# of the line that the specification fixes is checked here, and the check is what makes the line a
# test and not a transcript.

import subprocess
import sys
from fractions import Fraction

PREC = 64
DIGITS = 20
COEFFS = ["-2", "0", "1"]


# ---- conventions 9.5, step by step, in exact arithmetic

def X(y):
    """X(y) = floor(log10 |y|) for y != 0."""
    y = abs(Fraction(y))
    assert y != 0
    d = len(str(y.numerator)) - len(str(y.denominator))
    while Fraction(10) ** d > y:
        d -= 1
    while Fraction(10) ** (d + 1) <= y:
        d += 1
    return d


def ceil2(E):
    """ceil2(E) for E > 0: E rounded up to two significant digits."""
    assert E > 0
    u = Fraction(10) ** (X(E) - 1)
    k = -((-E) // u)                      # the least integer k with k u >= E
    assert (k - 1) * u < E <= k * u
    return k * u


def round_q(y, q):
    """round(y, q): y rounded to the nearest multiple of 10^q, ties to the even multiple."""
    n = round(Fraction(y) / Fraction(10) ** q)      # Python rounds halves to even
    return n * Fraction(10) ** q


def is_multiple(y, q):
    z = Fraction(y) / Fraction(10) ** q
    return z == int(z)


def fmt(y):
    """fmt(y) of conventions 9.5: |y| = D 10^E with D a positive integer not divisible by 10,
    k the number of its digits and X = E + k - 1; positional when -4 <= X <= 20, else d.ddd e X.
    Written here from the definition: the decimal of |y| is exact (every y of 9.5 is a multiple of
    a power of 10), and the trailing zeros of its digits are the ones that lower E."""
    y = Fraction(y)
    if y == 0:
        return "0"
    sign = "-" if y < 0 else ""
    y = abs(y)
    d = y.denominator
    a = b = 0
    while d % 2 == 0:
        d //= 2
        a += 1
    while d % 5 == 0:
        d //= 5
        b += 1
    assert d == 1, "not a terminating decimal"
    t = max(a, b)
    N = y * 10 ** t
    assert N.denominator == 1
    s0 = str(N.numerator)
    s = s0.rstrip("0")
    E = (len(s0) - len(s)) - t          # |y| = s 10^E with s not divisible by 10
    k = len(s)
    X = E + k - 1
    if -4 <= X <= 20:
        if E >= 0:
            return sign + s + "0" * E
        if k > -E:
            return sign + s[:k + E] + "." + s[k + E:]
        return sign + "0." + "0" * (-E - k) + s
    return sign + (s if k == 1 else s[0] + "." + s[1:]) + "e" + str(X)


def print95(mid, rad, n):
    """the text of conventions 9.5 for the exact mid and rad >= 0, with n digits."""
    mid, rad = Fraction(mid), Fraction(rad)
    # step 1
    if rad == 0 and (mid == 0 or len(str(abs(mid.numerator))) <= n):
        return fmt(mid)
    # step 2
    if mid == 0:
        return "0 +/- " + fmt(ceil2(rad))
    # steps 3 to 5
    q = X(mid) - n + 1
    if rad > 0:
        q = max(q, X(rad) - 1)
    while True:
        M = round_q(mid, q)
        R = ceil2(rad + abs(M - mid))
        q2 = X(R) - 1 if M == 0 else max(X(M) - n + 1, X(R) - 1)
        if is_multiple(M, q2):
            break
        assert q2 > q
        q = q2
    # step 6
    return fmt(M) + " +/- " + fmt(R)


# ---- the balls of the probe

def balls(coeffs, prec):
    out = subprocess.run(["./build/real_probe", str(prec)] + list(coeffs),
                         capture_output=True, text=True, check=True)
    lines = out.stdout.splitlines()
    assert lines[0] == "status 0", out.stdout
    res, cur = [], None
    for line in lines:
        parts = line.split()
        if parts[0] == "ball":
            cur = {}
            res.append(cur)
        elif parts[0] in ("lo", "mid", "rad"):
            cur[parts[0]] = Fraction(int(parts[1])) * Fraction(2) ** int(parts[2][2:])
    return res


def top_bit(y):
    """the position of the top bit of y, that is e with y = m 2^e and 1/2 <= |m| < 1, which is
    the convention of FLINT for a binary exponent (refs/src/flint-3.0.1/arf.rst:14 to 20)."""
    y = abs(Fraction(y))
    if y == 0:
        return None
    e = 0
    while y < Fraction(1, 2):
        y *= 2
        e -= 1
    while y >= 1:
        y /= 2
        e += 1
    return e


def main():
    bs = balls(COEFFS, PREC)
    print("the %d balls of adf_roots_real at prec %d for X^2 - 2" % (len(bs), PREC))
    texts = []
    for k, b in enumerate(bs):
        lo, mid, rad = b["lo"], b["mid"], b["rad"]
        t = print95(mid, rad, DIGITS)                 # check 1
        texts.append(t)
        M, R = t.split(" +/- ")
        Rv = Fraction(R)
        assert Fraction(M) - Rv <= lo and mid + rad <= Fraction(M) + Rv, "not an enclosure"
        g_lo = lo * lo - 2                             # check 3
        g_hi = (mid + rad) ** 2 - 2
        assert g_lo * g_hi < 0, "the end points do not straddle a root of X^2 - 2"
        assert lo * (mid + rad) > 0, "the ball meets 0"
        assert min(lo ** 2, (mid + rad) ** 2) < 2 < max(lo ** 2, (mid + rad) ** 2), "no root"
        em, er = top_bit(mid), top_bit(rad)           # check 4
        acc = "exact" if rad == 0 else em - er - 1
        assert rad == 0 or acc >= PREC, ("accuracy below prec", acc)
        print("  ball %d: mid = %s, rad = %s, accuracy bits = %s" % (k, mid, rad, acc))
        print("    text of conventions 9.5 with n = %d: %s" % (DIGITS, t))
    line = "; ".join(texts)
    got = subprocess.run(["./build/adf"], input="realroots " + " ".join(COEFFS) + "\n",
                         capture_output=True, text=True)
    print("the line the driver prints at the setting digits %d: %s" % (DIGITS, got.stdout.strip()))
    if got.stdout.strip() != line:
        print("MISMATCH with the text of conventions 9.5")
        return 1
    print("the two agree")
    return 0


if __name__ == "__main__":
    sys.exit(main())
