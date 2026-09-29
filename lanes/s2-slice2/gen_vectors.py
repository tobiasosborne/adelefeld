#!/usr/bin/env python3
"""Writes tests/ref/vectors/s2-slice2/padic.jsonl from rootlist_padic of proto/solvers_checks.py (line 1707), which
runs padic_roots (Algorithm P, line 1649) on the normalised polynomial normalise_g (line 1701).

Run from the repository root: python3 lanes/s2-slice2/gen_vectors.py

Every line: {"f", "p", "prec", "depth", "status", "g", "reduced", "certs", "unres", "complete", "expect", "d0",
"what"}. f and g are lists of integer coefficients, f[i] the coefficient of X^i. status is the status of the strict
function adf_roots_padic: OK when the list is complete, NOT_DETERMINED when a class is unresolved, DOMAIN for
f = 0, prec < 1 or depth < 0 (then g, reduced, certs, unres, complete are null). certs is the sorted list of
[a, K, s], unres the sorted list of [a, e]; they are the list of adf_roots_padic_partial (and of the strict
function when complete = 1). reduced is 1 iff deg gcd(f, f') > 0 for nonconstant f. expect is the number of roots
in Z_p of PADIC_CASES (line 1811) or null. d0 is, when the list at depth 12 is complete, the least depth that
completes the run (then padic_verify_complete, line 1776, accepts the list at d0 and refuses it at d0 - 1);
otherwise null. The script imports the reference; it copies nothing. Every OK list is checked here with
padic_verify_entries (line 1754) and, when complete, padic_verify_complete. The lists are determined (every
certificate is (alpha mod p^K, K, s), solvers.md P3.5 and P3.2(1)), so the C functions must reproduce them."""
import json
import os
import random
import sys

sys.path.insert(0, "proto")
import solvers_checks as S  # noqa: E402

random.seed(20260929)
lines = []


def least_depth(f, p):
    for d in range(0, 13):
        st, L = S.rootlist_padic(f, p, 1, d)
        if L["complete"]:
            return d
    return None


def add(f, p, prec, depth, what, expect=None):
    st, L = S.rootlist_padic(f, p, prec, depth)
    rec = {"f": list(f), "p": p, "prec": prec, "depth": depth, "status": st, "g": None, "reduced": None,
           "certs": None, "unres": None, "complete": None, "expect": expect, "d0": None, "what": what}
    if st == S.DOMAIN:
        lines.append(rec)
        return st
    assert st in (S.OK, S.NOT_DETERMINED)
    assert S.padic_verify_entries(f, L)
    assert (st == S.OK) == (L["complete"] == 1)
    ft = S.ptrim(f)
    rec["g"] = L["g"]
    rec["reduced"] = 1 if len(ft) > 1 and len(L["g"]) < len(ft) else 0
    rec["certs"] = [list(c) for c in sorted(L["certs"])]
    rec["unres"] = [list(u) for u in sorted(L["unres"])]
    rec["complete"] = L["complete"]
    for a, K, s in L["certs"]:
        assert K == max(prec, s + 1)
    st12, L12 = S.rootlist_padic(f, p, prec, 12)
    if L12["complete"]:
        d0 = least_depth(f, p)
        assert S.padic_verify_complete(f, L12, d0)
        assert d0 == 0 or not S.padic_verify_complete(f, L12, d0 - 1)
        rec["d0"] = d0
        if L["complete"]:
            assert S.padic_verify_complete(f, L, 12)
    lines.append(rec)
    return st


# 1. the 31 named cases of PADIC_CASES with the depth limits 0, 1, 3, 8 of check_s2_descent (line 1927), at the
#    precisions 1 and 3; the number of roots is expected at depth 8 for every case (under S-D13 the multiple
#    roots are simple roots of g, and the reference gives complete lists for them)
for name, f, p, expect, simple in S.PADIC_CASES:
    for depth in (0, 1, 3, 8):
        for prec in (1, 3):
            add(f, p, prec, depth, "PADIC_CASES: " + name, expect)

# 2. note 1 of docs/api-s.md section 4: x^2 + 1 at 2 (solvers 3.11(2))
for depth in (0, 1, 2, 5):
    add([1, 0, 1], 2, 4, depth, "x^2+1 at 2, api-s note 1", 0)

# 3. the zero polynomial, constants, degree 1, and the DOMAIN of prec and depth
for p in (2, 3, 5, 7):
    add([], p, 2, 3, "zero polynomial")
    add([0, 0, 0], p, 2, 3, "zero polynomial with trailing zeros")
    for c in ([1], [-1], [12], [p ** 3], [-p]):
        add(c, p, 2, 3, "constant", 0)
    for f in ([3, 1], [0, 27], [-6, 4], [p, p * p], [1, p], [-5, 7], [0, 1]):
        add(f, p, 1, 0, "degree 1")
        add(f, p, 5, 2, "degree 1")
    add([-2, 0, 1], p, 0, 3, "prec 0")
    add([-2, 0, 1], p, -3, 3, "prec negative")
    add([-2, 0, 1], p, 2, -1, "depth negative")

# 4. random products with planted integer roots, repeated roots, roots close at p, factors without a root, and
#    random polynomials, at the primes up to 101
primes = [q for q in range(2, 102) if all(q % r for r in range(2, q))]
for _ in range(300):
    p = random.choice(primes[:6] if random.random() < 0.6 else primes)
    if random.random() < 0.3:
        f = S.ptrim([random.randint(-12, 12) for _ in range(random.randint(1, 6))]) or [1]
    else:
        roots = [random.randint(-60, 60) for _ in range(random.randint(1, 4))]
        if len(roots) >= 2 and random.random() < 0.5:
            roots[1] = roots[0] + random.choice((1, -1, 3)) * p ** random.randint(1, 4)
        if len(roots) >= 3 and random.random() < 0.3:
            roots[2] = roots[0]
        f = S.pfrom_roots(roots, lead=random.choice((1, 1, 2, 3, p)))
        if random.random() < 0.3:
            f = S.pmul(f, random.choice(([1, 0, 1], [1, 1, 1], [2, 0, 1], [-3, 0, 1])))
    add(f, p, random.randint(1, 8), random.choice((0, 1, 2, 3, 5, 8, 12)), "random")

# 5. two primes near the bound of the slice (2^20 = 1048576): 65537 and 1048573, planted roots
for p in (65537, 1048573):
    add(S.pfrom_roots([5, 5 + p, -11]), p, 3, 4, "large prime, planted", 3)

out = "tests/ref/vectors/s2-slice2/padic.jsonl"
os.makedirs(os.path.dirname(out), exist_ok=True)
with open(out, "w") as fh:
    for rec in lines:
        fh.write(json.dumps(rec, separators=(",", ":")) + "\n")
counts = {}
for rec in lines:
    counts[rec["status"]] = counts.get(rec["status"], 0) + 1
n_spos = sum(1 for r in lines if r["certs"] and any(c[2] for c in r["certs"]))
print(f"{out}: {len(lines)} lines; statuses {counts}; with unresolved classes "
      f"{sum(1 for r in lines if r['unres'])}; with s > 0 {n_spos}; "
      f"with d0 {sum(1 for r in lines if r['d0'] is not None)}")
