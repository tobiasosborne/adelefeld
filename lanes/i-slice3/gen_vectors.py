#!/usr/bin/env python3
"""lanes/i-slice3/gen_vectors.py: the vectors of slice 3 of milestone 2 (docs/api-2.md section 3), written from the
reference functions of proto/ideles_checks.py (part 4), which that file checks against the table M_k of part 1,
enumeration in Z/M and exact points (python3 proto/ideles_checks.py part4).

Writes, deterministically (seed 20260930 + 3):
  tests/ref/vectors/i-slice3/idpow.jsonl  adf_ucoset_pow, _pow_tight; adf_idele_pow, _pow_tight;
                                          adf_idclass_pow, _pow_tight
  tests/ref/vectors/i-slice3/idmap.jsonl  adf_adele_set_idele, _set_idele_simple, adf_idele_set_adele,
                                          adf_adele_div_idele (finite part and status)

Encodings as in lanes/i-slice2/gen_vectors.py. A coset is [c, N]. A rational is [n, d] in lowest terms, d > 0. A
dyadic number is [m, e], the value m 2^e. A real ball is {"mid": dyadic, "rad": dyadic}; the radius mantissa has
at most 30 bits. An idele is {"mid", "rad", "r", "u"}; a class is {"mid", "rad", "u"}; an adele is
{"mid", "rad", "fin"} with "fin" the canonical triple [A, H, d] of its finite ball. An exponent "k" is an integer
from -2^63 to 2^63 - 1. Fields of a result: "status"; for a real part "lo", "hi" (the rounded ends, dyadic),
"sign", and "L", "H" (the exact ends of the absolute value of the result set, rationals; left out when |k| is
large, where only exact inputs +-1 are used); "r"; "u" (the default power) and "ut" (the tight power); "small",
"simple", "fin" (canonical triples).

Run: timeout 300 python3 lanes/i-slice3/gen_vectors.py
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

OUT = os.path.join(ROOT, "tests", "ref", "vectors", "i-slice3")
sys.set_int_max_str_digits(0)          # the exact ends of powers of balls of exponent 3000 have 10^4 digits
rng = random.Random(20260933)

WMIN, WMAX = -(1 << 63), (1 << 63) - 1
P61 = 2 ** 61 - 1


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


COS = R.some_cosets(12) + [(5, 36), (7, 60), (11, 30), (13, 50), (1, 2), (3, 4), (17, 96), (31, 210), (2, 1155)]
BIG_N = (rng.getrandbits(300) | 1) * 4
COS_BIG = [R.ref_uc_set(rand_coprime(BIG_N), BIG_N)[1] for _ in range(2)] + \
          [R.ref_uc_set(rand_coprime(BIG_N // 2), BIG_N // 2)[1]]


def rand_content():
    k = rng.random()
    if k < 0.6:
        return F(rng.choice([1, 2, 3, 4, 6, 9, 12, 25, 35]) * rng.randint(1, 30),
                 rng.choice([1, 2, 5, 7, 16, 81]) * rng.randint(1, 30))
    if k < 0.8:
        return F(1)
    return F(rng.getrandbits(rng.choice([64, 200])) + 1, rng.getrandbits(64) + 1)


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


def adele_json(ball, fin):
    d = ball_json(ball)
    d["fin"] = list(fin)
    return d


def pow_ends(ball, k):
    a0, a1 = abs(ball[0]) - ball[1], abs(ball[0]) + ball[1]
    return (F(a0) ** k, F(a1) ** k) if k > 0 else (F(a1) ** k, F(a0) ** k)


PRECS = [0, 1, 2, 3, 5, 8, 16, 30, 53, 64, 100, 128, 256]
KS_SMALL = [-7, -5, -4, -3, -2, -1, 1, 2, 3, 4, 5, 6, 8, 12, 13]
KS_BIG = [WMIN, WMAX, 1 << 62, -(1 << 62), 3 ** 39, 720720, -720720, 2 ** 40 * 3 ** 5 * 5 ** 3, 999999999989,
          P61, 3 * P61]


def uc_records():
    recs = []
    cosets = COS + COS_BIG
    for u in cosets:
        ks = [0, 1, -1] + rng.sample(KS_SMALL, 4) + rng.sample(KS_BIG, 2)
        for k in ks:
            recs.append({"op": "uc_pow", "u": list(u), "k": k, "pow": list(R.ref_uc_pow(u, k)),
                         "tight": list(R.ref_uc_pow_tight(u, k))})
    for k in KS_BIG + [rng.randrange(WMIN, WMAX) for _ in range(6)]:
        for u in [(1, 1), (5, 6), (3, 4), (2, 5), (1, 0), (-1, 0)]:
            recs.append({"op": "uc_pow", "u": list(u), "k": k, "pow": list(R.ref_uc_pow(u, k)),
                         "tight": list(R.ref_uc_pow_tight(u, k))})
    return recs


def kernel_fields(rec, res, ends):
    st, ball, step, lo, hi, s = res
    rec.update({"status": st, "lo": dy(lo), "hi": dy(hi), "sign": s})
    if ends is not None:
        rec["L"], rec["H"] = fr(ends[0]), fr(ends[1])
    return st


def idele_pow_records():
    recs = []
    for _ in range(420):
        big = rng.random() < 0.08
        x = (R.random_ball(rng, rng.choice([0, 0, 3, 20, 60]), big), rand_content(), rng.choice(COS + COS_BIG))
        k = rng.choice(KS_SMALL[4:11] if big else KS_SMALL)
        p = rng.choice(PRECS)
        rec = {"op": "idele_pow", "x": idele_json(x), "k": k, "prec": p}
        st = kernel_fields(rec, R.ref_real_pow(x[0], k, p), pow_ends(x[0], k))
        if st == R.OK:
            z = R.ref_idele_pow(x, k, p)[1]
            rec["r"], rec["u"], rec["ut"] = fr(z[1]), list(z[2]), list(R.ref_idele_pow(x, k, p, True)[1][2])
        recs.append(rec)
        # the class of the same ball (made positive)
        c = ((abs(x[0][0]), x[0][1]), x[2])
        rec = {"op": "idclass_pow", "x": class_json(c), "k": k, "prec": p}
        if kernel_fields(rec, R.ref_real_pow(c[0], k, p), pow_ends(c[0], k)) == R.OK:
            rec["u"] = list(R.ref_idclass_pow(c, k, p)[1][1])
            rec["ut"] = list(R.ref_idclass_pow(c, k, p, True)[1][1])
        recs.append(rec)
    # exact inputs: small integers and dyadics, the powers exact when they fit (Statement K.4)
    for m in (F(1), F(-1), F(2), F(-3), F(3, 4), F(-5, 8), F(7, 1024), F(-1, 2)):
        for k in (-3, -2, 2, 3, 5, 9):
            p = rng.choice([8, 16, 64])
            x = ((m, F(0)), F(3, 2), (5, 12))
            rec = {"op": "idele_pow", "x": idele_json(x), "k": k, "prec": p}
            if kernel_fields(rec, R.ref_real_pow(x[0], k, p), pow_ends(x[0], k)) == R.OK:
                z = R.ref_idele_pow(x, k, p)[1]
                rec["r"], rec["u"], rec["ut"] = fr(z[1]), list(z[2]), list(R.ref_idele_pow(x, k, p, True)[1][2])
            recs.append(rec)
    # huge exponents with the exact ball +-1 and content 1 (no limit); with content != 1: LIMIT
    for k in KS_BIG:
        for m in (F(1), F(-1)):
            u = rng.choice(COS)
            x = ((m, F(0)), F(1), u)
            p = rng.choice(PRECS)
            rec = {"op": "idele_pow", "x": idele_json(x), "k": k, "prec": p}
            assert kernel_fields(rec, R.ref_real_pow(x[0], k, p), (F(1), F(1))) == R.OK
            z = R.ref_idele_pow(x, k, p)[1]
            rec.update({"r": [1, 1], "u": list(z[2]), "ut": list(R.ref_idele_pow(x, k, p, True)[1][2])})
            recs.append(rec)
        x = ((F(-3), F(1)), F(3, 2), (5, 12))
        if abs(k) >= 1 << 30:
            recs.append({"op": "idele_pow", "x": idele_json(x), "k": k, "prec": 64, "status": R.LIMIT})
    # the limit of the content just above its edge: |k| (bits(n) + bits(d)) = ADF_IDELE_POW_BITS_MAX + 4 (the
    # edge itself, OK, is a hand case of tests/test_idpow.c: its content has 2^26 bits)
    r = F(3, 2)                                     # bits 2 + 2 = 4
    for k in (R.IDELE_POW_BITS_MAX // 4 + 1, -(R.IDELE_POW_BITS_MAX // 4 + 1)):
        assert R.pow_bits_exceeded(r, k) and not R.pow_bits_exceeded(r, k - (1 if k > 0 else -1))
        recs.append({"op": "idele_pow", "x": idele_json(((F(-3), F(1)), r, (5, 12))), "k": k, "prec": 64,
                     "status": R.LIMIT})
    for p in (R.IDELE_PREC_MAX + 1, WMAX):
        recs.append({"op": "idele_pow", "x": idele_json(((F(-3), F(1)), r, (5, 12))), "k": 0, "prec": p,
                     "status": R.LIMIT})
        recs.append({"op": "idclass_pow", "x": class_json(((F(3), F(1)), (5, 12))), "k": 2, "prec": p,
                     "status": R.LIMIT})
    # k = 0: the exact idele 1 and the exact class 1, whatever the input
    for _ in range(6):
        x = (R.random_ball(rng, rng.choice([0, 3, 60])), rand_content(), rng.choice(COS))
        recs.append({"op": "idele_pow", "x": idele_json(x), "k": 0, "prec": rng.choice(PRECS), "status": R.OK,
                     "lo": dy(1), "hi": dy(1), "sign": 1, "L": [1, 1], "H": [1, 1], "r": [1, 1], "u": [1, 0],
                     "ut": [1, 0]})
    return recs


def idmap_records():
    recs = []
    for _ in range(300):
        x = (R.random_ball(rng, rng.choice([0, 0, 3, 20]), rng.random() < 0.05), rand_content(),
             rng.choice(COS + COS_BIG))
        recs.append({"op": "hull", "x": idele_json(x), "small": list(R.ref_hull(x)[1]),
                     "simple": list(R.ref_hull_simple(x)[1])})
    # adele to idele: exact finite parts of both signs, balls, zeros, real balls with and without 0
    fins = [R.fb(F(-3, 4), 0), R.fb(F(35, 6), 0), R.fb(F(1), 0), R.fb(0, 0), R.fb(1, 6), R.fb(F(1, 2), 1),
            R.fb(F(-7, 3), F(5, 9)), R.fb(F(2) ** 300 + 1, 0), R.fb(F(-1, 3 ** 50), 0)]
    balls = [(F(2), F(1)), (F(-1), F(1, 2)), (F(0), F(0)), (F(0), F(1)), (F(1), F(1)), (F(-5, 8), F(0)),
             (F(3, 2), F(1, 2)), (F(1), F(1) - F(1, 1 << 30))]
    for fin in fins:
        for ball in balls:
            st, y = R.ref_idele_set_adele(ball, fin)
            rec = {"op": "set_adele", "x": adele_json(ball, fin), "status": st}
            if st == R.OK:
                rec["r"], rec["u"] = fr(y[1]), list(y[2])
            recs.append(rec)
    # division of an adele by an idele: the finite part by the formula of P19 / P18
    for _ in range(400):
        a = F(rng.randint(-40, 40), rng.randint(1, 12))
        M = F(rng.randint(0, 30), rng.randint(1, 6)) * rng.choice([1, 1, 1, 0])
        if rng.random() < 0.05:
            a, M = F(rng.getrandbits(200), rng.getrandbits(80) + 1), F(rng.getrandbits(100) + 1, 3)
        fin = R.fb(a, M)
        xball = R.random_ball(rng, rng.choice([0, 3, 20]))
        if rng.random() < 0.2:
            xball = (F(rng.randint(-5, 5), 4), F(rng.randint(0, 9), 8))          # may contain 0, or be 0
        y = (R.random_ball(rng, rng.choice([0, 0, 3, 20])), rand_content(), rng.choice(COS + COS_BIG))
        p = rng.choice(PRECS)
        recs.append({"op": "div", "x": adele_json(xball, fin), "y": idele_json(y), "prec": p, "status": R.OK,
                     "fin": list(R.ref_div_fin(fin, y))})
    for p in (R.IDELE_PREC_MAX + 1, WMAX):
        y = ((F(3), F(0)), F(2), (5, 12))
        recs.append({"op": "div", "x": adele_json((F(1), F(0)), R.fb(1, 4)), "y": idele_json(y), "prec": p,
                     "status": R.LIMIT})
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
    for name, recs in (("idpow.jsonl", uc_records() + idele_pow_records()), ("idmap.jsonl", idmap_records())):
        path = write(name, recs)
        ops, sts = {}, {}
        for r in recs:
            ops[r["op"]] = ops.get(r["op"], 0) + 1
            if "status" in r:
                sts[r["status"]] = sts.get(r["status"], 0) + 1
        out.append(f"{path}: {len(recs)} records, {os.path.getsize(path)} bytes; by op {ops}; statuses {sts}")
    print("\n".join(out))


if __name__ == "__main__":
    main()
