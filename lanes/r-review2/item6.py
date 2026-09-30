import sys; sys.set_int_max_str_digits(0)
import random, subprocess, time
from flint import fmpz_poly, fmpz
from oracle import *

random.seed(int(sys.argv[1]) if len(sys.argv) > 1 else 6)
X = fmpz_poly([0, 1])
def lin(a, b): return fmpz_poly([-b, a])
def rnd(bits): return random.randint(-2 ** bits, 2 ** bits)

fam = []
def add(name, p, prec=2):
    fam.append((name, p, prec))

for bits in (1000, 5000, 10000):
    add("dense d=50 random %d bits" % bits, fmpz_poly([rnd(bits) for _ in range(51)]))
    add("dense d=20 random %d bits" % bits, fmpz_poly([rnd(bits) for _ in range(21)]), 200)
    add("sparse X^50 + rnd", fmpz_poly([rnd(bits)] + [0] * 49 + [rnd(bits) or 1]))
    add("X^49 - rnd", fmpz_poly([rnd(bits)] + [0] * 48 + [1]))
    add("x^50 - 2^bits X + 1", fmpz_poly([1, -2 ** bits] + [0] * 48 + [1]))
    add("2^bits X^50 - X + 1", fmpz_poly([1, -1] + [0] * 48 + [2 ** bits]))
for k in (200, 1000, 4000, 9000):
    add("prod(X - 2^k - i), 25 roots k=%d" % k, __import__('functools').reduce(lambda a, b: a * b, [lin(1, 2 ** k + i) for i in range(1, 26)]))
    add("prod(2^k X - i), 25 roots k=%d" % k, __import__('functools').reduce(lambda a, b: a * b, [lin(2 ** k, i) for i in range(1, 26)]))
    add("prod(2^k X - 1 - 2^-i) k=%d" % k, __import__('functools').reduce(lambda a, b: a * b, [lin(2 ** (k + i), 2 ** i + 1) for i in range(1, 30)]))
    add("(2^k X - 1)^49 - 1  k=%d" % k, fmpz_poly([-1, 2 ** k]) ** 49 - 1)
    add("(2^k X - 1)^50 - 1 prec 200 k=%d" % k, fmpz_poly([-1, 2 ** k]) ** 50 - 1, 200)
    add("(X - 2^k)^50 - 1 (k=%d)" % k, fmpz_poly([-2 ** k, 1]) ** 50 - 1)
    add("(3X-1)^50 - 2^-k like: (3X-1)^50 - 2^k  k=%d" % k, fmpz_poly([-1, 3]) ** 50 - 2 ** k)
    add("X^50 - 2 (2^k X - 1)^2 k=%d prec 64" % k, X ** 50 - 2 * fmpz_poly([-1, 2 ** k]) ** 2, 64)
    add("X^49 - 2^k X + 1 (k=%d)" % k, fmpz_poly([1, -2 ** k] + [0] * 47 + [1]))
    add("real roots near +-2^k pairs k=%d" % k, __import__('functools').reduce(lambda a, b: a * b, [(X - 2 ** k) ** 2 - 4 ** i for i in range(1, 12)]))
    add("prod (X^2 - 2^{2k}) - small perturb k=%d" % k, __import__('functools').reduce(lambda a, b: a * b, [X ** 2 - 4 ** (k + i) + 1 for i in range(0, 10)]))
add("wilkinson 50 prec 1000", __import__('functools').reduce(lambda a, b: a * b, [lin(1, i) for i in range(1, 51)]), 1000)
add("wilkinson 30 prec 5000", __import__('functools').reduce(lambda a, b: a * b, [lin(1, i) for i in range(1, 31)]), 5000)
add("wilk 50 rescaled 2^-3000", __import__('functools').reduce(lambda a, b: a * b, [lin(2 ** 3000, i) for i in range(1, 51)]), 2)
add("wilk 50 x 2^3000", __import__('functools').reduce(lambda a, b: a * b, [lin(1, i * 2 ** 3000) for i in range(1, 51)]), 2)
add("cluster 50 roots near 1/3 spread 2^-200", __import__('functools').reduce(lambda a, b: a * b, [lin(3 * 2 ** 200, 2 ** 200 + i) for i in range(1, 51)]), 2)
add("cluster 50 roots near 1/3 spread 2^-200 prec 400", __import__('functools').reduce(lambda a, b: a * b, [lin(3 * 2 ** 200, 2 ** 200 + i) for i in range(1, 51)]), 400)
add("cluster 24 near 1/3 spread 2^-4000, prec 8000", __import__('functools').reduce(lambda a, b: a * b, [lin(3 * 2 ** 4000, 2 ** 4000 + i) for i in range(1, 25)]), 8000)
add("prod(2^k X - 1) k=0..49 (roots 2^-k)", __import__('functools').reduce(lambda a, b: a * b, [lin(2 ** (40 * k), 1) for k in range(0, 50)]), 2)
add("roots 2^(40k), k=0..49", __import__('functools').reduce(lambda a, b: a * b, [lin(1, 2 ** (40 * k)) for k in range(0, 50)]), 2)
add("roots 2^(200k), k=0..49 prec 100", __import__('functools').reduce(lambda a, b: a * b, [lin(1, 2 ** (200 * k)) for k in range(0, 50)]), 100)
add("roots +-2^(200k)", __import__('functools').reduce(lambda a, b: a * b, [X ** 2 - 4 ** (200 * k) - 1 for k in range(0, 25)]), 2)
add("chebyshev 50 prec 3000", fmpz_poly.chebyshev_t(50), 3000)
add("legendre-like x^50-1 prec 20000", X ** 50 - 1, 20000)
add("X^2-2 prec 2^21", X ** 2 - 2, 2 ** 21)
add("X^50 - 2 prec 100000", X ** 50 - 2, 100000)

rows = []
for name, p, prec in fam:
    if p.degree() > 50 or max(abs(int(c)).bit_length() for c in p.coeffs()) > 10 ** 4 + 100 and False:
        pass
    co = [int(c) for c in p.coeffs()]
    line = "%d %s\n" % (prec, " ".join(map(str, co)))
    t = time.time()
    try:
        r = subprocess.run([DRV], input=line, capture_output=True, text=True, timeout=int(sys.argv[2]) if len(sys.argv) > 2 else 25)
        o = r.stdout.strip(); rc = r.returncode
    except subprocess.TimeoutExpired:
        o = "TIMEOUT"; rc = -1
    dt = time.time() - t
    flag = ""
    if o != "TIMEOUT" and o and o.split()[0] == '0':
        bad = check_one(prec, co, o) if dt < 20 else ["not checked"]
        flag = "BAD " + str(bad) if bad else "ok"
    else:
        flag = o[:40]
    bits = max(abs(c).bit_length() for c in co)
    print("%7.2fs rc=%d deg=%d bits=%d prec=%d %s : %s" % (dt, rc, p.degree(), bits, prec, name, flag), flush=True)
