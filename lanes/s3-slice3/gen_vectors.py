#!/usr/bin/env python3
"""Writes tests/ref/vectors/s3-slice3/recon_first.jsonl from recon_first of proto/solvers_checks.py
(imported, not copied), and tests/ref/vectors/s3-slice3/sets.jsonl for adf_resid_set_rat,
adf_resid_contains_rat and adf_resid_set_fball_forget.

Run from the repository root: python3 lanes/s3-slice3/gen_vectors.py

recon_first.jsonl, one line per call of recon_first(m, c, A, B, limit):
    {"m","c","A","B","limit","status","n","d","count","cert"}
status is the name of the status the reference returns (OK, NO_SOLUTION, NOT_DETERMINED); n, d are the
solution it returns for OK and null otherwise; count is its count; cert is [R', T', R, T] or null (empty
box, A >= m). Integers are JSON integers; the reader keeps them as text, so the large ones are exact.

Groups: (1) the whole grid m <= 6, every c in [0, m) and c = -1, m + 1 (the set depends on c only modulo m,
but the caller may pass anything), every A in 0..2 m, the values of B of check_s3_complete, the limits
-1, 0, 1, 2, 5, 1000000; (2) a random sample of the grid m <= 24 of the test; (3) large operands of 64,
300 and 1000 bits, where the search is decided by 2 A B against m, by |T| against B, and by X against
the limit, so that the first point and the count are read on values above a word.

sets.jsonl, one line per claim:
    {"kind": "set_rat", "n", "d", "m", "status", "c", "inside"}   the claim of adf_resid_set_rat and
        adf_resid_contains_rat: the status of set_rat(n/d, m), the c it gives, and, for every c in
        [0, m), whether contains_rat says that n/d is in P(m, c);
    {"kind": "forget", "A", "H", "d", "status", "c", "k", "n", "dn", "inside"}  the claim of
        adf_resid_set_fball_forget for the canonical triple (A, H, d) (the reader builds the ball with
        adf_fball_set_fmpz3, which canonicalises, so A is taken in [0, H) and gcd(A, H, d) = 1) and the
        membership of the rational (A + H k)/d, reduced, in the result.
"""
import json
import os
import random
import sys
from collections import Counter
from fractions import Fraction
from math import gcd

sys.path.insert(0, "proto")
import solvers_checks as S  # noqa: E402

random.seed(20261001)
NAMES = {S.OK: "OK", S.NO_SOLUTION: "NO_SOLUTION", S.NOT_DETERMINED: "NOT_DETERMINED"}
out_dir = "tests/ref/vectors/s3-slice3"
os.makedirs(out_dir, exist_ok=True)

# ---------------------------------------------------------------- recon_first.jsonl

lines = []
seen = set()


def add_first(m, c, A, B, limit):
    key = (m, c, A, B, limit)
    if m < 1 or key in seen:
        return
    seen.add(key)
    st, sol, count, cert = S.recon_first(m, c, A, B, limit)
    assert st in NAMES, (key, st)
    n = d = None
    if st == S.OK:
        n, d = sol
    lines.append({"m": m, "c": c, "A": A, "B": B, "limit": limit, "status": NAMES[st], "n": n, "d": d,
                  "count": count, "cert": list(cert) if cert else None})


def bs(m):
    return sorted(set(b for b in (1, 2, 3, 5, m - 1, m, m + 1, 2 * m) if b >= 1))


LIMITS = (-1, 0, 1, 2, 5, 1000000)
# group 1: the whole small grid
for m in range(1, 7):
    for c in list(range(m)) + [-1, m + 1]:
        for A in range(0, 2 * m + 1):
            for B in bs(m):
                for lim in LIMITS:
                    add_first(m, c, A, B, lim)
n1 = len(lines)
# group 2: a random sample of the grid of the test (m <= 24, A up to 2 m, B in {1, 2, 3, 5, m, 2 m})
for _ in range(4000):
    m = random.randrange(1, 25)
    c = random.randrange(0, m)
    A = random.randrange(0, 2 * m + 1)
    B = random.choice(sorted(set((1, 2, 3, 5, m, 2 * m))))
    add_first(m, c, A, B, random.choice(LIMITS))
n2 = len(lines) - n1

# group 3: large operands
for bits in (64, 300, 1000):
    for _ in range({64: 6, 300: 4, 1000: 2}[bits]):
        A = random.getrandbits(bits) + 1
        B = random.getrandbits(bits) + 1
        for delta in (-1, 0, 1, 5):          # 2 A B = m + delta
            m = 2 * A * B - delta
            if m < 2:
                continue
            for c in (1, 0, m - 1, random.randrange(m)):
                for lim in (0, 3, 1000000):
                    add_first(m, c, A, B, lim)
        m = A * B + random.getrandbits(4)    # A B just below m
        for c in (1, random.randrange(m)):
            for lim in (0, 3, 1000000):
                add_first(m, c, A, B, lim)
        # c = 1 gives T = 1, so X = B: a limit of 0 or 1 does not decide, a large one finds the first
        m = random.getrandbits(bits) | (1 << (bits - 1))
        B = m // random.choice((2, 3, 5)) + random.getrandbits(3)
        A = random.randrange(1, 40)
        for lim in (0, 1, 2, 1000000):
            add_first(m, 1, A, B, lim)
n3 = len(lines) - n1 - n2

with open(os.path.join(out_dir, "recon_first.jsonl"), "w") as f:
    for r in lines:
        f.write(json.dumps(r, separators=(",", ":")) + "\n")
print("recon_first.jsonl:", len(lines), "lines; groups", n1, n2, n3)
print("  statuses:", Counter(r["status"] for r in lines))
print("  counts:", Counter(r["count"] for r in lines))
print("  status/count:", Counter((r["status"], r["count"]) for r in lines))
print("  largest m:", max(r["m"].bit_length() for r in lines), "bits")

# ---------------------------------------------------------------- sets.jsonl

slines = []


def add_set_rat(n, dd, m):
    st = S.OK if gcd(dd, m) == 1 else S.DOMAIN
    c = None
    inside = []
    if st == S.OK:
        c = (n * pow(dd, -1, m)) % m if m > 1 else 0
    for cc in range(m):
        inside.append(1 if (gcd(dd, m) == 1 and (cc * dd - n) % m == 0) else 0)
    slines.append({"kind": "set_rat", "n": n, "d": dd, "m": m, "status": NAMES[st] if st in NAMES
                   else "DOMAIN", "c": c, "inside": inside})


for m in list(range(1, 26)) + [30, 36, 42, 60, 210]:
    for dd in range(1, 13):
        for n in range(-12, 13):
            if gcd(n, dd) == 1:
                add_set_rat(n, dd, m)
n_rat = len(slines)


def add_forget(A, H, d, k):
    """The claim for the canonical triple (A, H, d) with 0 <= A < H, and the rational (A + H k)/d."""
    g = gcd(gcd(A, H), d)
    a, h, dd = A // g, H // g, d // g
    if h == 0 or gcd(dd, h) != 1:
        st = "DOMAIN"
    else:
        st = "OK"
    c = None
    if st == "OK":
        c = (a * pow(dd, -1, h)) % h if h > 1 else 0
    q = Fraction(a + h * k, dd)
    inside = None
    if st == "OK":
        inside = 1 if (gcd(q.denominator, h) == 1 and (q.numerator - c * q.denominator) % h == 0) else 0
    slines.append({"kind": "forget", "A": a, "H": h, "d": dd, "status": st, "c": c, "k": k,
                   "n": q.numerator, "dn": q.denominator, "inside": inside})


for H in range(1, 13):
    for d in range(1, 9):
        for A in range(0, H):
            for k in (-20, -1, 0, 1, 20):
                add_forget(A, H, d, k)
n_forget = len(slines) - n_rat

with open(os.path.join(out_dir, "sets.jsonl"), "w") as f:
    for r in slines:
        f.write(json.dumps(r, separators=(",", ":")) + "\n")
print("sets.jsonl:", len(slines), "lines; set_rat", n_rat, "forget", n_forget)
print("  set_rat statuses:", Counter(r["status"] for r in slines[:n_rat]))
print("  forget statuses:", Counter(r["status"] for r in slines[n_rat:]))
