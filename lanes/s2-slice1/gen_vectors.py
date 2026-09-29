#!/usr/bin/env python3
"""Writes tests/ref/vectors/s2-slice1/seed.jsonl from seed_root of proto/solvers_checks.py (line 1721).

Run from the repository root: python3 lanes/s2-slice1/gen_vectors.py
Every line: {"f", "p", "a", "prec", "status", "g", "reduced", "cert", "what"}. f and g are lists of integer
coefficients, f[i] the coefficient of X^i; p is a prime below 2^64 (an adf_place_t holds no other); a is any
integer; prec is prec_p; status is OK, NOT_DETERMINED or DOMAIN. For OK: g is the normalised polynomial
(normalise_g, line 1701), reduced is 1 iff deg gcd(f, f') > 0 for nonconstant f (then deg g < deg f), and cert
is [a', K, s]; otherwise g, reduced and cert are null. Every OK line is checked here with root_cert_ok and
padic_verify_entries of the reference before it is written, and a' = a modulo p^(s+1). The certificate is
determined: a' is the root modulo p^K (solvers.md P3.12(2)), so the C function must reproduce it. Integers are
JSON integers (the C reader keeps them as text)."""
import json
import os
import random
import sys
from collections import Counter

sys.path.insert(0, "proto")
import solvers_checks as S  # noqa: E402

random.seed(20260929)
lines = []


def add(f, p, a, prec, what):
    st, L = S.seed_root(f, p, a, prec)
    rec = {"f": list(f), "p": p, "a": a, "prec": prec, "status": st, "g": None, "reduced": None, "cert": None,
           "what": what}
    if st == S.OK:
        (a1, K, s), = L["certs"]
        assert S.root_cert_ok(L["g"], p, a1, K, s)
        assert S.padic_verify_entries(f, L)
        assert (a1 - a) % p ** (s + 1) == 0 and K == max(prec, s + 1)
        ft = S.ptrim(f)
        rec["g"] = L["g"]
        rec["reduced"] = 1 if len(ft) > 1 and len(L["g"]) < len(ft) else 0
        rec["cert"] = [a1, K, s]
    else:
        assert st in (S.NOT_DETERMINED, S.DOMAIN)
    lines.append(rec)
    return st


# 1. the 6552 seeds of check_s2_seed (proto/solvers_checks.py:2033 to 2039): the PADIC_CASES and five more, at
#    2, 3, 5, the seeds range(-3, 2 p^2), p^5 + 1, -p^4 - 1, and the precisions 1 and 4
polys = [f for _, f, _, _, _ in S.PADIC_CASES] + [[0, 27], [0, 9, 0, 3], [-9, 6, -1], [4, -4, 1], [0, 0, 1]]
for f in polys:
    f = S.ptrim(f)
    for p in (2, 3, 5):
        for a in list(range(-3, 2 * p ** 2)) + [p ** 5 + 1, -p ** 4 - 1]:
            for prec in (1, 4):
                add(f, p, a, prec, "check_s2_seed")

# 2. large seeds (thousands of bits, both signs) in the class of a root, and large precisions
for f, p, a0 in (([-2, 0, 1], 7, 3), ([-17, 0, 1], 2, 1), ([1, 2, 2, -7, 1], 3, 2), ([-8, -2, -1, 1], 2, 0),
                 ([-10, 0, 0, 1], 3, 4)):
    for bits in (64, 300, 3000):
        for sign in (1, -1):
            t = sign * (random.getrandbits(bits) | 1)
            for prec in (1, 60, 300):
                add(f, p, a0 + p ** 12 * t, prec, "large seed")

# 3. primes of one word: 2^31 - 1, 2^61 - 1, 2^64 - 59 (the largest prime below 2^64); planted roots
for p in (2 ** 31 - 1, 2 ** 61 - 1, 18446744073709551557):
    for _ in range(12):
        r = [random.randrange(-10 ** 30, 10 ** 30) for _ in range(random.randint(1, 3))]
        if random.random() < 0.5 and len(r) >= 2:
            r[1] = r[0] + p ** random.randint(1, 3)             # two roots close at p: s > 0
        f = S.pfrom_roots(r, lead=random.choice((1, 2, p)))
        if random.random() < 0.3:
            f = S.pmul(f, [1, 0, 1])
        for prec in (1, 7, 50):
            add(f, p, r[0] + p ** 5 * random.randrange(-10 ** 20, 10 ** 20), prec, "word prime, planted")
            add(f, p, r[0] + random.randrange(1, p), prec, "word prime, off the root")

# 4. random polynomials and seeds at small primes
for _ in range(400):
    p = random.choice((2, 3, 5, 7, 11, 13))
    f = S.ptrim([random.randrange(-20, 21) for _ in range(random.randint(1, 6))])
    if not f:
        continue
    add(f, p, random.randrange(-p ** 6, p ** 6), random.choice((1, 2, 3, 8, 25)), "random")

# 5. the statuses DOMAIN: f = 0, prec < 1
add([], 3, 0, 2, "DOMAIN f = 0")
add([0, 0], 5, 1, 2, "DOMAIN f = 0 with zero coefficients")
add([0, 27], 3, 0, 0, "DOMAIN prec 0")
add([-2, 0, 1], 7, 3, -4, "DOMAIN prec < 0")

# the fixed case of the review (proto/solvers_checks.py:2065): 27 X at 3, seed 0, prec 2: g = X, (0, 2, 0)
st = add([0, 27], 3, 0, 2, "27 X at 3")
assert st == S.OK and lines[-1]["cert"] == [0, 2, 0] and lines[-1]["g"] == [0, 1]

out = "tests/ref/vectors/s2-slice1/seed.jsonl"
os.makedirs(os.path.dirname(out), exist_ok=True)
with open(out, "w") as fh:
    for rec in lines:
        fh.write(json.dumps(rec, separators=(", ", ": ")) + "\n")
print(f"{out}: {len(lines)} lines;", dict(Counter(r["status"] for r in lines)),
      "; p above 32 bits:", sum(1 for r in lines if r["p"] >= 2 ** 32))
