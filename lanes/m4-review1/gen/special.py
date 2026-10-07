"""Targeted cases of hunts 2 and 4: shifted Gaussian sign, root branch per quadrant, NOT_DETERMINED with untouched
outputs, theta, the witness c in [1, 2], bits DOMAIN after the prec cap, tiny Re(A) LIMIT time."""
import sys, time
from fractions import Fraction as Fr
import mpmath
from mpmath import mpf, mpc
from m import *
from m2 import *

prog = sys.argv[1] if len(sys.argv) > 1 else "h"
Z = (0, 0, 0, 0)
class R(list):
    def append(self, x):
        print(x, flush=True)
res = R()


def term(P, A, B=(0, 0), C=(0, 0), radC=Z, radP=None):
    return ([((Fr(c[0]), Fr(c[1])), (radP or Z)) for c in P], ((Fr(A[0]), Fr(A[1])), Z), ((Fr(B[0]), Fr(B[1])), Z),
            ((Fr(C[0]), Fr(C[1])), radC))


def one(line):
    return run([line], prog)[0]


# 1. shifted Gaussian q = 1/3: translate exp(-pi x^2) by 1/3, transform, evaluate at y = 1/4
g = enc_rfun([term([(1, 0)], (1, 0))])
o = Tok(one("rtrans 128 %s 1 3" % g)); assert o.int() == 0
sh = o.rfun()
from hunt2_enc import enc_out_rfun
o = Tok(one("rfour 128 %s" % enc_out_rfun(sh))); assert o.int() == 0
fs = o.rfun()
o = one("revalm 128 %s 1 %s" % (enc_out_rfun(fs), enc_real(Fr(1, 4))))
t = Tok(o); st = t.int(); b = t.acb()
want = mpmath.exp(-mpmath.pi / 16) * mpmath.expjpi(mpf(2) / 12)
res.append(("shifted Gaussian F(phi)(1/4)", st, float(b[1][0]), "Im positive" if b[1][0] - b[1][1] > 0 else "Im NOT certified positive",
            has_c(b, want, mpf(10) ** -60)))

ONLYP = len(sys.argv) > 2
# 2. root branch: A in every quadrant of the right half-plane, P = 1 and P = x, B = 1 + i
for A in ([] if ONLYP else [(1, 3), (1, -3), (Fr(1, 8), 5), (Fr(1, 8), -5), (5, Fr(1, 8)), (5, -Fr(1, 8)), (2, 0)]):
    for P in ([(1, 0)], [(0, 0), (1, 0)]):
        T = [term(P, A, (1, 1))]
        Tc = [([(Fr(c[0]), Fr(c[1])) for c in P], (Fr(A[0]), Fr(A[1])), (Fr(1), Fr(1)), (Fr(0), Fr(0)))]
        o = Tok(one("rfour 100 %s" % enc_rfun(T))); st = o.int()
        if st != 0:
            res.append(("root branch A=%s P=%s" % (A, P), "status", st)); continue
        FR = o.rfun()
        ys = [Fr(k, 4) for k in (-5, -1, 0, 2, 6)]
        r = one("revalm 100 %s %d %s" % (enc_out_rfun(FR), len(ys), " ".join(enc_real(y) for y in ys))).split(" ; ")
        bad = 0
        mpmath.mp.dps = 50
        amin = F2m(Fr(A[0]))
        W = 2 / (mpmath.pi * amin) + mpmath.sqrt(200 / (mpmath.pi * amin)) + 2
        for y, rs in zip(ys, r):
            t = Tok(rs); s = t.int(); bb = t.acb()
            Q = quad_line(lambda x: ev(Tc, x) * mpmath.exp(2j * mpmath.pi * x * F2m(y)), W)
            if has_c(bb, Q, mpf(10) ** -35 * max(1, abs(Q))) == 0:
                bad += 1
        mpmath.mp.dps = 80
        res.append(("root branch A=%s P=%s vs quad at 5 y" % (A, P), "bad", bad))

# 3. NOT_DETERMINED with untouched outputs at prec 2: Re(1/A) lost for A = 1 + 100 i; derivative/translate too
T = enc_rfun([term([(1, 0), (1, 1)], (Fr(1, 64), 100), (3, 1))])
for cmd in ("rfour 2 %s" % T, "rtrans 2 %s 7 3" % T, "rdil 2 %s 7 3" % T, "rmul 2 %s %s" % (T, T), "rderiv 2 %s" % T,
            "rint 2 %s" % T, "rnorm 2 %s" % T, "reval 2 %s %s" % (T, enc_real(Fr(1, 3)))):
    res.append(("prec 2: " + cmd.split()[0], one(cmd)[:60]))

# 4. Poisson: theta, witness, bits domain, prec cap, tiny Re(A)
F1 = enc_ffun(1, 1, [(Fr(1), Fr(0))])
G = enc_rfun([term([(1, 0)], (1, 0))])
for bits, prec in ((53, 20), (100, 20), (30, 2), (200, 64)):
    t = Tok(one("poisson %d %d %s %s" % (bits, prec, G, F1))); st = t.int()
    if st == 0:
        NL, NR = t.int(), t.int(); l, r = t.acb(), t.acb()
        th = mpmath.jtheta(3, 0, mpmath.exp(-mpmath.pi))
        res.append(("theta bits=%d prec=%d" % (bits, prec), NL, NR, has_c(l, th, mpf(10) ** -60), has_c(r, th, mpf(10) ** -60),
                    float(2 * l[0][1]), float(2 * r[0][1]), mpmath.nstr(th, 25)))
    else:
        res.append(("theta bits=%d prec=%d" % (bits, prec), "status", st))
W_ = enc_rfun([([((Fr(3, 2), Fr(0)), (1, -1, 0, 0))], ((Fr(1), Fr(0)), Z), ((Fr(0), Fr(0)), Z), ((Fr(0), Fr(0)), Z))])
t0 = time.time()
res.append(("witness c in [1,2] bits 20 prec 20", one("poisson 20 20 %s %s" % (W_, F1)), round(time.time() - t0, 2)))
res.append(("witness c in [1,2] bits 0 prec 20", one("poisson 0 20 %s %s" % (W_, F1))))
for bits, prec in ((-1, 64), (2097153, 64), (2097152, 64), (-1, 2097153), (5, 2097153)):
    t0 = time.time()
    res.append(("bits=%d prec=%d" % (bits, prec), one("poisson %d %d %s %s" % (bits, prec, G, F1)), round(time.time() - t0, 2)))
for are in (Fr(1, 2 ** 10), Fr(1, 2 ** 20), Fr(1, 2 ** 30), Fr(1, 10 ** 8)):
    T = enc_rfun([term([(1, 0)], (are, 0))])
    t0 = time.time()
    o = one("poisson 20 64 %s %s" % (T, F1))
    res.append(("tiny Re(A)=%s" % are, o[:50], round(time.time() - t0, 2)))
for r in res:
    print(r)
