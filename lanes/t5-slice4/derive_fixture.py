"""Expected lines of tests/driver/tate-continue.out and tests/julia/tate_continue.jl, derived before the run:
conventions 9.5 printing on exact rationals (the helpers of lanes/t5-slice2/derive_fixture.py, copied), mpmath at
60 digits for the midpoints (Lambda by the functional equation of analysis P13 where Gamma(z) has a pole), the
oracle's tail and quadrature bound (proto/tate_checks.py continuation, the same cutoffs N, R and degrees J as
src/tate.c) for the imaginary radii. timeout 120 python3 -B lanes/t5-slice4/derive_fixture.py"""
import os
import sys
from fractions import Fraction as F
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'proto'))
sys.dont_write_bytecode = True
import mpmath as mp  # noqa: E402
import tate_checks as T  # noqa: E402
mp.mp.dps = 60


def X(y):
    y = abs(F(y)); e = 0
    while y >= 10: y /= 10; e += 1
    while y < 1: y *= 10; e -= 1
    return e


def ceil2(E):
    E = F(E); s = F(10)**(X(E)-1); return -((-E)//s)*s


def rnd(y, q):
    s = F(10)**q; t = y/s; f = t.numerator//t.denominator; r = t - f
    if r > F(1, 2) or (r == F(1, 2) and f % 2): f += 1
    return f*s


def fmt(y):
    if y == 0: return '0'
    sign = '-' if y < 0 else ''; y = abs(y); E = 0
    while y.denominator != 1: y *= 10; E -= 1
    D = y.numerator
    while D % 10 == 0: D //= 10; E += 1
    k = len(str(D)); Xe = E + k - 1
    if -4 <= Xe <= 20:
        s = str(D)
        if E >= 0: return sign + s + '0'*E
        if -E >= k: return sign + '0.' + '0'*(-E-k) + s
        return sign + s[:k+E] + '.' + s[k+E:]
    s = str(D); return sign + (s[0] + ('.' + s[1:] if k > 1 else '')) + 'e' + str(Xe)


def pr(mid, rad, n):
    mid, rad = F(mid), F(rad)
    if rad == 0 and mid == 0: return '0'
    if mid == 0: return '0 +/- ' + fmt(ceil2(rad))
    q = X(mid) - n + 1
    if rad > 0: q = max(q, X(rad) - 1)
    while True:
        M = rnd(mid, q); R = ceil2(rad + abs(M - mid))
        qq = X(R) - 1 if M == 0 else max(X(M) - n + 1, X(R) - 1)
        if M % F(10)**qq != 0: q = qq; continue
        return fmt(M) + ' +/- ' + fmt(R)


def mpf(x): return F(mp.nstr(x, 55, min_fixed=-10**9, max_fixed=10**9))


def lam(chi, s):
    """Lambda at a real point s; at Gamma's poles s = -e - 2k by the functional equation (W = 1 here)."""
    s = F(s)
    z = (s + chi.e)/2
    if z <= 0 and z.denominator == 1:
        return mp.re(T.root_number(chi)*lam(chi.conj(), 1 - s))
    z, s = T.mpq(z), T.mpq(s)
    return mp.re((chi.C/mp.pi)**z*mp.gamma(z)*T.l_reference(chi, s))


def line(label, q, n, s, bits, completed, digits):
    chi = T.character(q, n)
    v = lam(chi, s)
    r = T.continuation(chi, F(s), bits)
    err = F(T.endpoints(r.tail + r.quadrature)[1])
    z = (F(s) + chi.e)/2
    if not completed:
        v = v*mp.mpf(chi.C)**(-T.mpq(z))
        if chi.C > 1 and z != 0:
            err = err*F(chi.C)**(-z) if z.denominator == 1 else err   # |C^-z| for integral z, exact
    print(f'{label}: q={q} s={s} N={r.N} R={r.R} value={mp.nstr(v, 25)} err={float(err):.6e}')
    print(f'  ({pr(mpf(v), err, digits)}) + ({pr(0, err, digits)})*i')


line('completed zeta 1/2', 1, 1, F(1, 2), 80, True, 10)
line('continue zeta 1/2', 1, 1, F(1, 2), 80, False, 10)
line('completed zeta -2 (trivial zero)', 1, 1, -2, 80, True, 10)
line('continue chi4 -1 (trivial zero)', 4, 3, -1, 80, False, 10)
line('completed chi4 1', 4, 3, 1, 80, True, 10)
line('continue chi4 1', 4, 3, 1, 80, False, 10)
line('completed chi5 0', 5, 4, 0, 80, True, 10)
print('Lambda_zeta(1/2) =', mp.nstr(lam(T.character(1), F(1, 2)), 30), ' 4G/pi =', mp.nstr(4*mp.catalan/mp.pi, 25))
# tests/julia/tate_continue.jl: digits 5, bits 80, prec 160
line('julia completed zeta 1/2', 1, 1, F(1, 2), 80, True, 5)
line('julia continue chi4 1', 4, 3, 1, 80, False, 5)
line('julia completed chi4 -1', 4, 3, -1, 80, True, 5)
