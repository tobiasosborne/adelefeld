#!/usr/bin/env python3
"""reduce_oracle.py: checks reduce_run output with Python integers.
RED a r_1..r_k: r_i == a mod q_i (Python % gives the residue in [0, q_i)).
REC r_1..r_k A: 0 <= A < K = prod q_i and A % q_i == r_i."""
import sys
from math import prod

sys.set_int_max_str_digits(0)
q, bad, n = None, 0, 0
for line in open(sys.argv[1]):
    t = line.split()
    if t[0] == "CTX":
        q = [int(x) for x in t[1].split(",")]
        K = prod(q)
    elif t[0] == "RED":
        a, r = int(t[1]), [int(x) for x in t[2].split(",")]
        n += 1
        if r != [a % m for m in q]:
            bad += 1
            print("RED wrong: k=%d a=%s" % (len(q), t[1][:40]))
    elif t[0] == "REC":
        r, A = [int(x) for x in t[1].split(",")], int(t[2])
        n += 1
        if not (0 <= A < K and all(A % m == x for m, x in zip(q, r))):
            bad += 1
            print("REC wrong: k=%d" % len(q))
print("calls checked:", n, "wrong:", bad)
sys.exit(1 if bad else 0)
