import sys; sys.set_int_max_str_digits(0)
import random, sys, time
from flint import fmpz_poly, fmpz
from oracle import *

seed = int(sys.argv[1]); N = int(sys.argv[2])
random.seed(seed)
X = fmpz_poly([0, 1])


def rnd_factor():
    k = random.choice([0, 0, 1, 3, 10, 40, 200, 700])
    num = random.randint(-9, 9)
    den = random.choice([1, 1, 2, 3, 5, 7])
    return num, den, k


def make():
    kind = random.choice([0,1,2,3,4,5,6,7,8,8,8,9,9,9])
    d = random.randint(3, 13)
    p = fmpz_poly([1])
    T = random.choice([0, 1, -1, 3, 7])
    if kind == 0:   # roots near p/q, spread 2^-k ; hits translate path
        q = random.choice([1, 2, 3, 5, 7, 11]); pp = random.randint(-9, 9); k = random.choice([30, 100, 500, 1500])
        for _ in range(d):
            eps = random.randint(-9, 9)
            p *= fmpz_poly([-pp * 2 ** k - eps, q * 2 ** k])
        return p
    if kind == 1:   # huge roots
        k = random.choice([1100, 2000])
        for _ in range(d):
            p *= fmpz_poly([-random.randint(-9, 9) * 2 ** random.randint(0, k), 1])
        return p
    if kind == 2:   # tiny roots (negative scale)
        k = random.choice([50, 200, 1500])
        for _ in range(d):
            p *= fmpz_poly([-random.randint(-9, 9), 2 ** random.randint(0, k)])
        return p
    if kind == 3:   # translate by huge T, small poly
        base = fmpz_poly([random.randint(-20, 20) for _ in range(d + 1)])
        if base.degree() < 1: base = fmpz_poly([-3, 0, 1])
        Tt = random.choice([1, -1, 5]) * fmpz(2) ** random.choice([100, 1100, 3000]) * random.randint(1, 9)
        return base(X + Tt)
    if kind == 4:   # non-squarefree: repeated factors, roots at 0, content
        for _ in range(d):
            r = random.randint(-3, 3)
            p *= fmpz_poly([-r, 1]) ** random.randint(1, 3)
        p = p(X * 2 ** random.randint(0, 1200) + random.randint(-2, 2)) if random.random() < .5 else p
        return p * random.randint(1, 10) ** random.randint(0, 60)
    if kind == 5:   # scale/translate of random coefficient poly by a rational with big powers
        base = fmpz_poly([random.randint(-50, 50) for _ in range(d + 1)])
        if base.degree() < 1: base = fmpz_poly([-3, 0, 1])
        k = random.choice([40, 80, 900, 1200]); sgn_ = random.choice([1, -1])
        # base(sgn 2^-k X + c) * 2^(k d)
        c = random.randint(-9, 9)
        r = fmpz_poly([0])
        dd = base.degree()
        Y = fmpz_poly([c * 2 ** k, sgn_])
        for i in range(dd + 1):
            r += base[i] * Y ** i * fmpz(2) ** (k * (dd - i))
        return r
    if kind == 6:   # negative leading coefficient, odd/even, x -> -x
        for _ in range(d):
            p *= fmpz_poly([random.randint(-9, 9) * 2 ** random.choice([0, 60, 600]), -random.randint(1, 9) * 2 ** random.choice([0, 1, 700])])
        return p
    if kind in (8, 9):
        q = random.choice([1, 2, 3, 5, 7, 11, 255]); pp = random.randint(-9, 9); k = random.choice([30, 100, 500, 1500, 4000])
        L = fmpz_poly([-pp, q])
        p = fmpz_poly([1])
        for _ in range(random.randint(1, 6)):
            eps = random.randint(0, 9)
            if kind == 8 or random.random() < .5:
                # symmetric pair: (q 2^k X - pp 2^k)^2 - eps^2 = roots at center +- eps/(q 2^k)
                Y = fmpz_poly([-pp * 2 ** k, q * 2 ** k])
                p *= Y * Y - eps * eps * random.choice([1, 1, 4])
            else:
                Y = fmpz_poly([-pp * 2 ** k, q * 2 ** k])
                p *= Y * Y + (eps + 1) * random.choice([1, 2 ** 20])
        if random.random() < .5:
            p *= fmpz_poly([-pp * 2 ** k, q * 2 ** k]) * random.choice([1, 1, 3])
        if random.random() < .3:
            p *= (fmpz_poly([-pp * 2 ** k, q * 2 ** k]) ** 2 - random.randint(0, 3) ** 2 * 2 ** random.choice([0, 7]))
        return p
    # kind 7: root at exact translate center? g_{d-1}=0 (center 0) but big coeffs
    base = fmpz_poly([random.randint(-30, 30) for _ in range(d + 1)])
    base = base - base[d - 1] * X ** (d - 1) if base.degree() == d else base
    if base.degree() < 3: base = fmpz_poly([-1, -3, 0, 1])
    return base(X * 2 ** random.choice([0, 700, 1300])) if random.random() < .5 else base(X + 2 ** 1300)


lines = []; polys = []
while len(lines) < N:
    p = make()
    if p.degree() < 1 or p == 0:
        continue
    co = [int(c) for c in p.coeffs()]
    polys.append(co); lines.append("%d %s\n" % (random.choice([2, 64]), " ".join(map(str, co))))
t = time.time()
out, rc, err = run(lines)
print("seed", seed, "N", N, "rc", rc, "time", round(time.time() - t, 1), "outlines", len(out), flush=True)
nb = 0; stat = {}
for co, ln, o in zip(polys, lines, out):
    prec = int(ln.split()[0])
    bad = check_one(prec, co, o)
    if bad:
        nb += 1
        if nb <= 5:
            print("DEFECT", bad, "prec", prec, "poly", co if len(str(co)) < 400 else str(co)[:400] + "...", "out", o[:200])
    stat[o.split()[0]] = stat.get(o.split()[0], 0) + 1
from fractions import Fraction
cat = {"plain": 0, "translate": 0, "scale-only": 0}
for co in polys:
    n_ = sqfree(co); 
    g = [int(x) for x in fmpz_poly([int(x) for x in n_.numer().coeffs()]).coeffs()] if n_.degree() > 0 else co
    d = len(g) - 1
    if d < 3 or max(abs(int(x)).bit_length() for x in g) < 1024: cat["plain"] += 1; continue
    tr = False
    if g[d - 1] != 0:
        c = Fraction(-g[d - 1], d * g[d])
        tr = d >= 5 and abs(c.numerator).bit_length() + c.denominator.bit_length() <= 16
    if tr: cat["translate"] += 1
    elif g[0] != 0 and (abs(g[0]).bit_length() - abs(g[d]).bit_length()) // d < -32 or False: cat["scale-only"] += 1
    else: cat["plain"] += 1
print("paths (approx, on sqfree part)", cat)
print("defects", nb, "status histogram", stat)
