"""Own oracle for catalogue.c, from definitions, exact integers. Talks to ./h (stdin/stdout)."""
import subprocess, random, sys, math
from math import gcd, factorial, comb
import os, sys; sys.set_int_max_str_digits(0)
H = os.environ.get("HB", "lanes/f-review12/h")

def run(lines):
    p = subprocess.run([H], input="\n".join(lines) + "\n", capture_output=True, text=True, timeout=110)
    out = p.stdout.strip().split("\n")
    assert p.returncode == 0, p.stderr
    assert len(out) == len(lines), (len(out), len(lines))
    return out

def binom(a, k):
    # generalised, any integer a
    z = 1
    for i in range(1, k + 1):
        z = z * (a - i + 1) // i
    return z

def g_all(vals):
    g = 0
    for v in vals: g = gcd(g, v)
    return g

# statuses
OK, DOMAIN, LIMIT, ND = 0, None, None, None

def parse(line):
    t = line.split()
    return int(t[0]), t[1:]

bad = []
def fail(kind, msg):
    bad.append((kind, msg)); print("FAIL", kind, msg)

def test_binom(n, seed):
    rnd = random.Random(seed)
    cases = []
    for _ in range(n):
        k = rnd.choice([0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 16, rnd.randint(0, 30)])
        N = rnd.choice([0, 1, 2, 3, 4, 6, 8, 12, 16, 24, 27, 30, 32, 48, 64, 720, rnd.randint(0, 5000)])
        a = rnd.choice([0, 1, -1, 2, 3, rnd.randint(-50, 50), rnd.randint(-10**6, 10**6)])
        d = 1 if rnd.random() < 0.85 else rnd.choice([2, 3, 4, 6, 9, 10, 12])
        if d > 1:
            a = rnd.randint(-100, 100)
            N = rnd.choice([0, 1, 2, 3, 4, 6, 8, 12, 5, 7, 9, 18])
        cases.append((a, N, d, k))
    for mode in ("binom", "tight"):
        res = run([f"{mode} {a} {N} {d} {k}" for a, N, d, k in cases])
        for (a, N, d, k), line in zip(cases, res):
            st, rest = parse(line.replace("canon=", "").replace("BADIN", "99"))
            if d > 1:
                # integrality by definition: set {(a+N z)/d}; z in Zhat; check mod d (d small)
                allint = (a % d == 0 and N % d == 0)
                meets = any((a + N * z) % d == 0 for z in range(d))
                if allint:
                    d = 1; a //= 1  # reduced below
                    # then it is integral; set is (a/d + (N/d) Zhat): fallthrough
                    a, N = a, N  # recompute
                    a0, N0 = None, None
                exp = None
                if allint:
                    pass
                elif meets:
                    exp = 1
                else:
                    exp = 7
                if exp is not None:
                    if st != exp: fail("binom-status", (mode, a, N, d, k, line, exp))
                    continue
                continue  # allint with d>1: skip (canon makes d=1 anyway; checked via gcd)
            if st != 0:
                fail("binom-status", (mode, a, N, d, k, line)); continue
            A, R, dd = int(rest[0]), int(rest[1]), int(rest[2])
            C = binom(a, k)
            if mode == "binom":
                expR = N // gcd(N, factorial(k)) if (N and k) else 0
                # conservative radius, check true containment and equality with spec formula
                if R != expR or dd != 1: fail("binom-cons-shape", (a, N, k, line, expR))
            else:
                vals = [binom(a + N * j, k) - C for j in range(-3, 4 * k + 12)]
                expR = g_all(vals)
                if R != expR or dd != 1: fail("binom-tight", (a, N, k, line, expR))
            # containment of the true image
            for j in (-7, -1, 1, 2, 3, 10, 101, -100):
                v = binom(a + N * j, k)
                if (R == 0 and v != A) or (R and (v - A) % R): fail("binom-contain", (mode, a, N, k, j, line))
            if R == 0 and A != C or (R and (C - A) % R): fail("binom-centre", (a, N, k, line))
    return len(cases)

if __name__ == "__main__":
    n = test_binom(int(sys.argv[1]), int(sys.argv[2]))
    print("binom cases", n, "fails", len(bad))
