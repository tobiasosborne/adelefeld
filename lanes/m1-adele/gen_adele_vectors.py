#!/usr/bin/env python3
"""Generate the m1-adele vectors from the Python reference.

Run from the repository root:

    python3 lanes/m1-adele/gen_adele_vectors.py

It writes tests/ref/vectors/m1-adele/*.jsonl. The file format is JSON lines; see
tests/ref/vectors/m1-adele/README.md for the schema. The finite part is adfref.fball.
"""

import json
import os
import random
import sys
from fractions import Fraction

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "tests", "ref"))

from adfref.adele_ref import (  # noqa: E402
    Adele, Cadele, CBox, Interval, dyadic_exact,
)
from adfref.fball import Fball  # noqa: E402


OUT = os.path.join(os.path.dirname(__file__), "..", "..", "tests", "ref", "vectors", "m1-adele")


def rat_json(q):
    q = Fraction(q)
    return {"num": q.numerator, "den": q.denominator}


def interval_json(I):
    return {"lo": rat_json(I.lo), "hi": rat_json(I.hi)}


def ball_json(b):
    return {"A": b.A, "H": b.H, "d": b.d}


def adele_json(a):
    return {"re": interval_json(a.re), "fin": ball_json(a.fin)}


def cadele_json(a):
    return {"re": interval_json(a.inf.re), "im": interval_json(a.inf.im),
            "fin": ball_json(a.fin)}


# ---------------------------------------------------------------- generators

def random_interval(rng, den_max=6, num_max=8):
    lo = Fraction(rng.randint(-num_max, num_max), rng.randint(1, den_max))
    hi = Fraction(rng.randint(-num_max, num_max), rng.randint(1, den_max))
    if lo > hi:
        lo, hi = hi, lo
    return Interval(lo, hi)


def random_ball(rng):
    d = rng.choice((1, 1, 2, 3, 4, 6))
    H = rng.choice((0, 1, 2, 3, 4, 6, 12, 18))
    A = rng.randint(0, max(0, H)) if H > 0 else rng.randint(-6, 6)
    return Fball(A, H, d)


def random_rat(rng, allow_zero=False):
    while True:
        n = rng.randint(-9, 9)
        d = rng.choice((1, 1, 2, 3, 4, 5, 6))
        if allow_zero or n != 0:
            return Fraction(n, d)


def random_adele(rng):
    return Adele(random_interval(rng), random_ball(rng))


def random_cadele(rng):
    inf = CBox(random_interval(rng), random_interval(rng))
    return Cadele(inf, random_ball(rng))


def witness_adele(a, b, op, q, rng):
    """One witness: a rational point of each input and the exact image point."""
    w = {"a_re": rat_json(a.re.sample(rng)), "a_fin": rat_json(_ball_point(a.fin, rng))}
    ra = Fraction(w["a_re"]["num"], w["a_re"]["den"])
    fa = Fraction(w["a_fin"]["num"], w["a_fin"]["den"])
    if b is not None:
        w["b_re"] = rat_json(b.re.sample(rng))
        w["b_fin"] = rat_json(_ball_point(b.fin, rng))
        rb = Fraction(w["b_re"]["num"], w["b_re"]["den"])
        fb = Fraction(w["b_fin"]["num"], w["b_fin"]["den"])
    if op == "add":
        w["re_image"] = rat_json(ra + rb)
        w["fin_image"] = rat_json(fa + fb)
    elif op == "sub":
        w["re_image"] = rat_json(ra - rb)
        w["fin_image"] = rat_json(fa - fb)
    elif op == "mul":
        w["re_image"] = rat_json(ra * rb)
        w["fin_image"] = rat_json(fa * fb)
    elif op == "neg":
        w["re_image"] = rat_json(-ra)
        w["fin_image"] = rat_json(-fa)
    elif op == "add_rat":
        w["re_image"] = rat_json(ra + q)
        w["fin_image"] = rat_json(fa + q)
    elif op == "mul_rat":
        w["re_image"] = rat_json(ra * q)
        w["fin_image"] = rat_json(fa * q)
    elif op == "div_rat":
        w["re_image"] = rat_json(ra / q)
        w["fin_image"] = rat_json(fa / q)
    else:
        raise ValueError(op)
    return w


def witness_cadele(a, b, op, q, rng):
    w = {"a_re": rat_json(a.inf.re.sample(rng)), "a_im": rat_json(a.inf.im.sample(rng)),
         "a_fin": rat_json(_ball_point(a.fin, rng))}
    za = (Fraction(w["a_re"]["num"], w["a_re"]["den"]),
          Fraction(w["a_im"]["num"], w["a_im"]["den"]))
    fa = Fraction(w["a_fin"]["num"], w["a_fin"]["den"])
    if b is not None:
        w["b_re"] = rat_json(b.inf.re.sample(rng))
        w["b_im"] = rat_json(b.inf.im.sample(rng))
        w["b_fin"] = rat_json(_ball_point(b.fin, rng))
        zb = (Fraction(w["b_re"]["num"], w["b_re"]["den"]),
              Fraction(w["b_im"]["num"], w["b_im"]["den"]))
        fb = Fraction(w["b_fin"]["num"], w["b_fin"]["den"])

    def cmul(x, y):
        return (x[0] * y[0] - x[1] * y[1], x[0] * y[1] + x[1] * y[0])

    if op == "add":
        z, f = (za[0] + zb[0], za[1] + zb[1]), fa + fb
    elif op == "sub":
        z, f = (za[0] - zb[0], za[1] - zb[1]), fa - fb
    elif op == "mul":
        z, f = cmul(za, zb), fa * fb
    elif op == "neg":
        z, f = (-za[0], -za[1]), -fa
    elif op == "add_rat":
        z, f = (za[0] + q, za[1]), fa + q
    elif op == "mul_rat":
        z, f = (za[0] * q, za[1] * q), fa * q
    elif op == "div_rat":
        z, f = (za[0] / q, za[1] / q), fa / q
    else:
        raise ValueError(op)
    w["re_image"] = rat_json(z[0])
    w["im_image"] = rat_json(z[1])
    w["fin_image"] = rat_json(f)
    return w


def _ball_point(ball, rng):
    if ball.H == 0:
        return Fraction(ball.A, ball.d)
    k = rng.randint(-4, 4)
    return Fraction(ball.A + k * ball.H, ball.d)


# ------------------------------------------------------------------- writers

def write_adele_ops(path, rng):
    records = []
    ops_bin = ["add", "sub", "mul"]
    ops_un = ["neg"]
    ops_sc = ["add_rat", "mul_rat", "div_rat"]

    pres = [2, 10, 53, 64, 128, 4096]

    for op in ops_bin:
        for i in range(14):
            a = random_adele(rng)
            b = random_adele(rng)
            prec = rng.choice(pres)
            result = getattr(a, op)(b)
            witnesses = [witness_adele(a, b, op, None, rng) for _ in range(4)]
            records.append({
                "op": op, "prec": prec, "a": adele_json(a), "b": adele_json(b),
                "result_fin": ball_json(result.fin), "result_re": interval_json(result.re),
                "witness": witnesses})

    for op in ops_un:
        for i in range(8):
            a = random_adele(rng)
            prec = rng.choice(pres)
            result = getattr(a, op)()
            witnesses = [witness_adele(a, None, op, None, rng) for _ in range(4)]
            records.append({
                "op": op, "prec": prec, "a": adele_json(a),
                "result_fin": ball_json(result.fin), "result_re": interval_json(result.re),
                "witness": witnesses})

    for op in ops_sc:
        for i in range(14):
            a = random_adele(rng)
            q = random_rat(rng, allow_zero=(op != "div_rat"))
            prec = rng.choice(pres)
            result = getattr(a, op)(q)
            witnesses = [witness_adele(a, None, op, q, rng) for _ in range(4)]
            records.append({
                "op": op, "prec": prec, "a": adele_json(a), "q": rat_json(q),
                "result_fin": ball_json(result.fin), "result_re": interval_json(result.re),
                "witness": witnesses})

    # The table of docs/SPEC.md 4.3, as finite parts with a point for the real coordinate.
    x312 = Adele(Interval.point(0), Fball(3, 12, 1))
    x518 = Adele(Interval.point(0), Fball(5, 18, 1))
    x18_2 = Adele(Interval.point(0), Fball(1, 8, 2))
    x29_3 = Adele(Interval.point(0), Fball(2, 9, 3))
    spec_bin = [(x312, x518, "add"), (x312, x518, "mul"), (x18_2, x29_3, "mul")]
    for (a, b, op) in spec_bin:
        result = getattr(a, op)(b)
        witnesses = [witness_adele(a, b, op, None, rng) for _ in range(3)]
        records.append({"op": op, "prec": 53, "a": adele_json(a), "b": adele_json(b),
                        "result_fin": ball_json(result.fin),
                        "result_re": interval_json(result.re), "witness": witnesses})
    for q in (Fraction(12, 1), Fraction(1, 3)):
        result = x518.mul_rat(q)
        witnesses = [witness_adele(x518, None, "mul_rat", q, rng) for _ in range(3)]
        records.append({"op": "mul_rat", "prec": 53, "a": adele_json(x518), "q": rat_json(q),
                        "result_fin": ball_json(result.fin),
                        "result_re": interval_json(result.re), "witness": witnesses})

    with open(path, "w") as fh:
        for r in records:
            fh.write(json.dumps(r, separators=(",", ":")) + "\n")
    return len(records)


def write_cadele_ops(path, rng):
    records = []
    pres = [2, 10, 53, 64, 4096]

    def add_bin(op):
        for i in range(12):
            a = random_cadele(rng)
            b = random_cadele(rng)
            prec = rng.choice(pres)
            result = getattr(a, op)(b)
            witnesses = [witness_cadele(a, b, op, None, rng) for _ in range(4)]
            records.append({"op": op, "prec": prec, "a": cadele_json(a), "b": cadele_json(b),
                            "result_fin": ball_json(result.fin), "witness": witnesses})

    def add_un(op):
        for i in range(8):
            a = random_cadele(rng)
            prec = rng.choice(pres)
            result = getattr(a, op)()
            witnesses = [witness_cadele(a, None, op, None, rng) for _ in range(4)]
            records.append({"op": op, "prec": prec, "a": cadele_json(a),
                            "result_fin": ball_json(result.fin), "witness": witnesses})

    def add_sc(op):
        for i in range(12):
            a = random_cadele(rng)
            q = random_rat(rng, allow_zero=(op != "div_rat"))
            prec = rng.choice(pres)
            result = getattr(a, op)(q)
            witnesses = [witness_cadele(a, None, op, q, rng) for _ in range(4)]
            records.append({"op": op, "prec": prec, "a": cadele_json(a), "q": rat_json(q),
                            "result_fin": ball_json(result.fin), "witness": witnesses})

    for op in ["add", "sub", "mul"]:
        add_bin(op)
    add_un("neg")
    for op in ["add_rat", "mul_rat", "div_rat"]:
        add_sc(op)

    with open(path, "w") as fh:
        for r in records:
            fh.write(json.dumps(r, separators=(",", ":")) + "\n")
    return len(records)


def write_set_rat(path):
    cases = []
    for prec in (2, 10, 53, 64, 4096):
        for q in (Fraction(1, 3), Fraction(-1, 3), Fraction(0),
                  Fraction(-7, 5), Fraction(22, 7)):
            cases.append((q, prec))
    # Huge rationals, numerator and denominator of about 4096 bits.
    big = Fraction(2 ** 4095 + 12345, 2 ** 4000 + 7)
    cases += [(big, 4096), (-big, 4096), (Fraction(2 ** 4095 + 1), 4096),
              (Fraction(-(2 ** 4095 + 1)), 4096), (Fraction(1, 2 ** 4000 + 1), 4096)]
    # Dyadic numbers that fit or do not fit.
    cases += [(Fraction(3, 4), 2), (Fraction(3, 4), 1), (Fraction(-5, 8), 3),
              (Fraction(-5, 8), 2), (Fraction(2 ** 60 + 1), 61), (Fraction(2 ** 60 + 1), 60),
              (Fraction(0), 2)]
    records = []
    for q, prec in cases:
        records.append({"q": rat_json(q), "prec": prec, "exact": dyadic_exact(q, prec)})
    with open(path, "w") as fh:
        for r in records:
            fh.write(json.dumps(r, separators=(",", ":")) + "\n")
    return len(records)


def main():
    os.makedirs(OUT, exist_ok=True)
    rng = random.Random(20260928)
    n1 = write_adele_ops(os.path.join(OUT, "adele_ops.jsonl"), rng)
    n2 = write_cadele_ops(os.path.join(OUT, "cadele_ops.jsonl"), rng)
    n3 = write_set_rat(os.path.join(OUT, "set_rat.jsonl"))
    print("wrote adele_ops.jsonl: %d records" % n1)
    print("wrote cadele_ops.jsonl: %d records" % n2)
    print("wrote set_rat.jsonl: %d records" % n3)


if __name__ == "__main__":
    main()
