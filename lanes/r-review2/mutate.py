import sys; sys.set_int_max_str_digits(0)
import random, subprocess, time
from flint import fmpz_poly, fmpq
from oracle import *
random.seed(int(sys.argv[1])); N = int(sys.argv[2])
X = fmpz_poly([0, 1])
def lin(a, b): return fmpz_poly([-b, a])

import os
def rndpoly():
    if os.environ.get('HIGH'):
        k = random.choice([3000, 4000]); d = random.randint(5, 8); sg = random.choice([1, -1]); j = random.choice([0, 1, 5])
        p = fmpz_poly([1])
        for i in range(d):
            p *= lin(2 ** k, sg * 2 ** j * (2 ** k + random.choice([-2, -1, 1, 2, 3, 0])))
        return p
    r = random.random()
    d = random.randint(2, 9)
    if r < .4:
        p = fmpz_poly([1])
        for _ in range(d):
            p *= lin(random.choice([1, 2, 3, 4, 8, 2 ** 20]), random.randint(-9, 9) * random.choice([1, 2, 4, 1]))
        return p
    if r < .7:
        k = random.choice([0, 30, 300, 2000])
        p = fmpz_poly([1])
        for i in range(d):
            p *= lin(2 ** k * random.choice([1, 3]), 2 ** k * random.randint(-3, 3) + random.randint(-3, 3))
        return p
    return fmpz_poly([random.randint(-40, 40) for _ in range(d + 1)])

cases = []
for _ in range(N):
    p = rndpoly()
    if p.degree() < 1: continue
    co = [int(c) for c in p.coeffs()]
    prec = random.choice([4200, 5000, 7000]) if os.environ.get('HIGH') else random.choice([2, 5, 30, 100])
    out, rc, _ = run(["%d %s\n" % (prec, " ".join(map(str, co)))])
    t = out[0].split()
    if t[0] != '0': continue
    m = int(t[2]); v = t[6:]
    balls = [(int(v[3 * i]), int(v[3 * i + 1]), int(v[3 * i + 2])) for i in range(m)]
    n_true, s, ch = nroots(co)
    # normalise all to a common exponent for mutation
    E = min([b[2] for b in balls] + [0]) if balls else 0
    def norm(b): return (b[0] * 2 ** (b[2] - E), b[1] * 2 ** (b[2] - E))
    nb = [norm(b) for b in balls]
    for _k in range(random.randint(3, 8)):
        mb = [list(x) for x in nb]
        kind = random.randrange(11)
        if not mb: mb = [[0, 0]];
        i = random.randrange(len(mb))
        w = max(1, (mb[i][1] - mb[i][0]))
        if kind == 0: mb[i][0] += random.randint(-3 * w, 3 * w)
        elif kind == 1: mb[i][1] += random.randint(-3 * w, 3 * w)
        elif kind == 2: s_ = random.randint(-5 * w, 5 * w); mb[i][0] += s_; mb[i][1] += s_
        elif kind == 3 and len(mb) > 1: del mb[i]
        elif kind == 4: mb.insert(i, list(mb[i]))
        elif kind == 5 and len(mb) > 1: j = random.randrange(len(mb)); mb[i], mb[j] = mb[j], mb[i]
        elif kind == 6 and i + 1 < len(mb): mb[i] = [mb[i][0], mb[i + 1][1]]; del mb[i + 1]
        elif kind == 7: mb.insert(i, [mb[i][0] + w // 2, mb[i][0] + w // 2 + 1] if w > 3 else [mb[i][1] + 5 * w, mb[i][1] + 6 * w])
        elif kind == 8: mb[i] = [mb[i][1], mb[i][0]]
        elif kind == 9: mb[i] = [mb[i][0], mb[i][0]]
        else: mb[i][0] -= 2 * w; mb[i][1] += 2 * w
        if any(b[0] > b[1] and random.random() < .5 for b in mb): pass
        cases.append((prec, co, n_true, E, mb))

lines = []
for prec, co, n_true, E, mb in cases:
    lines.append("%d %d %d %s %d %s\n" % (prec, n_true, len(mb), " ".join("%d %d %d" % (b[0], b[1], E) for b in mb), len(co), " ".join(map(str, co))))
t = time.time()
r = subprocess.run(["/tmp/x/fdrv"], input="".join(lines), capture_output=True, text=True, timeout=170)
outs = r.stdout.splitlines()
print("mutants", len(cases), "rc", r.returncode, "outs", len(outs), "time", round(time.time() - t, 1))
ok = 0; bad = 0; st = {}
for (prec, co, n_true, E, mb), o in zip(cases, outs):
    t_ = o.split(); st[t_[0]] = st.get(t_[0], 0) + 1
    if t_[0] != '0': continue
    ok += 1
    n, s, ch = nroots(co)
    vals = t_[1:]; m = len(vals) // 3
    problems = []
    if m != n: problems.append("count %d vs %d" % (m, n))
    prev = None
    for i in range(m):
        a, b, e = int(vals[3 * i]), int(vals[3 * i + 1]), int(vals[3 * i + 2])
        lo = fmpq(a) * fmpq(2) ** e; hi = fmpq(b) * fmpq(2) ** e
        if prev is not None and not prev < lo: problems.append("unordered")
        prev = hi
        c = (1 if lo == hi and s(lo) == 0 else 0) if lo == hi else roots_in(ch, lo, hi) + (1 if s(lo) == 0 else 0)
        if c != 1: problems.append("ball %d holds %d roots" % (i, c))
    if problems:
        bad += 1
        if bad < 6: print("DEFECT", problems, "prec", prec, "poly", co[:10], "mutant", mb, "E", E, "out", o[:150])
print("status histogram", st, "accepted (OK)", ok, "accepted but wrong", bad)
