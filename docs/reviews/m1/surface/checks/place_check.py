#!/usr/bin/env python3
"""place_check.py: adf_place_prime against a deterministic Miller-Rabin written here.
Bases 2..37 (the first 12 primes) are deterministic for n < 3.3e24 (Sorenson and Webster 2015).
Run from the repository root after `make`; builds build/review_place_check."""
import random, subprocess
subprocess.run(["cc", "-Iinclude", "-O1", "-g", "docs/reviews/m1/surface/checks/place_check.c",
                "build/libadelefeld.a", "-lflint", "-lgmp", "-lm", "-o", "build/review_place_check"],
               check=True)


def isprime(n):
    if n < 2:
        return False
    small = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37]
    for p in small:
        if n % p == 0:
            return n == p
    d, s = n - 1, 0
    while d % 2 == 0:
        d //= 2
        s += 1
    for a in small:
        x = pow(a, d, n)
        if x in (1, n - 1):
            continue
        for _ in range(s - 1):
            x = x * x % n
            if x == n - 1:
                break
        else:
            return False
    return True


rng = random.Random(5)
M = 2 ** 64
nums = list(range(0, 3000)) + [M - k for k in range(1, 3000)]
nums += [rng.randrange(M) for _ in range(100000)]
nums += [rng.randrange(2 ** 32) for _ in range(50000)]
# strong pseudoprimes to several bases, Carmichael numbers, products of two large primes
nums += [2047, 1373653, 25326001, 3215031751, 2152302898747, 3474749660383, 341550071728321,
         3825123056546413051, 561, 1105, 1729, 2465, 2821, 6601, 8911, 4294967291 ** 2,
         4294967279 * 4294967291]
nums = [n for n in nums if 0 <= n < M]
p = subprocess.run(["build/review_place_check"], input="\n".join(map(str, nums)) + "\n",
                   capture_output=True, text=True)
bad = 0
for line in p.stdout.splitlines():
    f = line.split()
    if f[0] == "TOUCHED":
        bad += 1
        print(line)
        continue
    n, st, arch, get = int(f[0]), int(f[1]), int(f[2]), int(f[3])
    want = isprime(n)
    if want != (st == 0) or (want and (arch != 0 or get != n)) or (not want and st != 7):
        bad += 1
        print("BAD", line, want)
print(p.stderr.strip())
print("numbers %d, primes %d, disagreements %d, exit %d"
      % (len(nums), sum(map(isprime, nums)), bad, p.returncode))
