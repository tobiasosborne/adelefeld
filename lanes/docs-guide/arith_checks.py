#!/usr/bin/env python3
"""lanes/docs-guide/arith_checks.py: the numbers of docs/GUIDE.md that are derived in the prose, not printed
by the driver. Exact arithmetic with the standard library; each line is 'claim: value' and an assert."""
from fractions import Fraction as F
import math


def base(n, b):
    d = ""
    while n:
        d = str(n % b) + d
        n //= b
    return d or "0"


out = []
def say(claim, value):
    out.append("%s: %s" % (claim, value))

# 1.1 the product formula for 22/7 over real, 2, 7, 11 (and 1 elsewhere)
prod = F(22, 7) * F(1, 2) * 7 * F(1, 11)
assert prod == 1
say("(22/7) * (1/2) * 7 * (1/11)", prod)
# 1.3 base-5 digits of the centres 624 and 10417, and 1/3 modulo 5^6
assert base(624, 5) == "4444" and (624 + 1) % 5**4 == 0
say("624 in base 5", base(624, 5))
assert base(10417, 5) == "313132" and (3 * 10417) % 5**6 == 1
say("10417 in base 5", base(10417, 5))
say("3 * 10417", "%d = 2 * 5^6 + %d" % (3 * 10417, 3 * 10417 - 2 * 5**6))
# the 5-adic digits of 1/3, ten of them: (1/3) mod 5^10 in base 5
c = pow(3, -1, 5**10)
say("1/3 mod 5^10 in base 5", base(c, 5))
# 1.4 Chinese remainder: 146 mod 8 and mod 125
say("146 mod 8, 146 mod 125", "%d, %d" % (146 % 8, 146 % 125))
# 1.5 22/7 against 146 + 1000 Zhat
say("7 * 146", 7 * 146)
say("(22/7 - 146) / 1000", F(22, 7) - 146 and (F(22, 7) - 146) / 1000)
# 1.6 idempotents modulo 30
idem = [x for x in range(30) if (x * x - x) % 30 == 0]
assert len(idem) == 8
say("x^2 = x modulo 30", idem)
# 3.2 cap: gcd(1000, 10), gcd(1000, 7)
say("gcd(1000, 10), gcd(1000, 7)", "%d, %d" % (math.gcd(1000, 10), math.gcd(1000, 7)))
# 3.5 the roots: 3^2 = 2 + 7, 2^2 = -1 + 5, 36^2 = 46 + 10 * 125
assert 3**2 == 2 + 7 and 2**2 == -1 + 5 and 36**2 == 46 + 10 * 125
say("36^2", "%d = 46 + %d * 125" % (36**2, (36**2 - 46) // 125))
# 3.8 1/2 and -1/2 modulo 7
say("1/2 mod 7, -1/2 mod 7", "%d, %d" % (pow(2, -1, 7), (-pow(2, -1, 7)) % 7))
# 3.11 haar volume of 1/2 + (8/3) Zhat
say("1 / (8/3)", 1 / F(8, 3))
# 3.15 pi/6, 1/pi, 4/3
say("pi/6", repr(math.pi / 6))
say("1/pi", repr(1 / math.pi))
say("(1 - 2^-2)^-1", 1 / (1 - F(1, 4)))
# 3.16 the dump midpoint of 0.1 at 64 bits: 0x3333333333333333 * 2^-65
m = F(0x3333333333333333, 2**65)
from decimal import Decimal, getcontext
getcontext().prec = 40
say("0x3333333333333333 * 2^-65", Decimal(m.numerator) / Decimal(m.denominator))
say("0.1 - that midpoint", float(F(1, 10) - m))
print("\n".join(out))
