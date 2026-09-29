#!/usr/bin/env python3
"""Writes tests/ref/vectors/s3-slice2/recon.jsonl from recon_partial of proto/solvers_checks.py
(imported, not copied).

Run from the repository root: python3 lanes/s3-slice2/gen_vectors.py
Every line: {"m","c","A","B","limit","status","n","d","cert","bigX"}. status is the name of the status
recon_partial returns (OK, NO_SOLUTION, NOT_UNIQUE, NOT_DETERMINED); n, d are the solution for OK (else
null); cert is [R', T', R, T] or null (empty box, A >= m); bigX is 1 if A < m, |T| <= B and
floor(B/|T|) >= 2^64 (the round counter does not fit a word), else 0. Integers are JSON integers (the
reader keeps them as text).

Three groups: (1) the grid m <= 5, c in [0, m) and c = -1, m + 1, every A in 0..2m, the values of B
of check_s3_complete, limits -1, 0, 1, 2, 5, 1000000; (2) a random sample of the same grid for
9 <= m <= 24; (3) large operands of 64, 300 and 2000 bits with 2 A B just below, at and just above m and
with A B near m, small limits 0 and 3 (and WORD_MAX where the reference decides at limit 200 already, so
that it ends at once), with B / |T| above 2^64 in a counted part."""
import json
import os
import random
import sys
from collections import Counter
from math import gcd

sys.path.insert(0, "proto")
import solvers_checks as S  # noqa: E402

WORD_MAX = (1 << 63) - 1
random.seed(20260930)
lines = []
seen = set()
NAMES = {S.OK: "OK", S.NO_SOLUTION: "NO_SOLUTION", S.NOT_UNIQUE: "NOT_UNIQUE", S.NOT_DETERMINED: "NOT_DETERMINED"}


def add(m, c, A, B, limit):
    key = (m, c, A, B, limit)
    if m < 1 or key in seen:
        return
    seen.add(key)
    st, sols, cert = S.recon_partial(m, c, A, B, limit)
    assert st in NAMES, (key, st)
    n = d = None
    if st == S.OK:
        assert len(sols) == 1
        n, d = sols[0]
    cert_l = list(cert) if cert else None
    bigX = 0
    if cert and abs(cert[3]) <= B and B // abs(cert[3]) >= (1 << 64):
        bigX = 1
    lines.append({"m": m, "c": c, "A": A, "B": B, "limit": limit, "status": NAMES[st], "n": n, "d": d,
                  "cert": cert_l, "bigX": bigX})


def bs(m):
    return sorted(set(b for b in (1, 2, 3, 5, m - 1, m, m + 1, 2 * m) if b >= 1))


LIMITS = (-1, 0, 1, 2, 5, 1000000)
# group 1
for m in range(1, 6):
    for c in list(range(m)) + [-1, m + 1]:
        for A in range(0, 2 * m + 1):
            for B in bs(m):
                for lim in LIMITS:
                    add(m, c, A, B, lim)
n1 = len(lines)
# group 2
for _ in range(3000):
    m = random.randrange(9, 25)
    c = random.randrange(-m, 2 * m)
    A = random.randrange(0, m if random.random() < 0.7 else 2 * m + 1)
    B = random.choice(bs(m))
    add(m, c, A, B, random.choice(LIMITS))
n2 = len(lines) - n1


# group 3
def planted(m, A, B):
    for _ in range(50):
        d = random.randrange(1, B + 1)
        nn = random.randrange(-A, A + 1)
        g = gcd(nn, d)
        nn, d = nn // g, d // g
        if gcd(d, m) == 1:
            return nn * pow(d, -1, m) % m + m * random.randrange(-1, 2)
    return None


def add_big(m, c, A, B):
    for lim in (0, 3):
        add(m, c, A, B, lim)
    st, _, _ = S.recon_partial(m, c, A, B, 200)
    if st != S.NOT_DETERMINED:
        add(m, c, A, B, WORD_MAX)


for bits in (64, 300, 2000):
    reps = {64: 8, 300: 5, 2000: 1}[bits]
    for _ in range(reps):
        # A B near m / 2: 2 A B = m + delta with delta = 1, 0, -1, -5
        A = random.getrandbits(bits) + 1
        B = random.getrandbits(bits) + 1
        for delta in (-1, 0, 1, 5):
            m = 2 * A * B - delta
            if m < 2:
                continue
            for c in (planted(m, A, B), random.randrange(m), 1, 0, m - 1):
                if c is not None:
                    add_big(m, c, A, B)
        # A B near m: m = A B + small
        m = A * B + random.getrandbits(4)
        for c in (planted(m, A, B), random.randrange(m)):
            if c is not None:
                add_big(m, c, A, B)
        # A a fraction of m, B huge: |T| small, B / |T| above 2^64 (c small or planted)
        m = random.getrandbits(bits) | (1 << (bits - 1))
        A = m // random.choice((2, 3, 5, 7))
        B = m * random.getrandbits(70) + random.getrandbits(70) + 1
        for c in (1, 2, random.randrange(m), planted(m, A, B)):
            if c is not None:
                add_big(m, c, A, B)
        # A small, B about m / (2 A): c = 1 gives T = 1, a cut search with one solution
        A = random.randrange(1, 40)
        m = random.getrandbits(bits) | (1 << (bits - 1))
        B = m // (2 * A) + random.getrandbits(3)
        for c in (1, random.randrange(m), planted(m, A, B)):
            if c is not None:
                add_big(m, c, A, B)
n3 = len(lines) - n1 - n2

os.makedirs("tests/ref/vectors/s3-slice2", exist_ok=True)
with open("tests/ref/vectors/s3-slice2/recon.jsonl", "w") as f:
    for r in lines:
        f.write(json.dumps(r, separators=(",", ":")) + "\n")
print(len(lines), "lines; groups", n1, n2, n3)
print(Counter(r["status"] for r in lines))
print("bigX lines:", sum(r["bigX"] for r in lines),
      "; WORD_MAX lines:", sum(r["limit"] == WORD_MAX for r in lines))
print("group 3 statuses:", Counter(r["status"] for r in lines[n1 + n2:]))
