#!/usr/bin/env python3
"""Deterministic test vectors for adf_rat, written to tests/ref/vectors/m1-rat/rat.jsonl.

The oracle is the exact rational arithmetic of the Python standard library, `fractions.Fraction`
(GMP-backed, the same canonical form as conventions 5.1: denominator > 0, gcd = 1), used the way
tests/ref/adfref/fball.py uses it for rational centres. The canonical form of a result is checked
here with `math.gcd`, so a vector never records a non-canonical pair.

Every line has an "op" field, the inputs and the result or the status name of one operation.
The format follows tests/ref/README.md, "Vector format", and adds the files of this lane.

Run:  python3 lanes/m1-rat/gen_rat_vectors.py
The output is deterministic: the seed is fixed, so two runs are byte-identical.
"""
import json
import os
import random
from fractions import Fraction
from math import gcd

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
OUT_DIR = os.path.join(ROOT, "tests", "ref", "vectors", "m1-rat")

SEED = 20260928

# The edge cases every one of the operations is run on: the exact zero, one, minus one, small
# fractions, a numerator and a denominator that share a factor, a huge operand.
EDGE = [
    Fraction(0),
    Fraction(1),
    Fraction(-1),
    Fraction(1, 2),
    Fraction(-1, 3),
    Fraction(2, 3),
    Fraction(5, 7),
    Fraction(12),
    Fraction(-12),
    Fraction(10 ** 40 + 7, 10 ** 40 + 3),
    Fraction(-(10 ** 60), 10 ** 60 + 1),
]


def rat(q):
    """The vector form of a rational: lowest terms, denominator > 0."""
    q = Fraction(q)
    assert q.denominator > 0 and gcd(q.numerator, q.denominator) == 1, q
    return {"num": q.numerator, "den": q.denominator}


def unrat(o):
    return Fraction(o["num"], o["den"])


def write(name, lines):
    os.makedirs(OUT_DIR, exist_ok=True)
    path = os.path.join(OUT_DIR, name)
    with open(path, "w", encoding="utf-8") as fh:
        for line in lines:
            fh.write(json.dumps(line, sort_keys=True) + "\n")
    return len(lines)


def raw_pairs():
    """Raw (num, den) pairs for the constructors from raw data: not reduced, den of any sign."""
    return [
        (0, 1),
        (0, 5),
        (1, 1),
        (-1, 1),
        (6, 3),        # reduces to 2
        (6, -3),       # negative denominator
        (-6, 3),
        (-6, -3),
        (12, 18),
        (5, 1),
        (-5, 1),
        (10 ** 50 + 11, 10 ** 50 + 22),   # a common factor, huge
        (2 ** 300, 4 ** 150),             # equal, huge: the value 1
        (-(2 ** 300), 4 ** 150),          # the value -1
        (0, -(10 ** 30)),
    ]


def random_rat(rng, bits=0):
    if bits:
        m = 1 << bits
        return Fraction(rng.randint(-m, m), rng.randint(1, m))
    return Fraction(rng.randint(-40, 40), rng.randint(-12, 12) or 1)


def gen_set(rng, n=200):
    lines = []
    for num, den in raw_pairs():
        q = Fraction(num, den)
        lines.append({"op": "set_fmpz2", "num": num, "den": den, "result": rat(q)})
        lines.append({"op": "set_fmpq", "num": num, "den": den, "result": rat(q)})
        if den != 0:
            lines.append({"op": "div", "a": rat(q), "b": {"num": 0, "den": 1},
                          "status": "NOT_UNIT"})
        if q == 0:
            # Only the exact 0 has no inverse; the other inverses are in rat_inv.jsonl.
            lines.append({"op": "inv", "a": rat(q), "status": "NOT_UNIT"})
    for _ in range(n):
        m = rng.choice([0, 1, 2, 3, 7, 40])
        k = rng.randint(-12, 12)
        num, den = rng.randint(-m, m), rng.choice([k, -k, 0])
        if num == 0:
            num = 3
        # A zero denominator is DOMAIN, not a value: the zero denominator of the raw data is
        # the one input the two constructors reject (conventions 4.4).
        rec = {"op": "set_fmpz2", "num": num, "den": den}
        if den == 0:
            rec["status"] = "DOMAIN"
        else:
            rec["result"] = rat(Fraction(num, den))
        lines.append(rec)
        rec = {"op": "set_fmpq", "num": num, "den": den}
        if den == 0:
            rec["status"] = "DOMAIN"
        else:
            rec["result"] = rat(Fraction(num, den))
        lines.append(rec)
    return lines


def gen_set_si(rng, n=120):
    lines = []
    for k in [0, 1, -1, 2, -2, 2 ** 62, -(2 ** 62), 2 ** 62 - 1]:
        lines.append({"op": "set_si", "n": k, "result": rat(Fraction(k))})
    for _ in range(n):
        k = rng.randint(-(2 ** 62), 2 ** 62)
        lines.append({"op": "set_si", "n": k, "result": rat(Fraction(k))})
    return lines


def gen_binary(rng, op, fn, n=300):
    lines = []
    for x in EDGE:
        for y in EDGE:
            lines.append({"op": op, "a": rat(x), "b": rat(y), "result": rat(fn(x, y))})
    for _ in range(n):
        x, y = random_rat(rng), random_rat(rng)
        lines.append({"op": op, "a": rat(x), "b": rat(y), "result": rat(fn(x, y))})
    for _ in range(n // 6):
        x, y = random_rat(rng, 4096), random_rat(rng, 4096)
        lines.append({"op": op, "a": rat(x), "b": rat(y), "result": rat(fn(x, y))})
    return lines


def gen_unary(rng, op, fn, n=200):
    lines = []
    for x in EDGE:
        lines.append({"op": op, "a": rat(x), "result": rat(fn(x))})
    for _ in range(n):
        x = random_rat(rng)
        lines.append({"op": op, "a": rat(x), "result": rat(fn(x))})
    for _ in range(n // 6):
        x = random_rat(rng, 4096)
        lines.append({"op": op, "a": rat(x), "result": rat(fn(x))})
    return lines


def gen_div(rng, n=300):
    lines = []
    for x in EDGE:
        for y in EDGE:
            if y != 0:
                lines.append({"op": "div", "a": rat(x), "b": rat(y), "result": rat(x / y)})
            else:
                lines.append({"op": "div", "a": rat(x), "b": rat(y), "status": "NOT_UNIT"})
    for _ in range(n):
        x, y = random_rat(rng), random_rat(rng)
        if y == 0:
            y = Fraction(2, 3)
        lines.append({"op": "div", "a": rat(x), "b": rat(y), "result": rat(x / y)})
    return lines


def gen_inv(rng, n=200):
    lines = []
    for x in EDGE:
        if x == 0:
            lines.append({"op": "inv", "a": rat(x), "status": "NOT_UNIT"})
        else:
            lines.append({"op": "inv", "a": rat(x), "result": rat(1 / x)})
    for _ in range(n):
        x = random_rat(rng) or Fraction(1)
        lines.append({"op": "inv", "a": rat(x), "result": rat(1 / x)})
    return lines


def gen_pred(rng, op, fn, n=300):
    lines = []
    for x in EDGE:
        for y in EDGE:
            lines.append({"op": op, "a": rat(x), "b": rat(y), "result": bool(fn(x, y))})
    for _ in range(n):
        x, y = random_rat(rng), random_rat(rng)
        lines.append({"op": op, "a": rat(x), "b": rat(y), "result": bool(fn(x, y))})
    return lines


def gen_sgn(rng, n=200):
    lines = []
    for x in EDGE:
        lines.append({"op": "sgn", "a": rat(x), "result": (0 if x == 0 else (1 if x > 0 else -1))})
    for _ in range(n):
        x = random_rat(rng)
        lines.append({"op": "sgn", "a": rat(x), "result": (0 if x == 0 else (1 if x > 0 else -1))})
    return lines


def gen_is_zero(rng, n=200):
    lines = []
    for x in EDGE:
        lines.append({"op": "is_zero", "a": rat(x), "result": x == 0})
    for _ in range(n):
        x = random_rat(rng)
        lines.append({"op": "is_zero", "a": rat(x), "result": x == 0})
    return lines


def main():
    rng = random.Random(SEED)
    total = 0
    total += write("rat_set.jsonl", gen_set(rng))
    total += write("rat_set_si.jsonl", gen_set_si(rng))
    total += write("rat_add.jsonl", gen_binary(rng, "add", lambda a, b: a + b))
    total += write("rat_sub.jsonl", gen_binary(rng, "sub", lambda a, b: a - b))
    total += write("rat_mul.jsonl", gen_binary(rng, "mul", lambda a, b: a * b))
    total += write("rat_neg.jsonl", gen_unary(rng, "neg", lambda a: -a))
    total += write("rat_div.jsonl", gen_div(rng))
    total += write("rat_inv.jsonl", gen_inv(rng))
    total += write("rat_equal.jsonl", gen_pred(rng, "equal", lambda a, b: a == b))
    total += write("rat_identical.jsonl", gen_pred(rng, "identical", lambda a, b: a == b))
    total += write("rat_sgn.jsonl", gen_sgn(rng))
    total += write("rat_is_zero.jsonl", gen_is_zero(rng))
    print("wrote %d vectors into %s" % (total, OUT_DIR))


if __name__ == "__main__":
    main()
