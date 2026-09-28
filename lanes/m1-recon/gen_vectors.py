#!/usr/bin/env python3
"""Write the extra vector files of lane m1-recon (work package 1.6).

The oracle is the Python reference tests/ref/adfref/recon.py, function `reconstruct`, exactly
like tests/ref/gen_vectors.py does for tests/ref/vectors/recon.jsonl. Nothing here is read by the
C tests at run time: the files are fixtures.

    python3 lanes/m1-recon/gen_vectors.py

Two files are written under tests/ref/vectors/m1-recon/:

  recon_edges.jsonl  the format of recon.jsonl (op reconstruct, ball, lo, hi, status, solutions)
                     for the cases the committed file does not cover: a candidate equal to an end
                     point of the closed interval (M0-D3), an interval of the width of the radius
                     and one just below it, radii with a denominator, negative centres, N = 0, an
                     empty interval, and operands of 4096 bits.
  recon_adele.jsonl  a real ball, given as the exact numbers mid = mid_num * 2^mid_exp and
                     rad = 2^rad_exp, the finite ball, the end points of the closed interval
                     (which the C test checks against mid - rad and mid + rad), the status and the
                     solutions. It drives adf_adele_reconstruct.

Every interval written has |lo|, |hi| <= 8 and every radius is at least 1/6, except for the lines
of 4096 bits whose interval is at the centre of the ball; the C tests enumerate the candidates
a + N k over k in [-64, 64], so those bounds are what makes the enumeration complete.
"""

import json
import os
import sys
from fractions import Fraction

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tests", "ref"))

from adfref import recon  # noqa: E402
from adfref.fball import Fball  # noqa: E402

OUT = os.path.join(ROOT, "tests", "ref", "vectors", "m1-recon")


def ball(B):
    return {"A": B.A, "H": B.H, "d": B.d}


def rat(q):
    q = Fraction(q)
    return {"num": q.numerator, "den": q.denominator}


def fball(c, N):
    return Fball.from_center_radius(Fraction(c), Fraction(N))


def line_for(B, lo, hi, op="reconstruct"):
    r = recon.reconstruct(B, lo, hi)
    return {"op": op, "ball": ball(B), "lo": rat(lo), "hi": rat(hi), "status": r.status,
            "solutions": [rat(s) for s in r.solutions]}


# The cases of recon_edges.jsonl: a list of (centre, radius) and a list of (lo, hi) pairs. The
# end points below are chosen so that some of them are candidates of the progression, which is
# what M0-D3 is about: a real ball is a closed set and an end point may be the answer.
BALLS = [
    (0, 1),        # Zhat
    (0, Fraction(1, 6)),
    (Fraction(1, 2), Fraction(5, 6)),
    (Fraction(1, 3), Fraction(2, 3)),
    (Fraction(1, 3), Fraction(7, 3)),
    (Fraction(-1, 3), Fraction(2, 3)),
    (Fraction(-1, 2), Fraction(3, 2)),
    (Fraction(0), Fraction(0)),        # exact 0
    (Fraction(3, 7), Fraction(0)),     # exact fraction
    (Fraction(-5, 6), Fraction(0)),    # exact negative fraction
    (Fraction(2), Fraction(0)),        # exact integer
]

INTERVALS = [
    (Fraction(0), Fraction(0)),
    (Fraction(1, 2), Fraction(1, 2)),
    (Fraction(1, 2), Fraction(5, 12)),        # the candidate is the left end point
    (Fraction(5, 12), Fraction(1, 2)),        # the candidate is the right end point
    (Fraction(1, 3), Fraction(1, 2)),
    (Fraction(0), Fraction(2, 3)),
    (Fraction(-1, 2), Fraction(1, 2)),
    (Fraction(-1, 3), Fraction(1, 3)),
    (Fraction(1, 2), Fraction(3, 2)),         # the width of the radius, one candidate
    (Fraction(0), Fraction(1)),               # the width of the radius, two candidates
    (Fraction(0), Fraction(1) + Fraction(1, 12)),
    (Fraction(1, 2), Fraction(3, 2) - Fraction(1, 12)),
    (Fraction(1, 4), Fraction(3, 4)),         # just below the width of the radius
    (Fraction(0), Fraction(3, 7)),
    (Fraction(1, 7), Fraction(3, 7)),
    (Fraction(1, 2), Fraction(1, 3)),         # the empty interval
    (Fraction(1), Fraction(0)),
    (Fraction(1, 3), Fraction(1, 3)),
    (Fraction(-1, 3), Fraction(-1, 3)),
    (Fraction(2, 7), Fraction(3, 7)),
    (Fraction(-2, 7), Fraction(-1, 7)),
]


def gen_edges():
    lines = []
    for c, N in BALLS:
        B = fball(c, N)
        for lo, hi in INTERVALS:
            lines.append(line_for(B, lo, hi))
    # Operands of 4096 bits. The ball is given by the raw triple (A, H, d) with every bit whose
    # index is congruent to seed modulo 3 set, the same integers the C test builds.
    def big(bits, seed):
        v = 0
        for i in range(bits):
            if (i + seed) % 3 == 0:
                v |= 1 << i
        return v

    A, H, d = big(4096, 1), big(4096, 2), big(4096, 0)
    B = Fball(A, H, d)
    a, N = B.center, B.radius
    for lo, hi in [(a, a), (a, a + N), (a + N, a + N), (a - N, a - N), (a - N, a + N),
                   (a + 2 * N, a + 3 * N), (a + N / 2, a + N / 2), (Fraction(1, 2), Fraction(3, 4))]:
        lines.append(line_for(B, lo, hi))
    return lines


# The real balls of recon_adele.jsonl: (mid_num, mid_exp, rad_exp), so that mid = mid_num * 2^mid_exp
# and rad = 2^rad_exp are both dyadic and the ball [mid - rad, mid + rad] is exact.
REALS = [
    (1, -1, -100),     # the point 1/2
    (0, 0, -100),      # the point 0
    (3, -3, -100),     # the point 3/8
    (9, -4, -4),       # [1/2, 5/8]
    (7, -4, -4),       # [3/8, 1/2]
    (13, -5, -4),      # [1/2, 7/12]
    (11, -5, -4),      # [5/12, 1/2]
    (3, -1, -1),       # [1/2, 1]
    (1, 0, -2),        # [3/4, 5/4]
    (0, 0, -1),        # [-1/2, 1/2]
    (1, -2, -2),       # [0, 1/2]
    (1, -2, -1),       # [-1/4, 3/4]
    (-3, -3, -3),      # [-3/4, -3/4]
]

BALLS_ADELE = [
    (0, 1),                    # Zhat
    (0, Fraction(1, 6)),
    (Fraction(1, 2), Fraction(5, 6)),
    (Fraction(1, 3), Fraction(2, 3)),
    (Fraction(-1, 3), Fraction(2, 3)),
    (Fraction(3, 7), Fraction(0)),
]


def gen_adele():
    lines = []
    for mn, me, re in REALS:
        mid = Fraction(mn) * Fraction(2) ** me
        rad = Fraction(2) ** re
        for c, N in BALLS_ADELE:
            B = fball(c, N)
            r = recon.reconstruct(B, mid - rad, mid + rad)
            lines.append({"op": "adele_reconstruct", "mid_num": mn, "mid_exp": me,
                          "rad_exp": re, "ball": ball(B), "lo": rat(mid - rad),
                          "hi": rat(mid + rad), "status": r.status,
                          "solutions": [rat(s) for s in r.solutions]})
    return lines


def write(name, lines):
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, name)
    with open(path, "w") as f:
        for ln in lines:
            f.write(json.dumps(ln, sort_keys=True) + "\n")
    print("%s: %d lines" % (path, len(lines)))


if __name__ == "__main__":
    write("recon_edges.jsonl", gen_edges())
    write("recon_adele.jsonl", gen_adele())
