"""Find a cut-search case whose full solution set is empty."""

from math import gcd

for m in range(2, 30):
    for c in range(m):
        for a in range(m):
            for b in range(1, 20):
                if 2 * a * b < m:
                    continue
                solutions = [
                    (n, d)
                    for d in range(1, b + 1)
                    for n in range(-a, a + 1)
                    if gcd(n, d) == 1 and (n - c * d) % m == 0
                ]
                if not solutions:
                    print(m, c, a, b)
                    raise SystemExit
