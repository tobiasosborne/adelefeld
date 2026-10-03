#!/usr/bin/env python3
"""f-repair5: differential run of adf_lball_roots (mode A) and adf_lball_root_seed (mode S), the code at 8f57a51
(h_old) against this lane's (h_new), through the harness of review f-review7 (lanes/f-review7/h.c; aliasing and
untouched outputs checked inside, TOUCHED / ALIASDIFF printed).
Inputs: odd p < 300, 65537, 2^64 - 59 and 2; x exact or a ball, unit or scaled by p^(n j), centre a power t^n times
a random unit (so that roots often exist), exponents 1..40 and 2^27, 2^40, 2^60; n from 1 to 40, divisors of p - 1,
p - 1 and its multiples; N small, near E, huge, LONG_MAX, LONG_MIN; capacity d, d - 1, 0.
Assertion (R6 step 6, R9 of docs/api-1f5.md): every output line identical, statuses included. The values may not
change (R9: the reduced representative is unique), and the status of a list may not change (R6 step 6: the early
status is the status of the evaluation).
Usage: diff_roots.py COUNT SEED"""
import random, subprocess, sys
sys.set_int_max_str_digits(0)
HERE = sys.path[0]
SMALL = [q for q in range(3, 300) if all(q % r for r in range(2, int(q ** 0.5) + 1))]
P64 = 2 ** 64 - 59


def divisors(m, cap=10 ** 6):
    out = [k for k in range(1, min(m, 2000) + 1) if m % k == 0]
    return out


def gen(rng):
    p = rng.choice(SMALL[:20] + SMALL[20:] + [65537, P64, 2, 2, 3, 5, 7])
    big = p > 10 ** 4
    if p == 2:
        n = rng.choice([1, 2, 3, 4, 5, 6, 8, 12, 2 ** 20 + 1])
    elif big:
        n = rng.choice([2, 3, 4, 8, 16, 1094, 6017, 2 * 1094, 3014, 11, 137, 547, 2 ** 16, 4096, 3 * 2 ** 16])
    else:
        n = rng.choice(divisors(p - 1) + [rng.randrange(1, 41), p - 1, 2 * (p - 1), p, p * (p - 1)])
    j = rng.choice([0, 0, 0, 1, -1, 2])
    # a centre with roots: t^n times a unit close to 1
    t = rng.randrange(1, min(p, 10 ** 6))
    if t % p == 0:
        t = 1
    if p == 2:
        t = rng.randrange(1, 64, 2)
    exact = rng.random() < 0.3
    M = rng.choice([1, 2, 3, 5, 8, 12, 20, 40, 2 ** 27, 2 ** 40, 2 ** 60]) if not exact else 0
    pw = p ** min(max(M, 1), 30)
    if exact:
        c = pow(t, min(n, 60), 10 ** 30) * rng.choice([1, 1, 1 + p ** 3, 1 + p * rng.randrange(1, 50)])
        if c % p == 0:
            c = 1
        u, ud = c, 1
        if rng.random() < 0.2:
            u, ud = rng.choice([(1, 1), (-1, 1), (4, 1), (8, 1), (1, 4) if p != 2 else (1, 9)])
            if u % p == 0 or ud % p == 0:
                u, ud = 1, 1
        v = n * j if n < 50 else 0
        return "%d %d %d %d 0 1" % (p, u, ud, v), n, p
    c = (pow(t, n, pw) * rng.choice([1, 1, 1 + p ** 2, 1 + p * rng.randrange(1, 50)])) % pw
    if c % p == 0:
        c = 1
    if M > 30 and c >= 2 ** 64:
        c = c % (2 ** 64) or 1
        if c % p == 0:
            c += 1
    v = n * j if n < 50 else 0
    if M <= 30:
        c %= p ** M
        if c == 0:
            c = 1
    return "%d %d 1 %d %d 0" % (p, c, v, v + M), n, p


def main():
    count, seed = int(sys.argv[1]), int(sys.argv[2])
    rng = random.Random(seed)
    lines = []
    for _ in range(count):
        x, n, p = gen(rng)
        N = rng.choice([rng.randrange(-3, 25), 0, 20, 2 ** 40, 2 ** 60, 2 ** 63 - 1, -(2 ** 63), 2 ** 27 - 1])
        cap = rng.choice([10 ** 6, 10 ** 6, 0, 1, 2])
        lines.append("A %s %d %d %d" % (x, n, N, min(cap, 400000)))
        if rng.random() < 0.3:
            lines.append("S %s %d %d %d" % (x, n, rng.randrange(0, min(p, 100)), N))
    inp = "\n".join(lines) + "\n"
    old = subprocess.run([HERE + "/h_old"], input=inp, capture_output=True, text=True, timeout=1500).stdout.split("\n")
    new = subprocess.run([HERE + "/h_new"], input=inp, capture_output=True, text=True, timeout=1500).stdout.split("\n")
    stats, fail, branches = {}, 0, 0
    for i, line in enumerate(lines):
        o, n = old[i], new[i]
        key = line[0] + " " + o.split()[0]
        stats[key] = stats.get(key, 0) + 1
        if o != n or "TOUCHED" in n.split() or "ALIAS" in n:
            fail += 1
            print("DIFF", line, "| old", o[:300], "| new", n[:300])
        if line[0] == "A" and o.startswith("OK"):
            branches += int(o.split()[1])
    print("lines %d (lists and seeded calls): failures %d; branches compared %d; statuses %s"
          % (len(lines), fail, branches, dict(sorted(stats.items()))))
    sys.exit(1 if fail else 0)


main()
