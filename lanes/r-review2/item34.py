import sys; sys.set_int_max_str_digits(0)
import random, time
from flint import fmpz_poly, fmpz
from oracle import *

which = sys.argv[1]; seed = int(sys.argv[2]); N = int(sys.argv[3])
random.seed(seed)
X = fmpz_poly([0, 1])


def lin(a, b):
    return fmpz_poly([-b, a])


def make3():
    d = random.randint(2, 12)
    p = fmpz_poly([1])
    mode = random.randrange(6)
    base_j = random.choice([0, 1, 3, 10, 30, 100])
    for i in range(d):
        j = random.choice([0, base_j, random.randint(0, 40)])
        a = 2 ** j * random.choice([1, 1, 1, 3, 5])
        if mode == 0:       # dyadic roots at cell end points
            b = random.randint(-9, 9) * (a // random.choice([1, 2, 4]) if a % 4 == 0 else 1)
        elif mode == 1:     # clusters: roots b/a with b consecutive
            b = random.randint(-3, 3) * a + random.randint(-2, 2)
        elif mode == 2:     # exact dyadic 2^t
            a = 2 ** random.randint(0, 20); b = random.choice([-1, 1]) * 2 ** random.randint(0, 25)
        elif mode == 3:     # multiple roots
            b = random.randint(-4, 4)
        elif mode == 4:     # close pairs around a dyadic
            m = random.randint(-5, 5) * a
            b = m + random.choice([-1, 1]) * random.randint(0, 2)
        else:               # roots at +-1/2, +-1/4 ... boundaries of the bisection grid
            a = 2 ** random.randint(0, 6); b = random.randint(-3 * a, 3 * a)
        mult = random.choice([1, 1, 1, 2, 3]) if mode in (3, 4) or random.random() < .15 else 1
        p *= lin(a, b) ** mult
    if random.random() < .3:
        p = p * X ** random.randint(1, 2)
    if random.random() < .3:
        p = p * fmpz_poly([1, 0, 1]) * random.randint(1, 7)
    if random.random() < .2:
        p = p(-X)
    return p


def make4():
    kind = random.randrange(6)
    if kind == 0:   # Wilkinson-ish
        n = random.choice([10, 15, 20, 25, 30, 30, 40])
        p = fmpz_poly([1])
        for i in range(1, n + 1):
            p *= lin(1, i)
        if random.random() < .5:
            p = p + random.choice([0, 1, -1, 2 ** 20]) * X ** random.randint(0, n - 1)
        return p
    if kind == 1:   # (2^k X - 1)^d - c
        d = random.randint(8, 60); k = random.choice([1, 5, 20, 100])
        c = random.choice([1, 2, 3, 2 ** 10])
        p = fmpz_poly([-1, 2 ** k]) ** d - c
        return p
    if kind == 2:   # nearly dyadic roots: prod (2^k X - (2^j + e)) with e in {0, +-1}
        d = random.randint(8, 40); k = random.choice([8, 40, 100])
        p = fmpz_poly([1])
        for i in range(d):
            p *= lin(2 ** k, random.randint(-6, 6) * 2 ** (k - 3) + random.choice([-1, 0, 0, 1]))
        return p
    if kind == 3:   # Wilkinson scaled by dyadic translate
        n = random.choice([12, 20, 30]); p = fmpz_poly([1])
        for i in range(1, n + 1):
            p *= lin(2 ** 4, 2 ** 4 * i + random.choice([0, 1, -1]))
        return p
    if kind == 4:   # X^d - 2^e like
        d = random.randint(8, 60)
        return fmpz_poly([-2 ** random.choice([1, 20, 100])] + [0] * (d - 1) + [1]) * (X - 1) * (X + 1)
    d = random.randint(8, 30)  # Mignotte: X^d - 2 (2^k X - 1)^2
    k = random.choice([10, 60, 300])
    return X ** d - 2 * fmpz_poly([-1, 2 ** k]) ** 2


def make7():
    d = random.randint(5, 16)
    if random.random() < .5:
        return fmpz_poly([random.randint(-30, 30) for _ in range(d)] + [random.randint(1, 5)])
    return make3()
def make9():
    d = random.randint(1, 60); b = random.choice([1, 2, 4, 8, 16, 64, 200]) if random.random() < .3 else random.choice([1, 2, 4, 8])
    if d > 25: b = min(b, 8)
    r = random.random()
    if r < .6:
        co = [random.randint(-2 ** b, 2 ** b) for _ in range(d + 1)]
    elif r < .8:   # sparse
        co = [0] * (d + 1)
        for _ in range(random.randint(2, 5)): co[random.randint(0, d)] = random.randint(-2 ** b, 2 ** b)
    else:          # many small roots: product of random small linear/quadratic factors
        p = fmpz_poly([1])
        for _ in range(random.randint(1, 12)):
            p *= fmpz_poly([random.randint(-20, 20), random.randint(1, 9)]) if random.random() < .7 else fmpz_poly([random.randint(0, 20), random.randint(-9, 9), random.randint(1, 9)])
        co = [int(c) for c in p.coeffs()]
    return fmpz_poly(co)
mk = make9 if which == '9' else make7 if which == '7' else make3 if which == '3' else make4 if which == '4' else (lambda: make3() if random.random() < .5 else make4())
precs = [2, 64, 1000] if which == '4' else [2, 3, 64]
if which == '5': precs = [None]
if which == '9': precs = [2, 3, 10, 64, 200]
if which == '7': precs = [4097, 4500, 6000, 9000]
if which == '5':
    nb = 0; cnt = 0; t = time.time()
    while cnt < N:
        p = mk()
        if p.degree() < 1 or p == 0: continue
        co = [int(c) for c in p.coeffs()]
        p1 = random.choice([2, 3, 5, 20, 64]); p2 = p1 + random.choice([1, 2, 10, 60, 300])
        o, rc, err = run(["%d %s\n" % (p1, " ".join(map(str, co))), "%d %s\n" % (p2, " ".join(map(str, co)))])
        cnt += 1
        t1 = o[0].split(); t2 = o[1].split()
        if t1[0] != '0' or t2[0] != '0':
            print("STATUS", t1[0], t2[0], co[:6]); nb += 1; continue
        if t1[2] != t2[2]: print("N differs"); nb += 1; continue
        n = int(t1[2])
        for i in range(n):
            def iv(t):
                a, b, e = int(t[6+3*i]), int(t[7+3*i]), int(t[8+3*i])
                return a * fmpq(2) ** e, b * fmpq(2) ** e
            l1, h1 = iv(t1); l2, h2 = iv(t2)
            if not (l1 <= l2 and h2 <= h1):
                nb += 1; print("NESTING FAIL prec", p1, p2, "ball", i, "poly", co[:8]); break
    print("item5 pairs", cnt, "failures", nb, "time", round(time.time() - t, 1)); sys.exit()
lines = []; polys = []
while len(lines) < N:
    p = mk()
    if p.degree() < 1 or p == 0:
        continue
    co = [int(c) for c in p.coeffs()]
    polys.append(co); lines.append("%d %s\n" % (random.choice(precs), " ".join(map(str, co))))
import os
if os.environ.get('DUMP'):
    open(os.environ['DUMP'], 'w').write(''.join(lines)); sys.exit()
t = time.time()
out, rc, err = run(lines)
print(which, "seed", seed, "N", N, "rc", rc, "time", round(time.time() - t, 1), "outlines", len(out), flush=True)
nb = 0; stat = {}
for co, ln, o in zip(polys, lines, out):
    prec = int(ln.split()[0])
    bad = check_one(prec, co, o)
    if bad:
        nb += 1
        if nb <= 6:
            print("DEFECT", bad, "prec", prec, "poly", co if len(str(co)) < 500 else str(co)[:500] + "...", "out", o[:200])
    stat[o.split()[0]] = stat.get(o.split()[0], 0) + 1
print("defects", nb, "status histogram", stat)
