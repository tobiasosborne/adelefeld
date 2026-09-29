#!/usr/bin/env python3
"""Independent small-ring enumeration of solver results and the Howell conditions."""
import argparse
import itertools
import math
import random
import subprocess


def span(rows, n, width):
    out = {(0,) * width}
    for row in rows:
        out = {tuple((v[j] + a * row[j]) % n for j in range(width))
               for v in out for a in range(n)}
    return out


def matrix(words, pos):
    r, c = words[pos:pos + 2]
    pos += 2
    return [tuple(words[pos + i * c:pos + (i + 1) * c]) for i in range(r)], c, pos + r * c


def check(n, a, b, reply):
    r, c = len(a), len(a[0]) if a else b[1]
    words = list(map(int, reply.split()))
    st, canonical, verified = words[:3]
    pos = 3
    mats = []
    for _ in range(5):
        m, width, pos = matrix(words, pos)
        mats.append((m, width))
    assert pos == len(words)
    g, gc = mats[0]
    e, ec = mats[1]
    v, vc = mats[2]
    x0, xc = mats[3]
    y, yc = mats[4]
    assert (gc, ec, vc) == (c, r, c)
    points = list(itertools.product(range(n), repeat=c))
    sols = {x for x in points if all(sum(a[i][j] * x[j] for j in range(c)) % n == b[0][i] % n
                                    for i in range(r))}
    ker = {x for x in points if all(sum(a[i][j] * x[j] for j in range(c)) % n == 0
                                   for i in range(r))}
    assert st == (0 if sols else 5), (st, sols)
    assert canonical == 1 and verified == 1
    assert span(g, n, c) == ker, (g, ker)
    if st == 0:
        assert (len(x0), xc, len(y), yc) == (c, 1, 0, 0)
        z = tuple(row[0] for row in x0)
        assert {tuple((z[j] + k[j]) % n for j in range(c)) for k in ker} == sols
    else:
        assert (len(x0), xc, len(y), yc) == (0, 0, r, 1)
        yy = tuple(row[0] for row in y)
        assert all(sum(yy[i] * a[i][j] for i in range(r)) % n == 0 for j in range(c))
        assert sum(yy[i] * b[0][i] for i in range(r)) % n != 0
    # Test (E1) to (E4) directly from their definitions, using the enumerated kernel.
    pivots = []
    for row in g:
        js = [j for j, x in enumerate(row) if x]
        assert js
        j = js[0]
        h = row[j]
        assert n % h == 0 and (not pivots or pivots[-1][0] < j)
        pivots.append((j, h))
    for i, (j, h) in enumerate(pivots):
        assert all(0 <= g[l][j] < h for l in range(i))
        assert {x for x in ker if all(x[t] == 0 for t in range(j + 1))} == span(g[i + 1:], n, c)
    # The complete left certificate is independently tested by enumeration of the image.
    assert len(e) == len(v)
    for erow, vrow in zip(e, v):
        assert erow == tuple(sum(a[i][j] * vrow[j] for j in range(c)) % n for i in range(r))
    nonfree = n > 1 and len(ker) not in {n ** d for d in range(c + 1)}
    return st, nonfree


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--cases', type=int, default=5000)
    parser.add_argument('--seed', type=int, default=371)
    parser.add_argument('--probe', default='lanes/s1-review/probe')
    args = parser.parse_args()
    rng = random.Random(args.seed)
    proc = subprocess.Popen([args.probe], stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, text=True, bufsize=1)
    counts = {0: 0, 5: 0}
    categories = dict(n1=0, r_gt_c=0, r_lt_c=0, r0=0, c0=0, r0_c0=0,
                      all_nonunits=0, all_nonzero_zero_divisors=0, nonfree_kernel=0,
                      planted=0, negative_a=0,
                      nondividing_pivot=0)
    moduli = [1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 16, 18, 30]
    for z in range(args.cases):
        n = moduli[z % len(moduli)]
        r = rng.randrange(4)
        c = rng.randrange(4)
        if n ** c > 6000:
            c = 2
        a = [[rng.randrange(-3 * n, 3 * n + 1) for _ in range(c)] for _ in range(r)]
        bb = [rng.randrange(-3 * n, 3 * n + 1) for _ in range(r)]
        if z % 5 == 0:
            a = [[rng.randrange(-3, 4) * n for _ in range(c)] for _ in range(r)]
        if z % 7 == 0:
            a = [[rng.randrange(0, n) * rng.randrange(0, n) for _ in range(c)] for _ in range(r)]
        if z % 11 == 0:
            bb = [sum(a[i][j] * rng.randrange(n) for j in range(c)) for i in range(r)]
            categories['planted'] += 1
        categories['n1'] += n == 1
        categories['r_gt_c'] += r > c
        categories['r_lt_c'] += r < c
        categories['r0'] += r == 0
        categories['c0'] += c == 0
        categories['r0_c0'] += r == c == 0
        categories['negative_a'] += any(x < 0 for row in a for x in row)
        categories['all_nonunits'] += bool(r * c) and all(math.gcd(x, n) != 1
                                                         for row in a for x in row)
        categories['all_nonzero_zero_divisors'] += bool(r * c) and all(
            x % n != 0 and math.gcd(x, n) != 1 for row in a for x in row)
        if r == c == 1 and n > 1 and math.gcd(a[0][0], n) > 1 and bb[0] % math.gcd(a[0][0], n):
            categories['nondividing_pivot'] += 1
        fields = [r, c, n] + [x for row in a for x in row] + bb
        proc.stdin.write(' '.join(map(str, fields)) + '\n')
        proc.stdin.flush()
        reply = proc.stdout.readline()
        if not reply:
            raise RuntimeError(f'probe stopped at case {z}, exit {proc.poll()}')
        try:
            st, nonfree = check(n, a, (bb, c), reply)
        except AssertionError as ex:
            print('FAIL case', z, 'input', fields, 'reply', reply.strip(), 'detail', ex)
            raise
        counts[st] += 1
        categories['nonfree_kernel'] += nonfree
    proc.stdin.close()
    assert proc.wait() == 0
    print(f"cases={args.cases} OK={counts[0]} NO_SOLUTION={counts[5]} disagreements=0")
    print(' '.join(f'{key}={value}' for key, value in categories.items()))


if __name__ == '__main__':
    main()
