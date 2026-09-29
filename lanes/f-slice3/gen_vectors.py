"""Generates tests/ref/vectors/f-slice3/lball_slice3.jsonl for tests/test_lball_decomp.c (lane f-slice3).

Run from the repository root:  python3 -B lanes/f-slice3/gen_vectors.py

The reference is the section "f-slice3" at the end of proto/functions_checks.py (statements L9 to L13 of
docs/api-1f.md), which is itself checked by enumeration and by three computations of the Teichmueller representative
(python3 -B lanes/f-slice3/run_python_checks.py). The seeds are fixed; the file is reproduced byte for byte by a second
run. Format: a value is {"p", "exact", "un", "ud", "v", "N"} (docs/conventions.md 5.8, u = un/ud); a rational is
{"num", "den"}; one object per line with "op" in teich, split, frac, unit_mod, pow and the fields named below.
"""

import json
import random
import sys
from fractions import Fraction as F
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "proto"))
import functions_checks as R  # noqa: E402

OUT = ROOT / "tests" / "ref" / "vectors" / "f-slice3"
BIG = 2 ** 64 - 59
PRIMES = (2, 3, 5, 7, 11, BIG)
KMAX = 4


def rat(q):
    q = F(q)
    return {"num": q.numerator, "den": q.denominator}


def rand_unit_int(rng, p, k):
    while True:
        u = rng.randrange(1, p ** k)
        if u % p:
            return u


def rand_value(rng, p, vmax=3):
    r = rng.random()
    v = rng.randint(-vmax, vmax)
    if r < 0.65:
        k = rng.randint(1, KMAX if p < 100 else 2)
        return R.LB(p, 0, rand_unit_int(rng, p, k), v, v + k)
    if r < 0.72:
        return R.LB(p, 0, 0, 0, rng.randint(-vmax, KMAX))
    if r < 0.76:
        return R.lb_exact(p, 0)
    a, b = rng.randint(1, 40), rng.randint(1, 40)
    if a % p == 0 or b % p == 0:
        a, b = 1, 3 if p != 3 else 2
    return R.lb_exact(p, F(rng.choice((1, -1)) * a, b) * F(p) ** v)


def write(name, rows):
    OUT.mkdir(parents=True, exist_ok=True)
    with open(OUT / name, "w") as fh:
        for row in rows:
            fh.write(json.dumps(row, sort_keys=True) + "\n")
    return len(rows)


def rows_all():
    rng = random.Random(20260930)
    rows = []
    for p in PRIMES:
        for _ in range(60):
            r = rng.randrange(0, 4 * p if p < 100 else p)
            prec = rng.choice((-1, 0, 1, 2, 3, 5, 8))
            st, w = R.s3_teich(p, r, prec)
            row = {"op": "teich", "p": p, "r": r, "prec": prec, "status": st}
            if w is not None:
                row["w"] = w.json()
            rows.append(row)
        for _ in range(120):
            x = rand_value(rng, p)
            prec = rng.choice((-2, 1, 1, 2, 4, 7))
            st, m, idx, w, u = R.s3_split(x, prec)
            row = {"op": "split", "x": x.json(), "prec": prec, "status": st}
            if st == "OK":
                row.update({"m": m, "index": idx, "w": w.json(), "u": u.json()})
            rows.append(row)
        for _ in range(80):
            x = rand_value(rng, p, vmax=4)
            st, r = R.s3_frac(x)
            row = {"op": "frac", "x": x.json(), "status": st}
            if r is not None:
                row["r"] = rat(r)
            rows.append(row)
        for _ in range(80):
            x = rand_value(rng, p)
            k = rng.randint(-1, 5)
            st, o = R.s3_unit_mod(x, k)
            row = {"op": "unit_mod", "x": x.json(), "k": k, "status": st}
            if o is not None:
                row["out"] = o
            rows.append(row)
        for _ in range(160):
            x = rand_value(rng, p)
            k = rng.choice((-12, -9, -7, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 16, 25, 30, 49))
            st, y = R.s3_pow(x, k)
            row = {"op": "pow", "x": x.json(), "k": k, "status": st}
            if y is not None:
                row["result"] = y.json()
            rows.append(row)
    return rows


if __name__ == "__main__":
    n = write("lball_slice3.jsonl", rows_all())
    print(f"lball_slice3.jsonl: {n} lines")
