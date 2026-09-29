"""Generates tests/ref/vectors/f-slice2/*.jsonl for tests/test_sball.c and tests/test_rfunc.c (lane f-slice2).

Run from the repository root:  python3 -B lanes/f-slice2/gen_vectors.py

The reference is the section "f-slice2" at the end of proto/functions_checks.py (statements S1 to S7 of
docs/api-1f.md) and, for the components at a prime, the section "f-slice1" (L0 to L8). The real functions are
enclosed by mpmath intervals at 800 bits (exp, log, sin, cos) and by exact integer roots (sqrt, root): the C
library is not used and neither is arb. The seeds are fixed; a second run reproduces the files byte for byte.

Vector format (tests/ref/README.md style).
  real ball   {"m", "e", "rm", "re"}: the interval [m 2^e - rm 2^re, m 2^e + rm 2^re], rm < 2^30.
  dyadic      {"m", "e"}: m 2^e.
  lball       {"p", "exact", "un", "ud", "v", "N"} as in f-slice1.
  sball       {"arch": 0 or 1, "inf": real ball or null, "loc": [lball, ...] with increasing primes}.
  rfunc_real.jsonl   {"f", "n", "prec", "x", "status"} and, when the status is OK, "lo", "hi": dyadics of 400 bits,
                     rounded outward, with lo <= min of f over x, max of f over x <= hi (the enclosure itself is
                     computed at 800 bits); "tight": 1 if the radius of the result must be within a factor 4 of
                     hi - lo (plus rounding).
  sball_project.jsonl  {"x": {"inf": real ball, "A", "H", "d"}, "places": ["inf" | prime, ...] in any order,
                     "status", and "result" (sball) or "where" (the repeated place, "inf" or a prime)}.
  sball_ops.jsonl    {"op", "prec", "x", "y", "status"} and "result": {"arch", "real": {"lo", "hi"} or null,
                     "loc": [lball]} (real: the exact set of results, dyadics) or "where" for DOMAIN.
  sball_pred.jsonl   {"op": "equal_set" | "overlaps" | "contains", "x", "y", "want": 0 or 1}.
"""

import json
import random
import sys
from fractions import Fraction as F
from math import gcd
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "proto"))
sys.path.insert(0, str(ROOT / "lanes" / "f-slice1"))
import functions_checks as R  # noqa: E402
import gen_vectors as G1  # noqa: E402

OUT = ROOT / "tests" / "ref" / "vectors" / "f-slice2"
BIG = 2 ** 64 - 59
PRECS = (-3, 0, 1, 2, 3, 10, 24, 53, 64, 100, 128, 256, 300)


def write(name, rows):
    OUT.mkdir(parents=True, exist_ok=True)
    with open(OUT / name, "w") as fh:
        for row in rows:
            fh.write(json.dumps(row, sort_keys=True, separators=(",", ":")) + "\n")
    return len(rows)


# ------------------------------------------------------------------------------------------- real functions

def ball_around(rng, m, e, rel):
    """A ball with midpoint m 2^e and a radius about |mid| rel (0: exact); rel is a Fraction."""
    mid = R.dy(m, e)
    target = abs(mid) * rel
    if target == 0:
        return R.RB(m, e, 0, 0)
    re = target.numerator.bit_length() - target.denominator.bit_length() - 22
    rm = int(target / F(2) ** re)
    assert 0 < rm < 2 ** 30
    return R.RB(m, e, rm, re)


def rel_choice(rng, wide=False):
    return rng.choice((F(0), F(1, 2 ** 40), F(1, 1000), F(1, 64), F(1, 16), F(1, 4), F(1, 2))
                      + ((F(3, 4), F(99, 100), F(3, 2)) if wide else ()))


def rand_mid(rng, emin, emax, mbits=20):
    m = rng.randint(1, 2 ** mbits)
    return m, rng.randint(emin, emax)


def real_rows():
    rng = random.Random(20260929)
    rows = []

    def add(f, n, rb, precs=None):
        for prec in (precs or [rng.choice(PRECS)]):
            rows.append(R.rf_vector(f, n, prec, rb))

    for i in range(110):                                   # exp
        m, e = rand_mid(rng, -18, -3, 15)
        m *= rng.choice((1, -1))
        add("exp", 0, ball_around(rng, m, e, rel_choice(rng)))
    for m, e, rm, re in ((0, 0, 0, 0), (1, 0, 0, 0), (-1, 0, 0, 0), (1, 12, 0, 0), (-1, 12, 0, 0), (0, 0, 1, 0),
                         (0, 0, 1, -10), (1, 8, 3, 2), (3, 8, 0, 0), (-5, 9, 1, 0), (1, 10, 1, 0)):
        add("exp", 0, R.RB(m, e, rm, re), (2, 64, 300, 0))
    for i in range(90):                                    # log
        m, e = rand_mid(rng, -30, 30)
        add("log", 0, ball_around(rng, m, e, rel_choice(rng)))
    for i in range(20):
        m, e = rand_mid(rng, -6, 6)
        add("log", 0, ball_around(rng, m, e, rng.choice((F(3, 4), F(99, 100), F(1), F(3, 2), F(3)))))
        add("log", 0, ball_around(rng, -m, e, rel_choice(rng, True)))
    for i in range(90):                                    # log_abs
        m, e = rand_mid(rng, -20, 20)
        add("log_abs", 0, ball_around(rng, m * rng.choice((1, -1)), e, rel_choice(rng, True)))
    for i in range(70):                                    # sin, cos
        for f in ("sin", "cos"):
            m, e = rand_mid(rng, -10, 8)
            m *= rng.choice((1, -1))
            rb = ball_around(rng, m, e, rng.choice((F(0), F(1, 2 ** 30), F(1, 2 ** 12))))
            if rng.random() < 0.3:
                rb = R.RB(rb.m, rb.e, rb.rm or rng.randint(1, 2 ** 29), rng.randint(-12, -1))
            try:
                add(f, 0, rb)
            except ValueError:
                continue
    for f in ("sin", "cos"):                               # huge arguments and wide balls
        for m, e, rm, re in ((1, 60, 0, 0), (3, 100, 1, 0), (5, 200, 0, 0), (7, 70, 1, 10), (0, 0, 0, 0),
                             (0, 0, 1, 0), (0, 0, 7, 0), (1, 1, 1, 0), (1, 0, 1, 0), (-1, 0, 3, 0)):
            try:
                add(f, 0, R.RB(m, e, rm, re), (2, 64, 300))
            except ValueError:
                pass
    for i in range(80):                                    # sqrt
        m, e = rand_mid(rng, -20, 20)
        add("sqrt", 0, ball_around(rng, m, e, rel_choice(rng)))
    for m, e, rm, re in ((0, 0, 0, 0), (4, 0, 0, 0), (1, 0, 1, 0), (1, 0, 1, -1), (1, -1, 1, -1), (0, 0, 1, 0),
                         (-1, 0, 0, 0), (-1, 0, 1, -2), (-1, 0, 1, 0), (1, 0, 1, -2), (9, 0, 0, 0),
                         (1, 2000, 0, 0), (1, -2001, 0, 0)):
        add("sqrt", 0, R.RB(m, e, rm, re), (2, 64, 300, 0))
    for n in (1, 2, 3, 4, 5, 6, 7, 9, 10, 15, 16, 31, 64, 65, 100, 1001, 2 ** 20):   # root
        for i in range(8):
            m, e = rand_mid(rng, -20, 20)
            m *= rng.choice((1, -1)) if n % 2 else 1
            add("root", n, ball_around(rng, m, e, rel_choice(rng)))
        for m, e, rm, re in ((0, 0, 0, 0), (8, 0, 0, 0), (-8, 0, 0, 0), (0, 0, 1, 0), (0, 0, 1, -10),
                             (1, 0, 1, 0), (-1, 0, 1, 0), (1, 0, 1, -1), (-1, 0, 1, -1), (27, 0, 0, 0),
                             (-27, 0, 3, 0), (1, 5, 0, 0)):
            add("root", n, R.RB(m, e, rm, re), (2, 64))
    add("root", 0, R.RB(1, 0, 0, 0), (2, 64))
    add("root", 0, R.RB(0, 0, 0, 0), (2,))
    for m, e, rm, re in ((0, 0, 0, 0), (1, 0, 0, 0), (-1, 0, 0, 0), (0, 0, 1, 0), (1, 0, 1, 0), (-1, 0, 1, 0),
                         (1, 0, 1, -1), (-1, 0, 1, -1), (1, -3, 1, -3), (-1, -3, 1, -3), (2, 0, 1, 0), (-2, 0, 1, 0)):
        for f in ("log", "log_abs"):
            add(f, 0, R.RB(m, e, rm, re), (2, 64))
    return rows


# ---------------------------------------------------------------------------------------------- partial balls

def rand_real(rng):
    m = rng.randint(-200, 200)
    e = rng.randint(-8, 6)
    if rng.random() < 0.3:
        return R.RB(m, e, 0, 0)
    return R.RB(m, e, rng.randint(1, 2 ** 20), rng.randint(-30, -2))


def rand_sball(rng, primes, arch):
    loc = [G1.rand_value(rng, p, 3, 3) for p in primes]
    return (arch, rand_real(rng) if arch else None, loc)


def place_json(v):
    return v


def project_rows():
    rng = random.Random(20260930)
    rows = []
    pool = [2, 3, 5, 7, 11, 13, 101, BIG]
    for i in range(220):
        d = rng.choice((1, 1, 2, 3, 6, 12, 45, 1000, rng.randint(1, 5000)))
        H = rng.choice((0, 0, rng.randint(1, 50), rng.randint(1, 10 ** 6), 2 ** rng.randint(1, 20),
                        3 ** rng.randint(1, 12), 6 ** rng.randint(1, 9), 5 * 7 * 11 * 13 * rng.randint(1, 30)))
        A = rng.randint(-10 ** 6, 10 ** 6)
        g = gcd(gcd(A, H), d)
        A, H, d = A // g, H // g, d // g
        if H > 0:
            A %= H
        ps = rng.sample(pool, rng.randint(0, 5))
        if rng.random() < 0.6:
            ps.append("inf")
        rng.shuffle(ps)
        rb = rand_real(rng)
        ref = R.sb_ref_project(rb, A, H, d, ps)
        row = {"x": {"inf": rb.json(), "A": A, "H": H, "d": d}, "places": ps, "status": ref[0]}
        row["result"] = R.sb_json(ref[1], ref[2], ref[3])
        rows.append(row)
    for i in range(20):                                    # a repeated place
        ps = rng.sample(pool, 3) + ["inf"]
        v = rng.choice(ps)
        ps.insert(rng.randint(0, len(ps)), v)
        rb = rand_real(rng)
        ref = R.sb_ref_project(rb, 1, 6, 1, ps)
        assert ref[0] == "DOMAIN"
        rows.append({"x": {"inf": rb.json(), "A": 1, "H": 6, "d": 1}, "places": ps, "status": "DOMAIN", "where": ref[1]})
    for big in (10 ** 300 + 1, 3 ** 700):                  # a big modulus: a ball of relative precision in the hundreds
        H = 2 ** 900 * 5 ** 500
        A = big % H
        rb = rand_real(rng)
        ps = [2, 5, "inf", 7]
        ref = R.sb_ref_project(rb, A, H, 1, ps)
        rows.append({"x": {"inf": rb.json(), "A": A, "H": H, "d": 1}, "places": ps, "status": "OK",
                     "result": R.sb_json(ref[1], ref[2], ref[3])})
    return rows


def where_of(px, py):
    """The first place in canonical order (inf first, then primes increasing) in one set and not in the other."""
    diff = set(px) ^ set(py)
    if not diff:
        return None
    return "inf" if "inf" in diff else min(diff)


def ops_rows():
    rng = random.Random(20260931)
    rows = []
    pool = [2, 3, 5, 7, 11, 13, BIG]
    for i in range(250):
        k = rng.randint(0, 4)
        primes = sorted(rng.sample(pool, k))
        arch = rng.randint(0, 1)
        X = rand_sball(rng, primes, arch)
        prec = rng.choice(PRECS)
        op = rng.choice(("add", "sub", "mul", "neg"))
        if op == "neg":
            arch_r, real, loc = R.sb_ref_op("neg", X)
            rows.append({"op": op, "prec": prec, "x": R.sb_json(*X), "status": "OK",
                         "result": {"arch": arch_r, "real": None if real is None else
                                    {"lo": R.dy_json(real[0]), "hi": R.dy_json(real[1])},
                                    "loc": [z.json() for z in loc]}})
            continue
        Y = rand_sball(rng, primes, arch)
        Y = (arch, Y[1], [G1.related(rng, a) if rng.random() < 0.5 else b for a, b in zip(X[2], Y[2])])
        arch_r, real, loc = R.sb_ref_op(op, X, Y)
        rows.append({"op": op, "prec": prec, "x": R.sb_json(*X), "y": R.sb_json(*Y), "status": "OK",
                     "result": {"arch": arch_r, "real": None if real is None else
                                {"lo": R.dy_json(real[0]), "hi": R.dy_json(real[1])},
                                "loc": [z.json() for z in loc]}})
    for i in range(60):                                    # different places: DOMAIN, with the place
        while True:
            px = sorted(rng.sample(pool, rng.randint(0, 4)))
            py = sorted(rng.sample(pool, rng.randint(0, 4)))
            ax, ay = rng.randint(0, 1), rng.randint(0, 1)
            sx = set(px) | ({"inf"} if ax else set())
            sy = set(py) | ({"inf"} if ay else set())
            if sx != sy:
                break
        X, Y = rand_sball(rng, px, ax), rand_sball(rng, py, ay)
        rows.append({"op": rng.choice(("add", "sub", "mul")), "prec": 53, "x": R.sb_json(*X), "y": R.sb_json(*Y),
                     "status": "DOMAIN", "where": where_of(sx, sy)})
    return rows


def pred_rows():
    rng = random.Random(20260932)
    rows = []
    pool = [2, 3, 5, 7]
    for i in range(200):
        k = rng.randint(0, 3)
        primes = sorted(rng.sample(pool, k))
        arch = rng.randint(0, 1)
        X = rand_sball(rng, primes, arch)
        r = rng.random()
        if r < 0.15:
            Y = X
        elif r < 0.30:                                     # other places
            pj = sorted(rng.sample(pool, rng.randint(0, 3)))
            Y = rand_sball(rng, pj, rng.randint(0, 1))
        else:                                              # the same places, near components
            loc = [G1.related(rng, a) if rng.random() < 0.7 else a for a in X[2]]
            rb = None
            if arch:
                x = X[1]
                c = rng.random()
                if c < 0.3:
                    rb = x
                elif c < 0.5:                              # a ball inside x
                    rb = R.RB(x.m, x.e, x.rm // 2, x.re)
                elif c < 0.7:                              # a ball around x
                    rb = R.RB(x.m, x.e, x.rm + 1, x.re) if x.rm < 2 ** 29 else x
                elif c < 0.85:                             # shifted: overlaps or not
                    rb = R.RB(x.m + rng.randint(-3, 3), x.e, x.rm, x.re)
                else:
                    rb = rand_real(rng)
            Y = (arch, rb, loc)
        for op in ("equal_set", "overlaps", "contains"):
            for (a, b) in ((X, Y), (Y, X)):
                rows.append({"op": op, "x": R.sb_json(*a), "y": R.sb_json(*b), "want": pred_ref(op, a, b)})
    return rows


def pred_ref(op, a, b):
    if a[0] != b[0] or [z.p for z in a[2]] != [z.p for z in b[2]]:
        return 0
    if a[0]:
        x, y = a[1], b[1]
        if op == "equal_set":
            ok = x.lo == y.lo and x.hi == y.hi
        elif op == "overlaps":
            ok = max(x.lo, y.lo) <= min(x.hi, y.hi)
        else:
            ok = y.lo <= x.lo and x.hi <= y.hi
        if not ok:
            return 0
    fn = {"equal_set": R.lb_ref_equal_set, "overlaps": R.lb_ref_overlaps, "contains": R.lb_ref_contains}[op]
    return int(all(fn(u, v) for u, v in zip(a[2], b[2])))


def main():
    n = {}
    n["rfunc_real.jsonl"] = write("rfunc_real.jsonl", real_rows())
    n["sball_project.jsonl"] = write("sball_project.jsonl", project_rows())
    n["sball_ops.jsonl"] = write("sball_ops.jsonl", ops_rows())
    n["sball_pred.jsonl"] = write("sball_pred.jsonl", pred_rows())
    for k, v in n.items():
        print(f"{k}: {v} lines")


if __name__ == "__main__":
    main()
