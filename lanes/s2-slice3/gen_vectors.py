#!/usr/bin/env python3
"""Writes tests/ref/vectors/s2-slice3/real.jsonl from real_roots_ref of proto/solvers_checks.py (line 2555:
Algorithm RR of solvers.md 3.10 in the reference: Sturm count, isolation by bisection, refinement), which works on
the normalised polynomial squarefree_part (line 2377).

Run from the repository root: python3 lanes/s2-slice3/gen_vectors.py

Every line: {"f", "prec", "status", "g", "reduced", "count", "enc", "bits", "what"}. f and g are lists of integer
coefficients, f[i] the coefficient of X^i. prec is the precision to give to adf_roots_real. status is the status the
C function must return: OK for every f != 0, DOMAIN for f = 0 (then g, reduced, count, enc are null). count is the
number of distinct real roots (the Sturm count of the reference, line 2414). enc is the list of the rational
enclosures of the roots in increasing order, each [lo_num, lo_den, hi_num, hi_den]: the reference balls of
real_roots_ref at "bits" bits (bits = 100, more than any prec of the file except 200 and 2000, so the enclosures
are much narrower than the gaps between the roots). Each holds exactly one root; the C balls must overlap them one
to one: C ball i meets enclosure j exactly when i = j. reduced is 1 iff deg gcd(f, f') > 0 for nonconstant f.

The script imports the reference; it copies nothing. It replaces S.sturm_chain by a memoised wrapper of the same
function (the reference recomputes the chain for every count; the result is the same list, only faster). Every list
is checked here with real_verify_complete (line 2524) and real_cert_ok (line 2431) before it is written."""
import json
import os
import random
import sys
from fractions import Fraction as F

sys.path.insert(0, "proto")
import solvers_checks as S  # noqa: E402

_chain = S.sturm_chain
_memo = {}


def sturm_chain_memo(g):
    key = tuple(F(c) for c in g)
    if key not in _memo:
        _memo[key] = _chain(g)
    return _memo[key]


S.sturm_chain = sturm_chain_memo

random.seed(20260929)
BITS = 100
lines = []


def add(f, prec, what):
    ft = S.ptrim(list(f))
    rec = {"f": list(f), "prec": prec, "status": "OK", "g": None, "reduced": None, "count": None, "enc": None,
           "bits": BITS, "what": what}
    if not ft:
        rec["status"] = "DOMAIN"
        lines.append(rec)
        return
    st, n, balls = S.real_roots_ref(ft, BITS)
    assert st == S.OK, (what, f)
    g = S.squarefree_part(ft)
    assert S.real_verify_complete(ft, balls, n)
    assert S.real_cert_ok(g, n, balls)
    rec["g"] = g
    rec["reduced"] = 1 if len(ft) > 1 and len(g) < len(ft) else 0
    rec["count"] = n
    rec["enc"] = [[lo.numerator, lo.denominator, hi.numerator, hi.denominator] for lo, hi in balls]
    lines.append(rec)


def from_roots(roots, lead=1):
    """lead * product of (den X - num) over the rational roots, with repetition as given."""
    f = [lead]
    for r in roots:
        r = F(r)
        f = S.pmul(f, [-r.numerator, r.denominator])
    return f


# 1. the 18 cases of REAL_CASES (solvers_checks.py line 2613), at the precisions 2, 20, 53
for name, f, expect in S.REAL_CASES:
    for prec in (2, 20, 53):
        add(f, prec, "REAL_CASES: " + name)

# 2. planted roots of check_s2_real_planted (line 2716): the pool of that check, products of 1 to 4 roots with
#    multiplicities 1 to 3 and a factor without real roots now and then; the precisions 2 and 20
pool = [F(-3), F(-1, 2), F(0), F(1, 3), F(1), F(1) + F(1, 2 ** 40), F(10) ** 30, -F(1, 10 ** 20), F(7, 5), F(-7, 5),
        F(1, 2 ** 30)]
for _ in range(40):
    k = random.randint(1, 4)
    roots = random.sample(pool, k)
    f = [random.choice((1, 2, -3, 5))]
    for r in roots:
        for _ in range(random.choice((1, 1, 2, 3))):
            f = S.pmul(f, [-r.numerator, r.denominator])
    if random.random() < 0.4:
        f = S.pmul(f, random.choice(([1, 0, 1], [1, 1, 1], [2, -1, 3])))
    add(f, random.choice((2, 20)), "planted")

# 3. the regressions of check_s2_real_planted: X - 10^400 and (X - 10^400)(X - 10^400 - 1)
add([-10 ** 400, 1], 6, "X - 10^400")
add(S.pmul([-10 ** 400, 1], [-(10 ** 400) - 1, 1]), 6, "(X - 10^400)(X - 10^400 - 1)")

# 4. the zero polynomial and constants
add([], 53, "zero polynomial")
add([0, 0], 53, "zero polynomial with trailing zeros")
for c in (1, -1, 7, -12, 10 ** 30):
    add([c], 53, "constant")

# 5. random polynomials of degree 1 to 8 with small and with large coefficients (up to 200 bits), and products
#    with planted rational roots
for _ in range(60):
    if random.random() < 0.5:
        big = random.random() < 0.3
        f = [random.randrange(-2 ** 200, 2 ** 200) if big else random.randint(-20, 20)
             for _ in range(random.randint(2, 9))]
        f = S.ptrim(f)
        if len(f) < 2:
            continue
    else:
        roots = [F(random.randint(-50, 50), random.choice((1, 1, 2, 3, 7, 1024)))
                 for _ in range(random.randint(1, 5))]
        if len(roots) >= 2 and random.random() < 0.4:
            roots[1] = roots[0]
        f = from_roots(roots, lead=random.choice((1, -1, 2, 5)))
        if random.random() < 0.3:
            f = S.pmul(f, [1, 0, 1])
    add(f, random.choice((2, 10, 53)), "random")

out = "tests/ref/vectors/s2-slice3/real.jsonl"
os.makedirs(os.path.dirname(out), exist_ok=True)
with open(out, "w") as fh:
    for rec in lines:
        fh.write(json.dumps(rec, separators=(",", ":")) + "\n")
counts = {}
for rec in lines:
    counts[rec["status"]] = counts.get(rec["status"], 0) + 1
print(f"{out}: {len(lines)} lines; statuses {counts}; roots in all "
      f"{sum(r['count'] or 0 for r in lines)}; reduced {sum(1 for r in lines if r['reduced'])}; "
      f"no real root {sum(1 for r in lines if r['count'] == 0)}")
