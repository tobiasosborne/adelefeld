#!/usr/bin/env python3
"""Deterministic test vectors for the C tests (work package 1.1, item 6).

Writes JSON lines to tests/ref/vectors/*.jsonl. Every line has an "op" field and the inputs and
the expected output of one operation. The format is documented in tests/ref/README.md.

Run:  python3 tests/ref/gen_vectors.py
The output is deterministic: the seed is fixed, so two runs are byte-identical.
"""
import json
import os
import random
from fractions import Fraction

from adfref import fball, membership, policies, recon
from adfref.fball import Fball

HERE = os.path.dirname(os.path.abspath(__file__))
VECTORS = os.path.join(HERE, "vectors")

EDGE_BALLS = [
    (0, 0, 1),      # exact zero
    (5, 0, 1),      # exact integer
    (3, 0, 4),      # exact fraction
    (-3, 0, 4),     # exact negative fraction
    (0, 2, 1),      # zero centre, positive radius
    (0, 8, 4),      # shared factor
    (-3, 12, 1),    # negative centre
    (3, 12, 1),     # shared factor between centre and radius
    (2, 3, 1),      # fractional radius
    (1, 3, 2),      # fractional centre and radius
    (0, 1, 1),      # Zhat
    (0, 1, 2),      # (1/2) Zhat
]

EDGE_RATIONALS = [
    Fraction(0), Fraction(1), Fraction(-1), Fraction(1, 2), Fraction(-1, 3),
    Fraction(5, 7), Fraction(12), Fraction(3, 2),
]


def ball(B):
    return {"A": B.A, "H": B.H, "d": B.d}


def unball(o):
    return Fball(o["A"], o["H"], o["d"])


def rat(q):
    q = Fraction(q)
    return {"num": q.numerator, "den": q.denominator}


def write(name, lines):
    path = os.path.join(VECTORS, name)
    with open(path, "w", encoding="utf-8") as fh:
        for line in lines:
            fh.write(json.dumps(line, sort_keys=True) + "\n")
    return len(lines)


def random_ball(rng):
    A = rng.randint(-40, 40)
    H = rng.choice([0, 0, 1, 2, 3, 5, 6, 8, 12, 18, 24, 30, rng.randint(0, 40)])
    d = rng.choice([1, 1, 2, 3, 4, 6, rng.randint(1, 12)])
    return Fball(A, H, d)


def random_scale_and_residue(rng, K):
    s = Fraction(rng.randint(1, 20), rng.randint(1, 6))
    return s, rng.randrange(K)


def gen_canonical(rng, n=200):
    lines = []
    for A, H, d in EDGE_BALLS:
        lines.append({"op": "canonical", "input": [A, H, d],
                      "result": list(Fball(A, H, d).as_tuple())})
    for _ in range(n):
        A = rng.randint(-40, 40)
        H = rng.randint(0, 40)
        d = rng.randint(1, 12)
        lines.append({"op": "canonical", "input": [A, H, d],
                      "result": list(Fball(A, H, d).as_tuple())})
    return lines


def gen_binary(rng, op, fn, n=300):
    lines = []
    edge = [Fball(*t) for t in EDGE_BALLS]
    for x in edge:
        for y in edge:
            r = fn(x, y)
            lines.append({"op": op, "a": ball(x), "b": ball(y), "result": ball(r)})
    for _ in range(n):
        x, y = random_ball(rng), random_ball(rng)
        r = fn(x, y)
        lines.append({"op": op, "a": ball(x), "b": ball(y), "result": ball(r)})
    return lines


def gen_unary(rng, op, fn, n=200):
    lines = []
    for t in EDGE_BALLS:
        x = Fball(*t)
        lines.append({"op": op, "a": ball(x), "result": ball(fn(x))})
    for _ in range(n):
        x = random_ball(rng)
        lines.append({"op": op, "a": ball(x), "result": ball(fn(x))})
    return lines


def gen_scale(rng, n=300):
    lines = []
    edge = [Fball(*t) for t in EDGE_BALLS]
    for x in edge:
        for q in EDGE_RATIONALS:
            lines.append({"op": "scale", "a": ball(x), "q": rat(q),
                          "result": ball(fball.scale(x, q))})
    for _ in range(n):
        x = random_ball(rng)
        q = Fraction(rng.randint(-30, 30), rng.randint(1, 8))
        lines.append({"op": "scale", "a": ball(x), "q": rat(q),
                      "result": ball(fball.scale(x, q))})
    return lines


def gen_predicates(rng, n=300):
    lines = []
    edge = [Fball(*t) for t in EDGE_BALLS]
    pairs = [(x, y) for x in edge for y in edge]
    for _ in range(n):
        pairs.append((random_ball(rng), random_ball(rng)))
    for x, y in pairs:
        lines.append({"op": "equal_set", "a": ball(x), "b": ball(y),
                      "result": fball.equal_set(x, y)})
        lines.append({"op": "overlaps", "a": ball(x), "b": ball(y),
                      "result": fball.overlaps(x, y)})
        lines.append({"op": "contains", "a": ball(x), "b": ball(y),
                      "result": fball.contains(x, y)})
    return lines


def gen_compare(rng, n=300):
    lines = []
    edge = [Fball(*t) for t in EDGE_BALLS]
    pairs = [(x, y) for x in edge for y in edge]
    for _ in range(n):
        pairs.append((random_ball(rng), random_ball(rng)))
    for x, y in pairs:
        lines.append({"op": "compare", "a": ball(x), "b": ball(y),
                      "result": fball.compare(x, y).value})
    return lines


def gen_membership(rng, n=400):
    lines = []
    edge = [Fball(*t) for t in EDGE_BALLS]
    for x in edge:
        for q in EDGE_RATIONALS:
            lines.append({"op": "membership", "ball": ball(x), "x": rat(q),
                          "result": membership.rational_in_fball(q, x)})
        for k in range(-3, 4):
            q = x.center + x.radius * k
            lines.append({"op": "membership", "ball": ball(x), "x": rat(q),
                          "result": True})
    for _ in range(n):
        x = random_ball(rng)
        q = Fraction(rng.randint(-60, 60), rng.randint(1, 9))
        lines.append({"op": "membership", "ball": ball(x), "x": rat(q),
                      "result": membership.rational_in_fball(q, x)})
    return lines


def gen_recon(rng, n=300):
    lines = []
    edge = [Fball(*t) for t in EDGE_BALLS]
    for x in edge:
        for lo, hi in [(0, 5), (1, 1), (2, 5), (-3, 3), (0, 100)]:
            r = recon.reconstruct(x, lo, hi)
            lines.append({"op": "reconstruct", "ball": ball(x), "lo": rat(lo),
                          "hi": rat(hi), "status": r.status,
                          "solutions": [rat(s) for s in r.solutions]})
    for _ in range(n):
        x = random_ball(rng)
        lo = Fraction(rng.randint(-30, 30), rng.randint(1, 6))
        hi = lo + Fraction(rng.randint(0, 40), rng.randint(1, 6))
        r = recon.reconstruct(x, lo, hi)
        lines.append({"op": "reconstruct", "ball": ball(x), "lo": rat(lo),
                      "hi": rat(hi), "status": r.status,
                      "solutions": [rat(s) for s in r.solutions]})
    return lines


def policy_value(v):
    if isinstance(v, policies.Exact):
        return {"exact": rat(v.q)}
    return {"s": rat(v.s), "u": v.u, "K": v.K}


def gen_policies(rng, n=300):
    lines = []
    edge = [Fball(*t) for t in EDGE_BALLS]
    for x in edge:
        for K in (1, 2, 3, 6, 12):
            v, lost = policies.convert_from_tight(x, K)
            lines.append({"op": "convert_from_tight", "ball": ball(x), "K": K,
                          "value": policy_value(v), "lost": lost})
    for _ in range(n):
        K = rng.randint(1, 12)
        x = random_ball(rng)
        v, lost = policies.convert_from_tight(x, K)
        lines.append({"op": "convert_from_tight", "ball": ball(x), "K": K,
                      "value": policy_value(v), "lost": lost})
    for _ in range(n):
        K = rng.randint(1, 12)
        s, u = random_scale_and_residue(rng, K)
        t, w = random_scale_and_residue(rng, K)
        a, b = policies.ScaledBall(s, u, K), policies.ScaledBall(t, w, K)
        lines.append({"op": "scaled_add", "a": policy_value(a), "b": policy_value(b),
                      "result": policy_value(policies.scaled_add(a, b))})
        lines.append({"op": "scaled_mul", "a": policy_value(a), "b": policy_value(b),
                      "result": policy_value(policies.scaled_mul(a, b))})
    for _ in range(n):
        x = random_ball(rng)
        C = Fraction(rng.randint(1, 30), rng.randint(1, 6))
        lines.append({"op": "absolute_cap", "ball": ball(x), "C": rat(C),
                      "result": ball(policies.absolute_cap(x, C))})
    return lines


def main():
    os.makedirs(VECTORS, exist_ok=True)
    rng = random.Random(20260927)
    counts = {}
    counts["canonical.jsonl"] = write("canonical.jsonl", gen_canonical(rng))
    counts["add.jsonl"] = write("add.jsonl", gen_binary(rng, "add", fball.add))
    counts["sub.jsonl"] = write("sub.jsonl", gen_binary(rng, "sub", fball.sub))
    counts["mul.jsonl"] = write("mul.jsonl", gen_binary(rng, "mul", fball.mul))
    counts["neg.jsonl"] = write("neg.jsonl", gen_unary(rng, "neg", fball.neg))
    counts["scale.jsonl"] = write("scale.jsonl", gen_scale(rng))
    counts["predicates.jsonl"] = write("predicates.jsonl", gen_predicates(rng))
    counts["compare.jsonl"] = write("compare.jsonl", gen_compare(rng))
    counts["membership.jsonl"] = write("membership.jsonl", gen_membership(rng))
    counts["recon.jsonl"] = write("recon.jsonl", gen_recon(rng))
    counts["policies.jsonl"] = write("policies.jsonl", gen_policies(rng))
    for name in sorted(counts):
        print("%-20s %5d lines" % (name, counts[name]))


if __name__ == "__main__":
    main()
