"""Expected lines of tests/driver/tate.out derived before the run: conventions 9.5 printing (exact rationals),
mpmath at 60 digits for the midpoints, the oracle's tail and quadrature bounds (proto/tate_checks.py continuation,
the same cutoffs N, R and degrees J as src/tate.c) for the radii. timeout 120 python3 -B this-file"""
import os, sys
from fractions import Fraction as F
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'proto'))
import mpmath as mp
import tate_checks as T
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

for (q, n), bits in (((1, 1), 53), ((4, 3), 53)):
    chi = T.character(q, n)
    v = mp.re(mp.pi**(-(2+chi.e)/mp.mpf(2))*mp.gamma((2+chi.e)/mp.mpf(2))*T.l_reference(chi, mp.mpf(2)))
    r = T.continuation(chi, F(2), bits)
    err = F(T.endpoints((r.tail + r.quadrature))[1])
    err = err/8 if q == 4 else err
    print(f'q={q} N={r.N} R={r.R} I={mp.nstr(v, 25)} err={float(err):.6e}')
    print(f'({pr(mpf(v), err, 10)}) + ({pr(0, err, 10)})*i')
print('pi/6 =', mp.nstr(mp.pi/6, 25), ' G/(2 pi) =', mp.nstr(mp.catalan/(2*mp.pi), 25))
# tests/julia/tate.jl: digits 5, bits 64, prec 128
for (q, n) in ((1, 1), (4, 3)):
    chi = T.character(q, n); r = T.continuation(chi, F(2), 64)
    err = F(T.endpoints(r.tail + r.quadrature)[1]); err = err/8 if q == 4 else err
    v = mp.re(mp.pi**(-(2+chi.e)/mp.mpf(2))*mp.gamma((2+chi.e)/mp.mpf(2))*T.l_reference(chi, mp.mpf(2)))
    print('julia', q, r.N, r.R, float(err), '((%s) + (%s)*i ; 0)' % (pr(mpf(v), err, 5), pr(0, err, 5)))
