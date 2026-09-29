"""Generates tests/ref/vectors/f-slice1/*.jsonl for tests/test_lball.c (lane f-slice1).

Run from the repository root:  python3 -B lanes/f-slice1/gen_vectors.py

The reference is the section "f-slice1" at the end of proto/functions_checks.py (statements L0 to L8 of
docs/api-1f.md). Every binary and unary arithmetic vector at a prime up to 11 is also checked by enumeration
(lb_enum_check: all results of the points modulo p^(K + 1) lie in the result ball and fill its p classes). At the
prime 2^64 - 59 the enumeration is not possible; there a sample of points (random values of a, 0 included) is
checked to lie in the result ball, which tests enclosure and not tightness. The seeds are fixed; the file is
reproduced byte for byte by a second run.

Vector format (tests/ref/README.md style). A value is {"p", "exact", "un", "ud", "v", "N"}: the canonical fields
of docs/conventions.md 5.8 with u = un/ud. A rational is {"num", "den"}.
"""

import json
import random
import sys
from fractions import Fraction as F
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "proto"))
import functions_checks as R  # noqa: E402

OUT = ROOT / "tests" / "ref" / "vectors" / "f-slice1"
BIG = 2 ** 64 - 59
PRIMES = (2, 3, 5, 7, 11, BIG)


def rat(q):
    q = F(q)
    return {"num": q.numerator, "den": q.denominator}


def rand_unit_int(rng, p, k):
    while True:
        u = rng.randrange(1, p ** k)
        if u % p:
            return u


def rand_value(rng, p, kmax=4, vmax=3):
    """A random canonical value: a ball of relative precision 1..kmax, a ball around 0, an exact rational."""
    r = rng.random()
    v = rng.randint(-vmax, vmax)
    if r < 0.60:
        k = rng.randint(1, kmax)
        return R.LB(p, 0, rand_unit_int(rng, p, k), v, v + k)
    if r < 0.70:
        return R.LB(p, 0, 0, 0, rng.randint(-vmax, kmax))
    if r < 0.75:
        return R.lb_exact(p, 0)
    a, b = rng.randint(1, 40), rng.randint(1, 40)
    if a % p == 0 or b % p == 0:
        a, b = 1, 3 if p != 3 else 2
    return R.lb_exact(p, F(rng.choice((1, -1)) * a, b) * F(p) ** v)


def related(rng, x):
    """A value near x: same valuation, same or nearby precision (to hit the interesting cases of add and mul)."""
    p = x.p
    if x.exact or x.u == 0 or rng.random() < 0.3:
        return rand_value(rng, p)
    k = max(1, x.N - x.v + rng.randint(-1, 1))
    return R.LB(p, 0, rand_unit_int(rng, p, k), x.v, x.v + k)


def check_sampled(op, x, y, ref):
    """For the big prime: 20 random points of the operands give results inside the reference ball."""
    rng = random.Random(99)
    p = x.p

    def pts(z):
        if z.exact:
            return [R.lb_val(z)]
        return [R.lb_val(z)] + [R.lb_val(z) + F(p) ** z.N * rng.randrange(p) for _ in range(6)]

    if isinstance(ref, str) or ref.exact:
        return
    c = R.lb_val(ref)
    for s in pts(x):
        for t in (pts(y) if y is not None else [None]):
            if op == "add":
                r = s + t
            elif op == "sub":
                r = s - t
            elif op == "mul":
                r = s * t
            elif op == "div":
                if t == 0:
                    continue
                r = s / t
            elif op == "neg":
                r = -s
            else:
                if s == 0:
                    continue
                r = 1 / s
            assert r == c or R.vp(r - c, p) >= ref.N, (op, x, y, ref)


def enum_or_sample(op, x, y, ref):
    if x.p <= 11:
        got = R.lb_enum_check(op, x, y)
        assert got == ref or (isinstance(got, str) and got == ref), (op, x, y, got, ref)
    else:
        check_sampled(op, x, y, ref)


def status_of(ref):
    return ref if isinstance(ref, str) else "OK"


def write(name, rows):
    OUT.mkdir(parents=True, exist_ok=True)
    with open(OUT / name, "w") as fh:
        for row in rows:
            fh.write(json.dumps(row, sort_keys=True) + "\n")
    return len(rows)


def binary_rows():
    rng = random.Random(20260929)
    rows = []
    fn = {"add": R.lb_ref_add, "sub": R.lb_ref_sub, "mul": R.lb_ref_mul, "div": R.lb_ref_div}
    for p in PRIMES:
        for op in fn:
            for i in range(140 if p <= 11 else 200):
                x = rand_value(rng, p)
                y = related(rng, x) if i % 2 else rand_value(rng, p)
                ref = fn[op](x, y)
                enum_or_sample(op, x, y, ref)
                row = {"op": op, "p": p, "x": x.json(), "y": y.json(), "status": status_of(ref)}
                if not isinstance(ref, str):
                    row["result"] = ref.json()
                rows.append(row)
    # every exact-zero, ball-around-zero and exact pair at small primes, all four operations
    for p in (2, 3):
        special = [R.lb_exact(p, 0), R.LB(p, 0, 0, 0, -1), R.LB(p, 0, 0, 0, 0), R.LB(p, 0, 0, 0, 3),
                   R.lb_exact(p, 1), R.lb_exact(p, F(1, p)), R.lb_exact(p, F(p ** 3)),
                   R.LB(p, 0, 1, 0, 1), R.LB(p, 0, 1, 2, 3), R.LB(p, 0, 1, -2, 2)]
        for x in special:
            for y in special:
                for op in fn:
                    ref = fn[op](x, y)
                    enum_or_sample(op, x, y, ref)
                    row = {"op": op, "p": p, "x": x.json(), "y": y.json(), "status": status_of(ref)}
                    if not isinstance(ref, str):
                        row["result"] = ref.json()
                    rows.append(row)
    # centres of thousands of bits: p = 5, relative precision 900 to 1500 (u about 2100 to 3500 bits), and 3 and
    # 2^64 - 59 at 200 and 60 digits
    for p, kmin, kmax, count in ((5, 900, 1500, 12), (3, 1500, 2200, 6), (BIG, 40, 60, 6), (2, 3000, 4000, 6)):
        for _ in range(count):
            v1, v2 = rng.randint(-3, 3), rng.randint(-3, 3)
            k1, k2 = rng.randint(kmin, kmax), rng.randint(kmin, kmax)
            x = R.LB(p, 0, rand_unit_int(rng, p, k1), v1, v1 + k1)
            y = R.LB(p, 0, rand_unit_int(rng, p, k2), v2, v2 + k2)
            for op in fn:
                ref = fn[op](x, y)
                row = {"op": op, "p": p, "x": x.json(), "y": y.json(), "status": status_of(ref)}
                if not isinstance(ref, str):
                    row["result"] = ref.json()
                rows.append(row)
    return rows


def unary_rows():
    rng = random.Random(20260930)
    rows = []
    vals = []
    for p in PRIMES:
        vals += [rand_value(rng, p) for _ in range(150)]
    for p in (2, 3, 5):
        vals += R.lb_universe(p, 2, 2 if p == 5 else 3)
    for x in vals:
        for op, fn in (("neg", R.lb_ref_neg), ("inv", R.lb_ref_inv)):
            ref = fn(x)
            enum_or_sample(op, x, None, ref)
            row = {"op": op, "p": x.p, "x": x.json(), "status": status_of(ref)}
            if not isinstance(ref, str):
                row["result"] = ref.json()
            rows.append(row)
        st, v, inf_ = R.lb_ref_valuation(x)
        row = {"op": "valuation", "p": x.p, "x": x.json(), "status": st}
        if st == "OK":
            row["v"], row["is_inf"] = v, inf_
        rows.append(row)
        st, a = R.lb_ref_abs(x)
        row = {"op": "abs", "p": x.p, "x": x.json(), "status": st}
        if st == "OK":
            row["result"] = rat(a)
        rows.append(row)
        st, m, unit = R.lb_ref_decompose(x)
        row = {"op": "decompose", "p": x.p, "x": x.json(), "status": st}
        if st == "OK":
            row["m"], row["unit"] = m, unit.json()
        rows.append(row)
    for p, k in ((5, 1200), (3, 1700), (2, 3500), (BIG, 50)):
        v = rng.randint(-3, 3)
        x = R.LB(p, 0, rand_unit_int(rng, p, k), v, v + k)
        for op, fn in (("neg", R.lb_ref_neg), ("inv", R.lb_ref_inv)):
            ref = fn(x)
            rows.append({"op": op, "p": p, "x": x.json(), "status": "OK", "result": ref.json()})
        st, m, unit = R.lb_ref_decompose(x)
        rows.append({"op": "decompose", "p": p, "x": x.json(), "status": st, "m": m, "unit": unit.json()})
    return rows


def predicate_rows():
    rng = random.Random(20260931)
    rows = []
    fns = {"equal_set": R.lb_ref_equal_set, "overlaps": R.lb_ref_overlaps, "contains": R.lb_ref_contains}
    for p in PRIMES:
        for i in range(300):
            x = rand_value(rng, p)
            y = related(rng, x) if i % 3 else rand_value(rng, p)
            if i % 7 == 0:
                y = R.LB(p, x.exact, x.u, x.v, x.N)          # equal
            if i % 11 == 0 and not x.exact and x.u != 0 and x.N - x.v > 1:
                # a smaller ball inside x, and the ball x extended by one digit
                k = x.N - x.v + 1
                y = R.LB(p, 0, x.u + p ** (k - 1) * rng.randrange(p), x.v, x.v + k)
            for op, fn in fns.items():
                rows.append({"op": op, "p": p, "x": x.json(), "y": y.json(), "result": bool(fn(x, y))})
                rows.append({"op": op, "p": p, "x": y.json(), "y": x.json(), "result": bool(fn(y, x))})
    x, y = R.LB(2, 0, 1, 0, 3), R.LB(3, 0, 1, 0, 3)          # two primes
    for op in fns:
        rows.append({"op": op, "p": 2, "x": x.json(), "y": y.json(), "result": False})
    return rows


def construct_rows():
    rng = random.Random(20260932)
    rows = []
    for p in PRIMES:
        for _ in range(120):
            q = F(rng.randint(-10 ** 5, 10 ** 5), rng.randint(1, 10 ** 3)) * F(p) ** rng.randint(-3, 3)
            rows.append({"op": "set_rat", "p": p, "q": rat(q), "status": "OK", "result": R.lb_exact(p, q).json()})
            N = rng.randint(-4, 8)
            rows.append({"op": "set_rat_ball", "p": p, "q": rat(q), "N": N, "status": "OK",
                         "result": R.lb_ball(p, q, N).json()})
        rows.append({"op": "set_rat", "p": p, "q": rat(0), "status": "OK", "result": R.lb_exact(p, 0).json()})
        for N in (-3, 0, 5):
            rows.append({"op": "set_rat_ball", "p": p, "q": rat(0), "N": N, "status": "OK",
                         "result": R.lb_ball(p, 0, N).json()})
        # the example of the brief: 1/3 at p = 5 to precision 10
        if p == 5:
            rows.append({"op": "set_rat_ball", "p": 5, "q": rat(F(1, 3)), "N": 10, "status": "OK",
                         "result": R.lb_ball(5, F(1, 3), 10).json()})
        for _ in range(120):
            d = rng.randint(1, 60)
            H = rng.choice([0, rng.randint(1, 800)])
            A = rng.randint(-100, 100)
            from math import gcd
            g = gcd(gcd(A, H), d)
            A, H, d = A // g, H // g, d // g
            if H > 0:
                A %= H
            rows.append({"op": "set_fball", "p": p, "A": A, "H": H, "d": d, "status": "OK",
                         "result": R.lb_ref_project(p, A, H, d).json()})
    return rows


def main():
    n = {}
    n["lball_binary.jsonl"] = write("lball_binary.jsonl", binary_rows())
    n["lball_unary.jsonl"] = write("lball_unary.jsonl", unary_rows())
    n["lball_predicates.jsonl"] = write("lball_predicates.jsonl", predicate_rows())
    n["lball_construct.jsonl"] = write("lball_construct.jsonl", construct_rows())
    for k, v in n.items():
        print(f"{k}: {v} lines")


if __name__ == "__main__":
    main()
