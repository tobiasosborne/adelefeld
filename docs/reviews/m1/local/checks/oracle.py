#!/usr/bin/env python3
"""Independent oracle: hulls from rational members, with integer/Fraction arithmetic.

For a+b and a*b, use u,v in {0,1}. Differences of these four witnesses generate
all differences: the product difference is u*a*M + v*b*N + u*v*N*M.
The three coefficients are integer combinations of the witness differences.
Thus their rational gcd is the tight radius. No project implementation is imported
by this oracle. --reference separately compares the project Python reference.
"""
from fractions import Fraction as F
from math import gcd, lcm, prod
from pathlib import Path
import argparse
import random
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[5]
HERE = Path(__file__).resolve().parent
P = []
n = 2
while len(P) < 128:
    if all(n % p for p in P if p*p <= n):
        P.append(n)
    n += 1
BLOCKS = {0: (), 1: (4,), 2: (4, 9, 25), 3: (25, 9, 4), 4: (4, 9, 25),
          5: (2**64-1,), 6: (2**64-2, 2**64-1), 7: tuple(P), 8: (), 9: (2,)}
K = {i: prod(b) for i, b in BLOCKS.items()}
rng = random.Random(20260928)


def ratgcd(values):
    D = lcm(*(v.denominator for v in values))
    return F(gcd(*(int(v*D) for v in values)), D)


def canon(c, r):
    c, r = F(c), F(r)
    if r:
        c %= r
    D = lcm(c.denominator, r.denominator)
    return int(c*D), int(r*D), D


def cr(t):
    a, h, d = t
    return F(a, d), F(h, d)


def hull(op, x, y):
    a, N = cr(x)
    b, M = cr(y)
    vals = [a + i*N + b + j*M if op == '+' else
            a + i*N - b - j*M if op == '-' else
            (a + i*N)*(b + j*M) for i in (0, 1) for j in (0, 1)]
    return canon(vals[0], ratgcd([v-vals[0] for v in vals]))


def inside(x, y):
    a, N = cr(x)
    b, M = cr(y)
    if M == 0:
        return N == 0 and a == b
    return (N/M).denominator == 1 and ((a-b)/M).denominator == 1


def predicates(x, y, q, cx, cy):
    a, N = cr(x)
    b, M = cr(y)
    g = ratgcd([N, M])
    ov = a == b if g == 0 else ((a-b)/g).denominator == 1
    eq = inside(x, y) and inside(y, x)
    comp = 0 if N == M == 0 and eq else 2 if ov else 1
    return [int(eq), int(ov), int(inside(x, y)), comp,
            int(inside(canon(q, 0), x)), int(cx == cy and eq)]


def local_possible(t, ci):
    c, R = cr(t)
    return R != 0 and (F(K[ci])/R).denominator == 1 and (c*K[ci]/R).denominator == 1


def expected(op, cx, cy, alias, target, x, y, q, C):
    x, y = canon(*cr(x)), canon(*cr(y))
    if alias == 3:
        y, cy = x, cx
    out = x if alias in (1, 3) else y if alias == 2 else (1, K[7], 1)
    outctx = cx if alias in (1, 3) else cy if alias == 2 else 7
    if op == 'p':
        return predicates(x, y, q, cx, cy)
    st, lost, ci = 0, -1, 0
    if op in '+-*abt':
        t = hull({'a': '+', 'b': '-', 't': '*'}.get(op, op), x, y)
        if op in '+-*' and cx and cx == cy and local_possible(t, cx):
            ci = cx
    elif op in 'swn':
        a, R = cr(x)
        t = canon(-a if op == 'n' else a, R)
        ci = cx
    elif op in 'mdr':
        a, R = cr(x)
        if op == 'd' and q == 0:
            st, t, ci = 6, out, outctx
        else:
            s = 1/q if op == 'd' else q
            t = canon(s*a, abs(s)*R)
            if op != 'r' and cx and local_possible(t, cx):
                ci = cx
    elif op in 'le':
        a, R = cr(x)
        if not BLOCKS[target]:
            st = 8
        elif R == 0 or (op == 'l' and not local_possible(x, target)):
            st = 7
        if st:
            t, ci = out, outctx
        else:
            kr = F(K[target])/R
            D = lcm(kr.numerator, a.denominator)
            t, ci = canon(a, F(K[target], D)), target
            if op == 'e':
                lost = int(t != x)
    elif op in 'gc':
        t = x
    else:
        raise AssertionError(op)
    if op in 'cabtr':
        # Valid global cap cases only. Rejection of local inputs is a separate finding.
        assert cx == 0 and (cy == 0 or op in 'cr')
        if C <= 0:
            st, t, ci = 7, out, outctx
        else:
            a, R = cr(t)
            t = canon(a, ratgcd([R, C])) if R else t
    A, H, d = t
    rawd = int(F(K[ci]*d, H)) if ci else d
    res = [int(F(A*rawd, d)) % b for b in BLOCKS[ci]] if ci else []
    return [st, lost, 1, ci, A, H, d, rawd, *res]


def record(op, cx, cy, alias, target, x, y, q=F(1), C=F(1)):
    fields = [op, cx, cy, alias, target, *x, *y, q.numerator, q.denominator,
              C.numerator, C.denominator]
    return ' '.join(map(str, fields)), expected(op, cx, cy, alias, target, x, y, q, C)


def makeball(ci, big=False):
    H = K[ci] if ci else rng.randrange(0, 1000)
    A = rng.choice([0, 1, 2, 6, H-1, rng.randrange(-3*max(H, 1), 3*max(H, 1))])
    d = rng.choice([1, 2, 4, 6, 12, 30, 900, 1800, rng.randrange(1, 10000)])
    if big:
        d *= 2**4096 + 17
    return A, H, d


def run(probe, mode):
    rows, counts = [], {}
    def add(*args):
        row = record(*args)
        rows.append(row)
        counts[args[0]] = counts.get(args[0], 0) + 1
    # Exhaust all residues and denominators 1..8 at block 4 for all binary alias patterns.
    for A in range(4):
        for B in range(4):
            for d in range(1, 9):
                for e in range(1, 9):
                    for op in '+-*':
                        for alias in range(4):
                            add(op, 1, 1, alias, 1, (A, 4, d), (B, 4, e))
    # Context order, max word, composite blocks, cross-context outputs, large denominators.
    for i in range(1800):
        cx = rng.choice([0, 1, 2, 3, 4, 5, 6, 7, 9])
        cy = rng.choice([0, cx, cx, 2, 3, 4])
        x, y = makeball(cx, i % 97 == 0), makeball(cy, i % 131 == 0)
        q = F(rng.randrange(-20, 21), rng.randrange(1, 21))
        alias = i % 4
        for op in ['+', '-', '*', 'n', 'm', 'd', 'p', 's', 'g', 'w']:
            add(op, cx, cy, alias, 1, x, y, q)
        for op in 'le':
            add(op, cx, cy, alias, rng.choice(list(range(1, 10))), x, y, q)
    # Same set, different raw data in contexts (4), (2) and global; identity is different.
    for d in range(1, 81):
        for a in range(2):
            add('p', 1, 9, 0, 1, (2*a, 4, 2*d), (a, 2, d), F(a, d))
            add('p', 2, 3, 0, 1, (a, 900, d), (a, 900, d), F(a, d))
    for i in range(800):
        x, y = makeball(0), makeball(0)
        C = F(rng.randrange(-2, 30), rng.randrange(1, 30))
        q = F(rng.randrange(-20, 21), rng.randrange(1, 21))
        for op in 'cabtr':
            add(op, 0, 0, i % 4, 1, x, y, q, C)
    # Fifty-step expression chains; compare separately accumulated tight and capped results.
    chainsteps = 0
    for chain in range(20):
        C = F(chain + 1, chain + 3)
        tight = canon(F(1, 2), F(chain+2, chain+5))
        capped = canon(cr(tight)[0], ratgcd([cr(tight)[1], C]))
        for step in range(50):
            y = makeball(0)
            op = rng.choice('abt')
            newtight = hull({'a': '+', 'b': '-', 't': '*'}[op], tight, y)
            inp = record(op, 0, 0, step % 3, 1, capped, y, F(1), C)
            rows.append(inp)
            counts[op] = counts.get(op, 0) + 1
            newcap = tuple(inp[1][4:7])
            assert inside(newtight, newcap)
            R = cr(newcap)[1]
            assert R == 0 or (C/R).denominator == 1
            add('c', 0, 0, 1, 1, newcap, y, F(1), C)  # idempotence
            tight, capped = newtight, newcap
            chainsteps += 1
    if mode == 'small':
        rows = rows[:400] + rows[12288:12588] + rows[-200:]
    payload = '\n'.join(line for line, want in rows) + '\n'
    got = subprocess.run([probe], input=payload, text=True, capture_output=True, timeout=150)
    if got.returncode:
        print(got.stderr)
        raise SystemExit(got.returncode)
    lines = got.stdout.splitlines()
    assert len(lines) == len(rows), (len(lines), len(rows))
    failures = 0
    for i, ((inp, want), line) in enumerate(zip(rows, lines)):
        result = list(map(int, line.split()))
        if result != want:
            failures += 1
            if failures <= 5:
                print('FAIL', i, inp, '\n got', result, '\nwant', want)
    print('requests=', len(rows), 'mismatches=', failures, 'mode=', mode)
    if mode != 'small':
        print('operations=', counts, 'chain_steps=', chainsteps, 'chains=20, length=50')
        print('contexts=9; max_blocks=128; max_block=18446744073709551615; large_denominator_bits>=4097')
    assert failures == 0


def reference():
    sys.path.insert(0, str(ROOT / 'tests/ref'))
    from adfref import local_ref as L
    mismatches = 0
    cases = 0
    for ci in (1, 2, 3, 5, 6, 7, 9):
        ctx = L.Ctx(BLOCKS[ci])
        for i in range(100):
            x, y = makeball(ci, i % 47 == 0), makeball(ci)
            X = L.Local(ctx, x[2], ctx.reduce(x[0]))
            Y = L.Local(ctx, y[2], ctx.reduce(y[0]))
            for op, fn in [('+', L.add), ('-', L.sub), ('*', L.mul)]:
                z = fn(X, Y)
                want = hull(op, x, y)
                g = L.to_global(z)
                actual = g.A, g.H, g.d
                loc = isinstance(z, L.Local)
                mismatches += actual != want or loc != local_possible(want, ci)
                cases += 1
    print('reference_cases=', cases, 'mismatches=', mismatches)
    assert mismatches == 0


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('--probe', default=str(HERE / 'probe'))
    ap.add_argument('--mode', default='full', choices=['full', 'small'])
    ap.add_argument('--reference', action='store_true')
    a = ap.parse_args()
    reference() if a.reference else run(a.probe, a.mode)
