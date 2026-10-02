#!/usr/bin/env python3
# Separate old-code fixture. Seed 20261002; stored.jsonl and its seed are unchanged.
# Per prime: 32 log/Log rows at K=39..64; 24 at K=65..3000; three rows on EACH
# side of the tagged-word boundary for v(z)=c; four exp rows at K=40,64,65,200.
# The word prime has three additional K=2 rows (its first non-shortcut route),
# and all 24 large rows have K<=1499. Each K=64 and K=65 has at least three rows.
# Families mix exact integers/rationals, dense balls, both signs at 2, the powered
# Log path, negative Log valuations, and requests above/below the ball precision.
# F9 uses v(z)=c in the large families: c=1 at odd primes, c=2 at 2. The latter
# cannot enter F9 with v(z)=1: Log_centre first changes the sign modulo 4.
# Counts on FLINT_BITS=64 (rows, word, F8, F9, shortcut, exp):
#   2: (66,8,30,24,0,4); 3,5,7,11,65537: each (66,3,35,24,0,4);
#   2^64-59: (69,0,35,24,6,4). Total (465,23,240,168,6,28).
# K=39..64 log/Log counts: 38 at 2, 32 at each other prime; K=65..3000: 24 each.
# At least 16 rows per prime have K>=200 and v(z)=c (17 except at 7).
# Tagged-word boundaries for v(z)=c: 57,36,24,21,16,3,0 in PRIMES order.
# Route counts are printed and asserted below; the route rule is read directly
# from src/lfunc.c:535-604, not the review's estimate K+e(2K).

import argparse
from collections import Counter
from fractions import Fraction
import json
from pathlib import Path
import random
import subprocess
import sys
import time

sys.set_int_max_str_digits(0)
ROOT = Path(__file__).resolve().parents[2]
LANE = ROOT / 'lanes/f-fixture1'
BIG = 2**64 - 59
PRIMES = (2, 3, 5, 7, 11, 65537, BIG)
SEED = 20261002


def floor_log(k, p):
    e = 0
    while k >= p:
        k //= p
        e += 1
    return e


def count_log(p, K, v):
    # src/lfunc.c:106-127: the first k with k*v-floor_log(k,p)>=K, minus 1.
    k = 1
    while k * v - floor_log(k, p) < K:
        k += 1
    return k - 1


def valuation(n, p):
    assert n
    v = 0
    while n % p == 0:
        n //= p
        v += 1
    return v


def word_boundary(p, bits):
    c = 2 if p == 2 else 1
    K = 0
    while True:
        n = K + 1
        W = n + floor_log(max(1, count_log(p, n, c)), p)
        if (p**W).bit_length() > bits - 2:
            return K
        K = n


def route(row, bits):
    x = row['x']
    p, un, ud = x['p'], x['un'], x['ud']
    c = 2 if p == 2 else 1
    K = row['N'] if x['exact'] else min(row['N'], max(c, x['N'] - x['v']))
    if row['f'] == 'exp':
        return 'exp', min(row['N'], x['N']) if not x['exact'] else row['N'], None, None
    if K <= c or abs(un) == ud:
        return 'shortcut', K, None, None
    if p == 2:
        zn = un - ud if (un - ud) % 4 == 0 else -un - ud
    elif (un - ud) % p == 0:
        zn = un - ud
    else:
        W = K + floor_log(max(1, count_log(p, K, 1)), p)
        P = p**W
        zn = (pow(un * pow(ud, -1, P) % P, p - 1, P) - 1) % P
        v = valuation(zn, p) if zn else W
        if v >= K:
            return 'shortcut', K, W, v
        return ('word' if P.bit_length() <= bits - 2 else 'F8' if K <= 64 else 'F9'), K, W, v
    v = valuation(zn, p)
    if v >= K:
        return 'shortcut', K, None, v
    W = K + floor_log(max(1, count_log(p, K, v)), p)
    return ('word' if (p**W).bit_length() <= bits - 2 else 'F8' if K <= 64 else 'F9'), K, W, v


def cases(bits):
    rng = random.Random(SEED)
    rows = []
    mid = [64]*3 + [39, 40, 41, 48, 55] + list(range(59, 65))*4
    large = [65]*3 + [80, 96, 128, 199, 200, 201, 223, 255, 256, 257, 299, 300,
                      383, 511, 512, 513, 767, 999, 1499, 2048, 3000]
    for p in PRIMES:
        c = 2 if p == 2 else 1
        b = word_boundary(p, bits)
        high = large if p != BIG else large[:-5] + [600, 767, 999, 1200, 1499]
        for group, precisions in [('mid', mid), ('large', high), ('boundary', [b]*3 + [b+1]*3)]:
            for i, K in enumerate(precisions):
                f = 'log' if i % 2 == 0 else 'Log'
                ex = int(i % 8 in (0, 3, 4, 5))
                v = 0 if f == 'log' else (-1 - i % 5 if i % 8 != 7 else 2)
                # Boundary cases have exactly v(z)=c and W based on that valuation.
                w = c if group != 'mid' else c + (i // 8) % 3
                E = max(c + 1, K + (3 if i % 4 == 2 else 0))
                N = K + (3 if not ex and E == K and i % 4 == 1 else 0)
                unit = rng.randrange(1, 20)
                while unit % p == 0:
                    unit += 1
                a = Fraction(1 + p**w * unit)
                if group != 'boundary':
                    if i % 8 in (1, 2, 6):
                        q = rng.randrange(1, p**max(1, E-w))
                        while q % p == 0:
                            q += 1
                        a = Fraction(1 + p**w * q)
                    elif i % 8 in (4, 5):
                        d = p + 1
                        a = Fraction(d + p**w * unit, d)
                    elif i % 8 in (3, 7) and p > 3:
                        # Unit outside 1+p Z_p: exercise a^(p-1), also with v<0.
                        a = Fraction(2 + p * rng.randrange(1, 20))
                    if p == 2 and i % 3 == 1:
                        a = -a
                M = 0 if ex else v + E
                if not ex:
                    modulus = p**E
                    a = Fraction(a.numerator * pow(a.denominator, -1, modulus) % modulus)
                rows.append(dict(f=f, N=N, x=dict(p=p, un=a.numerator, ud=a.denominator,
                                                 v=v, N=M, exact=ex)))
        if p == BIG:
            for i in range(3):
                rows.append(dict(f='log', N=2, x=dict(p=p, un=1 + (i+1)*p, ud=1, v=0, N=0, exact=1)))
        for i, K in enumerate((40, 64, 65, 200)):
            ex = i % 2
            rows.append(dict(f='exp', N=K, x=dict(p=p, un=1+p, ud=1, v=c, N=0 if ex else K,
                                                exact=ex)))
    return rows


def line(row):
    x = row['x']
    return ' '.join(map(str, (row['f'], x['p'], x['un'], x['ud'], x['v'], x['N'], x['exact'], row['N'])))


def audit(rows, bits):
    total = Counter()
    for p in PRIMES:
        part = [r for r in rows if r['x']['p'] == p]
        info = [route(r, bits) for r in part]
        counts = Counter(t[0] for t in info)
        middle = sum(39 <= K <= 64 and r != 'exp' for r, K, _, _ in info)
        high = sum(65 <= K <= 3000 and r != 'exp' for r, K, _, _ in info)
        deep = sum(r == 'F9' and K >= 200 and v == (2 if p == 2 else 1) for r, K, _, v in info)
        assert 20 <= middle <= 50 and 20 <= high <= 50 and counts['F8'] >= 20 and deep >= 5
        for edge in (64, 65, word_boundary(p, bits), word_boundary(p, bits)+1):
            assert sum(K == edge and r != 'exp' for r, K, _, _ in info) >= 3
        total.update(counts)
        print(f'p={p} rows={len(part)} boundary={word_boundary(p, bits)} routes={dict(counts)} '
              f'K39..64={middle} K65..3000={high} deep={deep}', flush=True)
    print(f'rows={len(rows)} routes={dict(total)}', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', type=Path, default=LANE / 'build/stored_probe')
    parser.add_argument('--plan', action='store_true', help='audit inputs only; no fixture write')
    parser.add_argument('--check', action='store_true', help='regenerate and compare without writing')
    args = parser.parse_args()
    bits = int(subprocess.check_output(['timeout', '5', str(args.probe), '--bits'], text=True))
    assert bits == 64, 'this word-prime fixture requires FLINT_BITS=64'
    rows = cases(bits)
    audit(rows, bits)
    if args.plan:
        return
    (LANE / 'stored_large.in').write_text('\n'.join(map(line, rows)) + '\n')
    output = []
    for p in PRIMES:
        part = [r for r in rows if r['x']['p'] == p]
        t0 = time.monotonic()
        r = subprocess.run(['timeout', '60', str(args.probe)], input='\n'.join(map(line, part)) + '\n',
                           capture_output=True, text=True, check=True)
        answers = r.stdout.splitlines()
        assert len(answers) == len(part)
        for row, answer in zip(part, answers):
            st, ex, v, N, un, ud, alias, unchanged, canonical = map(int, answer.split())
            assert (alias, unchanged, canonical) == (1, 1, 1)
            assert st == 0 and ex == 0 and N == route(row, bits)[1]
            output.append(dict(f=row['f'], N=row['N'], st=st, x=row['x'],
                               y=dict(p=p if st == 0 else 7, un=un, ud=ud, v=v, N=N, exact=ex)))
        print(f'old p={p} calls={2*len(part)} seconds={time.monotonic()-t0:.3f}', flush=True)
    data = ''.join(json.dumps(row, separators=(',', ':')) + '\n' for row in output)
    assert len(data.encode()) < 1000000
    dest = ROOT / 'tests/ref/vectors/f-slice5/stored_large.jsonl'
    if args.check:
        assert dest.read_text() == data
    else:
        dest.write_text(data)
    print(f'fixture rows={len(output)} bytes={len(data.encode())} check={args.check}', flush=True)


if __name__ == '__main__':
    main()
