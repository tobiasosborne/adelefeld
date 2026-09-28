#!/usr/bin/env python3
"""Vectors for adf_fball_add_cap, adf_fball_sub_cap, adf_fball_mul_cap, adf_fball_mul_rat_cap:
work package 1.7, lane m1-cap.

adf_fball_cap alone already has 300 rows of oracle data in tests/ref/vectors/policies.jsonl
(op "absolute_cap", tests/ref/gen_vectors.py function gen_policies); tests/test_cap_vectors.c
reads that file directly. No such file combines a tight operation with the cap, so this script
composes them in Python (tests/ref/adfref/fball.py add/sub/mul/scale, then
tests/ref/adfref/policies.py absolute_cap) and writes tests/ref/vectors/m1-cap/cap_ops.jsonl.
The composition happens only here, in Python; the C tests never read the Python reference, only
this file's JSON lines (tests/README.md).

Run from the repository root:  python3 lanes/m1-cap/gen_vectors.py
Deterministic: the seed is fixed, so two runs are byte-identical.
"""
import json
import os
import random
import sys
from fractions import Fraction

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
REF = os.path.join(ROOT, "tests", "ref")
sys.path.insert(0, REF)

from adfref import fball, policies                       # noqa: E402
from adfref.fball import Fball                            # noqa: E402

VECTORS = os.path.join(ROOT, "tests", "ref", "vectors", "m1-cap")

EDGE_BALLS = [
    (0, 0, 1),      # exact zero
    (5, 0, 1),      # exact integer
    (3, 0, 4),      # exact fraction
    (-3, 0, 4),     # exact negative fraction
    (0, 2, 1),      # zero centre, positive radius
    (0, 8, 4),      # shared factor
    (-3, 12, 1),    # negative centre
    (2, 3, 1),      # fractional radius
    (1, 3, 2),      # fractional centre and radius
    (0, 1, 1),      # Zhat
]

EDGE_RATIONALS = [
    Fraction(0), Fraction(1), Fraction(-1), Fraction(1, 2), Fraction(-1, 3),
    Fraction(5, 7), Fraction(12), Fraction(3, 2),
]

EDGE_CAPS = [Fraction(1), Fraction(2), Fraction(1, 3), Fraction(7, 2), Fraction(100)]


def ball(B):
    return {"A": B.A, "H": B.H, "d": B.d}


def rat(q):
    q = Fraction(q)
    return {"num": q.numerator, "den": q.denominator}


def random_ball(rng):
    A = rng.randint(-40, 40)
    H = rng.choice([0, 0, 1, 2, 3, 5, 6, 8, 12, 18, 24, 30, rng.randint(0, 40)])
    d = rng.choice([1, 1, 2, 3, 4, 6, rng.randint(1, 12)])
    return Fball(A, H, d)


def random_rat(rng):
    num = rng.randint(-30, 30)
    den = rng.choice([1, 1, 2, 3, 4, 5, 6, rng.randint(1, 12)])
    return Fraction(num, den)


def random_cap(rng):
    num = rng.randint(1, 40)
    den = rng.choice([1, 1, 2, 3, rng.randint(1, 8)])
    return Fraction(num, den)


def gen_binary_cap(rng, op, fn, n):
    """op in {add_cap, sub_cap, mul_cap}; fn is fball.add / fball.sub / fball.mul."""
    lines = []
    edge = [Fball(*t) for t in EDGE_BALLS]
    for x in edge:
        for y in edge:
            for C in EDGE_CAPS:
                tight = fn(x, y)
                result = policies.absolute_cap(tight, C)
                lines.append({"op": op, "a": ball(x), "b": ball(y), "C": rat(C),
                              "result": ball(result)})
    for _ in range(n):
        x, y, C = random_ball(rng), random_ball(rng), random_cap(rng)
        tight = fn(x, y)
        result = policies.absolute_cap(tight, C)
        lines.append({"op": op, "a": ball(x), "b": ball(y), "C": rat(C),
                      "result": ball(result)})
    return lines


def gen_mul_rat_cap(rng, n):
    lines = []
    edge = [Fball(*t) for t in EDGE_BALLS]
    for x in edge:
        for q in EDGE_RATIONALS:
            for C in EDGE_CAPS:
                tight = fball.scale(x, q)
                result = policies.absolute_cap(tight, C)
                lines.append({"op": "mul_rat_cap", "a": ball(x), "q": rat(q), "C": rat(C),
                              "result": ball(result)})
    for _ in range(n):
        x, q, C = random_ball(rng), random_rat(rng), random_cap(rng)
        tight = fball.scale(x, q)
        result = policies.absolute_cap(tight, C)
        lines.append({"op": "mul_rat_cap", "a": ball(x), "q": rat(q), "C": rat(C),
                      "result": ball(result)})
    return lines


def write(name, lines):
    path = os.path.join(VECTORS, name)
    with open(path, "w", encoding="utf-8") as fh:
        for line in lines:
            fh.write(json.dumps(line, sort_keys=True) + "\n")
    return len(lines)


def main():
    os.makedirs(VECTORS, exist_ok=True)
    rng = random.Random(20260928)
    lines = []
    lines += gen_binary_cap(rng, "add_cap", fball.add, 150)
    lines += gen_binary_cap(rng, "sub_cap", fball.sub, 150)
    lines += gen_binary_cap(rng, "mul_cap", fball.mul, 150)
    lines += gen_mul_rat_cap(rng, 150)
    count = write("cap_ops.jsonl", lines)
    print("cap_ops.jsonl: %d lines" % count)


if __name__ == "__main__":
    main()
