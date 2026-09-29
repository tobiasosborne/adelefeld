#!/usr/bin/env python3
"""Independent rational remainder-chain and planted-root attacks. Standard library only."""
from fractions import Fraction as Q
from math import gcd, lcm
import argparse
import random
import subprocess
import sys
import time

sys.set_int_max_str_digits(0)


def trim(a):
    a = list(a)
    while a and a[-1] == 0:
        a.pop()
    return a


def mul(a, b):
    c = [0] * (len(a) + len(b) - 1)
    for i, x in enumerate(a):
        for j, y in enumerate(b):
            c[i + j] += x * y
    return trim(c)


def divrem(a, b):
    a = list(map(Q, trim(a)))
    out = [Q(0)] * max(0, len(a) - len(b) + 1)
    while len(a) >= len(b):
        k = len(a) - len(b)
        c = a[-1] / b[-1]
        out[k] = c
        for i, x in enumerate(b):
            a[k + i] -= c * x
        a = trim(a)
    return trim(out), a


def derivative(a):
    return [i * a[i] for i in range(1, len(a))]


def positive_scale(a):
    den = lcm(*(x.denominator for x in map(Q, a)))
    ints = [int(x * den) for x in a]
    content = gcd(*ints)
    return [Q(x // content) for x in ints]


def normalise(f):
    if len(f) <= 1:
        return [Q(1)]
    a, b = list(map(Q, f)), list(map(Q, derivative(f)))
    while b:
        a, b = b, divrem(a, b)[1]
        if b:
            b = positive_scale(b)
    return positive_scale(divrem(f, a)[0])


def chain(g):
    if len(g) == 1:
        return []
    out = [g, derivative(g)]
    while len(out[-1]) > 1:
        r = divrem(out[-2], out[-1])[1]
        assert r, "normalised input is squarefree"
        out.append(positive_scale([-x for x in r]))
    return out


def evaluate(g, x):
    y = Q(0)
    for c in reversed(g):
        y = y * x + c
    return y


def sign(x):
    return (x > 0) - (x < 0)


def variations(ch, x=None, side=0):
    seq = []
    for f in ch:
        s = sign(evaluate(f, x)) if side == 0 else sign(f[-1]) * (side ** (len(f) - 1))
        if s:
            seq.append(s)
    return sum(x != y for x, y in zip(seq, seq[1:]))


class Probe:
    def __init__(self, path):
        self.p = subprocess.Popen(["timeout", "170", path], stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE, text=True, bufsize=1)
        self.calls = self.roots = self.pairs = self.exact = self.checks = 0
        self.max_time = 0

    def call(self, f, prec, expected=None, oracle=True):
        self.p.stdin.write(" ".join(map(str, [prec] + f)) + "\n")
        self.p.stdin.flush()
        line = self.p.stdout.readline()
        assert line, "probe exited without a return marker"
        st, n, seconds = line.split()
        st, n = int(st), int(n)
        self.max_time = max(self.max_time, float(seconds))
        balls = []
        for _ in range(n):
            a, b, e = map(int, self.p.stdout.readline().split())
            scale = Q(2) ** e
            balls.append((a * scale, b * scale))
        self.calls += 1
        self.checks += 1
        assert st == (7 if not trim(f) else 0), (f, prec, st)
        if st == 7:
            return []
        self.roots += n
        self.exact += sum(a == b for a, b in balls)
        self.checks += max(0, n - 1)
        assert all(b < c for (_, b), (c, _) in zip(balls, balls[1:])), (f, prec, balls)
        if expected is not None:
            self.checks += len(expected) + 1
            assert n == len(expected), (n, len(expected))
            for r in expected:
                assert sum(a <= r <= b for a, b in balls) == 1, (f, prec, r, balls)
        if oracle:
            g = normalise(trim(f))
            ch = chain(g)
            cnt = variations(ch, side=-1) - variations(ch, side=1)
            self.checks += 1
            assert n == cnt, (f, n, cnt)
            for a, b in balls:
                self.checks += 2
                if a == b:
                    assert evaluate(g, a) == 0
                else:
                    assert evaluate(g, a) * evaluate(g, b) < 0
                    assert variations(ch, a) - variations(ch, b) == 1
        return balls

    def nested(self, f, precisions, expected=None, oracle=True):
        old = None
        for prec in sorted(precisions):
            new = self.call(f, prec, expected, oracle)
            if old is not None:
                self.pairs += 1
                self.checks += len(new)
                assert len(new) == len(old)
                assert all(a <= c <= d <= b for (a, b), (c, d) in zip(old, new)), (f, prec, old, new)
            old = new

    def close(self):
        self.p.stdin.close()
        assert self.p.wait(timeout=10) == 0


def planted(roots, content=1, repeated=False):
    f = [content]
    for i, r in enumerate(roots):
        for _ in range(2 if repeated and i % 2 == 0 else 1):
            f = mul(f, [-r.numerator, r.denominator])
    return f


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--probe", default="lanes/r-review1/probe")
    ap.add_argument("--random", type=int, default=400)
    args = ap.parse_args()
    p = Probe(args.probe)
    rng = random.Random(8300926)
    start = time.monotonic()
    for f in ([], [0], [0, 0, 0], [1], [-59], [1 << 100000], [0, 1], [0, 0, -13]):
        p.nested(f, [-2**63, 0, 2, 3, 53])
    for e in (0, 1, 2, 3, 7, 16, 31, 63, 127, 511, 1000):
        roots = sorted(set([Q(0), Q(1, 2**e), Q(3, 2**e), Q(-1, 2**e), Q(-3, 2**e)]))
        p.nested(planted(roots, -1234567, True), [2, 3, 7, 53, 128], roots)
    for e in (3, 31, 127, 511, 1500, 4000):
        roots = [Q(2**e) + Q(1, 3), Q(2**e) + Q(2, 3)]
        p.nested(planted(roots), [2, 3, 53, 256], roots)
    for c in range(3, 200, 7):
        roots = [Q(c, 3), Q(c + 2, 3)]
        p.nested(planted(roots), [2, 3, 4, 5, 7, 11, 32, 64], roots)
    for e in (10000, 100000):
        roots = [Q(1, 2**e)]
        p.nested(planted(roots), [2, 3, 53], roots)
    # Products of quadratic factors with positive, negative, and zero discriminants.
    for _ in range(60):
        f = [rng.choice((-7, 1, 13))]
        for _ in range(rng.randint(1, 5)):
            f = mul(f, [rng.randint(-13, 13), rng.randint(-7, 7), rng.randint(1, 7)])
        p.nested(f, [2, 3, 11, 64])
    for _ in range(args.random):
        if rng.random() < 0.5:
            roots = sorted(set(Q(rng.randint(-100, 100), rng.choice((1, 2, 3, 7, 31, 2**40)))
                               for _ in range(rng.randint(1, 9))))
            f = planted(roots, rng.choice((-11, 1, 97)), True)
            p.nested(f, [2, rng.choice((3, 7, 53, 100))], roots)
        else:
            f = [rng.randint(-1000, 1000) for _ in range(rng.randint(2, 15))]
            p.nested(f, [2, rng.choice((3, 7, 53, 100))])
    p.close()
    print(f"calls={p.calls} roots={p.roots} exact={p.exact} nested_pairs={p.pairs} "
          f"oracle_checks={p.checks} errors=0 max_call_s={p.max_time:.6f} "
          f"elapsed_s={time.monotonic() - start:.3f}")


if __name__ == "__main__":
    main()
