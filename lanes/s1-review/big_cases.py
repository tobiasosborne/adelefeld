#!/usr/bin/env python3
"""One-equation oracle from elementary gcd arithmetic, with large signed entries."""
import math
import random
import subprocess


def matrix(words, pos):
    rows, cols = words[pos:pos + 2]
    pos += 2
    return [words[pos + i * cols:pos + (i + 1) * cols]
            for i in range(rows)], pos + rows * cols


def main():
    rng = random.Random(98317)
    n_values = [2 ** 1024, (2 * 3 * 5 * 7 * 11 * 13 * 17 * 19) ** 30,
                (1000000007 * 1000000009) ** 20]
    proc = subprocess.Popen(['lanes/s1-review/probe'], stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, text=True, bufsize=1)
    yes = no = 0
    for z in range(150):
        n = n_values[z % 3]
        a = (rng.randrange(-2 ** 2048, 2 ** 2048) // 2 ** (z % 80)) * (2 ** (z % 23))
        if z % 7 == 0:
            a = (z % 9 - 4) * n
        b = rng.randrange(-2 ** 2048, 2 ** 2048)
        if z % 3 == 0:
            b = a * (z - 75) + n * (z + 1)
        proc.stdin.write(f'1 1 {n} {a} {b}\n')
        proc.stdin.flush()
        words = list(map(int, proc.stdout.readline().split()))
        assert words, z
        st, canonical, verified = words[:3]
        gmat, pos = matrix(words, 3)
        _, pos = matrix(words, pos)  # E
        _, pos = matrix(words, pos)  # V
        xmat, pos = matrix(words, pos)
        ymat, pos = matrix(words, pos)
        assert pos == len(words)
        d = math.gcd(a, n)
        expected_status = 0 if b % d == 0 else 5
        expected_g = [] if d == 1 else [[n // d]]
        assert (st, canonical, verified, gmat) == (expected_status, 1, 1, expected_g), z
        if st == 0:
            assert (a * xmat[0][0] - b) % n == 0
            yes += 1
        else:
            assert (ymat[0][0] * a) % n == 0 and (ymat[0][0] * b) % n != 0
            no += 1
    proc.stdin.close()
    assert proc.wait() == 0
    print(f'cases=150 OK={yes} NO_SOLUTION={no} disagreements=0')


if __name__ == '__main__':
    main()
