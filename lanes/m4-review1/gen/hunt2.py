"""Hunt 2: real test functions against own models. Usage: hunt2.py N seed nquad"""
import sys, random
from fractions import Fraction as Fr
import mpmath
from mpmath import mpf, mpc
from m import *
from m2 import *

N = int(sys.argv[1]); seed = int(sys.argv[2]); NQ = int(sys.argv[3]) if len(sys.argv) > 3 else 0
prog = sys.argv[4] if len(sys.argv) > 4 else "h"
rng = random.Random(seed)
fails = []
st_ = {"checks": 0, "undecided": 0, "maxrel_eval": 0.0, "quad": 0, "quaddiff": 0.0}
Z4 = (0, 0, 0, 0)


def note(kind, msg):
    fails.append((kind, msg))


def rq(lo, hi, dens=(1, 2, 4, 8, 3, 16)):
    d = rng.choice(dens)
    return Fr(rng.randint(int(lo * d), int(hi * d)), d)


def rterm(rad):
    l = rng.choice([0, 1, 1, 2, 3, 4, 5, 6, 7])
    P = [(rq(-3, 3), rq(-3, 3)) for _ in range(l)]
    while P and P[-1] == (0, 0):
        P[-1] = (Fr(1), Fr(0))
    are = rng.choice([Fr(1, 8), Fr(1, 4), Fr(1, 2), Fr(1), Fr(2), Fr(3, 2), rq(Fr(1, 8), 4)])
    if are <= 0:
        are = Fr(1, 8)
    A = (are, rq(-4, 4))
    B = (rq(-3, 3), rq(-3, 3))
    C = (rq(-2, 2), rq(-2, 2))
    def r4(scale):
        if not rad or rng.random() < 0.5:
            return Z4
        return (rng.randint(1, 3), -rng.randint(scale, 40), rng.randint(0, 3), -rng.randint(scale, 40))
    return ([(c, r4(10)) for c in P], (A, r4(12)), (B, r4(10)), (C, r4(10)))


def centers(T):
    return [([c for c, _ in P], A[0], B[0], C[0]) for (P, A, B, C) in T]


def enc_out_rfun(R):
    """Parsed C rfun -> input encoding with exact balls."""
    def eb(b):
        m_, r_ = b
        if r_ == 0:
            return "%d %d 0 0" % (m_.numerator, m_.denominator)
        e = 0
        while r_.denominator != 1:
            r_ *= 2; e -= 1
        while r_.numerator % 2 == 0 and r_ > 1:
            r_ /= 2; e += 1
        return "%d %d %d %d" % (m_.numerator, m_.denominator, int(r_), e)
    parts = [str(len(R))]
    for (P, A, B, C) in R:
        parts.append(str(len(P)))
        for c in list(P) + [A, B, C]:
            parts.append(eb(c[0]) + " " + eb(c[1]))
    return " ".join(parts)


def chk_ex(kind, ball, z, info):
    st_["checks"] += 1
    h = acb_has(ball, z)
    if h == 0:
        note(kind, "%s got %s want %s" % (info, ball, z))


def chk_num(kind, ball, z, info, tol=None):
    st_["checks"] += 1
    t = tol if tol is not None else mpf(10) ** -55 * max(1, abs(z))
    h = has_c(ball, z, t)
    if h == 0:
        note(kind, "%s got %s want %s" % (info, ball, mpmath.nstr(z, 30)))
    elif h == 2:
        st_["undecided"] += 1


def chk_rfun_exact(kind, R, want, info, numeric=False):
    """R parsed C rfun; want list of (P, A, B, C) with exact Fractions or mpc (numeric)."""
    if len(R) != len(want):
        note(kind, "%s len %d want %d" % (info, len(R), len(want))); return
    for i, ((P, A, B, C), (wP, wA, wB, wC)) in enumerate(zip(R, want)):
        if len(P) != len(wP):
            note(kind, "%s term %d plen %d want %d" % (info, i, len(P), len(wP))); continue
        for k, (b, w) in enumerate(zip(list(P) + [A, B, C], list(wP) + [wA, wB, wC])):
            if isinstance(w, tuple):
                chk_ex(kind, b, w, "%s term %d field %d" % (info, i, k))
            else:
                chk_num(kind, b, w, "%s term %d field %d" % (info, i, k))


lines, jobs = [], []
for n in range(N):
    rad = rng.random() < 0.3
    T = [rterm(rad) for _ in range(rng.randint(1, 3))]
    T2 = [rterm(False) for _ in range(rng.randint(1, 2))]
    p = rng.choice([20, 64, 128, 200])
    q = Fr(rng.randint(-9, 9), rng.choice([1, 2, 3, 5, 7]))
    h = Fr(rng.choice([-3, -2, -1, 1, 2, 3, 5]), rng.choice([1, 2, 3, 4]))
    R = enc_rfun(T); R2 = enc_rfun(T2)
    xs = []
    for _ in range(100):
        x = rq(-4, 4, (1, 2, 3, 7, 1024))
        if rng.random() < 0.3:
            xs.append((x, (rng.randint(1, 3), -rng.randint(4, 30))))
        else:
            xs.append((x, (0, 0)))
    ys = [rq(-3, 3, (1, 2, 3, 4, 5, 8)) for _ in range(50)]
    jobs.append(dict(T=T, T2=T2, p=p, q=q, h=h, xs=xs, ys=ys, rad=rad))
    lines += ["rtrans %d %s %d %d" % (p, R, q.numerator, q.denominator),
              "rdil %d %s %d %d" % (p, R, h.numerator, h.denominator),
              "rmul %d %s %s" % (p, R, R2),
              "rrefl %s" % R, "rconj %s" % R, "rderiv %d %s" % (p, R), "rfour %d %s" % (p, R),
              "revalm %d %s %d %s" % (p, R, len(xs), " ".join(enc_real(x, *r) for x, r in xs)),
              "rint %d %s" % (p, R), "rnorm %d %s" % (p, R)]
K = 10
out = run(lines, prog)
assert len(out) == len(lines)
lines2, jobs2 = [], []
pi = mpmath.pi
for n, job in enumerate(jobs):
    o = out[K * n:K * n + K]
    T, p = job["T"], job["p"]
    Tc = centers(T)
    info = "fn %d p=%d" % (n, p)
    # translate
    t = Tok(o[0]); s = t.int()
    q = job["q"]
    if s != 0:
        note("rtrans", "%s status %d" % (info, s))
    else:
        want = [(shift(P, q), A, C2m(B) + 2 * pi * C2m(A) * F2m(q), C2m(C) - C2m(B) * F2m(q) - pi * C2m(A) * F2m(q) ** 2)
                for (P, A, B, C) in Tc]
        chk_rfun_exact("rtrans", t.rfun(), want, info + " q=%s" % q)
    # dilate
    t = Tok(o[1]); s = t.int(); h = job["h"]
    if s != 0:
        note("rdil", "%s status %d" % (info, s))
    else:
        want = [(norm([(c[0] * h ** j, c[1] * h ** j) for j, c in enumerate(P)]), (A[0] * h * h, A[1] * h * h),
                 (B[0] * h, B[1] * h), C) for (P, A, B, C) in Tc]
        chk_rfun_exact("rdil", t.rfun(), want, info + " h=%s" % h)
    # mul
    t = Tok(o[2]); s = t.int()
    T2c = centers(job["T2"])
    if s != 0:
        note("rmul", "%s status %d" % (info, s))
    else:
        want = [(polymul(P, P2), cadd(A, A2), cadd(B, B2), cadd(C, C2)) for (P, A, B, C) in Tc for (P2, A2, B2, C2) in T2c]
        chk_rfun_exact("rmul", t.rfun(), want, info)
    # reflect, conj
    t = Tok(o[3]); s = t.int()
    want = [([(c[0] * (-1) ** j, c[1] * (-1) ** j) for j, c in enumerate(P)], A, (-B[0], -B[1]), C) for (P, A, B, C) in Tc]
    chk_rfun_exact("rrefl", t.rfun(), want, info) if s == 0 else note("rrefl", info)
    t = Tok(o[4]); s = t.int()
    cj = lambda z: (z[0], -z[1])
    want = [([cj(c) for c in P], cj(A), cj(B), cj(C)) for (P, A, B, C) in Tc]
    chk_rfun_exact("rconj", t.rfun(), want, info) if s == 0 else note("rconj", info)
    # derivative: P' + (B - 2 pi A x) P
    t = Tok(o[5]); s = t.int()
    if s != 0:
        note("rderiv", "%s status %d" % (info, s))
    else:
        want = []
        for (P, A, B, C) in Tc:
            if not P:
                want.append(([], A, B, C)); continue
            co = [mpc(0)] * (len(P) + 1)
            for j, c in enumerate(P):
                if j:
                    co[j - 1] += j * C2m(c)
                co[j] += C2m(B) * C2m(c)
                co[j + 1] -= 2 * pi * C2m(A) * C2m(c)
            want.append((co, A, B, C))
        chk_rfun_exact("rderiv", t.rfun(), want, info)
    # fourier: parameters A' = 1/A (exact), B' = i B/A (exact); polynomial via evaluation below
    t = Tok(o[6]); s = t.int()
    if s != 0:
        note("rfour", "%s status %d" % (info, s))
    else:
        FR = t.rfun()
        for i, ((P, A, B, C), (cP, cA, cB, cC)) in enumerate(zip(FR, Tc)):
            a = Fr(1) / (cA[0] ** 2 + cA[1] ** 2)
            inv = (cA[0] * a, -cA[1] * a)
            chk_ex("rfour", A, inv, info + " A' term %d" % i)
            chk_ex("rfour", B, cmul((Fr(0), Fr(1)), cmul(cB, inv)), info + " B' term %d" % i)
            chk_num("rfour", C, C2m(cC) + C2m(cB) ** 2 / (4 * pi * C2m(cA)), info + " C' term %d" % i)
        enc = enc_out_rfun(FR)
        jobs2.append((n, "fev"))
        lines2.append("revalm %d %s %d %s" % (p, enc, len(job["ys"]), " ".join(enc_real(y) for y in job["ys"])))
        jobs2.append((n, "ff"))
        lines2.append("rfour %d %s" % (p, enc))
    # eval at 100 points
    res = o[7].split(" ; ")
    for (x, r), rs in zip(job["xs"], res):
        t = Tok(rs); s = t.int()
        if s != 0:
            note("reval", "%s x=%s status %d" % (info, x, s)); continue
        b = t.acb()
        pts = [x]
        if r[0]:
            R_ = Fr(r[0]) * Fr(2) ** r[1]
            pts += [x + R_, x - R_, x + R_ * Fr(rng.randint(-99, 99), 100)]
        for xx in pts:
            chk_num("reval", b, ev(Tc, F2m(xx)), info + " x=%s" % xx)
        if not job["rad"] and r[0] == 0 and x.denominator in (1, 2, 1024):
            v = ev(Tc, F2m(x))
            rr = max(F2m(b[0][1]), F2m(b[1][1]))
            if abs(v) > 0:
                st_["maxrel_eval"] = max(st_["maxrel_eval"], float(rr / max(abs(v), 1) * 2 ** p))
    # integral
    t = Tok(o[8]); s = t.int()
    I = ft(Tc, 0)
    if s != 0:
        note("rint", "%s status %d" % (info, s))
    else:
        chk_num("rint", t.acb(), I, info)
    # norm2: |phi|^2 = phi conj(phi) as exact product, closed-form integral at y = 0
    t = Tok(o[9]); s = t.int()
    conjT = [([cj(c) for c in P], cj(A), cj(B), cj(C)) for (P, A, B, C) in Tc]
    prodT = [(polymul(P, P2), cadd(A, A2), cadd(B, B2), cadd(C, C2)) for (P, A, B, C) in Tc for (P2, A2, B2, C2) in conjT]
    NV = ft(prodT, 0)
    if s != 0:
        note("rnorm", "%s status %d" % (info, s))
    else:
        b = t.arb()
        chk_num("rnorm", (b, (Fr(0), Fr(0))), mpc(mpmath.re(NV), 0), info)
    # quadrature cross-checks of the closed forms (independent of the C code)
    if n < NQ:
        amin = min(F2m(A[0]) for (P, A, B, C) in Tc)
        bmax = max(abs(F2m(B[0])) for (P, A, B, C) in Tc) + 1
        W = bmax / (pi * amin) + mpmath.sqrt(200 / (pi * amin)) + 2
        mpmath.mp.dps = 60
        for y in job["ys"][:3]:
            Q = quad_line(lambda x: ev(Tc, x) * mpmath.exp(2j * pi * x * y), W)
            Fv = ft(Tc, F2m(y))
            st_["quad"] += 1
            st_["quaddiff"] = max(st_["quaddiff"], float(abs(Q - Fv) / max(1, abs(Fv))))
        Q = quad_line(lambda x: abs(ev(Tc, x)) ** 2, W)
        st_["quaddiff"] = max(st_["quaddiff"], float(abs(Q - mpmath.re(NV)) / max(1, abs(NV))))
        st_["quad"] += 1
        mpmath.mp.dps = 80

out2 = run(lines2, prog) if lines2 else []
lines3, jobs3 = [], []
for (n, kind), line in zip(jobs2, out2):
    job = jobs[n]
    Tc = centers(job["T"])
    info = "fn %d p=%d" % (n, job["p"])
    if kind == "fev":
        for y, rs in zip(job["ys"], line.split(" ; ")):
            t = Tok(rs); s = t.int()
            if s != 0:
                note("fev", "%s y=%s status %d" % (info, y, s)); continue
            chk_num("rfour-eval", t.acb(), ft(Tc, F2m(y)), info + " y=%s" % y)
    else:
        t = Tok(line); s = t.int()
        if s != 0:
            note("ff", "%s status %d" % (info, s)); continue
        FF = t.rfun()
        xs = [x for x, r in job["xs"][:10]]
        lines3.append("revalm %d %s %d %s" % (job["p"], enc_out_rfun(FF), len(xs), " ".join(enc_real(x) for x in xs)))
        jobs3.append((n, xs))
out3 = run(lines3, prog) if lines3 else []
for (n, xs), line in zip(jobs3, out3):
    Tc = centers(jobs[n]["T"])
    for x, rs in zip(xs, line.split(" ; ")):
        t = Tok(rs); s = t.int()
        if s != 0:
            note("ff", "eval status %d" % s); continue
        chk_num("F^2", t.acb(), ev(Tc, -F2m(x)), "fn %d x=%s" % (n, x))

print("functions", N, "seed", seed, st_)
print("fails", len(fails))
for f in fails[:40]:
    print(f)
