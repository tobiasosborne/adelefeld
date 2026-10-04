import sys; sys.set_int_max_str_digits(0)
import random, sys
from math import gcd, factorial
sys.path.insert(0, "lanes/f-review12")
from orc import run, binom
rnd = random.Random(int(sys.argv[1]))
cases = []
for _ in range(int(sys.argv[2])):
    k = rnd.choice([100, 200, 255, 256, 128, 64, 97])
    a = rnd.getrandbits(700) * rnd.choice([1, -1])
    N = rnd.choice([2**rnd.randint(1, 200), 3**rnd.randint(1, 100) * 2**rnd.randint(0, 50), rnd.getrandbits(300), 0,
                    factorial(rnd.randint(2, 60)) * rnd.getrandbits(100)])
    cases.append((a, N, k))
res = run([f"tight {a} {N} 1 {k}" for a, N, k in cases])
bad = 0
for (a, N, k), line in zip(cases, res):
    t = line.split()
    assert t[0] == "0", line
    R = int(t[2]); A = int(t[1])
    C = binom(a, k)
    g = 0
    for j in list(range(-2, k + 3)) + [rnd.randint(-10**9, 10**9) for _ in range(2)]:
        g = gcd(g, binom(a + N * j, k) - C)
    if g != R or (R and (C - A) % R) or (R == 0 and A != C):
        bad += 1; print("FAIL tight", k, N.bit_length(), line[:60], g.bit_length())
print("big tight cases", len(cases), "bad", bad)
cases = [(a, N, rnd.choice([1000, 4096, 4095, 3000])) for a, N, k in cases[:6]]
res = run([f"binom {a} {N} 1 {k}" for a, N, k in cases])
bad = 0
for (a, N, k), line in zip(cases, res):
    t = line.split(); assert t[0] == "0"
    R = int(t[2]); A = int(t[1])
    exp = N // gcd(N, factorial(k)) if N else 0
    if R != exp: bad += 1; print("FAIL cons", k, N.bit_length())
    for j in (-3, 1, 2, 7, 1000):
        v = binom(a + N * j, k)
        if (v - A) % R if R else v != A: bad += 1; print("FAIL cons contain", k, j)
print("big conservative cases", len(cases), "bad", bad)
