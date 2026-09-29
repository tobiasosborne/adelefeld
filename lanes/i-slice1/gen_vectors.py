#!/usr/bin/env python3
"""lanes/i-slice1/gen_vectors.py: the vectors of slice 1 of milestone 2 (docs/api-2.md section 1), written from
the reference functions ref_* of proto/ideles_checks.py (part 2), which that file checks against enumeration and
exact end points (python3 proto/ideles_checks.py part2).

Writes, deterministically (seed 20260929):
  tests/ref/vectors/i-slice1/ucoset.jsonl   adf_ucoset: set, normal, mul, inv, predicates
  tests/ref/vectors/i-slice1/idele.jsonl    adf_idele: set_rat, mul, inv

Encodings. A coset is [c, N]. A rational is [n, d] in lowest terms, d > 0. A dyadic number is [m, e], the
value m 2^e (m an integer, 0 for zero). A real ball is {"mid": dyadic, "rad": dyadic}; the radius mantissa has
at most 30 bits, so that mag_set_ui_2exp_si can hold it exactly (the C test checks that it did).
An idele is {"mid", "rad", "r", "u"}. Fields of a result: "status" (OK, NOT_DETERMINED, NOT_UNIT, DOMAIN);
for the real kernel "lo", "hi" (the rounded end points of Statement E, dyadic), "sign", and "L", "H" (the exact
end points of the absolute value of the result set, rationals); the finite part "r", "u".

Run: timeout 300 python3 lanes/i-slice1/gen_vectors.py
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

OUT = os.path.join(ROOT, "tests", "ref", "vectors", "i-slice1")
rng = random.Random(20260929)


def dy(x):
    """A dyadic Fraction as [m, e] with m odd (or 0)."""
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


SMALL = R.some_cosets(8) + [(5, 6), (2, 3), (5, 36), (7, 12), (11, 30), (3, 4), (1, 2), (13, 50)]


HUGE_A = rng.getrandbits(900) | 1


def huge_cosets():
    """Twelve cosets with moduli of 1000 to 2000 bits that share the factor HUGE_A of 900 bits."""
    out = []
    for _ in range(12):
        B = rng.getrandbits(rng.choice([64, 200, 1100])) | 1
        N = HUGE_A * B * rng.choice([1, 2, 4, 8, 6])
        out.append(R.ref_uc_set(rand_coprime(N), N)[1])
    return out


HUGE = huge_cosets()


def ucoset_records():
    recs = []
    # set: raw pairs, valid and invalid
    raw = [(c, N) for N in range(0, 13) for c in range(-13, 14)]
    raw += [(-1, 6), (0, 1), (3, 2), (13, 12), (-5, 12), (1, 0), (-1, 0), (5, 0), (0, 0), (2, 0), (-2, 0),
            (1, -1), (5, -6), (0, -1)]
    big = rng.getrandbits(3000) | 1
    raw += [(1, 2 * big), (-1, 2 * big), (big + 2, big), (-(big ** 2) - 1, big), (2, 4 * big), (big, big),
            (-big, big * 2 + 1), (3, big * 3)]
    for c, N in raw:
        st, v = R.ref_uc_set(c, N)
        rec = {"op": "set", "c": c, "N": N, "status": st}
        if st == R.OK:
            rec["z"] = list(v)
            rec["normal"] = list(R.ref_uc_normal(v))
        recs.append(rec)
    for u in SMALL + HUGE:
        recs.append({"op": "inv", "x": list(u), "z": list(R.ref_uc_inv(u))})
    pairs = [(u, v) for u in SMALL for v in SMALL] + [(u, v) for u in HUGE for v in HUGE]
    pairs += [(u, v) for u in HUGE for v in rng.sample(SMALL, 5)]
    pairs += [(v, u) for u in HUGE for v in rng.sample(SMALL, 5)]
    # huge cosets with a coarser one of the same residue (inside, overlapping) and of another residue
    for u in HUGE:
        c, N = u
        for M in (HUGE_A, 2 * HUGE_A if N % 2 == 0 else HUGE_A, N // 2 if N % 4 == 2 else N):
            pairs.append((u, R.ref_uc_set(c, M)[1]))
            pairs.append((R.ref_uc_set(c, M)[1], u))
            if gcd(c + 2, M) == 1:
                pairs.append((u, R.ref_uc_set(c + 2, M)[1]))
    for u, v in pairs:
        recs.append({"op": "mul", "x": list(u), "y": list(v), "z": list(R.ref_uc_mul(u, v)),
                     "equal_set": R.ref_uc_equal_set(u, v), "contains": R.ref_uc_contains(u, v),
                     "overlaps": R.ref_uc_overlaps(u, v)})
    return recs


def ball_json(b):
    return {"mid": dy(b[0]), "rad": dy(b[1])}


def exact_ends_mul(x, y):
    ax = (abs(x[0]) - x[1], abs(x[0]) + x[1])
    ay = (abs(y[0]) - y[1], abs(y[0]) + y[1])
    return ax[0] * ay[0], ax[1] * ay[1]


def exact_ends_inv(x):
    return 1 / (abs(x[0]) + x[1]), 1 / (abs(x[0]) - x[1])


def rand_content():
    k = rng.choice([8, 40, 300])
    return F(rng.getrandbits(k) + 1, rng.getrandbits(k) + 1)


def rand_idele(cos):
    big = rng.random() < 0.15
    return (R.random_ball(rng, rng.choice([0, 0, 3, 30, 70, 200]), big), rand_content(), rng.choice(cos))


def idele_json(x):
    d = ball_json(x[0])
    d["r"] = fr(x[1])
    d["u"] = list(x[2])
    return d


def idele_records():
    recs = []
    precs = [0, 1, 2, 3, 5, 8, 16, 30, 53, 64, 100, 128, 256]
    # set_rat
    qs = [F(1), F(-1), F(2), F(-3, 2), F(1, 3), F(-1, 3), F(5), F(10 ** 40 + 1, 7), F(-(2 ** 200) - 1, 2 ** 90),
          F(3, 2 ** 1000), F(0), F(2 ** 3000 + 1, 3 ** 1200)]
    qs += [F(rng.getrandbits(rng.choice([4, 60, 500])) + 1, rng.getrandbits(rng.choice([4, 60, 500])) + 1)
           * rng.choice([1, -1]) for _ in range(40)]
    for q in qs:
        for p in rng.sample(precs, 4) + [2]:
            rec = {"op": "set_rat", "q": fr(q), "prec": p}
            if q == 0:
                rec["status"] = R.NOT_UNIT
            else:
                st, ball, step, lo, hi, s = R.ref_real_rat(q, p)
                rec.update({"status": st, "lo": dy(lo), "hi": dy(hi), "sign": s, "L": fr(abs(q)), "H": fr(abs(q)),
                            "r": fr(abs(q)), "u": [s, 0]})
            recs.append(rec)
    cos = SMALL + HUGE[:4]
    spec = (R.spec5_example(), F(1), (1, 0))
    pairs = [(spec, spec, p) for p in (2, 30, 59, 60, 61, 62, 64, 128, 1024)]
    for _ in range(700):
        pairs.append((rand_idele(cos), rand_idele(cos), rng.choice(precs)))
    # exact inputs, some whose product fits in p bits
    for _ in range(40):
        a = F(rng.randint(1, 1 << 20) * rng.choice([1, -1]), 1 << rng.randint(0, 20))
        b = F(rng.randint(1, 1 << 20) * rng.choice([1, -1]), 1 << rng.randint(0, 20))
        pairs.append((((a, F(0)), rand_content(), rng.choice(cos)), ((b, F(0)), rand_content(), rng.choice(cos)),
                      rng.choice([16, 30, 45, 64])))
    n_nd = 0
    for x, y, p in pairs:
        st, ball, step, lo, hi, s = R.ref_real_mul(x[0], y[0], p)
        L, H = exact_ends_mul(x[0], y[0])
        rec = {"op": "mul", "x": idele_json(x), "y": idele_json(y), "prec": p, "status": st, "lo": dy(lo),
               "hi": dy(hi), "sign": s, "L": fr(L), "H": fr(H)}
        if st == R.OK:
            stz, z = R.ref_idele_mul(x, y, p)
            rec["r"] = fr(z[1])
            rec["u"] = list(z[2])
        else:
            n_nd += 1
        recs.append(rec)
        st, ball, step, lo, hi, s = R.ref_real_inv(x[0], p)
        L, H = exact_ends_inv(x[0])
        rec = {"op": "inv", "x": idele_json(x), "prec": p, "status": st, "lo": dy(lo), "hi": dy(hi), "sign": s,
               "L": fr(L), "H": fr(H)}
        if st == R.OK:
            stz, z = R.ref_idele_inv(x, p)
            rec["r"] = fr(z[1])
            rec["u"] = list(z[2])
        else:
            n_nd += 1
        recs.append(rec)
    return recs, n_nd


def write(name, recs):
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, name)
    with open(path, "w") as f:
        for r in recs:
            f.write(json.dumps(r, separators=(",", ":")) + "\n")
    return path


def main():
    u = ucoset_records()
    pu = write("ucoset.jsonl", u)
    i, n_nd = idele_records()
    pi = write("idele.jsonl", i)
    ops = {}
    for r in u + i:
        ops[r["op"]] = ops.get(r["op"], 0) + 1
    print(f"{pu}: {len(u)} records; {pi}: {len(i)} records, {n_nd} NOT_DETERMINED; by op {ops}")


if __name__ == "__main__":
    main()
