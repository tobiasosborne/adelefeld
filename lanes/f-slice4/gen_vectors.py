"""lanes/f-slice4/gen_vectors.py: the vector files of lane f-slice4 from the reference proto/lfunc_checks.py.

    python3 -B lanes/f-slice4/gen_vectors.py        (from the repository root)

Writes tests/ref/vectors/f-slice4/:
  lfunc_points.jsonl  the value f(t) modulo p^n at every point t of a grid (p = 2: 0 <= t < 2^8; p = 3: 3^5;
                      p = 5: 5^4; p = 7: 7^3), for f = exp on its domain, log on its domain, Log on t != 0; n = the
                      grid exponent + 2. The C test encloses the image of every ball of the grid with these values.
  lfunc_cases.jsonl   input lball, requested N, status and result of the C function as lfunc.h states it
                      (reference: result() of proto/lfunc_checks.py): the precision cases of SPEC 9.3.2, the
                      statuses, random exact and ball inputs at p = 2, 3, 5, 7, 11, 13 and 2^64 - 59, precision 2000.
Deterministic (fixed seeds)."""

import json
import os
import random
import sys
from fractions import Fraction

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "proto"))
import lfunc_checks as R  # noqa: E402

OUT = os.path.join("tests", "ref", "vectors", "f-slice4")
BIGP = 2 ** 64 - 59
GRIDS = ((2, 8), (3, 5), (5, 4), (7, 3))


def in_domain_point(f, p, t):
    c = R.c_of(p)
    if f == "exp":
        return t == 0 or R.val(t, p) >= c
    if f == "log":
        return t != 0 and (t == 1 or R.val(t - 1, p) >= 1)
    return t != 0


def points():
    rows = []
    for p, M in GRIDS:
        n = M + 2
        for f in ("exp", "log", "Log"):
            for t in range(0, p ** M):
                if not in_domain_point(f, p, Fraction(t)):
                    continue
                rows.append({"f": f, "p": p, "x": {"num": t, "den": 1}, "n": n,
                             "value": R.point_value(f, p, Fraction(t), n)})
    return rows


def case(f, x, N, note=""):
    st, y = R.result(f, x, N)
    return {"f": f, "x": x, "N": N, "status": st, "result": y, "note": note}


def spec_cases():
    E, B = R.lb_exact, R.lb_ball
    F = Fraction
    rows = [
        # review N4 (SPEC 9.3.2 "Implementation"): exp 3 and exp 12 at precision 8 differ at digit 2 ...
        case("exp", E(3, 3), 8, "exp(3) mod 3^8"),
        case("exp", E(3, 12), 8, "exp(12) mod 3^8, differs from exp(3) at digit 2"),
        # ... so exp of the ball 3 + 9 Z_3 is known to 2 digits only, whatever N is asked
        case("exp", B(3, 3, 2), 8, "exp(3 + 9 Z_3): radius 9 kept, not 3^8"),
        case("exp", B(3, 12, 2), 8, "the same ball"),
        case("exp", B(3, 3, 8), 8, "exp(3 + 3^8 Z_3)"),
        case("exp", B(3, 3, 8), 5, "N < M: exponent 5"),
        case("exp", B(2, 4, 3), 10, "exp(4 + 8 Z_2): radius 8"),
        case("exp", B(2, 0, 2), 10, "exp(4 Z_2) = 1 + 4 Z_2 (edge of the domain)"),
        case("exp", B(3, 0, 1), 10, "exp(3 Z_3) = 1 + 3 Z_3 (edge of the domain)"),
        case("exp", E(2, 4), 8, "exp(4) at 2 (probe: 77 mod 2^8)"),
        case("exp", E(5, 0), 8, "exp(0) = 1 exactly"),
        # domain statuses of exp
        case("exp", E(5, 1), 20, "exp(1) at 5: DOMAIN"),
        case("exp", E(2, 2), 20, "exp(2) at 2: DOMAIN (v = 1 < c = 2)"),
        case("exp", E(3, F(1, 3)), 20, "exp(1/3) at 3: DOMAIN"),
        case("exp", B(2, 2, 3), 10, "exp(2 + 8 Z_2): DOMAIN"),
        case("exp", B(2, 2, 2), 10, "exp(2 + 4 Z_2): DOMAIN"),
        case("exp", B(2, 0, 1), 10, "exp(2 Z_2): meets 4 Z_2 and its complement"),
        case("exp", B(3, 0, 0), 10, "exp(Z_3): NOT_DETERMINED"),
        case("exp", B(3, 0, -3), 10, "exp(3^-3 Z_3): NOT_DETERMINED"),
        case("exp", B(3, 1, 3), 10, "exp(1 + 27 Z_3): DOMAIN"),
        case("exp", B(3, F(1, 3), 5), 10, "exp(1/3 + 3^5 Z_3): DOMAIN"),
        # log
        case("log", E(2, -1), 20, "log(-1) = 0 at 2, exactly"),
        case("log", E(2, 1), 20, "log(1) = 0 exactly"),
        case("log", E(5, 1), 20, "log(1) = 0 exactly"),
        case("log", E(2, 3), 20, "log(3) at 2: v_2 = 2 (SPEC 9.3.2)"),
        case("log", E(2, -3), 20, "log(-3) = log(3) at 2"),
        case("log", E(2, 5), 20, "log(5) at 2"),
        case("log", E(3, 4), 8, "log(4) at 3 (probe of lane d-functions: 3 * 664 mod 3^8)"),
        case("log", B(2, 1, 1), 10, "log(1 + 2 Z_2) = 4 Z_2"),
        case("log", B(2, 3, 1), 10, "log(3 + 2 Z_2), the same ball: 4 Z_2"),
        case("log", B(2, 1, 1), 1, "log(1 + 2 Z_2) at N = 1: 2 Z_2"),
        case("log", B(2, 3, 2), 10, "log(3 + 4 Z_2) = log(3) + 4 Z_2"),
        case("log", B(2, 7, 3), 10, "log(-1 + 8 Z_2) = log(7) + 8 Z_2"),
        case("log", B(2, 5, 3), 10, "log(5 + 8 Z_2): isometry on 1 + 4 Z_2"),
        case("log", B(3, 4, 1), 10, "log(1 + 3 Z_3) = 3 Z_3 (edge)"),
        case("log", B(3, 4, 5), 10, "log(4 + 3^5 Z_3)"),
        case("log", E(3, 2), 10, "log(2) at 3: DOMAIN"),
        case("log", E(3, 3), 10, "log(3) at 3: DOMAIN"),
        case("log", E(3, -1), 10, "log(-1) at 3: DOMAIN"),
        case("log", E(3, 0), 10, "log(0): DOMAIN"),
        case("log", B(3, 0, 1), 10, "log(3 Z_3): DOMAIN"),
        case("log", B(3, 0, 5), 10, "log(O(3^5)), an uncertain zero outside the domain: DOMAIN"),
        case("log", B(3, 0, 0), 10, "log(Z_3): NOT_DETERMINED"),
        case("log", B(3, 0, -1), 10, "log(3^-1 Z_3): NOT_DETERMINED"),
        case("log", B(3, F(1, 3), 0), 10, "log(1/3 + Z_3): does not contain 1, DOMAIN"),
        case("log", B(3, 2, 1), 10, "log(2 + 3 Z_3): DOMAIN"),
        case("log", B(2, 0, 0), 10, "log(Z_2): NOT_DETERMINED"),
        # Log
        case("Log", E(3, 3), 10, "Log(3) = 0 exactly"),
        case("Log", E(3, -27), 10, "Log(-27) = 0 exactly"),
        case("Log", E(2, 2), 10, "Log(2) = 0 exactly"),
        case("Log", E(2, F(-1, 8)), 10, "Log(-1/8) = 0 exactly"),
        case("Log", E(3, 12), 10, "Log(12) = log(4) at 3"),
        case("Log", B(3, 3, 2), 10, "Log(3 + 9 Z_3) = 3 Z_3: one digit lost (m = 1)"),
        case("Log", B(3, 12, 2), 10, "the same ball"),
        case("Log", B(3, 12, 6), 10, "Log(12 + 3^6 Z_3) = Log(12) + 3^5 Z_3"),
        case("Log", B(2, 2, 3), 10, "Log(2 + 8 Z_2) = 4 Z_2 (r = 2)"),
        case("Log", B(2, 10, 3), 10, "the same ball"),
        case("Log", B(2, 10, 6), 10, "Log(10 + 2^6 Z_2) = Log(10) + 2^5 Z_2"),
        case("Log", B(2, 2, 2), 10, "Log(2 + 4 Z_2), r = 1: 4 Z_2"),
        case("Log", B(2, 6, 3), 10, "Log(6 + 8 Z_2), r = 2"),
        case("Log", B(3, F(7, 9), 3), 10, "Log(7/9 + 27 Z_3): m = -2 gains 2 digits"),
        case("Log", B(3, F(7, 9), 3), 4, "the same, N = 4 < r = 5"),
        case("Log", B(5, F(2, 25), 0), 10, "Log(2/25 + Z_5): r = 2"),
        case("Log", B(5, 1, 3), 10, "Log(1 + 5^3 Z_5) = 5^3 Z_5"),
        case("Log", E(5, 0), 10, "Log(0): DOMAIN"),
        case("Log", B(5, 0, 5), 10, "Log(O(5^5)): NOT_DETERMINED"),
        case("Log", B(5, 0, -5), 10, "Log(O(5^-5)): NOT_DETERMINED"),
        case("Log", E(7, 2), -3, "Log(2) at 7, N = -3"),
        case("Log", E(7, 2), 0, "Log(2) at 7, N = 0"),
        case("Log", E(7, 2), 1, "Log(2) at 7, N = 1"),
        case("exp", E(7, 7), -2, "exp(7), N = -2"),
        case("exp", E(7, 7), 1, "exp(7), N = 1"),
        case("log", E(7, 8), 0, "log(8), N = 0"),
    ]
    return rows


def rand_rational(rng, p, v):
    """A rational of valuation v at p with a small unit part (numerator and denominator prime to p)."""
    while True:
        a = rng.randrange(1, 10 ** 6) * rng.choice((1, -1))
        b = rng.randrange(1, 10 ** 4)
        if a % p and b % p:
            return Fraction(a, b) * Fraction(p) ** v


def random_cases():
    rng = random.Random(20260930)
    rows = []
    for p, count, maxN in ((2, 120, 40), (3, 120, 40), (5, 80, 30), (7, 80, 30), (11, 50, 20), (13, 50, 20),
                           (BIGP, 30, 5)):
        c = R.c_of(p)
        for i in range(count):
            f = ("exp", "log", "Log")[i % 3]
            N = rng.randrange(-2, maxN + 1)
            kind = rng.randrange(4)
            if f == "exp":
                v = rng.randrange(c, c + 4)
                q = rand_rational(rng, p, v)
            elif f == "log":
                q = 1 + rand_rational(rng, p, rng.randrange(1, 4))
                if p == 2 and rng.randrange(2):
                    q = -q
            else:
                q = rand_rational(rng, p, rng.randrange(-4, 5))
            if kind == 0:
                x = R.lb_exact(p, q)
            else:
                vq = R.val(q, p)
                M = vq + rng.randrange(1, maxN + 2)
                x = R.lb_ball(p, q, M)
            rows.append(case(f, x, N, "random"))
    # random inputs outside the domain or meeting its edge
    for p in (2, 3, 5):
        for i in range(60):
            f = ("exp", "log", "Log")[i % 3]
            q = rand_rational(rng, p, rng.randrange(-3, 3))
            M = rng.randrange(-3, 5)
            x = R.lb_ball(p, q, M) if i % 2 else R.lb_exact(p, q)
            rows.append(case(f, x, 12, "random, any status"))
    return rows


def big_cases():
    """Precision 2000, and the prime 2^64 - 59 at higher precision."""
    E, B = R.lb_exact, R.lb_ball
    rows = [
        case("exp", E(3, 3), 2000, "precision 2000"),
        case("Log", E(2, 3), 2000, "precision 2000"),
        case("log", B(5, 6, 2000), 2000, "precision 2000, ball"),
        case("exp", E(BIGP, BIGP), 60, "exp(p) at p = 2^64 - 59"),
        case("log", E(BIGP, 1 + BIGP), 60, "log(1 + p) at p = 2^64 - 59"),
        case("Log", E(BIGP, 2), 40, "Log(2) at p = 2^64 - 59: the power a^(p-1)"),
        case("Log", B(BIGP, Fraction(3, BIGP), 7), 40, "Log(3/p + p^7 Z_p): r = 8"),
    ]
    return rows


def write(name, rows):
    path = os.path.join(OUT, name)
    with open(path, "w") as fh:
        for r in rows:
            fh.write(json.dumps(r, sort_keys=True) + "\n")
    print(name, len(rows), "rows,", os.path.getsize(path), "bytes")


def main():
    os.makedirs(OUT, exist_ok=True)
    write("lfunc_points.jsonl", points())
    write("lfunc_cases.jsonl", spec_cases() + random_cases() + big_cases())
    return 0


if __name__ == "__main__":
    sys.exit(main())
