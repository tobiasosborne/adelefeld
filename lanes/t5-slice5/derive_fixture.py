"""Expected lines of tests/driver/tate-fe.out and tests/julia/tate_fe.jl, derived before the run.

timeout 300 python3 -B lanes/t5-slice5/derive_fixture.py

Midpoints: the 60-digit mpmath values of tests/ref/vectors/t5-slice5/fe.jsonl (field L, Lambda(s, chi)); both
sides print from the same number, A = Lambda(s, chi) and R = W Lambda(1 - s, conj chi) = Lambda(s, chi) (analysis
P13). Printing: conventions 9.5 on exact rationals (the helpers of lanes/t5-slice4/derive_fixture.py, copied):
the decimal exponent q of the last printed digit is limited by n digits and by the radius, M = mid rounded half even
at 10^q, R = ceil2(rad + |M - mid|). Radii: where a coordinate is not the exact 0, the radius of the ball (below
1e-24 at bits >= 80) does not show at digits 10: the script asserts that moving the midpoint or adding a radius of
up to 1e-24 does not change the text. Where a coordinate is the exact 0 (zeta and chi_4 at real points, W = 1
exactly for C = 1 and for chi_4: tau = 2i from exact phases, divided by sqrt(4) = 2 and by i), the printed radius
is the bound added to both radii (api-5b.md S6): the oracle's tail + quadrature of proto/tate_checks.py
continuation (the same cutoffs and degrees as src/tate.c, api-5b.md S4, S9), A at bits, B at bits + 4.
"""
import json
import os
import sys
from fractions import Fraction as F
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, 'proto'))
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


def stable(mid, n):
    """The text of a nonzero coordinate, asserted not to depend on a shift or a radius of up to 1e-24."""
    t = F(10)**-24
    texts = {pr(mid + d, r, n) for d in (-t, 0, t) for r in (0, t)}
    assert len(texts) == 1, (float(mid), texts)
    return texts.pop()


def mid_of(text):
    return F(mp.nstr(mp.mpf(text.split(' +/- ')[0]), 55, min_fixed=-10**9, max_fixed=10**9))


VEC = {}
with open(os.path.join(ROOT, 'tests', 'ref', 'vectors', 't5-slice5', 'fe.jsonl')) as fh:
    for line in fh:
        r = json.loads(line)
        if r['kind'] == 'value':
            VEC[(r['q'], r['n'], F(r['s'][0]), F(r['s'][1]))] = r


def bound(q, n, s, bits):
    chi = T.character(q, n)
    r = T.continuation(chi, mp.mpc(T.mpq(s[0]), T.mpq(s[1])), bits)
    return F(T.endpoints(r.tail + r.quadrature)[1]), r.N, r.R


def side_text(re, im, n, rad):
    a = stable(re, n) if re != 0 else pr(0, rad, n)
    b = stable(im, n) if im != 0 else pr(0, rad, n)
    return f'({a}) + ({b})*i'


def fe_line(q, n, s, bits, digits, stext, real_axis=False):
    rec = VEC[(q, n, F(s[0]), F(s[1]))]
    re, im = mid_of(rec['L'][0]), mid_of(rec['L'][1])
    ea = eb = 0
    if real_axis:                       # the imaginary coordinate is the exact 0 on both sides (W = 1 exactly)
        im = F(0)
        ea, na, ra = bound(q, n, s, bits)
        u = (1 - F(s[0]), -F(s[1]))
        cq, cn = (q, n)                   # real characters: conj(chi) = chi
        eb, nb, rb = bound(cq, cn, u, bits + 4)
        print(f'  bounds: A N={na} R={ra} E={float(ea):.5e}; B N={nb} R={rb} E={float(eb):.5e}')
    A = side_text(re, im, digits, ea)
    Rt = side_text(re, im, digits, eb)
    return f'{A} | {Rt} | overlap=yes | s={stext}'


def main():
    out = []
    out.append(fe_line(1, 1, (F(1, 2), 0), 80, 10, '(0.5) + (0)*i', real_axis=True))
    out.append(fe_line(4, 3, (F(3, 8), F(3, 4)), 80, 10, '(0.375) + (0.75)*i'))
    out.append(fe_line(8, 7, (F(3, 8), F(3, 4)), 80, 10, '(0.375) + (0.75)*i'))
    out.append(fe_line(5, 2, (F(3, 8), F(3, 4)), 80, 10, '(0.375) + (0.75)*i'))
    out.append(fe_line(7, 3, (F(2), F(3)), 80, 10, '(2) + (3)*i'))
    out.append(fe_line(15, 2, (F(1, 2), F(0)), 80, 10, '(0.5) + (0)*i'))
    out.append(fe_line(4, 3, (F(-1), F(0)), 80, 10, '(-1) + (0)*i', real_axis=True))
    for line in out:
        print(line)
    # tests/julia/tate_fe.jl, digits 5: (5,2) at 3/8 + 3i/4, chi_4 at -1 (A side text only, the values)
    for q, n, s in ((5, 2, (F(3, 8), F(3, 4))), (4, 3, (F(-1), F(0)))):
        rec = VEC[(q, n, F(s[0]), F(s[1]))]
        re, im = mid_of(rec['L'][0]), mid_of(rec['L'][1])
        print(f'julia ({q},{n}) s={s}: re {stable(re, 5)}; im {stable(im, 5) if im != 0 else "0"}; '
              f'L = {rec["L"][0][:25]} {rec["L"][1][:25]}')


main()
