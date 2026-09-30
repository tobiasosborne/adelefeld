import sys; sys.set_int_max_str_digits(0)
import random, time
from flint import fmpz_poly
from oracle import *
random.seed(int(sys.argv[1])); N = int(sys.argv[2])
def lin(a, b): return fmpz_poly([-b, a])
lines = []; polys = []
for _ in range(N):
    k = random.choice([3000, 4000, 6000]); prec = random.choice([4200, 5000, 7000, 12000])
    d = random.randint(4, 10)
    p = fmpz_poly([1])
    j = random.choice([0, 1, 5, -3])          # root near 2^j
    sg = random.choice([1, -1])
    for i in range(d):
        eps = random.choice([-2, -1, 1, 2, 3, 0, random.randint(-9, 9)])
        # root sg 2^j (1 + eps 2^-k)
        if j >= 0:
            p *= lin(2 ** k, sg * 2 ** j * (2 ** k + eps))
        else:
            p *= lin(2 ** (k - j), sg * (2 ** k + eps))
    if random.random() < .3: p *= fmpz_poly([1, 0, 1])
    co = [int(c) for c in p.coeffs()]
    polys.append(co); lines.append("%d %s\n" % (prec, " ".join(map(str, co))))
t = time.time(); out, rc, err = run(lines)
print("N", N, "rc", rc, "time", round(time.time() - t, 1), "lines", len(out), flush=True)
nb = 0; st = {}
for co, ln, o in zip(polys, lines, out):
    prec = int(ln.split()[0]); bad = check_one(prec, co, o)
    st[o.split()[0]] = st.get(o.split()[0], 0) + 1
    if bad:
        nb += 1
        if nb < 5: print("DEFECT", bad, prec, str(co)[:200], o[:150])
print("defects", nb, st)
