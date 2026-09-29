"""i-review1: set_fmpz2 on raw data (negative c, c > N, N = 0, 1, 2, 4, negative N, big moduli), and big-modulus
mul/inv/contains/overlaps/equal_set against formulas of Python integers (pow(c, -1, N), gcd)."""
import subprocess, os, random, sys
sys.set_int_max_str_digits(0)
from math import gcd
HERE = os.path.dirname(os.path.abspath(__file__))
UC = os.path.join(HERE, "uc")
rng = random.Random(int(sys.argv[1]))

def run(lines):
    o = subprocess.run([UC], input="".join(lines), capture_output=True, text=True, timeout=170)
    return o.returncode, o.stdout.splitlines(), o.stderr

def norm(c, N):
    if N == 0: return (c, 0)
    if N % 4 == 2: N //= 2
    c %= N
    return (c if c else N, N)

def gg(a, b): return a if b == 0 else b if a == 0 else gcd(a, b)

lines = []; exp = []
# set_fmpz2
cs = [-7, -6, -5, -1, 0, 1, 2, 3, 5, 6, 7, 11, 13, 100, -100, 2**70 + 1, -(2**70) - 1]
Ns = [-5, -1, 0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 2**64, 2**64 + 1, 2 * 3**50, 10**30]
for N in Ns:
    for c in cs:
        lines.append("set %d %d\n" % (c, N))
        if N < 0: want = (7, None)
        elif N == 0: want = (0, (c, 0)) if c in (1, -1) else (7, None)
        elif gcd(c, N) != 1: want = (7, None)
        else:
            r = c % N
            want = (0, (r if r else N, N))
        exp.append(("set", c, N, want))
big = []
for _ in range(60):
    b = rng.choice([2000, 5000, 20000])
    N = rng.getrandbits(b) | 1
    if rng.random() < .3: N *= 2
    if rng.random() < .2: N = 2 * N + 0; N = (N // 2) * 2 + 2 * ((N // 2) % 2 == 0)  # mixed
    while True:
        c = rng.randrange(1, N)
        if gcd(c, N) == 1: break
    M = rng.getrandbits(b) | 1
    while True:
        d = rng.randrange(1, M)
        if gcd(d, M) == 1: break
    big.append(((c, N), (d, M)))
BIGX = []
for x, y in big:
    x = (x[0], x[1]); y = (y[0], y[1])
    lines.append("mul %d %d %d %d\n" % (x + y)); exp.append(("mul", x, y))
    lines.append("inv %d %d\n" % x); exp.append(("inv", x))
    lines.append("norm %d %d\n" % x); exp.append(("norm", x))
    for op in ("contains", "eq", "ov"):
        lines.append("%s %d %d %d %d\n" % ((op,) + x + y)); exp.append((op, x, y))
    # a coset and a refinement: y' = (c mod N, N * k) inside x
    k = 3 if N % 3 else 5
    r = None
    for t in range(5, 200):
        if gcd(t, N * k) == 1 and t % N == x[0] % N: pass
    # find c2 = x0 + N j coprime to N*k
    j = 0
    while True:
        c2 = x[0] + x[1] * j
        if gcd(c2, x[1] * k) == 1: break
        j += 1
    z = (c2, x[1] * k)
    lines.append("contains %d %d %d %d\n" % (z + x)); exp.append(("contains", z, x))
    lines.append("contains %d %d %d %d\n" % (x + z)); exp.append(("contains", x, z))
    lines.append("ov %d %d %d %d\n" % (z + x)); exp.append(("ov", z, x))
    lines.append("mul %d %d %d %d\n" % (z + x)); exp.append(("mul", z, x))
    lines.append("mul %d %d %d %d\n" % (z + (1, 0))); exp.append(("mul", z, (1, 0)))
    lines.append("mul %d %d %d %d\n" % (z + (-1, 0))); exp.append(("mul", z, (-1, 0)))
    lines.append("contains %d %d %d %d\n" % ((1, 0) + x)); exp.append(("contains", (1, 0), x))
rc, out, err = run(lines)
if rc != 0 or len(out) != len(lines):
    print("ABORT rc", rc, len(out), len(lines), err[:300]); sys.exit(1)
bad = 0
def contains(x, y):
    a = norm(*x); b = norm(*y)
    if b[1] == 0: return int(a[1] == 0 and a[0] == b[0])
    if (a[0] - b[0]) % b[1]: return 0
    return int(a[1] == 0 or a[1] % b[1] == 0)
def ov(x, y):
    g = gg(x[1], y[1])
    return int(x[0] == y[0]) if g == 0 else int((x[0] - y[0]) % g == 0)
for e, line in zip(exp, out):
    t = line.split()
    if e[0] == "set":
        _, c, N, want = e
        st = int(t[0])
        if st != want[0] or (st == 0 and (int(t[1]), int(t[2])) != want[1]) or len(t) > 3:
            print("SET WRONG", c, N, "got", line, "want", want); bad += 1
    elif e[0] == "mul":
        x, y = e[1], e[2]
        g = gg(x[1], y[1]); w = norm(x[0] * y[0], g)
        if (int(t[0]), int(t[1])) != w or len(t) > 2: print("MUL WRONG", x[1].bit_length() if x[1] else 0, line[:60], w if g < 1000 else ""); bad += 1
    elif e[0] == "inv":
        x = e[1]; w = norm(pow(x[0], -1, x[1]), x[1])
        if (int(t[0]), int(t[1])) != w or len(t) > 2: print("INV WRONG", line[:60]); bad += 1
    elif e[0] == "norm":
        x = e[1]; w = norm(*x)
        if (int(t[0]), int(t[1])) != w or len(t) > 2: print("NORM WRONG", line[:60]); bad += 1
    elif e[0] == "contains":
        if int(t[0]) != contains(e[1], e[2]): print("CONTAINS WRONG", e[1][1].bit_length(), line); bad += 1
    elif e[0] == "eq":
        if int(t[0]) != int(norm(*e[1]) == norm(*e[2])): print("EQ WRONG", line); bad += 1
    elif e[0] == "ov":
        if int(t[0]) != ov(e[1], e[2]): print("OV WRONG", line); bad += 1
print("checks", len(exp), "bad", bad)
