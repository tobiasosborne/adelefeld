#!/usr/bin/env python3
# lanes/drv-s/gen_s_hostile.py: write tests/driver/s-hostile.cmd and tests/driver/s-hostile.out.
#
# The two long lines of the case (a coefficient of 100000 digits and one of 60000 digits) are
# written by this script, so that they are exact; everything else is short and is written here in
# the script as well, with the reason of every expected line.  Run from the repository root:
#
#   python3 lanes/drv-s/gen_s_hostile.py
#
# No output of the driver is used anywhere: the expected lines are derived in the header of the
# case file and in the report of the lane.

import os

HEADER = """\
# s-hostile: hostile inputs of the three solver commands of the driver (roots, realroots and
# recover of tools/adf/README.md).  Every expected line is derived here; the two long lines at
# the end are written by lanes/drv-s/gen_s_hostile.py, which is the only reason that script
# exists.
#!exit 1
# P is the prime of the place, which adf_place_prime certifies with n_is_prime; a value that is
# not a prime of a place is outside the domain of the command.  8 and 1 are composite or below
# the least prime, 0 is no place, -7 is negative, 7/2 is not an integer, and 2^64 is not below
# 2^64, which is the bound of a place (conventions 7, place.h:38).  Every one of them is
# ADF_DOMAIN.
roots -2 0 1 with 8 with 5
roots -2 0 1 with 1 with 5
roots -2 0 1 with 0 with 5
roots -2 0 1 with -7 with 5
roots -2 0 1 with 7/2 with 5
roots -2 0 1 with 18446744073709551616 with 5
# K is the precision prec_p of adf_roots_padic, a slong of the interface.  Below 1 it is outside
# the domain of the function (roots.h, "Statuses"): ADF_DOMAIN.  A precision of 30 digits does not
# fit in a slong, and a size bound of the driver is ADF_LIMIT (conventions 3.1), the status the
# driver gives for a setting above ADF_PRINT_EXP_MAX as well.
roots -2 0 1 with 7 with 0
roots -2 0 1 with 7 with -1
roots -2 0 1 with 7 with 1000000000000000000000000000000
# A precision that fits in a slong but is refused by the function itself: 2 K bits(p) with K = 3000000
# and p = 7 is 2 * 3000000 * 3 = 18000000 bits, above ADF_ROOTS_BITS_MAX = 16777216 (decision
# S-D18), so the library answers ADF_LIMIT before any power of p is formed.
roots -2 0 1 with 7 with 3000000
# A line with two operands or with four is not a sentence of the grammar of the driver, which has
# one to three: ADF_PARSE.  So is a polynomial operand that is not a list of decimal integers
# separated by single spaces: the first operand of the third line below is the word "with" and the
# second is "1 2 x", both read as the polynomial and neither of that form.  The last line has the
# three zero coefficients, which is the zero polynomial, and that is ADF_DOMAIN.
roots -2 0 1 with 7
roots -2 0 1 with 7 with 5 with 6
roots with 7 with 5
roots 1 2 x with 7 with 5
roots 0 0 0 with 7 with 5
# A line of 100024 bytes, one coefficient of 100000 digits: the driver reads a line of at most
# 65536 bytes and answers ADF_LIMIT for a longer one, whatever it holds (tools/adf/README.md, "The
# language"), so the polynomial is never parsed.
"""

TAIL = """\
# One coefficient of 60000 digits, the line is 60025 bytes and is read.  The polynomial is
# X^2 - C with C = 10^60000 - 1, which is under the line limit but not under any bound of the
# library.  v_7(C) = 1: 10^6 = 1 mod 7 with 10^6 - 1 = 999999 = 7 * 142857 and 7 not dividing
# 142857, and v_7(10000) = 0, so v_7((10^6)^10000 - 1) = 1 by the lifting-the-exponent rule.  A
# square in Z_7 has an even valuation, so X^2 = C has no root in Z_7 at all, the list is complete
# and empty, and the answer is none.
"""

TAIL2 = """\
# 5000 coefficients, the polynomial X^2 - 3 with 4997 zero coefficients.  3 is not a square
# modulo 7, whose squares are 0, 1, 2 and 4, so no root of the polynomial is in Z_7: the answer is
# none.
"""

TAIL3 = """\
# 5000 coefficients, the polynomial X^4999 - 2, whose 4998 zero coefficients are between the
# constant term and the leading one.  4999 mod 6 = 1, so for every unit of Z / 7 Z the power
# x^4999 is x, and the only root modulo 7 is x = 2.  It is simple, since f' = 4999 X^4998 and
# 4999 mod 7 = 1, so s = 0 and K = max(5, 1) = 5.  Lifting the digit 2 to 7^5 = 16807 gives the
# centre 7891, and the check is that 7891^4999 = 2 modulo 16807.  The root is the only one in
# Z_7, because each root of the polynomial is a unit and the number of roots of X^4999 = 2 among
# the units is one when gcd(4999, 6) = 1.
"""


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(os.path.dirname(here))
    cmd = os.path.join(root, "tests", "driver", "s-hostile.cmd")
    out = os.path.join(root, "tests", "driver", "s-hostile.out")

    lines = []
    lines.append(HEADER)
    lines.append("roots " + "9" * 100000 + " 0 1 with 7 with 5\n")
    lines.append(TAIL)
    lines.append("roots -" + "9" * 60000 + " 0 1 with 7 with 5\n")
    lines.append(TAIL2)
    lines.append("roots -3 0 1" + " 0" * 4997 + " with 7 with 5\n")
    lines.append(TAIL3)
    lines.append("roots -2" + " 0" * 4998 + " 1 with 7 with 5\n")
    with open(cmd, "w") as f:
        f.write("".join(lines))

    expected = ["error: DOMAIN"] * 6 + ["error: DOMAIN"] * 2 + ["error: LIMIT"] * 2 \
        + ["error: PARSE"] * 4 + ["error: DOMAIN"] \
        + ["error: LIMIT", "none", "none", "7891 mod 7^5"]
    with open(out, "w") as f:
        f.write("".join(x + "\n" for x in expected))
    print("wrote", cmd, "and", out, "with", len(expected), "expected lines")


if __name__ == "__main__":
    main()
