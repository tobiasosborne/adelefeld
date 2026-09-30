#!/usr/bin/env python3
"""lanes/f-slice6/mk_thousand.py: writes tests/driver/f-places-1000.cmd and .out (run from the repository root).

The expected lines follow from one rule and no output of the program: the projection of the rational 1/3 to a prime is
the exact rational 1/3, printed "<p>: 1/3"; the components are in the canonical order (the real place first, the
primes increasing, whatever the order in the list); the real component is 1/3 at 64 bits printed with 6 digits,
"0.333333 +/- 3.4e-7" (lanes/f-slice6/expected.py, proto/text_grammar.py print_real); a repeated place or a composite is
DOMAIN, a syntax fault is PARSE.
"""


def primes(n):
    ps = []
    k = 2
    while len(ps) < n:
        if all(k % q for q in ps if q * q <= k):
            ps.append(k)
        k += 1
    return ps


ps = primes(999)
toks = ["real"] + [str(p) for p in ps]
head = """# f-places-1000: 1000 places at once (the real place and the first 999 primes, the largest 7907).  The projection of
# the rational 1/3 to each prime is the exact rational 1/3, so every component is "<p>: 1/3", in the canonical order
# (the real place first, the primes increasing, whatever the order of the list), and the real component is 1/3 at 64
# bits printed with 6 digits: 0.333333 +/- 3.4e-7 (conventions 9.5; lanes/f-slice6/expected.py).  The lines are
# written by lanes/f-slice6/mk_thousand.py from that rule alone.  A repeated place in a list of 1000 places is
# DOMAIN, a composite in it is DOMAIN, a syntax fault in it is PARSE (which wins over the DOMAIN of the composite
# only where both occur; here each fault is alone).
#!exit 1
digits 6"""
cmd = [head,
       "project 1/3 with " + " ".join(toks),
       "project 1/3 with " + " ".join(reversed(toks)),
       "project 1/3 with " + " ".join(toks + ["7907"]),
       "project 1/3 with " + " ".join(toks[:500] + ["1000"] + toks[500:]),
       "project 1/3 with " + " ".join(toks[:500] + ["x1"] + toks[500:]),
       "project 1/3 with " + " ".join(toks + ["real"])]
open("tests/driver/f-places-1000.cmd", "w").write("\n".join(cmd) + "\n")
line = "real: 0.333333 +/- 3.4e-7; " + "; ".join("%d: 1/3" % p for p in ps)
out = [line, line, "error: DOMAIN", "error: DOMAIN", "error: PARSE", "error: DOMAIN"]
open("tests/driver/f-places-1000.out", "w").write("\n".join(out) + "\n")
