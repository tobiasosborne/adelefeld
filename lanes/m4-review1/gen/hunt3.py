"""Hunt 3: E1 evaluation. Usage: hunt3.py N seed
Own model: the values of f on X = (A + H Zhat)/d are {f(x_z)} over the rational points x_z = (A + H z)/d,
z = 0 .. L d D - 1 (Z is dense in Zhat; cell membership of x_z depends only on z mod L d), not the gcd test of
api-4.md E1 step 1. Partial balls: the ultrametric criterion v_p(c - j/D) >= min(e, v_p(M)) per supplied p,
written here from the definition, and the zero value always (an unsupplied prime)."""
import sys, random
from fractions import Fraction as Fr
from math import gcd
import mpmath
from mpmath import mpf, mpc
from m import *
from m2 import *

N = int(sys.argv[1]); seed = int(sys.argv[2]); prog = sys.argv[3] if len(sys.argv) > 3 else "h"
rng = random.Random(seed)
fails = []
S_ = {"evals": 0, "loc": 0, "locfail": 0, "excl": 0, "teval": 0, "sball": 0, "boundpts": 0, "maxboundratio": 0.0}


def note(k, m):
    fails.append((k, m))


def vp(q, p):
    q = Fr(q)
    if q == 0:
        return 10 ** 9
    v = 0
    n, d = q.numerator, q.denominator
    while n % p == 0:
        n //= p; v += 1
    while d % p == 0:
        d //= p; v -= 1
    return v


def canon3(A, H, d):
    """canonical triple: 0 <= A < H if H > 0, gcd(A, H, d) = 1 (own normalisation for the generator)."""
    g = gcd(gcd(A, H), d)
    A, H, d = A // g, H // g, d // g
    if H > 0:
        A %= H
    return A, H, d


def values_on_ball(D, M, vals, A, H, d):
    L = D * M
    out = set()
    if H == 0:
        return {f_point(D, M, vals, Fr(A, d))}
    for z in range(L * d * D):
        out.add(f_point(D, M, vals, Fr(A + H * z, d)))
    return out


def hull(vs):
    re = [v[0] for v in vs]; im = [v[1] for v in vs]
    return (min(re), max(re), min(im), max(im))


def rq(lo, hi, dens=(1, 2, 4, 3)):
    d = rng.choice(dens)
    return Fr(rng.randint(int(lo * d), int(hi * d)), d)


cases, lines = [], []
for i in range(N):
    D, M = rng.randint(1, 8), rng.randint(1, 8)
    vals = [(Fr(0), Fr(0)) if rng.random() < 0.2 else (rq(-9, 9), rq(-9, 9)) for _ in range(D * M)]
    F = enc_ffun(D, M, vals)
    d = rng.choice([1, 1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 16, 25, 27])
    H = rng.choice([0, 0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 15, 16, 18, 20, 24, 30, 36, 40, 45, 60, 72, 360])
    A = rng.randint(-50, 50)
    A, H, d = canon3(A, H, d)
    p = rng.choice([10, 30, 64, 128])
    cases.append(("ffeval", D, M, vals, A, H, d, p))
    lines.append("ffeval %d %s %d %d %d" % (p, F, A, H, d))
    cases.append(("ffevalloc", D, M, vals, A, H, d, p))
    lines.append("ffevalloc %d %s %d %d %d" % (p, F, A, H, d))
    # tensor_eval with a small phi
    phi = [([(rq(-2, 2), rq(-2, 2)), (Fr(1), Fr(0))], (Fr(1, 2), rq(-1, 1)), (rq(-1, 1), Fr(0)), (Fr(0), Fr(0)))]
    x = rq(-3, 3, (1, 2, 4, 8))
    R = enc_rfun([([(c, (0, 0, 0, 0)) for c in P], (A_, (0, 0, 0, 0)), (B_, (0, 0, 0, 0)), (C_, (0, 0, 0, 0)))
                  for (P, A_, B_, C_) in phi])
    cases.append(("teval", D, M, vals, A, H, d, p, phi, x))
    lines.append("teval %d %s %s %s %d %d %d" % (p, R, F, enc_real(x, 1, -20), A, H, d))
    # sball: places {2,3}, {5}, {} ; arch NONE or REAL
    pl = rng.choice([[2, 3], [5], [], [2], [3, 5]])
    arch = rng.choice([0, 1])
    locs = []
    for pr in pl:
        c = Fr(rng.randint(-30, 30), rng.choice([1, 2, 3, 4, 5, 9, 25]))
        e = rng.randint(-3, 4)
        ex = rng.random() < 0.25
        locs.append((pr, c, e, ex))
    xr = rq(-3, 3, (1, 2, 4))
    cases.append(("tsball", D, M, vals, phi, arch, xr, locs, p))
    lines.append("tsball %d %s %s %d %s 0 1 0 0 %d %s" % (p, R, F, arch, enc_real(xr), len(locs), " ".join(
        "%d %d %d %d %d" % (pr, c.numerator, c.denominator, e, int(ex)) for (pr, c, e, ex) in locs)))
    if i < 20:
        cases.append(("tsballC", D, M, vals, phi, 2, xr, locs, p))
        lines.append("tsball %d %s %s 2 %s 1 2 0 0 %d %s" % (p, R, F, enc_real(xr), len(locs), " ".join(
            "%d %d %d %d %d" % (pr, c.numerator, c.denominator, e, int(ex)) for (pr, c, e, ex) in locs)))

out = run(lines, prog)
for case, line in zip(cases, out):
    kind = case[0]
    if line.startswith("LOCALFAIL"):
        S_["locfail"] += 1
        continue
    t = Tok(line); st = t.int()
    if kind in ("ffeval", "ffevalloc"):
        _, D, M, vals, A, H, d, p = case
        info = "%s D=%d M=%d A=%d H=%d d=%d p=%d vals=%s" % (kind, D, M, A, H, d, p, vals)
        S_["evals"] += 1
        if kind == "ffevalloc":
            S_["loc"] += 1
        if st != 0:
            note(kind, "status %d %s" % (st, info)); continue
        b = t.acb()
        vs = values_on_ball(D, M, vals, A, H, d)
        for v in vs:
            if acb_has(b, v) == 0:
                note(kind, "value %s met but outside %s; %s" % (v, b, info))
        if len(vs) == 1 and all((x.denominator & (x.denominator - 1)) == 0 for x in next(iter(vs))):
            v = next(iter(vs))
            if b[0][1] != 0 or b[1][1] != 0 or b[0][0] != v[0] or b[1][0] != v[1]:
                note(kind + "-exact", "single value %s not returned exactly: %s; %s" % (v, b, info))
        hl = hull(vs)
        tol = Fr(2) ** (-p + 6) * (1 + max(abs(x) for x in hl))
        for v in set(vals) - vs:
            outside = (v[0] < hl[0] - tol or v[0] > hl[1] + tol or v[1] < hl[2] - tol or v[1] > hl[3] + tol)
            if outside and acb_has(b, v) == 1:
                S_["excl"] += 1
                note(kind + "-excl", "value %s not met, outside hull %s, but inside %s; %s" % (v, hl, b, info))
    elif kind == "teval":
        _, D, M, vals, A, H, d, p, phi, x = case
        S_["teval"] += 1
        if st != 0:
            note(kind, "status %d" % st); continue
        b = t.acb()
        vs = values_on_ball(D, M, vals, A, H, d)
        for xx in (x, x + Fr(1, 2 ** 20), x - Fr(1, 2 ** 20)):
            ph = ev(phi, F2m(xx))
            for v in vs:
                z = ph * C2m(v)
                if has_c(b, z, mpf(10) ** -50 * max(1, abs(z))) == 0:
                    note(kind, "x=%s v=%s product outside %s" % (xx, v, b))
    elif kind in ("tsball", "tsballC"):
        _, D, M, vals, phi, arch, xr, locs, p = case
        info = "D=%d M=%d arch=%d x=%s locs=%s vals=%s" % (D, M, arch, xr, locs, vals)
        if kind == "tsballC":
            if st != 7:
                note("tsball-complex", "status %d (DOMAIN expected) %s" % (st, info))
            elif t.nxt() != "U":
                note("tsball-complex", "output written")
            continue
        S_["sball"] += 1
        if st != 0:
            note(kind, "status %d %s" % (st, info)); continue
        b = t.acb()
        L = D * M
        met = {(Fr(0), Fr(0))}
        for j in range(L):
            ok = True
            for (pr, c, e, ex) in locs:
                m = vp(M, pr)
                thr = m if ex else min(e, m)
                # canonical lball reduction: a centre with v_p(c) >= e is the ball O(p^e); same set
                if vp(c - Fr(j, D), pr) < thr:
                    ok = False
            if ok:
                met.add(vals[j])
        if arch == 1:
            ph = [ev(phi, F2m(xr))]
        else:
            ts = [mpf(rng.uniform(-8, 8)) for _ in range(30)]
            ph = [ev(phi, tt) for tt in ts]
            # E1 step 5 bound on 1000 sampled points: |phi(t)| <= R, R read from the returned ball / max |v|
        for v in met:
            for pv in ph:
                z = pv * C2m(v)
                if has_c(b, z, mpf(10) ** -50 * max(1, abs(z))) == 0:
                    note(kind, "value %s * phi %s outside %s; %s" % (v, mpmath.nstr(pv, 10), b, info))
        if arch == 0 and len(met) == 1:
            pass

# E1 step 5 alone: arch NONE with f = 1 on Zhat (D=M=1) and the empty place set returns [-R,R]+i[-R,R].
lines5, phis = [], []
for i in range(200 if N >= 200 else N):
    T = []
    for _ in range(rng.randint(1, 3)):
        l = rng.randint(1, 6)
        P = [(rq(-3, 3), rq(-3, 3)) for _ in range(l)]
        if P[-1] == (0, 0):
            P[-1] = (Fr(1), Fr(0))
        T.append((P, (rng.choice([Fr(1, 8), Fr(1, 4), Fr(1), Fr(3)]), rq(-3, 3)), (rq(-3, 3), rq(-3, 3)), (rq(-1, 1), rq(-1, 1))))
    phis.append(T)
    R = enc_rfun([([(c, (0, 0, 0, 0)) for c in P], (A_, (0, 0, 0, 0)), (B_, (0, 0, 0, 0)), (C_, (0, 0, 0, 0)))
                  for (P, A_, B_, C_) in T])
    lines5.append("tsball 64 %s %s 0 %s %s 0" % (R, enc_ffun(1, 1, [(Fr(1), Fr(0))]), enc_real(0), enc_real(0)))
out5 = run(lines5, prog)
for T, line in zip(phis, out5):
    t = Tok(line); st = t.int()
    if st != 0:
        note("bound", "status %d" % st); continue
    b = t.acb()
    Rr = F2m(b[0][1])
    # own bound alpha, beta, gamma
    mx = mpf(0)
    for k in range(1000):
        tt = mpf(rng.uniform(-12, 12)) if k % 2 else mpf(rng.gauss(0, 2))
        v = abs(ev(T, tt))
        S_["boundpts"] += 1
        if abs(mpmath.re(ev(T, tt))) > Rr or abs(mpmath.im(ev(T, tt))) > Rr:
            note("bound", "|phi(%s)| = %s > R = %s; %s" % (tt, v, Rr, T))
        mx = max(mx, v)
    if mx > 0:
        S_["maxboundratio"] = max(S_["maxboundratio"], float(Rr / mx))
print("cases", N, "seed", seed, S_)
import collections
print("fails", len(fails), dict(collections.Counter(k for k, _ in fails)))
for f in fails[:30]:
    print(f)
