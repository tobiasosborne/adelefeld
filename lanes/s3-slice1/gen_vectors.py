#!/usr/bin/env python3
"""Writes tests/ref/vectors/s3-slice1/recon.jsonl from recon_partial of proto/solvers_checks.py.

Run from the repository root: python3 lanes/s3-slice1/gen_vectors.py
Every line: {"m","c","A","B","status","n","d","why","cert"}; status is the name recon_partial returns
(OK or NO_SOLUTION; every case is inside 2 A B < m or has an empty box); n, d are the solution for OK
(else null); why is "empty" (A < 0 or B < 1), "b" (|T| > B) or "c" (gcd(R, T) > 1) for NO_SOLUTION, "" for
OK; cert is [R', T', R, T] or null. Integers are JSON integers (the reader keeps them as text)."""
import json
import os
import random
import sys
from collections import Counter
from math import gcd

sys.path.insert(0, "proto")
import solvers_checks as S  # noqa: E402

random.seed(20260929)
lines = []
seen = set()


def add(m, c, A, B):
    if m < 1:
        return
    if not (A < 0 or B < 1 or 2 * A * B < m):
        return
    key = (m, c, A, B)
    if key in seen:
        return
    seen.add(key)
    st, sols, cert = S.recon_partial(m, c, A, B, 0)
    assert st in (S.OK, S.NO_SOLUTION), (key, st)
    why = ""
    n = d = None
    if st == S.OK:
        assert len(sols) == 1
        n, d = sols[0]
        assert S.is_solution(m, c, A, B, n, d)
    else:
        if A < 0 or B < 1:
            why = "empty"
        else:
            why = "b" if abs(cert[3]) > B else "c"
    lines.append({"m": m, "c": c, "A": A, "B": B, "status": st, "n": n, "d": d, "why": why,
                  "cert": list(cert) if cert else None})


# the cases of check_s3_edge (proto/solvers_checks.py:605 to 613, copied: the list is local to that
# function) that lie in the range or have an empty box
edge = [(1, 0, 0, 1), (1, 7, 0, 5), (1, 0, 1, 1), (2, 1, 1, 1), (2, 1, 0, 9), (2, 0, 0, 9), (2, 1, 1, 0),
        (5, 1, -1, 1), (6, 5, 1, 5), (6, 5, 1, 4), (6, -1, 1, 4), (6, 11, 1, 4), (7, 0, 0, 3),
        (7, 3, 0, 3), (12, 6, 5, 50), (101, 34, 1, 3)]
for e in edge:
    add(*e)
add(12, 6, 1, 5)                      # gcd(R, T) = 2: the pair is (6, 1, 0, -2), no solution (Remark 1)
add(12, 18, 1, 5)
add(12, -6, 1, 5)

# random fractions n/d of 20, 64, 300 and 2000 bits; m just above 2 A B and far above it
for bits in (20, 64, 300, 2000):
    for kind in ("just", "far"):
        made = 0
        while made < (10 if bits == 2000 else 120):
            A = random.getrandbits(bits) + 1
            B = random.getrandbits(bits) + 1
            if kind == "just":
                m = 2 * A * B + 1 + random.getrandbits(3)
            else:
                m = 2 * A * B + 1 + random.getrandbits(bits)
            d = random.randrange(1, B + 1)
            nn = random.randrange(-A, A + 1)
            g = gcd(nn, d)
            nn, d = nn // g, d // g
            if gcd(d, m) != 1:
                continue
            c = nn * pow(d, -1, m) % m + m * random.randrange(-2, 3)
            add(m, c, A, B)
            made += 1
            # the same problem with the bounds moved: a random c (usually no solution), and B lowered
            add(m, random.getrandbits(bits + 3), A, B)
            if d > 1:
                add(m, c, A, d - 1)
# gcd(R, T) > 1 cases: m = g m', c = g c', small bounds
for _ in range(400):
    g = random.choice((2, 3, 4, 6, 10, 30))
    m = g * random.randrange(1, 60)
    c = random.randrange(-2 * m, 2 * m)
    A = random.randrange(0, 8)
    B = random.randrange(1, 12)
    add(m, c, A, B)
for bits in (64, 300):
    for _ in range(40):
        g = random.getrandbits(bits) | 1
        m = 6 * g * (random.getrandbits(bits) + 1)
        c = g * random.getrandbits(bits)
        A = random.getrandbits(bits // 2) + 1
        B = random.getrandbits(bits // 2) + 1
        add(m, c, A, B)

os.makedirs("tests/ref/vectors/s3-slice1", exist_ok=True)
with open("tests/ref/vectors/s3-slice1/recon.jsonl", "w") as f:
    for r in lines:
        f.write(json.dumps(r, separators=(",", ":")) + "\n")
print(len(lines), Counter((r["status"], r["why"]) for r in lines))
