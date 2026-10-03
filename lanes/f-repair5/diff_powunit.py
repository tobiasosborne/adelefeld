#!/usr/bin/env python3
"""f-repair5: differential run of adf_lball_powunit, the code at 8f57a51 (h_old) against this lane's (h_new),
through the harness of review f-review7 (lanes/f-review7/h.c, mode W, aliasing compared inside).

Inputs: p in 2, 3, 5, 7, 65537; u a ball in 1 + p Z_p (odd centre at 2) of exponent 1..9, or 2^30, 2^40, 2^60, or
an exact unit; s an exact integer (small, multiples of p, near 2^63, beyond a slong), an exact non-integer of Z_p,
or a ball; N small, near the exponents, huge, negative.
Assertion (P9 of docs/api-1f6.md): where the old code returned OK, the new returns the identical line; where it
returned another status, the new returns the same status, except old LIMIT -> new OK, which is counted and checked
against pow_si (mode I) and the coarse ball: the new value must be the ball of exponent min(N, R) containing pow_si.
Usage: diff_powunit.py COUNT SEED"""
import random, subprocess, sys
sys.set_int_max_str_digits(0)

HERE = sys.path[0]


def canon_ball(p, c, N):
    """the canonical ball c + p^N Z_p, c an integer with v_p(c) = 0 and N >= 1: fields (u, 1, 0, N, 0)"""
    return (c % p ** N, 1, 0, N, 0) if N <= 60 else (c, 1, 0, N, 0)


def exact_int(p, k):
    if k == 0:
        return (0, 1, 0, 0, 1)
    v = 0
    while k % p == 0:
        k //= p
        v += 1
    return (k, 1, v, 0, 1)


def gen(rng):
    p = rng.choice([2, 3, 5, 7, 65537])
    big = rng.random() < 0.2
    if rng.random() < 0.15:
        c = 1 + p * rng.randrange(1, 50) if p > 2 else rng.choice([1, 3, 5, 7, 11, -1])
        if p == 2 and c == -1:
            u = (-1, 1, 0, 0, 1)
        else:
            u = (c, 1, 0, 0, 1)
    else:
        A = rng.choice([2 ** 30, 2 ** 40, 2 ** 60]) if big else rng.randrange(1, 10)
        if p == 2:
            c = rng.randrange(1, 2 ** min(A, 12), 2)
        else:
            c = 1 + p * rng.randrange(0, p ** min(A - 1, 5) if A > 1 else 1)
        u = canon_ball(p, c, A)
    r = rng.random()
    if r < 0.6:
        k = rng.choice([rng.randrange(-30, 31), p * rng.randrange(-20, 21), p ** 3 * rng.randrange(-5, 6),
                        2 ** 62 + rng.randrange(-3, 3), -(2 ** 63), 2 ** 63 - 1, 2 ** 64 + 1, rng.randrange(-10 ** 6, 10 ** 6)])
        s = exact_int(p, k)
    elif r < 0.8:
        b = rng.choice([q for q in range(2, 30) if q % p])
        a = rng.randrange(-40, 41)
        from math import gcd
        if a == 0 or gcd(a, b) != 1:
            a, b = 1, b
        v = 0
        while a % p == 0:
            a //= p
            v += 1
        s = (a, b, v, 0, 1)
    else:
        B = rng.randrange(0, 8)
        k = rng.randrange(1, 200)
        if k % p == 0 and B > 0:
            k += 1
        s = (k % p ** B if B else 0, 1, 0, B, 0) if B else (0, 1, 0, 0, 0)
        if B and s[0] % p == 0:
            s = (0, 1, 0, B, 0)
    A = u[3] if not u[4] else 50
    N = rng.choice([rng.randrange(-3, 12), A, A - 1, A + 1, A + 2, 2 ** 60, 2 ** 63 - 1, -(2 ** 63), 2 ** 40 - 1])
    return p, u, s, N


def main():
    count, seed = int(sys.argv[1]), int(sys.argv[2])
    rng = random.Random(seed)
    lines = []
    for _ in range(count):
        p, u, s, N = gen(rng)
        lines.append("W %d %s %s %d" % (p, " ".join(map(str, u)), " ".join(map(str, s)), N))
    inp = "\n".join(lines) + "\n"
    old = subprocess.run([HERE + "/h_old"], input=inp, capture_output=True, text=True, timeout=1200).stdout.split("\n")
    new = subprocess.run([HERE + "/h_new"], input=inp, capture_output=True, text=True, timeout=1200).stdout.split("\n")
    same = improved = fail = noncanon = 0
    stats = {}
    checks = []
    for i, line in enumerate(lines):
        o, n = old[i], new[i]
        st = o.split()[0]
        stats[st] = stats.get(st, 0) + 1
        if "ALIAS" in o or "ALIAS" in n:
            print("ALIAS", line, "|", o, "|", n)
            fail += 1
        if o.startswith("NONCANON"):
            noncanon += 1
            continue
        if o == n:
            same += 1
        elif o.startswith("LIMIT") and n.startswith("OK"):
            improved += 1
            checks.append((line, n))
        else:
            fail += 1
            print("DIFF", line, "| old", o, "| new", n)
    # each improved line: the value must be the ball at min(N, R) containing pow_si(u, k)
    if checks:
        ilines = []
        for line, _ in checks:
            f = line.split()
            p = int(f[1])
            k = int(f[7]) * p ** int(f[9])
            ilines.append("I %d %s %d" % (p, " ".join(f[2:7]), k))
        out = subprocess.run([HERE + "/h_new"], input="\n".join(ilines) + "\n", capture_output=True, text=True,
                             timeout=1200).stdout.split("\n")
        for (line, n), pw in zip(checks, out):
            f = line.split()
            p, N = int(f[1]), int(f[-1])
            g = pw.split()
            if g[0] != "OK":
                print("POW_SI NOT OK", line, pw)
                fail += 1
                continue
            ex, v, R, c = int(g[1]), int(g[2]), int(g[3]), int(g[4].split("/")[0])
            K = min(N, R)
            if K >= R:
                want = pw
            elif K <= 0:
                want = "OK 0 0 %d 0" % K
            else:
                # for K > 64 the centre c of pow_si is its own residue when c < 2^K <= p^K
                if K > 64 and c.bit_length() > K:
                    print("UNCHECKED (centre of more than K bits)", line)
                    continue
                want = "OK 0 0 %d %d" % (K, c % p ** K if K <= 64 else c)
            if n != want:
                print("WRONG", line, "| new", n, "| want", want)
                fail += 1
    print("cases %d (noncanonical skipped %d): identical %d, LIMIT -> OK %d, failures %d; old statuses %s"
          % (count, noncanon, same, improved, fail, stats))
    sys.exit(1 if fail else 0)


main()
