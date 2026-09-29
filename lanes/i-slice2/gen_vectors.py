#!/usr/bin/env python3
"""lanes/i-slice2/gen_vectors.py: the vectors of slice 2 of milestone 2 (docs/api-2.md section 2), written from
the reference functions of proto/ideles_checks.py (part 3), which that file checks against exact points of the
input sets, the unit cosets at levels M and trial division (python3 proto/ideles_checks.py part3).

Writes, deterministically (seed 20260930):
  tests/ref/vectors/i-slice2/idele_maps.jsonl  adf_idele_mul_rat, _norm, _valuation_at, _abs_at, _abs_inf
  tests/ref/vectors/i-slice2/idclass.jsonl     adf_idclass_set_idele, _mul, _inv

Encodings as in lanes/i-slice1/gen_vectors.py. A coset is [c, N]. A rational is [n, d] in lowest terms, d > 0.
A dyadic number is [m, e], the value m 2^e. A real ball is {"mid": dyadic, "rad": dyadic}; the radius mantissa
has at most 30 bits. An idele is {"mid", "rad", "r", "u"}; a class is {"mid", "rad", "u"}. A place is a prime
(an integer below 2^64) or 0 for the archimedean place. Fields of a result: "status"; for the real kernel "lo",
"hi" (the rounded end points, dyadic), "sign", and "L", "H" (the exact end points of the absolute value of the
result set, rationals); the finite part "r", "u"; "v" and "a" for a valuation and an absolute value.

Run: timeout 300 python3 lanes/i-slice2/gen_vectors.py
"""
import json
import os
import random
import sys
from fractions import Fraction as F
from math import gcd

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "proto"))
import ideles_checks as R  # noqa: E402

OUT = os.path.join(ROOT, "tests", "ref", "vectors", "i-slice2")
rng = random.Random(20260930)

P61 = 2 ** 61 - 1                      # a prime
P64 = 18446744073709551557             # the largest prime below 2^64


def dy(x):
    x = F(x)
    if x == 0:
        return [0, 0]
    assert R.is_dyadic(x)
    m, e = x.numerator, -(x.denominator.bit_length() - 1)
    while m % 2 == 0:
        m //= 2
        e += 1
    return [m, e]


def fr(x):
    x = F(x)
    return [x.numerator, x.denominator]


def rand_coprime(N):
    while True:
        c = rng.randrange(-3 * N, 3 * N + 1)
        if gcd(c, N) == 1:
            return c


COS = R.some_cosets(8) + [(5, 6), (2, 3), (5, 36), (7, 12), (11, 30), (3, 4), (1, 2), (13, 50)]
BIG_N = (rng.getrandbits(300) | 1) * 4
COS_BIG = [R.ref_uc_set(rand_coprime(BIG_N), BIG_N)[1] for _ in range(3)]


def rand_content():
    """Contents with many small prime factors, with the big primes, and huge ones."""
    k = rng.random()
    if k < 0.5:
        num = rng.choice([1, 2, 3, 4, 6, 8, 9, 12, 25, 27, 35, 49, 64, 121, 1024]) * rng.randint(1, 50)
        den = rng.choice([1, 2, 3, 5, 7, 16, 81, 11 * 13]) * rng.randint(1, 50)
    elif k < 0.7:
        num = P61 ** rng.randint(0, 3) * rng.randint(1, 1000)
        den = P64 ** rng.randint(0, 2) * rng.randint(1, 1000)
    elif k < 0.85:
        num = 3 ** rng.randint(0, 400) * rng.randint(1, 1000)
        den = 2 ** rng.randint(0, 600) * 5 ** rng.randint(0, 100)
    else:
        num, den = rng.getrandbits(rng.choice([64, 300])) + 1, rng.getrandbits(rng.choice([64, 300])) + 1
    return F(num, den)


def rand_idele():
    big = rng.random() < 0.1
    return (R.random_ball(rng, rng.choice([0, 0, 3, 30, 70, 200]), big), rand_content(),
            rng.choice(COS + COS_BIG))


def rand_class():
    big = rng.random() < 0.1
    m, rho = R.random_ball(rng, rng.choice([0, 0, 3, 30, 70, 200]), big)
    return ((abs(m), rho), rng.choice(COS + COS_BIG))


def ball_json(b):
    return {"mid": dy(b[0]), "rad": dy(b[1])}


def idele_json(x):
    d = ball_json(x[0])
    d["r"] = fr(x[1])
    d["u"] = list(x[2])
    return d


def class_json(c):
    d = ball_json(c[0])
    d["u"] = list(c[1])
    return d


def abs_ends(b):
    return abs(b[0]) - b[1], abs(b[0]) + b[1]


def kernel_fields(rec, res, L, H):
    st, ball, step, lo, hi, s = res
    rec.update({"status": st, "lo": dy(lo), "hi": dy(hi), "sign": s, "L": fr(L), "H": fr(H)})
    return st


PRECS = [0, 1, 2, 3, 5, 8, 16, 30, 53, 64, 100, 128, 256]


def idele_maps_records():
    recs = []
    xs = [rand_idele() for _ in range(260)]
    xs += [R.ref_idele_set_rat(F(-6, 35), 64)[1], ((F(3, 4), F(0)), F(3, 4), (1, 0)),
           ((F(-5, 2), F(1, 4)), F(3, 2), (5, 36)), (R.spec5_example(), F(1), (1, 0))]
    for x in xs:
        p = rng.choice(PRECS)
        # mul_rat
        q = F(rng.choice([-1, 1]) * rng.choice([1, 2, 3, 7, 1024, 3 ** 40, P61]) * rng.randint(1, 30),
              rng.choice([1, 2, 5, 8, 9, P64]) * rng.randint(1, 30))
        L, H = abs_ends(x[0])
        rec = {"op": "mul_rat", "x": idele_json(x), "q": fr(q), "prec": p}
        sq = 1 if q > 0 else -1
        st = kernel_fields(rec, R.ref_real_scale(x[0], abs(q.numerator), q.denominator, R.sgn(x[0]) * sq, p),
                           L * abs(q), H * abs(q))
        if st == R.OK:
            z = R.ref_idele_mul_rat(x, q, p)[1]
            rec["r"], rec["u"] = fr(z[1]), list(z[2])
        recs.append(rec)
        # norm
        p = rng.choice(PRECS)
        rec = {"op": "norm", "x": idele_json(x), "prec": p}
        kernel_fields(rec, R.ref_idele_norm(x, p), L / x[1], H / x[1])
        recs.append(rec)
        # valuations and absolute values at the primes of r, small primes, the big primes, and inf
        primes = set(R.trial_factor(x[1].numerator) if x[1].numerator < 10 ** 12 else {}) | {2, 3, 5, 7, 11}
        primes |= set(rng.sample([13, 101, 65537, P61, P64], 2))
        vals = []
        for pr in sorted(primes) + [0]:
            place = R.INF if pr == 0 else pr
            st, v = R.ref_idele_valuation_at(x, place)
            if st == R.OK:
                vals.append({"place": pr, "status": st, "v": v, "a": fr(R.ref_idele_abs_at(x, place)[1])})
            else:
                vals.append({"place": pr, "status": st})
        recs.append({"op": "valuation", "x": idele_json(x), "at": vals})
        a = R.ref_idele_abs_inf(x)
        recs.append({"op": "abs_inf", "x": idele_json(x), "mid": dy(a[0]), "rad": dy(a[1])})
    # mul_rat by 0
    recs.append({"op": "mul_rat", "x": idele_json(xs[0]), "q": [0, 1], "prec": 64, "status": R.NOT_UNIT})
    # the norm of the idele of a rational: contains 1; exactly 1 when the ball is exact
    for _ in range(40):
        q = F(rng.choice([-1, 1]) * rng.randint(1, 3000), rng.choice([1, 2, 8, 64, 3, 7, 35, 3000]))
        p = rng.choice(PRECS)
        x = R.ref_idele_set_rat(q, p)[1]
        L, H = abs_ends(x[0])
        rec = {"op": "norm", "x": idele_json(x), "prec": p}
        kernel_fields(rec, R.ref_idele_norm(x, p), L / x[1], H / x[1])
        recs.append(rec)
    return recs


def idclass_records():
    recs = []
    for _ in range(300):
        x = rand_idele()
        p = rng.choice(PRECS)
        L, H = abs_ends(x[0])
        rec = {"op": "class", "x": idele_json(x), "prec": p}
        if kernel_fields(rec, R.ref_idele_norm(x, p), L / x[1], H / x[1]) == R.OK:
            rec["u"] = list(R.ref_idele_class(x, p)[1][1])
        recs.append(rec)
    for x in [((F(1), F(0)), F(1), (-1, 0)), ((F(-1), F(0)), F(1), (-1, 0)),
              ((F(-5, 2), F(1, 4)), F(3, 2), (5, 36)), R.ref_idele_set_rat(F(-6, 35), 64)[1]]:
        L, H = abs_ends(x[0])
        rec = {"op": "class", "x": idele_json(x), "prec": 64}
        kernel_fields(rec, R.ref_idele_norm(x, 64), L / x[1], H / x[1])
        rec["u"] = list(R.ref_idele_class(x, 64)[1][1])
        recs.append(rec)
    spec = (R.spec5_example(), (1, 0))
    pairs = [(spec, spec, p) for p in (2, 30, 59, 60, 61, 62, 64, 128, 1024)]
    pairs += [(rand_class(), rand_class(), rng.choice(PRECS)) for _ in range(300)]
    for x, y, p in pairs:
        Lx, Hx = abs_ends(x[0])
        Ly, Hy = abs_ends(y[0])
        rec = {"op": "mul", "x": class_json(x), "y": class_json(y), "prec": p}
        if kernel_fields(rec, R.ref_real_mul(x[0], y[0], p), Lx * Ly, Hx * Hy) == R.OK:
            rec["u"] = list(R.ref_idclass_mul(x, y, p)[1][1])
        recs.append(rec)
        rec = {"op": "inv", "x": class_json(x), "prec": p}
        if kernel_fields(rec, R.ref_real_inv(x[0], p), 1 / Hx, 1 / Lx) == R.OK:
            rec["u"] = list(R.ref_idclass_inv(x, p)[1][1])
        recs.append(rec)
    return recs


def write(name, recs):
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, name)
    with open(path, "w") as f:
        for r in recs:
            f.write(json.dumps(r, separators=(",", ":")) + "\n")
    return path


def main():
    out = []
    for name, recs in (("idele_maps.jsonl", idele_maps_records()), ("idclass.jsonl", idclass_records())):
        path = write(name, recs)
        ops, nd = {}, 0
        for r in recs:
            ops[r["op"]] = ops.get(r["op"], 0) + 1
            nd += r.get("status") == R.NOT_DETERMINED
        out.append(f"{path}: {len(recs)} records, {nd} NOT_DETERMINED, {os.path.getsize(path)} bytes; by op {ops}")
    print("\n".join(out))


if __name__ == "__main__":
    main()
