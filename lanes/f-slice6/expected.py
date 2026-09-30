#!/usr/bin/env python3
"""lanes/f-slice6/expected.py: the expected lines of tests/driver/f-*.out, computed from mathematics alone.

Independent of the library: exact Fractions (lanes/f-slice6/gen_vectors.py: series of exp, log, Log; their
definitions are in docs/proofs/functions.md Definition 1) and, for a real ball, the reference printer of
docs/conventions.md 9.5 (proto/text_grammar.py print_real) applied to mpmath values. A real ball printed with few
digits does not depend on the exact midpoint and radius of the ball that arb returns, only on the value and on a
radius far below the last printed digit: every real line is computed for two radii (2^-40 and 2^-70) and must not
change.

Usage: python3 lanes/f-slice6/expected.py          prints "label: line" for each case
"""
import os
import sys
from fractions import Fraction

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "proto"))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import mpmath                                   # noqa: E402
from text_grammar import print_real             # noqa: E402
import gen_vectors as g                         # noqa: E402


def local(f, p, x, N):
    r = g.case(f, p, Fraction(x), N)
    if r["status"] != "OK":
        return "error: " + r["status"]
    c = int(r["c"])
    if r["exact"]:
        return str(Fraction(c))
    return "%d + O(%d^%d)" % (c, p, N)


def real(value, digits):
    mpmath.mp.prec = 400
    texts = set()
    for rad in (Fraction(1, 2 ** 40), Fraction(1, 2 ** 70)):
        mid = Fraction(mpmath.mpf(value).man) * Fraction(2) ** int(mpmath.mpf(value).exp)
        texts.add(print_real(mid, rad, digits))
    assert len(texts) == 1, texts
    return texts.pop()


def main():
    out = []
    # prec 8: the absolute precision at a prime is 8
    out.append(("exp_at 5 with 5", local("exp", 5, 5, 8)))
    out.append(("log_at 6 with 5", local("log", 5, 6, 8)))
    out.append(("log_at -1 with 2", local("log", 2, -1, 8)))
    out.append(("exp_at 1 with 5", local("exp", 5, 1, 8)))
    out.append(("exp_at 0 with 5", local("exp", 5, 0, 8)))
    out.append(("exp_at 4 with 2", local("exp", 2, 4, 8)))
    out.append(("exp_at 2 with 2", local("exp", 2, 2, 8)))
    out.append(("log_at 3 with 2", local("log", 2, 3, 8)))
    out.append(("log_at 10 with 3", local("log", 3, 10, 8)))
    out.append(("log_at 1/2 with 3", local("log", 3, Fraction(1, 2), 8)))
    out.append(("exp_at 1/5 with 5", local("exp", 5, Fraction(1, 5), 8)))
    out.append(("exp_at 20/3 with 5", local("exp", 5, Fraction(20, 3), 8)))
    out.append(("log_at 26/5 with 5", local("log", 5, Fraction(26, 5), 8)))
    out.append(("log_at 1 with 7", local("log", 7, 1, 8)))
    # a finite ball (* ; 5 mod 25) = 5 + 25 Zhat: at 5 it is 5 + 5^2 Z_5, exp keeps N = min(8, 2) = 2
    out.append(("exp of 5 + 5^2 Z_5, N=8: c mod 25", str(int(g.case("exp", 5, Fraction(5), 2)["c"]))))
    # real place
    out.append(("exp_at 1 with real (digits 6)", real(mpmath.e, 6)))
    out.append(("log_at 2 with real (digits 6)", real(mpmath.log(2), 6)))
    out.append(("project 2/3, real (digits 6)", real(mpmath.mpf(2) / 3, 6)))
    out.append(("project 2/3 at 2 (prec 8)", "2/3 = 2 * 3^-1: v_2 = 1, unit part 3^-1 mod 2^7"))
    # 2/3 at 5: the ball is exact 2/3
    for label, line in out:
        print("%s: %s" % (label, line))
    # the exact centre of 2/3 at 5 and at 2 is the rational itself (an exact local ball): printed 2/3
    # Log values at 5: Log(10) = log(16)/4
    print("Log(10) at 5, N=8:", local("Log", 5, 10, 8))
    print("exp(5) mod 5^8 =", g.exp_series(Fraction(5), 5, 8), " 5^8 =", 5 ** 8)


if __name__ == "__main__":
    main()
