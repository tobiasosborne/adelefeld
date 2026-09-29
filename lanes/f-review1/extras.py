"""Predicate threshold and projection attacks. Uses only the reviewer's Fraction oracle."""
from oracle import Q, call, proc, ball, exact, vp
from collections import Counter
import random

counts = Counter()
primes = (2, 3, 5, 7, 2**64-59)
rng = random.Random(73019)
try:
    for p in primes:
        for k in range(1, 201):
            modulus = p**k
            for u in (0, 1, -1, modulus-1, modulus, modulus+1, modulus//2, modulus*2,
                      2**(modulus.bit_length()-1), 2**modulus.bit_length()-1):
                for v in (-3, 0, 3):
                    x = (p, 0, Q(u), v, v+k)
                    expected = v == 0 if u == 0 else (0 < u < modulus and u % p != 0)
                    assert call("canon", x)[3] == expected, (x, expected)
                    counts["canonical_thresholds"] += 1
        for v, n in ((-2**63, 2**63-1), (-2**63, -2**63+1), (2**63-2, 2**63-1)):
            for u in (0, 1, 2, 3, p, p+1):
                x = (p, 0, Q(u), v, n)
                k = n-v
                expected = v == 0 if u == 0 else (u % p != 0 and (k > 2 or u < p**k))
                assert call("canon", x)[3] == expected, (x, expected)
                counts["canonical_extremes"] += 1
        for _ in range(100):
            A, d, H = rng.randint(-500, 500), rng.randint(1, 200), rng.randint(0, 600)
            c = Q(A, d)
            want = exact(p, c) if H == 0 else ball(p, c, vp(Q(H, d), p))
            # The adapter sees the reduced A/d and chosen H, so compute the same raw triple.
            c = Q(c.numerator, c.denominator)
            want = exact(p, c) if H == 0 else ball(p, c, vp(Q(H, c.denominator), p))
            x, y = (p, 1, c, 0, 0), (2, 1, Q(H), 0, 0)
            assert call("project", x, y)[:2] == ("OK", want)
            counts["global_projections"] += 1
        for blocks in ((4, 9, 5), (8, 25, 7), (16, 27, 11), (2**64-59, 0, 0)):
            H = 1
            for block in blocks:
                if block:
                    H *= block
            for _ in range(50):
                c = Q(rng.randint(-1000, 1000), rng.randint(1, 200))
                x = (p, 1, c, 0, 0)
                y = (blocks[0], 1, Q(H), blocks[1], blocks[2])
                want = ball(p, c, vp(Q(H, c.denominator), p))
                for op in ("project", "projectlocal"):
                    assert call(op, x, y)[:2] == ("OK", want), (op, x, y, want)
                    counts[op] += 1
    print(dict(counts))
finally:
    proc.stdin.close()
    assert proc.wait(timeout=5) == 0
