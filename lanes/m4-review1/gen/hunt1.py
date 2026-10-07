"""Hunt 1: finite transform and algebra against own exact models. Usage: hunt1.py N seed [prog]"""
import sys, random
from fractions import Fraction as Fr
from math import gcd
from m import *

N = int(sys.argv[1]); seed = int(sys.argv[2]); prog = sys.argv[3] if len(sys.argv) > 3 else "h"
rng = random.Random(seed)
fails = []
stats = {"cells": 0, "undecided": 0, "maxratio": 0.0, "ops": {}}


def rq():
    k = rng.random()
    if k < 0.15:
        return Fr(0)
    den = rng.choice([1, 2, 4, 8, 3, 5, 7, 6, 12, 1024, 9])
    return Fr(rng.randint(-40, 40), den)


def rfun_vals(D, M, radp=0.3):
    vals, rads = [], []
    for _ in range(D * M):
        vals.append((rq(), rq()))
        if rng.random() < radp:
            rads.append((rng.randint(1, 7), -rng.randint(3, 40), rng.randint(0, 7), -rng.randint(3, 40)))
        else:
            rads.append((0, 0, 0, 0))
    return vals, rads


def member(vals, rads):
    """A random rational member of the input balls (inside the closed rectangles)."""
    out = []
    for v, r in zip(vals, rads):
        a = v[0] + (Fr(rng.randint(-1000, 1000), 1000) * r[0] * Fr(2) ** r[1] if r[0] else 0)
        b = v[1] + (Fr(rng.randint(-1000, 1000), 1000) * r[2] * Fr(2) ** r[3] if r[2] else 0)
        out.append((a, b))
    return out


def ball_point(D, M, balls, x):
    x = Fr(x)
    y = D * x
    if y.denominator != 1:
        return None
    return balls[int(y) % (D * M)]


def note(kind, msg):
    stats["ops"][kind] = stats["ops"].get(kind, 0)
    fails.append((kind, msg))


def check_fn(kind, out, model, D2, M2, info):
    """out = (D, M, balls) from C; model(x) = exact value at rational x. Cell representatives and off-grid."""
    oD, oM, balls = out
    if (oD, oM) != (D2, M2):
        note(kind, "layout %s got %s %s" % (info, (oD, oM), (D2, M2)))
        return
    L2 = oD * oM
    pts = [Fr(k, oD) + oM * rng.randint(-3, 3) for k in range(L2)]
    for _ in range(6):
        b = rng.choice([2, 3, 5, 7, 4, 9, 8, 11]) * oD
        pts.append(Fr(rng.randint(-5 * b, 5 * b), b))
    for x in pts:
        want = model(x)
        got = ball_point(oD, oM, balls, x)
        stats["cells"] += 1
        if got is None:
            if want != (0, 0):
                note(kind, "%s x=%s outside output support but model %s" % (info, x, want))
            continue
        h = acb_has(got, want)
        if h == 0:
            note(kind, "%s x=%s got %s want %s" % (info, x, got, want))
        elif h == 2:
            stats["undecided"] += 1


lines, jobs = [], []
for i in range(N):
    D, M = rng.randint(1, 12), rng.randint(1, 12)
    vals, rads = rfun_vals(D, M)
    F = enc_ffun(D, M, vals, rads)
    p = rng.choice([2, 10, 30, 64, 128, 200])
    D2, M2 = rng.randint(1, 12), rng.randint(1, 12)
    vals2, rads2 = rfun_vals(D2, M2)
    G = enc_ffun(D2, M2, vals2, rads2)
    t = rng.choice([1, D, rng.randint(1, 12), 2 * D])
    s = rng.randint(-20, 20)
    qt = Fr(s, t)
    qs, qd = rng.choice([-4, -3, -2, -1, 1, 2, 3, 4]), rng.randint(1, 4)
    qdil = Fr(qs, qd)
    rr = Fr(rng.randint(1, 4), rng.randint(1, 4))
    Nu = rng.choice([0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 15, 24, 30, rng.randint(1, 60)])
    if Nu == 0:
        cu = rng.choice([1, -1])
    else:
        while True:
            cu = rng.randint(0, max(Nu - 1, 0))
            if gcd(cu, Nu) == 1:
                break
    ra, rb = rng.randint(1, 3), rng.randint(1, 3)
    job = dict(D=D, M=M, vals=vals, rads=rads, p=p, D2=D2, M2=M2, vals2=vals2, rads2=rads2, qt=qt, qdil=qdil,
               rr=rr, Nu=Nu, cu=cu, ra=ra, rb=rb)
    jobs.append(job)
    lines.append("fourier %d %s" % (p, F))
    lines.append("add %d %s %s" % (p, F, G))
    lines.append("mul %d %s %s" % (p, F, G))
    lines.append("refine %s %d %d" % (F, D * ra, M * rb))
    lines.append("translate %s %d %d" % (F, qt.numerator, qt.denominator))
    lines.append("reflect %s" % F)
    lines.append("conj %s" % F)
    lines.append("dilrat %s %d %d" % (F, qdil.numerator, qdil.denominator))
    lines.append("dilid %s 1 1 0 0 %d %d %d %d" % (F, rr.numerator, rr.denominator, cu, Nu))
    lines.append("ffnorm %d %s" % (p, F))

out = run(lines, prog)
assert len(out) == len(lines), (len(out), len(lines))


def enc_out(o):
    """Re-encode a C ffun output as input (exact mid, exact radius)."""
    D, M, balls = o
    parts = ["%d %d" % (D, M)]
    for (re, im) in balls:
        for (m_, r_) in (re, im):
            n, e = r_.numerator, r_.denominator
            # radius as man*2^exp: r_ is dyadic
            if r_ == 0:
                rm, rexp = 0, 0
            else:
                ee = 0
                while r_.denominator != 1:
                    r_ *= 2; ee -= 1
                rm, rexp = int(r_), ee
            parts.append("%d %d %d %d" % (m_.numerator, m_.denominator, rm, rexp))
    return " ".join(parts)


lines2, jobs2 = [], []
for i, job in enumerate(jobs):
    o = out[10 * i:10 * i + 10]
    D, M, vals, rads, p = job["D"], job["M"], job["vals"], job["rads"], job["p"]
    L = D * M
    fm = lambda x, D=D, M=M, vals=vals: f_point(D, M, vals, x)
    # --- fourier
    t = Tok(o[0]); st = t.int()
    if st != 0:
        if not (L * L > 2 ** 20):
            note("fourier", "status %d D=%d M=%d p=%d" % (st, D, M, p))
    else:
        g = t.ffun()
        if (g[0], g[1]) != (M, D):
            note("fourier", "layout")
        ex = ftransform_exact(D, M, vals)
        mem = member(vals, rads)
        exm = ftransform_exact(D, M, mem)
        sr = sum(Fr(r[0]) * Fr(2) ** r[1] + Fr(r[2]) * Fr(2) ** r[3] for r in rads)
        sm = sum(abs(v[0]) + abs(v[1]) for v in vals) + sr
        for k in range(L):
            for ref in (ex[k], exm[k]):
                h = acb_has(g[2][k], (ref[0], ref[1]), ref[2])
                stats["cells"] += 1
                if h == 0:
                    note("fourier", "D=%d M=%d p=%d k=%d got %s want %s" % (D, M, p, k, g[2][k], ref[:2]))
                elif h == 2:
                    stats["undecided"] += 1
            # radius: rad <= sum r/M + c * L * sum(|m|+r)/M * 2^-p ; record c
            rad = max(g[2][k][0][1], g[2][k][1][1])
            excess = rad - sr / M
            if excess > 0 and sm > 0:
                ratio = float(excess / (sm / M * Fr(2) ** (-max(p, 2))))
                stats["maxratio"] = max(stats["maxratio"], ratio / L)
        jobs2.append((i, "ff2"))
        lines2.append("fourier %d %s" % (p, enc_out(g)))
        jobs2.append((i, "pars"))
        lines2.append("ffnorm %d %s" % (p, enc_out(g)))
        # covariance: fourier(dilrat(f, q))
    # --- add / mul
    D2, M2, vals2 = job["D2"], job["M2"], job["vals2"]
    gm = lambda x: f_point(D2, M2, vals2, x)
    for idx, kind in ((1, "add"), (2, "mul")):
        t = Tok(o[idx]); st = t.int()
        if st != 0:
            note(kind, "status %d" % st); continue
        res = t.ffun()
        if kind == "add":
            mod = lambda x: tuple(a + b for a, b in zip(fm(x), gm(x)))
        else:
            mod = lambda x: (fm(x)[0] * gm(x)[0] - fm(x)[1] * gm(x)[1], fm(x)[0] * gm(x)[1] + fm(x)[1] * gm(x)[0])
        check_fn(kind, res, mod, lcm(D, D2), lcm(M, M2), "D=%d M=%d D2=%d M2=%d" % (D, M, D2, M2))
    # --- refine
    t = Tok(o[3]); st = t.int()
    if st != 0:
        note("refine", "status %d" % st)
    else:
        check_fn("refine", t.ffun(), fm, D * job["ra"], M * job["rb"], "refine")
    # --- translate f(x - q)
    qt = job["qt"]
    t = Tok(o[4]); st = t.int()
    if st != 0:
        note("translate", "status %d" % st)
    else:
        check_fn("translate", t.ffun(), lambda x: fm(x - qt), lcm(D, qt.denominator), M, "q=%s D=%d M=%d" % (qt, D, M))
    t = Tok(o[5]); st = t.int()
    check_fn("reflect", t.ffun(), lambda x: fm(-x), D, M, "reflect") if st == 0 else note("reflect", "st")
    t = Tok(o[6]); st = t.int()
    check_fn("conj", t.ffun(), lambda x: (fm(x)[0], -fm(x)[1]), D, M, "conj") if st == 0 else note("conj", "st")
    q = job["qdil"]
    t = Tok(o[7]); st = t.int()
    if st != 0:
        note("dilrat", "status %d" % st)
    else:
        check_fn("dilrat", t.ffun(), lambda x: fm(q * x), abs(q.numerator) * D, q.denominator * M,
                 "q=%s D=%d M=%d" % (q, D, M))
    # --- dilate_idele: own unit image by lifts modulo lcm(N, L)
    rr, Nu, cu = job["rr"], job["Nu"], job["cu"]
    if Nu == 0:
        R = {cu % L}
    else:
        md = lcm(Nu, L)
        R = {w % L for w in range(md) if gcd(w, md) == 1 and (w - cu) % Nu == 0}
    t = Tok(o[8]); st = t.int()
    if len(R) == 1:
        if st != 0:
            note("dilid", "status %d with singleton R=%s L=%d c=%d N=%d" % (st, R, L, cu, Nu))
        else:
            v = R.pop()
            def unitlift(x, v=v, L=L, rr=rr):
                # an integer v' = v mod L that is a unit at every prime of x and of r (a stand-in for u)
                P = x.numerator * x.denominator * rr.numerator * rr.denominator or 1
                w = v
                while gcd(w, P) != 1:
                    w += L
                return w
            check_fn("dilid", t.ffun(), lambda x: fm(rr * unitlift(Fr(x)) * x), rr.numerator * D, rr.denominator * M,
                     "r=%s v=%d c=%d N=%d L=%d" % (rr, v, cu, Nu, L))
    else:
        if st == 0:
            note("dilid", "OK with |R|=%d L=%d c=%d N=%d" % (len(R), L, cu, Nu))
        elif t.nxt() != "U":
            note("dilid", "output written on failure")
    # --- ffnorm exact
    t = Tok(o[9]); st = t.int()
    nv = sum(v[0] ** 2 + v[1] ** 2 for v in vals) / M
    b = t.arb()
    if st != 0 or arb_has(b, nv) == 0:
        note("ffnorm", "st %d got %s want %s" % (st, b, nv))
    # --- covariance job: fourier(dilrat(f, q)) at the C level
    if Tok(o[7]).int() == 0:
        jobs2.append((i, "cov"))
        lines2.append("fourier %d %s" % (max(p, 64), enc_out(Tok(o[7][2:]).ffun())))

out2 = run(lines2, prog)
for (i, kind), line in zip(jobs2, out2):
    job = jobs[i]
    D, M, vals = job["D"], job["M"], job["vals"]
    L = D * M
    t = Tok(line); st = t.int()
    if kind == "ff2":
        if st != 0:
            note("ff2", "status %d" % st); continue
        h = t.ffun()
        check_fn("ff2", h, lambda x: f_point(D, M, vals, -x), D, M, "F^2 D=%d M=%d" % (D, M))
    elif kind == "pars":
        b = t.arb()
        nv = sum(v[0] ** 2 + v[1] ** 2 for v in vals) / M
        if st != 0 or arb_has(b, nv) == 0:
            note("parseval", "D=%d M=%d got %s want %s" % (D, M, b, nv))
    elif kind == "cov":
        if st != 0:
            continue
        G = t.ffun()
        q = job["qdil"]
        ex = ftransform_exact(D, M, vals)
        # hat(D_q f)(y) = |q| hat f(y/q); hat f(y) = g_{(M y) mod L} if M y in Z else 0 (layout D'=M, M'=D)
        for k in range(G[0] * G[1]):
            y = Fr(k, G[0])
            z = M * y / q
            if z.denominator != 1:
                want = (Fr(0), Fr(0), Fr(0))
            else:
                w = ex[int(z) % L]
                want = (abs(q) * w[0], abs(q) * w[1], abs(q) * w[2])
            h = acb_has(G[2][k], want[:2], want[2])
            stats["cells"] += 1
            if h == 0:
                note("cov", "q=%s D=%d M=%d k=%d got %s want %s" % (q, D, M, k, G[2][k], want[:2]))

print("functions", N, "seed", seed, "cells", stats["cells"], "undecided", stats["undecided"],
      "max (rad - sum r/M)/(L sum(|m|+r)/M 2^-p)", round(stats["maxratio"], 3))
print("fails", len(fails))
for f in fails[:40]:
    print(f)
