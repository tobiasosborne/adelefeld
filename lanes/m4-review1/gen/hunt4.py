"""Hunt 4: adf_tensor_poisson against own 60-digit sums. Usage: hunt4.py N seed
left = sum_j f_j sum_n phi(j/D + M n), right = sum_n g_(n mod L) phihat(n/M) (analysis P7:269-275); g by the own
exact finite transform (m.ftransform_exact), phihat by the own closed form (m2.ft). Both are summed until the
remaining Gaussian majorant is below 1e-70."""
import sys, random
from fractions import Fraction as Fr
import mpmath
from mpmath import mpf, mpc
from m import *
from m2 import *

N = int(sys.argv[1]); seed = int(sys.argv[2]); prog = sys.argv[3] if len(sys.argv) > 3 else "h"
rng = random.Random(seed)
fails = []
S_ = {"ok": 0, "nd": 0, "limit": 0, "other": 0, "checks": 0, "undec": 0, "prev_passes_own_L6": 0,
      "maxdiam_over": 0, "selfcheck": 0.0}
mpmath.mp.dps = 70


def note(k, m):
    fails.append((k, m))


def rq(lo, hi, dens=(1, 2, 4, 8, 3)):
    d = rng.choice(dens)
    return Fr(rng.randint(int(lo * d), int(hi * d)), d)


def gen_phi():
    T = []
    for _ in range(rng.randint(1, 2)):
        l = rng.choice([1, 1, 2, 3, 4, 5])
        P = [(rq(-2, 2), rq(-2, 2)) for _ in range(l)]
        if P[-1] == (0, 0):
            P[-1] = (Fr(1), Fr(0))
        are = rng.choice([Fr(1, 8), Fr(1, 4), Fr(1, 2), Fr(1), Fr(2), Fr(3)])
        T.append((P, (are, rq(-2, 2)), (rq(-2, 2), rq(-2, 2)), (rq(-1, 1), rq(-1, 1))))
    return T


def tail_majorant(phi, a, h, Nc):
    """sum_(|n| > Nc) |phi(a + h n)|, summed directly (an exact quantity <= any valid Lemma 6 bound)."""
    s = mpf(0)
    for sgn in (1, -1):
        n = Nc + 1
        while True:
            v = abs(ev(phi, a + h * sgn * n))
            s += v
            if n > Nc + 5 and v < mpf(10) ** -80 * (1 + s):
                break
            n += 1
    return s


def lemma6_left(phi, D, M, vals, Nc):
    """Own Lemma 6 bound (analysis.md:213-246) of the left tail at cutoff Nc, exact q_j by Taylor shift."""
    tot = mpf(0)
    for j, v in enumerate(vals):
        fa = abs(C2m(v))
        if fa == 0:
            continue
        a = Fr(j, D); h = Fr(M)
        for (P, A, B, C) in phi:
            # P(a + h n) = sum q_j n^j
            Q = shift(P, -a)  # P(x + a)
            q = [(c[0] * h ** k, c[1] * h ** k) for k, c in enumerate(Q)]
            Am, Bm, Cm = C2m(A), C2m(B), C2m(C)
            alpha = mpmath.pi * mpmath.re(Am) * F2m(h) ** 2
            beta = abs(mpmath.re(F2m(h) * (Bm - 2 * mpmath.pi * Am * F2m(a))))
            gamma = mpmath.re(Cm + Bm * F2m(a) - mpmath.pi * Am * F2m(a) ** 2)
            s = mpf(0)
            for k, c in enumerate(q):
                if c == (0, 0):
                    continue
                K = Nc + 1
                pre = mpf(0)
                while True:
                    rho = mpmath.exp(mpf(k) / K - alpha * (2 * K + 1) + beta)
                    term = mpf(K) ** k * mpmath.exp(-alpha * K * K + beta * K)
                    if rho <= mpf(1) / 2:
                        pre += term / (1 - rho)
                        break
                    pre += term
                    K += 1
                s += abs(C2m(c)) * pre
            tot += fa * 2 * mpmath.exp(gamma) * s
    return tot


def full_left(phi, D, M, vals):
    tot = mpc(0)
    for j, v in enumerate(vals):
        if v == (0, 0):
            continue
        a = Fr(j, D)
        s = ev(phi, F2m(a))
        for sgn in (1, -1):
            n = 1
            while True:
                t = ev(phi, F2m(a) + sgn * M * n)
                s += t
                if n > 3 and abs(t) < mpf(10) ** -75:
                    break
                n += 1
        tot += C2m(v) * s
    return tot


def full_right(phi, D, M, vals):
    L = D * M
    g = ftransform_exact(D, M, vals)
    gm = [mpc(F2m(x[0]), F2m(x[1])) for x in g]
    tot = gm[0] * ft(phi, 0)
    for sgn in (1, -1):
        n = 1
        quiet = 0
        while True:
            t = ft(phi, mpf(sgn * n) / M)
            tot += gm[(sgn * n) % L] * t
            quiet = quiet + 1 if abs(t) < mpf(10) ** -75 else 0
            if quiet > 3:
                break
            n += 1
    return tot, max(abs(x) for x in gm)


cases, lines = [], []
for i in range(N):
    D, M = rng.randint(1, 6), rng.randint(1, 6)
    vals = []
    for _ in range(D * M):
        vals.append((Fr(0), Fr(0)) if rng.random() < 0.25 else (rq(-3, 3), rq(-3, 3)))
    phi = gen_phi()
    bits = rng.choice([20, 53, 80, 128])
    prec = rng.choice([2, 30, bits + 10, bits + 40])
    cases.append((D, M, vals, phi, bits, prec))
    lines.append("poisson %d %d %s %s" % (bits, prec, enc_rfun([([(c, (0, 0, 0, 0)) for c in P], (A, (0, 0, 0, 0)),
                 (B, (0, 0, 0, 0)), (C, (0, 0, 0, 0))) for (P, A, B, C) in phi]), enc_ffun(D, M, vals)))
out = run(lines, prog)
for (D, M, vals, phi, bits, prec), line in zip(cases, out):
    info = "D=%d M=%d bits=%d prec=%d phi=%s f=%s" % (D, M, bits, prec, phi, vals)
    t = Tok(line); st = t.int()
    if st != 0:
        S_["nd" if st == 1 else ("limit" if st == 10 else "other")] += 1
        if t.nxt() != "U":
            note("poisson", "output written on status %d: %s" % (st, info))
        if st not in (1, 10):
            note("poisson", "status %d %s" % (st, info))
        if st == 1:
            note("poisson-ND", "NOT_DETERMINED on exact input: %s" % info)
        continue
    S_["ok"] += 1
    NL, NR = t.int(), t.int()
    lft, rgt = t.acb(), t.acb()
    Lv = full_left(phi, D, M, vals)
    Rv, gmax = full_right(phi, D, M, vals)
    S_["selfcheck"] = max(S_["selfcheck"], float(abs(Lv - Rv) / max(1, abs(Lv))))
    for nm, b, v in (("left", lft, Lv), ("right", rgt, Rv)):
        S_["checks"] += 1
        h = has_c(b, v, mpf(10) ** -55 * max(1, abs(v)))
        if h == 0:
            note("poisson-" + nm, "%s NL=%d NR=%d got %s want %s" % (info, NL, NR, b, mpmath.nstr(v, 40)))
        elif h == 2:
            S_["undec"] += 1
        for part in b:
            if 2 * part[1] > Fr(2) ** -bits:
                note("poisson-width", "%s %s diameter %s > 2^-%d" % (info, nm, float(2 * part[1]), bits))
    eps8 = mpf(2) ** (-bits - 3)
    for nm, Nc in (("NL", NL), ("NR", NR)):
        if Nc != 0 and (Nc & (Nc - 1)) != 0:
            note("poisson-cutoff", "%s %s=%d not in 0,1,2,4,..." % (info, nm, Nc))
    EL = sum((abs(C2m(v)) * tail_majorant(phi, F2m(Fr(j, D)), M, NL) for j, v in enumerate(vals) if v != (0, 0)), mpf(0))
    if EL > eps8:
        note("poisson-tailL", "%s NL=%d direct tail majorant %s > eps/8" % (info, NL, mpmath.nstr(EL, 10)))
    # right: max|g| * sum_(|n|>NR) |phihat(n/M)|
    s = mpf(0)
    for sgn in (1, -1):
        n = NR + 1
        while True:
            v = abs(ft(phi, mpf(sgn * n) / M))
            s += v
            if n > NR + 5 and v < mpf(10) ** -80:
                break
            n += 1
    if gmax * s > eps8:
        note("poisson-tailR", "%s NR=%d direct tail majorant %s > eps/8" % (info, NR, mpmath.nstr(gmax * s, 10)))
    if NL > 0:
        prev = 0 if NL == 1 else NL // 2
        if lemma6_left(phi, D, M, vals, prev) <= eps8 * (1 - mpf(2) ** -40):
            S_["prev_passes_own_L6"] += 1
print("tensors", N, "seed", seed, S_)
print("fails", len(fails))
for f in fails[:30]:
    print(f)
