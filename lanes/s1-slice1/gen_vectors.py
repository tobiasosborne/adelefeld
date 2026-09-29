#!/usr/bin/env python3
"""Writes tests/ref/vectors/s1-slice1/linsolve.jsonl from linsolve_mod of proto/solvers_checks.py.

Run from the repository root: python3 lanes/s1-slice1/gen_vectors.py
Every line: {"N","r","c","A","b","status","G","E","V","x0","y","what"}. A is a list of r rows of c integers
(any integers, the reference reduces them), b a list of r integers; status is OK or NO_SOLUTION; G, E, V
are lists of rows; x0 is the list of c entries for OK (else null), y the list of r entries for NO_SOLUTION
(else null). All of G, E, V, x0, y are determined by Algorithm L of docs/proofs/solvers.md 2.8 (the Howell
form of [A^T | I_c] is unique, P2.4), so the C function must reproduce them entry by entry. Every line is
checked here with linsol_check of the reference before it is written. Integers are JSON integers (the C
reader keeps them as text)."""
import json
import os
import random
import sys
from collections import Counter

sys.path.insert(0, "proto")
import solvers_checks as S  # noqa: E402

random.seed(20260929)
lines = []


def add(A, b, r, c, N, what):
    sol = S.linsolve_mod(A, b, r, c, N)
    assert sol["status"] in (S.OK, S.NO_SOLUTION), (A, b, N)
    assert S.linsol_check(sol, A, b, r, c, N), (A, b, N)
    lines.append({"N": N, "r": r, "c": c, "A": A, "b": b, "status": sol["status"], "G": sol["G"],
                  "E": sol["E"], "V": sol["V"], "x0": sol.get("x0"), "y": sol.get("y"), "what": what})
    return sol


# 1. the fixed cases of check_s1_edge (proto/solvers_checks.py:1324 to 1334, copied: the list is local to
#    that function); the expected G and x0 given there are asserted as well
edge = [
    ([[0, 0], [0, 0]], [0, 0], 2, 2, 12, S.OK, [[1, 0], [0, 1]], [0, 0]),
    ([[0, 0], [0, 0]], [0, 5], 2, 2, 12, S.NO_SOLUTION, [[1, 0], [0, 1]], None),
    ([[3, 7], [1, 1]], [5, 6], 2, 2, 1, S.OK, [], [0, 0]),
    ([], [], 0, 3, 8, S.OK, [[1, 0, 0], [0, 1, 0], [0, 0, 1]], [0, 0, 0]),
    ([[], []], [0, 0], 2, 0, 8, S.OK, [], []),
    ([[], []], [0, 4], 2, 0, 8, S.NO_SOLUTION, [], None),
    ([], [], 0, 0, 5, S.OK, [], []),
    ([[2]], [2], 1, 1, 4, S.OK, [[2]], None),
    ([[2]], [1], 1, 1, 4, S.NO_SOLUTION, [[2]], None),
    ([[6]], [3], 1, 1, 9, S.OK, [[3]], None),
    ([[2, 4]], [6], 1, 2, 8, S.OK, [[2, 1], [0, 2]], None),
]
for A, b, r, c, N, st, G, x0 in edge:
    sol = add(A, b, r, c, N, "edge")
    assert sol["status"] == st and sol["G"] == G and (x0 is None or sol["x0"] == x0)

# a few more special cases of solvers P2.8(4): N = 1 with r = 0 and c = 0, the zero matrix with r > c,
# c = 0 with b = 0 modulo N but b not 0 as an integer, one equation with a huge coefficient
add([], [], 0, 0, 1, "special")
add([], [], 0, 2, 1, "special")
add([[0], [0], [0]], [0, 12, -24], 3, 1, 12, "special")
add([[], [], []], [7, -14, 21], 3, 0, 7, "special")
add([[], [], []], [7, -14, 22], 3, 0, 7, "special")
add([[3 ** 400 + 2]], [5], 1, 1, 12, "special")
add([[-(2 ** 3000) - 1, 6]], [-(10 ** 700)], 1, 2, 18, "special")


def rand_big_rows(r, c, N):
    rows = S.rand_rows(r, c, N)
    if random.random() < 0.5:
        rows = [[(x * random.choice((2, 3, 2 ** 20, 3 ** 10))) % N for x in row] for row in rows]
    return rows


def lift(v, N, bits):
    """The same residues, as integers of any sign and size."""
    return [x + N * (random.getrandbits(bits) - (1 << (bits - 1))) for x in v]


# 2. N of 64 to 122 bits with a planted solution (the moduli of check_s1_edge, proto/solvers_checks.py:1357,
#    and random ones of 64 to 122 bits); 30 % of the right-hand sides are changed after planting
moduli = [2 ** 64, 2 ** 89 - 1, 3 ** 40 * 2 ** 30, (2 ** 61 - 1) ** 2, 10 ** 30]
moduli += [random.getrandbits(bits) | (1 << (bits - 1)) for bits in (64, 65, 90, 121, 122)]
for N in moduli:
    for _ in range(12):
        r, c = random.randint(1, 5), random.randint(1, 5)
        A = rand_big_rows(r, c, N)
        x = [random.randrange(N) for _ in range(c)]
        b = S.matvec(A, x, N)
        if random.random() < 0.3:
            b[0] = (b[0] + 1) % N
        if random.random() < 0.5:
            A = [lift(row, N, 150) for row in A]
            b = lift(b, N, 150)
        add(A, b, r, c, N, "big")

# 3. N of 2000 bits (a power of 2, a product of prime powers, random), planted as above
for N in (2 ** 2000, 3 ** 700 * 2 ** 800 * 5 ** 100, random.getrandbits(2000) | (1 << 1999)):
    for _ in range(5):
        r, c = random.randint(1, 4), random.randint(1, 4)
        A = rand_big_rows(r, c, N)
        if random.random() < 0.5:
            A = [[(x * 2 ** 1500) % N for x in row] for row in A]
        x = [random.randrange(N) for _ in range(c)]
        b = S.matvec(A, x, N)
        if random.random() < 0.3:
            b[-1] = (b[-1] + 2 ** 1000) % N
        add(A, b, r, c, N, "big2000")

# 4. small moduli, entries of any sign and size (residues with zero divisors), r, c up to 5
for N in (2, 4, 6, 8, 9, 12, 16, 18, 30, 36, 360):
    for _ in range(30):
        r, c = random.randint(0, 5), random.randint(0, 5)
        A = S.rand_rows(r, c, N)
        if random.random() < 0.4:
            d = random.choice([d for d in range(1, N + 1) if N % d == 0])
            A = [[(x * d) % N for x in row] for row in A]
        b = [random.randrange(N) for _ in range(r)]
        if random.random() < 0.5 and c:
            b = S.matvec(A, [random.randrange(N) for _ in range(c)], N)
        A = [lift(row, N, 80) for row in A]
        b = lift(b, N, 80) if r else b
        add(A, b, r, c, N, "small")

os.makedirs("tests/ref/vectors/s1-slice1", exist_ok=True)
with open("tests/ref/vectors/s1-slice1/linsolve.jsonl", "w") as f:
    for rec in lines:
        f.write(json.dumps(rec, separators=(",", ":")) + "\n")
print(len(lines), Counter((rec["what"], rec["status"]) for rec in lines))
