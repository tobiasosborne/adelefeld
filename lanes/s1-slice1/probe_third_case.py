#!/usr/bin/env python3
"""Probe for the survivor H-third of lanes/s1-slice1/mutants.py: is the pending pair ((N/g) w', j + 1) of the
third case of Algorithm H (docs/proofs/solvers.md:635 to 637) needed? Algorithm H is run as written (howell of
proto/solvers_checks.py) and without that pair, on random row sets, and the outputs are compared; the output
without the pair is also tested with is_howell and against the span of the input.

Run from the repository root: python3 lanes/s1-slice1/probe_third_case.py"""
import random
import sys

sys.path.insert(0, "proto")
import solvers_checks as S  # noqa: E402


def howell_no_third_push(rows, ncols, N):
    """proto/solvers_checks.py:759 to 797 with line 787 (the push of the third case) removed."""
    T = {}
    stack = [([x % N for x in v], 0) for v in reversed(rows)]
    third = 0
    while stack:
        v, j = stack.pop()
        while j < ncols:
            a = v[j]
            if a == 0:
                j += 1
                continue
            if j not in T:
                g, s, _ = S.xgcd(a, N)
                T[j] = [(s * x) % N for x in v]
                stack.append(([((N // g) * x) % N for x in v], j + 1))
                break
            w = T[j]
            h = w[j]
            if a % h == 0:
                q = a // h
                v = [(x - q * y) % N for x, y in zip(v, w)]
                j += 1
                continue
            g, s, t = S.xgcd(a, h)
            w2 = [(s * x + t * y) % N for x, y in zip(v, w)]
            v = [((h // g) * x - (a // g) * y) % N for x, y in zip(v, w)]
            T[j] = w2
            third += 1
            j += 1
    cols = sorted(T)
    H = [T[j] for j in cols]
    for i, j in enumerate(cols):
        h = H[i][j]
        for k in range(i):
            q = H[k][j] // h
            if q:
                H[k] = [(x - q * y) % N for x, y in zip(H[k], H[i])]
    return H, third


random.seed(7)
n = diff = third_total = 0
for N in (4, 6, 8, 9, 12, 16, 18, 24, 27, 32, 36, 48, 64, 72, 360, 2 ** 20, 3 ** 9 * 2 ** 5):
    for ncols in (1, 2, 3, 4, 5):
        for _ in range(400):
            rows = S.rand_rows(random.randint(1, 7), ncols, N)
            if random.random() < 0.6:
                rows = [[(x * random.choice([d for d in range(1, 50) if N % d == 0])) % N for x in r] for r in rows]
            H = S.howell(rows, ncols, N)
            H2, third = howell_no_third_push(rows, ncols, N)
            n += 1
            third_total += third
            if H2 != H:
                diff += 1
                if diff <= 3:
                    print("differs:", N, rows, H, H2)
            elif ncols <= 3 and N ** ncols <= 5000 and not S.is_howell(H2, ncols, N):
                diff += 1
print(f"{n} row sets, third case taken {third_total} times; {diff} outputs differ from Algorithm H as written")
