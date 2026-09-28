#!/usr/bin/env python3
"""Deterministic test vectors for tests/test_scaled_vectors.c (lane m1-scaled).

Writes tests/ref/vectors/m1-scaled/scaled_ops.jsonl, one JSON object per line, in the format of
tests/ref/gen_vectors.py (keys sorted, integers as literals, rationals as {num, den}).

Rows: the operations of include/adelefeld/scaled.h that tests/ref/vectors/policies.jsonl does not
cover: neg, sub, scale (adf_scaled_mul_rat), add_rat (adf_scaled_add_rat), mul with an exact
operand, mul_tight, set_context (conversion between scaled contexts), to_fball
(adf_scaled_get_fball).  The rows that policies.py has are taken from it (scaled_neg, scaled_sub,
scaled_scale, scaled_add, scaled_mul).  The rows it has not (scaled_mul_tight,
scaled_set_context) are computed from the formulas of docs/proofs/policies.md Proposition 10.3
(line 200) and Proposition 11 (line 218), and each such row is cross-checked against the tight
product of tests/ref/adfref/fball.py or against the containment and loss claims of the
proposition (assertions below); a disagreement raises and no vector file is written.

A scaled value is {"s": {num, den}, "u": U, "K": K}; an exact value is {"exact": {num, den}}.
A ball is the canonical triple {"A", "H", "d"}.  Every scaled operand of a binary row and its
result share one context of modulus K (adf_scaled: the shared-pointer rule, conventions 4.6).

Run:  python3 lanes/m1-scaled/gen_scaled_vectors.py
The output is deterministic: the seed is fixed, so two runs are byte-identical.
"""
import json
import os
import random
import sys
from fractions import Fraction
from math import gcd

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tests", "ref"))

from adfref import fball as fb            # noqa: E402
from adfref import policies               # noqa: E402
from adfref.fball import Fball            # noqa: E402
from adfref.rat import is_integer, qgcd   # noqa: E402

VECTORS = os.path.join(ROOT, "tests", "ref", "vectors", "m1-scaled")

KMIN, KMAX = 1, 12


def rat(q):
    q = Fraction(q)
    return {"num": q.numerator, "den": q.denominator}


def value(v):
    """The JSON encoding of a policy value: Exact or ScaledBall."""
    if isinstance(v, policies.Exact):
        return {"exact": rat(v.q)}
    return {"s": rat(v.s), "u": v.u, "K": v.K}


def ball(b):
    return {"A": b.A, "H": b.H, "d": b.d}


def ball_of(v):
    return ball(Fball.from_center_radius(*center_radius(v)))


def center_radius(v):
    if isinstance(v, policies.Exact):
        return v.q, Fraction(0)
    return v.s * v.u, v.s * v.K


def rand_scale(rng):
    return Fraction(rng.randint(1, 20), rng.randint(1, 6))


def rand_exact(rng):
    return Fraction(rng.randint(-30, 30), rng.randint(1, 6))


def rand_scaled(rng, K=None):
    if K is None:
        K = rng.randint(KMIN, KMAX)
    return policies.ScaledBall(rand_scale(rng), rng.randrange(K), K)


def rand_value(rng, K=None):
    """A scaled value or an exact rational; scaled values use one modulus K if given."""
    if rng.random() < 0.25:
        return policies.Exact(rand_exact(rng))
    return rand_scaled(rng, K)


def set_context(v, K2):
    """Conversion of a policy value to the context K2: policies.md Proposition 11 (line 218).
    Returns (value, lost) with lost = 1 exactly when the set changes (conventions 5.4, C5)."""
    if isinstance(v, policies.Exact):
        return policies.Exact(v.q), False
    K, s, u = v.K, v.s, v.u
    g = qgcd(Fraction(u), Fraction(K, K2))          # gcd(u, K/K') as rationals
    s2 = s * g
    u2 = int(s * u / s2) % K2
    lost = not is_integer(Fraction(u) * K2 / K)     # exact exactly when u K'/K is an integer
    out = policies.ScaledBall(s2, u2, K2)
    # Cross-checks of the proposition: the result contains the input, and it equals the input
    # exactly when lost is False; otherwise it is strictly coarser.
    assert fb.contains(v.to_fball(), out.to_fball()), (v, K2)
    if lost:
        assert not fb.equal_set(v.to_fball(), out.to_fball()), (v, K2)
    else:
        assert fb.equal_set(v.to_fball(), out.to_fball()), (v, K2)
    return out, lost


def mul_tight(x, y):
    """The tight scaled product: policies.md Proposition 10.3 (line 200).  Both operands scaled:
    (s t h)(((u v / h) mod K) + K Zhat), h = gcd(u, v, K); exact operands as in scaled_mul."""
    if isinstance(x, policies.Exact) or isinstance(y, policies.Exact):
        return policies.scaled_mul(x, y)
    h = gcd(gcd(x.u, y.u), x.K)
    out = policies.ScaledBall(x.s * y.s * h, (x.u * y.u // h) % x.K, x.K)
    # Cross-check: the result is the tight product of the two sets (proposition claim).
    assert fb.equal_set(out.to_fball(), fb.mul(x.to_fball(), y.to_fball())), (x, y)
    return out


def add_rat(v, q):
    """v + q for an exact rational q: policies.md Proposition 8 (line 159).  Returns (value,
    lost); lost = 1 exactly when the scale s does not divide q (the set changes)."""
    q = Fraction(q)
    if isinstance(v, policies.Exact):
        return policies.Exact(v.q + q), False
    if q == 0:
        return v, False
    out = policies.scaled_add(v, policies.Exact(q))
    lost = not is_integer(q / v.s)
    # Cross-check: the result contains the tight sum, and equals it exactly when nothing is lost
    # (policies.md Proposition 8, item 2 and 4).
    tight = fb.add(v.to_fball(), Fball.from_center_radius(q, 0))
    assert fb.contains(tight, out.to_fball()), (v, q)
    assert fb.equal_set(tight, out.to_fball()) == (not lost), (v, q)
    return out, lost


def emit(rows, op, **kw):
    rows.append(dict(op=op, **kw))


def gen(rng):
    rows = []

    # Hand-picked edges first.
    edges = [
        policies.Exact(0), policies.Exact(1), policies.Exact(-1), policies.Exact(Fraction(-3, 4)),
        policies.ScaledBall(1, 0, 1), policies.ScaledBall(1, 1, 1), policies.ScaledBall(Fraction(1, 2), 1, 2),
        policies.ScaledBall(Fraction(3, 2), 2, 3), policies.ScaledBall(2, 3, 6),
    ]
    for v in edges:
        emit(rows, "neg", a=value(v), result=value(policies.scaled_neg(v)))
        emit(rows, "to_fball", a=value(v), result=ball_of(v))
        for q in (Fraction(0), Fraction(1, 2), Fraction(-2, 3), Fraction(5), Fraction(-7, 2)):
            emit(rows, "scale", a=value(v), q=rat(q), result=value(policies.scaled_scale(v, q)))
            out, lost = add_rat(v, q)
            emit(rows, "add_rat", a=value(v), q=rat(q), value=value(out), lost=lost)
        for w in edges:
            if isinstance(v, policies.ScaledBall) and isinstance(w, policies.ScaledBall) and v.K != w.K:
                continue
            emit(rows, "sub", a=value(v), b=value(w), result=value(policies.scaled_sub(v, w)))
            emit(rows, "mul_exact", a=value(v), b=value(w),
                 result=value(policies.scaled_mul(v, w)))
            emit(rows, "mul_tight", a=value(v), b=value(w), result=value(mul_tight(v, w)))
        for K2 in (1, 2, 3, 6, 12):
            out, lost = set_context(v, K2)
            emit(rows, "set_context", a=value(v), K2=K2, value=value(out), lost=lost)

    # Random rows.
    for _ in range(120):
        v = rand_value(rng)
        emit(rows, "neg", a=value(v), result=value(policies.scaled_neg(v)))
    for _ in range(120):
        v = rand_value(rng)
        emit(rows, "to_fball", a=value(v), result=ball_of(v))
    for _ in range(200):
        K = rng.randint(KMIN, KMAX)
        v, w = rand_value(rng, K), rand_value(rng, K)
        emit(rows, "sub", a=value(v), b=value(w), result=value(policies.scaled_sub(v, w)))
    for _ in range(160):
        v = rand_value(rng)
        q = rand_exact(rng)
        emit(rows, "scale", a=value(v), q=rat(q), result=value(policies.scaled_scale(v, q)))
    for _ in range(200):
        v = rand_value(rng)
        q = rand_exact(rng)
        out, lost = add_rat(v, q)
        emit(rows, "add_rat", a=value(v), q=rat(q), value=value(out), lost=lost)
    for _ in range(160):
        K = rng.randint(KMIN, KMAX)
        v, w = rand_value(rng, K), rand_value(rng, K)
        emit(rows, "mul_exact", a=value(v), b=value(w), result=value(policies.scaled_mul(v, w)))
    for _ in range(220):
        K = rng.randint(KMIN, KMAX)
        v, w = rand_value(rng, K), rand_value(rng, K)
        emit(rows, "mul_tight", a=value(v), b=value(w), result=value(mul_tight(v, w)))
    for _ in range(240):
        v = rand_value(rng)
        K2 = rng.randint(KMIN, KMAX)
        out, lost = set_context(v, K2)
        emit(rows, "set_context", a=value(v), K2=K2, value=value(out), lost=lost)
    return rows


def main():
    os.makedirs(VECTORS, exist_ok=True)
    rng = random.Random(20260928)
    rows = gen(rng)
    path = os.path.join(VECTORS, "scaled_ops.jsonl")
    with open(path, "w", encoding="utf-8") as fh:
        for line in rows:
            fh.write(json.dumps(line, sort_keys=True) + "\n")
    counts = {}
    for line in rows:
        counts[line["op"]] = counts.get(line["op"], 0) + 1
    for op in sorted(counts):
        print("%-14s %5d rows" % (op, counts[op]))
    print("%-14s %5d rows" % ("total", len(rows)))


if __name__ == "__main__":
    main()
