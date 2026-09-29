#!/usr/bin/env python3
"""Writes tests/ref/vectors/s1-slice2/fball.jsonl from adelic_system and linsolve_mod of
proto/solvers_checks.py (imported, not copied): the system A x in balls, x in Zhat^c (solvers P2.9), its
modular answer, and the coordinate balls of P2.10.

Run from the repository root: python3 lanes/s1-slice2/gen_vectors.py
Every line: {"r","c","A","balls","N","status","G","E","V","x0","y","rho","xball","what"}.
A: r rows of c integers. balls: r triples [A_i, H_i, d_i], canonical (conventions 5.2), H_i >= 1.
N = lcm(H_i) (1 for r = 0). status OK or NO_SOLUTION; G, E, V rows; x0 (c entries) for OK, else null;
y (r entries) for NO_SOLUTION, else null. rho: list of c values gcd(N, G[.][j]) for OK, else null;
xball: list of c canonical triples [x0[j] mod rho_j, rho_j, 1] for OK, else null.
All of G, E, V, x0, y are determined by Algorithm L (solvers 2.8) for (A', b', N), so the C function must
reproduce them entry by entry. Every line is checked here with linsol_check of the reference."""
import json
import os
import random
import sys
from math import gcd

sys.path.insert(0, "proto")
import solvers_checks as S  # noqa: E402

random.seed(20260930)
lines = []


def add(A, balls, r, c, what):
    A2, b2, N = S.adelic_system(A, balls, r, c)
    sol = S.linsolve_mod(A2, b2, r, c, N)
    assert sol["status"] in (S.OK, S.NO_SOLUTION)
    assert S.linsol_check(sol, A2, b2, r, c, N)
    rho = xball = None
    if sol["status"] == S.OK:
        rho = []
        for j in range(c):
            g = N
            for row in sol["G"]:
                g = gcd(g, row[j])
            rho.append(g)
        xball = [[sol["x0"][j] % rho[j], rho[j], 1] for j in range(c)]
    lines.append({"r": r, "c": c, "A": A, "balls": [list(b) for b in balls], "N": N, "status": sol["status"],
                  "G": sol["G"], "E": sol["E"], "V": sol["V"], "x0": sol.get("x0"), "y": sol.get("y"),
                  "rho": rho, "xball": xball, "what": what})


# 1. the range of check_s1_adelic (proto/solvers_checks.py:1408 to 1414), more systems, r up to 4, c up to 3
for _ in range(600):
    r, c = random.randint(0, 4), random.randint(0, 3)
    balls = []
    for _ in range(r):
        H = random.choice((1, 2, 3, 4, 6, 8, 9, 12))
        d = random.choice((1, 1, 2, 3, 4, 6))
        balls.append(S.fball_canon(random.randrange(-20, 20), H, d))
    A = [[random.randrange(-6, 7) for _ in range(c)] for _ in range(r)]
    add(A, balls, r, c, "small")

# 2. radii that do not divide each other (lcm below the product)
for _ in range(60):
    r, c = random.randint(1, 3), random.randint(1, 3)
    Hs = random.choice(((4, 6), (6, 10, 15), (8, 12), (9, 6, 4), (12, 18), (2, 3, 5)))
    balls = [S.fball_canon(random.randrange(-50, 50), Hs[i % len(Hs)], random.choice((1, 2, 3, 5)))
             for i in range(r)]
    A = [[random.randrange(-9, 10) for _ in range(c)] for _ in range(r)]
    add(A, balls, r, c, "lcm")

# 3. large radii (up to 130 bits), denominators up to 40 bits, entries of A of any size and sign, with a
#    planted integer solution (the balls then contain A x), changed a little in 30 % of the balls
for _ in range(120):
    r, c = random.randint(1, 4), random.randint(1, 4)
    A = [[random.randrange(-(1 << 70), 1 << 70) for _ in range(c)] for _ in range(r)]
    x = [random.randrange(-(1 << 50), 1 << 50) for _ in range(c)]
    balls = []
    for i in range(r):
        H = random.getrandbits(random.choice((10, 64, 90, 130))) | 1
        if random.random() < 0.4:
            H *= random.choice((2, 4, 8, 3, 9)) ** random.randint(1, 20)
        d = random.choice((1, 1, 7, 2 ** 30 + 3, 6 ** 8))
        val = sum(A[i][j] * x[j] for j in range(c))
        centre = d * val
        if random.random() < 0.3:
            centre += random.randrange(1, 4)
        balls.append(S.fball_canon(centre, H, d))
    add(A, balls, r, c, "big")

os.makedirs("tests/ref/vectors/s1-slice2", exist_ok=True)
with open("tests/ref/vectors/s1-slice2/fball.jsonl", "w") as f:
    for line in lines:
        f.write(json.dumps(line, separators=(",", ":")) + "\n")
n_ok = sum(1 for l in lines if l["status"] == "OK")
print(f"{len(lines)} lines, OK {n_ok}, NO_SOLUTION {len(lines) - n_ok}, "
      f"N above 64 bits {sum(1 for l in lines if l['N'] >= 1 << 64)}")
