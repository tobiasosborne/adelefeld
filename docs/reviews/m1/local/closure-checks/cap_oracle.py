#!/usr/bin/env python3
"""Independent small rational oracle for the five caps on local and mixed inputs."""

from fractions import Fraction as Q
from math import gcd, lcm
import random
import subprocess
import sys


def rgcd(values):
    den = lcm(*(x.denominator for x in values))
    return Q(gcd(*(int(x * den) for x in values)), den)


def canonical(center, radius):
    if radius:
        center %= radius
    den = lcm(center.denominator, radius.denominator)
    return (int(center * den), int(radius * den), den)


def hull(op, x, y):
    a, n = x
    b, m = y
    if op == "a":
        values = [a + i * n + b + j * m for i in (0, 1) for j in (0, 1)]
    elif op == "b":
        values = [a + i * n - b - j * m for i in (0, 1) for j in (0, 1)]
    else:
        values = [(a + i * n) * (b + j * m) for i in (0, 1) for j in (0, 1)]
    return values[0], rgcd([v - values[0] for v in values])


def ball(ctx, residue, denominator):
    modulus = {0: 4, 1: 4, 2: 900, 3: 900, 9: 2}[ctx]
    center, radius = Q(residue, denominator), Q(modulus, denominator)
    return (center, radius), canonical(center, radius)


def run(probe):
    rng = random.Random(20260929)
    cases = []
    contexts = ((1, 1), (1, 2), (2, 3), (1, 0), (0, 1), (9, 1))
    caps = (Q(1, 2), Q(1), Q(3, 2), Q(2), Q(4), Q(5))
    for cx, cy in contexts:
        for op in "cabtr":
            for alias in range(4):
                for cap in caps:
                    for sign in (-1, 1):
                        x, xt = ball(cx, rng.randrange(4) * sign, rng.randrange(1, 9))
                        y, yt = ball(cy, rng.randrange(4) * sign, rng.randrange(1, 9))
                        scalar = Q(rng.choice((-3, -2, -1, 0, 1, 2, 3)), rng.randrange(1, 5))
                        if alias == 3:
                            y, yt = x, xt
                        if op == "c":
                            value = x
                        elif op == "r":
                            value = scalar * x[0], abs(scalar) * x[1]
                        else:
                            value = hull(op, x, y)
                        center, radius = value
                        expected = canonical(center, rgcd((radius, cap))) if radius else canonical(center, radius)
                        fields = [op, cx, cy, alias, 1, *xt, *yt,
                                  scalar.numerator, scalar.denominator, cap.numerator, cap.denominator]
                        cases.append((" ".join(map(str, fields)), expected))
    result = subprocess.run([probe], input="\n".join(s for s, _ in cases) + "\n",
                            text=True, capture_output=True, timeout=60)
    if result.returncode:
        raise RuntimeError(f"probe exit {result.returncode}: {result.stderr[:1000]}")
    lines = result.stdout.splitlines()
    assert len(lines) == len(cases), (len(lines), len(cases))
    failures = []
    for i, (line, (_, want)) in enumerate(zip(lines, cases)):
        fields = tuple(map(int, line.split()))
        if fields[0] != 0 or fields[2] != 1 or fields[4:7] != want:
            failures.append((i, fields[:7], want))
    print(f"cases={len(cases)} mismatches={len(failures)}")
    for failure in failures[:5]:
        print(failure)
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(run(sys.argv[1]))
