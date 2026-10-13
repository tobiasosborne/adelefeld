#!/usr/bin/env python3
"""Slice 5b fixtures: local gamma and epsilon factors, oracle values and point enclosures.

Run: timeout 180 python3 -B lanes/t5-slice3/generate_vectors.py
Definitions: docs/api-5.md:57-118 (the table of section 2), docs/proofs/analysis.md P9 (331-400),
P10 (401-451); proto/tate_checks.py local_gamma, local_epsilon, real_gamma, gauss(chi, negative=True),
exact_local (the cyclotomic expression of the ramified constant).

Every OK row carries, for the centre and every sampled corner of the input rectangle, a decimal interval
with a 60-digit midpoint from the oracle (mpmath) that is verified to contain an independent rigorous
point enclosure:
  finite unramified: (1 - alpha T)/(1 - p^(s-1)/alpha), T = exp(-s log p), python-flint balls at 512 bits;
  ramified: alpha^a exp(-a s log p) G, G the sum of exact phase balls E(-theta(u) - u/p^a);
  real: i^e L(1-s+e)/L(s+e), L(x) = pi^(-x/2) Gamma(x/2) by zeta_checks.point_enclosure (Taylor
  integration of the Gamma integral with tails, no FLINT Gamma); the exact value 0 at the zeros -e-2k.
The width test of the C program: 2*radius <= 64*width_lower + 2^(-prec+8)*image, where width_lower is
a certified separation of the centre and corner images and image an upper bound of |value| on the
rectangle (a python-flint ball evaluation of the same formula; a tolerance only, not a reference).
Status codes: 0 OK, 1 NOT_DETERMINED, 7 DOMAIN, 10 LIMIT.
"""
import sys
sys.dont_write_bytecode = True
from pathlib import Path
from fractions import Fraction as F
from itertools import product
from functools import lru_cache
import json
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'proto'))
import tate_checks as t
import zeta_checks as z
from flint import arb, acb, ctx
mp = t.mp
mp.mp.dps = 90
ctx.prec = 512
rows = []
cache = {}
gcache = {}


def m(q):
    return mp.mpf(q.numerator) / q.denominator


def dec(q):
    return mp.nstr(q, 60)


def gsum(q, n):
    """Ball and exact text of G_minus(eta0^-1) = sum eta0(u)^-1 E(-u/q), from exact phases."""
    if (q, n) not in gcache:
        chi = t.character(q, n)
        assert chi.C == q
        box = acb(0)
        for u, theta in enumerate(chi.phases):
            if theta is not None:
                box += t.phase_ball((-theta - F(u, q)) % 1)
        order, poly = t.exact_gauss(chi.conj(), negative=True)
        gcache[(q, n)] = (chi, box, f'G=({poly}) at X=E(1/{order})')
    return gcache[(q, n)]


def exponent(p, q):
    a = 0
    while q % p == 0:
        q //= p
        a += 1
    assert q == 1
    return a


def formula_box(fn, p, q, n, e, s, a):
    """Rigorous box at a point or a rectangle; s, a are acb balls. Returns acb."""
    if not p:
        if fn == 'epsilon':
            return acb(0, 1) if e else acb(1)
        w = (1 - s + e) / 2
        zz = (s + e) / 2
        return (acb(0, 1) ** e) * (arb.pi() ** (s - acb(1) / 2)) * w.gamma() * zz.rgamma()
    if q == 1:
        if fn == 'epsilon':
            return acb(1)
        logp = arb(p).log()
        T = (-s * logp).exp()
        P = ((s - 1) * logp).exp()
        return (1 - a * T) / (1 - P / a)
    _, G, _ = gsum(q, n)
    k = exponent(p, q)
    return a ** k * (-k * s * arb(p).log()).exp() * G


@lru_cache(None)
def g_axis(q, n):
    """Whether G is exactly real, exactly imaginary (sympy polynomials modulo the cyclotomic polynomial)."""
    chi = gsum(q, n)[0]
    order, poly = t.exact_gauss(chi.conj(), negative=True)
    x = t.sy.Symbol('X')
    phi = t.sy.Poly(t.sy.cyclotomic_poly(order, x), x)
    g = t.sy.Poly(poly, x)
    conj = t.sy.Poly(t.sy.expand(poly.subs(x, x ** (order - 1))), x)
    return (g - conj).rem(phi).is_zero, (g + conj).rem(phi).is_zero


def exact_zero_part(fn, p, q, n, s, alpha):
    """0 or 1 if that coordinate of the ramified value is exactly zero, else None. Exact argument: for real s,
    T^a > 0; for alpha on an axis, alpha^a is real or imaginary; G is decided real or imaginary exactly in
    Q(zeta_m): complex conjugation maps X to X^(m-1) modulo Phi_m."""
    if not p or q == 1 or s[1] != 0 or alpha == ALPHAS[3] or (alpha[0] != 0 and alpha[1] != 0):
        return None
    g_real, g_imag = g_axis(q, n)
    if not (g_real or g_imag):
        return None
    k = exponent(p, q)
    a_real = alpha[1] == 0 or k % 2 == 0
    return 1 if a_real == g_real else 0


def point(fn, p, q, n, e, s, alpha):
    """(re text, im text, (re box, im box)) at an exact rational point."""
    key = (fn, p, q, n, e, s, alpha)
    if key in cache:
        return cache[key]
    sc = mp.mpc(m(s[0]), m(s[1]))
    ac = mp.mpc(m(alpha[0]), m(alpha[1]))
    if not p:
        if fn == 'epsilon':
            value, box = mp.j ** e, ((arb(0), arb(1)) if e else (arb(1), arb(0)))
        elif s[1] == 0 and s[0].denominator == 1 and s[0] <= -e and (s[0] + e) % 2 == 0:
            value, box = mp.mpc(0), (arb(0), arb(0))   # the zeros -e-2k: exactly 0 (P10, rgamma)
        else:
            value = t.real_gamma(e, sc)
            num = z.point_enclosure('real', 1 - s[0] + e, -s[1])
            den = z.point_enclosure('real', s[0] + e, s[1])
            box = z.cdiv(num, den)
            if e:
                box = (-box[1], box[0])
    elif q == 1 and fn == 'epsilon':
        value, box = mp.mpc(1), (arb(1), arb(0))
    else:
        chi = t.character(1) if q == 1 else gsum(q, n)[0]
        value = (t.local_gamma if fn == 'gamma' else t.local_epsilon)(p, ac, chi, sc)
        sb = acb(z.aa(s[0]), z.aa(s[1]))
        ab = acb(z.aa(alpha[0]), z.aa(alpha[1]))
        b = formula_box('gamma', p, q, n, e, sb, ab)
        box = (b.real, b.imag)
    zero_part = exact_zero_part(fn, p, q, n, s, alpha)
    texts = []
    for j, v in enumerate((value.real, value.imag)):
        if j == zero_part:
            assert abs(v) < mp.mpf(10) ** -55 and box[j].contains(0), ('exact zero part', key, j)
            texts.append('0')
            continue
        lo, hi = t.endpoints(box[j])
        if lo == hi and lo.denominator == 1:
            assert v == lo.numerator, ('exact reference', key, j)
            texts.append(str(lo.numerator))   # an exact integer (0, 1, -1): an exact C output contains it
        else:
            mid = dec(v)
            err = (abs(box[j] - arb(mid)) + arb(10) ** -59 * max(arb(1), abs(box[j]))).upper()
            text = mid + ' +/- ' + upward(t.endpoints(err)[1])
            assert arb(text).contains(box[j]), ('decimal enclosure', key, j)
            texts.append(text)
    cache[key] = (*texts, box)
    return cache[key]


def upward(q):
    """A short decimal d*10^x >= q > 0 with three significant digits, by exact rationals."""
    x = len(str(q.numerator // q.denominator)) - 1 if q >= 1 else -len(str(q.denominator // q.numerator))
    while F(10) ** x > q:
        x -= 1
    while F(10) ** (x + 1) <= q:
        x += 1
    unit = F(10) ** (x - 2)
    d = -((-q) // unit)
    assert d * unit >= q
    return f'{d}e{x - 2}'


def directed(q, upward=False):
    if not q:
        return F(0)
    step = F(2) ** (q.numerator.bit_length() - q.denominator.bit_length() - 72)
    units = q / step
    return (-((-units.numerator) // units.denominator) if upward else
            units.numerator // units.denominator) * step


def emit(fn, p, q, n, s, a, sr=(F(0), F(0)), ar=(F(0), F(0)), status=0, e=0, exact=''):
    if a == ALPHAS[3] and ar == (F(0), F(0)):
        ar = (F(0), F(1, 2 ** 240))  # E(1/3) is not dyadic: an actual ball around it.
    row = dict(fn=fn, p=p, q=q, n=n, s=[str(x) for x in s], alpha=[str(x) for x in a],
               sr=[str(x) for x in sr], ar=[str(x) for x in ar], status=status, e=e, exact=exact)
    if status == 0:
        points = list(product(*[(x - r, x + r) if r else (x,) for x, r in zip(s + a, sr + ar)]))
        points.append(s + a)
        samples, boxes = [], []
        for x in dict.fromkeys(points):
            re, im, box = point(fn, p, q, n, e, x[:2], x[2:])
            samples.append([re, im])
            boxes.append(box)
        lower = F(0)
        for box in boxes:
            for j in range(2):
                lo1, hi1 = t.endpoints(box[j])
                lo0, hi0 = t.endpoints(boxes[-1][j])
                lower = max(lower, lo1 - hi0, lo0 - hi1)
        image = max(F(1), *(max(abs(x) for x in t.endpoints(b[j])) for b in boxes for j in range(2)))
        sb = acb(arb(z.aa(s[0]), z.aa(sr[0])), arb(z.aa(s[1]), z.aa(sr[1])))
        ab = acb(arb(z.aa(a[0]), z.aa(ar[0])), arb(z.aa(a[1]), z.aa(ar[1])))
        whole = formula_box(fn, p, q, n, e, sb, ab)
        assert whole.is_finite(), ('image bound', row)
        image = max(image, *(max(abs(x) for x in t.endpoints(c)) for c in (whole.real, whole.imag)))
        row.update(samples=samples, width_lower=str(directed(lower)), image=str(directed(image, True)))
    rows.append(row)


with mp.workdps(180):
    EI = F(int(mp.nint(mp.sqrt(3) / 2 * 2 ** 256)), 2 ** 256)
ALPHAS = [(F(1), F(0)), (F(-1), F(0)), (F(0), F(1)), (F(-1, 2), EI), (F(2), F(0)), (F(1, 2), F(1))]
ZERO = (F(0), F(0))
SR = (F(1, 4096), F(1, 8192))


def AR(a):
    """One alpha radius per row, alternately real and imaginary, to keep the file below 400 KB."""
    return (F(1, 8192), F(0)) if ALPHAS.index(a) % 2 == 0 else (F(0), F(1, 16384))
RAMIFIED = {2: ((4, 3), (8, 3), (16, 5)), 3: ((3, 2), (9, 2)), 5: ((5, 2), (25, 2)),
            7: ((7, 3), (49, 3)), 13: ((13, 2), (169, 2))}


def unramified_exact(p, a, s):
    """The rational value of P9's a=0 gamma at integer s and rational alpha, as text."""
    if s[1] != 0 or s[0].denominator != 1 or a == ALPHAS[3]:
        return '(1-alpha*p^(-s))/(1-p^(s-1)/alpha)'
    k = int(s[0])
    ar, ai = a
    T = F(p) ** (-k)
    P = F(p) ** (k - 1)
    # (1 - A T)/(1 - P/A) = A (1 - A T)/(A - P), exact Gaussian rationals.
    nr, ni = 1 - ar * T, -ai * T
    nr, ni = ar * nr - ai * ni, ar * ni + ai * nr
    dr, di = ar - P, ai
    d2 = dr * dr + di * di
    return f'{(nr * dr + ni * di) / d2} + ({(ni * dr - nr * di) / d2})*i'


for p in (2, 3, 5, 7, 13):
    for a in ALPHAS:
        for s in ((F(2), F(0)), (F(1, 2), F(3, 4)), (F(-1), F(1, 2))):
            if a == (F(p) ** (s[0] - 1), F(0)) and s[1] == 0:
                emit('gamma', p, 1, 1, s, a, status=7)   # p = 2, alpha = 2, s = 2: an exact gamma pole
            else:
                emit('gamma', p, 1, 1, s, a, exact=unramified_exact(p, a, s))
        emit('gamma', p, 1, 1, (F(2), F(1, 2)), a, SR, AR(a))
    for a in ALPHAS[:3]:
        emit('epsilon', p, 1, 1, (F(1, 2), F(3, 4)), a, exact='1')
        emit('epsilon', p, 1, 1, (F(2), F(1, 2)), a, SR, AR(a), exact='1')
    for k in (-1, 0, 1, 2):
        # Gamma's own poles: alpha^-1 p^(s-1) = 1 at s = k, alpha = p^(k-1).
        aa, ss = (F(p) ** (k - 1), F(0)), (F(k), F(0))
        if p != 2 and k - 1 < 0:
            with mp.workdps(180):
                aa = (F(int(mp.nint(m(aa[0]) * 2 ** 256)), 2 ** 256), F(0))
            emit('gamma', p, 1, 1, ss, aa, ar=(F(1, 2 ** 240), F(0)), status=1)
            emit('epsilon', p, 1, 1, ss, aa, ar=(F(1, 2 ** 240), F(0)), exact='1')
        else:
            emit('gamma', p, 1, 1, ss, aa, status=7)
            emit('epsilon', p, 1, 1, ss, aa, exact='1')
        emit('gamma', p, 1, 1, ss, aa, (F(1, 4096), F(1, 4096)), status=1)
        emit('epsilon', p, 1, 1, ss, aa, (F(1, 4096), F(1, 4096)), exact='1')
        emit('gamma', p, 1, 1, (F(k) + F(1, 1024), F(0)), aa)
        # The integral's poles alpha p^-s = 1 at s = k, alpha = p^k: gamma is 0 there, a value.
        aa = (F(p) ** k, F(0))
        if p != 2 and k < 0:
            with mp.workdps(180):
                aa = (F(int(mp.nint(m(aa[0]) * 2 ** 256)), 2 ** 256), F(0))
            emit('gamma', p, 1, 1, ss, aa, ar=(F(1, 2 ** 240), F(0)))
        else:
            emit('gamma', p, 1, 1, ss, aa, exact='0')
        emit('gamma', p, 1, 1, ss, aa, (F(1, 4096), F(1, 4096)))
    for a in ALPHAS[:4]:
        with mp.workdps(180):
            pole = 1 + mp.log(mp.mpc(m(a[0]), m(a[1]))) / mp.log(p)
            for k in (-1, 1):
                si = pole.imag + 2 * mp.pi * k / mp.log(p)
                ss = (F(int(mp.nint(pole.real * 2 ** 256)), 2 ** 256), F(int(mp.nint(si * 2 ** 256)), 2 ** 256))
                emit('gamma', p, 1, 1, ss, a, (F(1, 2 ** 100), F(1, 2 ** 100)), status=1)
                emit('epsilon', p, 1, 1, ss, a, (F(1, 2 ** 100), F(1, 2 ** 100)), exact='1')
    for q, n in RAMIFIED[p]:
        text = gsum(q, n)[2]
        k = exponent(p, q)
        for a in ALPHAS:
            for s in ((F(2), F(0)), (F(1, 2), F(3, 4))):
                emit('gamma', p, q, n, s, a, exact=f'A^{k}*T^{k}*G, {text}')
            if a in ALPHAS[:3]:
                emit('gamma', p, q, n, (F(-2), F(3, 4)), a, SR, AR(a))
        emit('epsilon', p, q, n, (F(1, 2), F(3, 4)), ALPHAS[2], exact=f'A^{k}*T^{k}*G, {text}')
        emit('epsilon', p, q, n, (F(1), F(0)), ALPHAS[0], SR, AR(ALPHAS[0]))
for fn in ('gamma', 'epsilon'):
    emit(fn, 65537, 65537, 3, (F(2), F(0)), ALPHAS[0], status=10)
    emit(fn, 2, 3, 2, (F(2), F(0)), ALPHAS[0], status=7)
    for q, n in ((1, 1), (4, 3)):
        emit(fn, 2, q, n, (F(2), F(0)), ZERO, status=7)
        emit(fn, 2, q, n, (F(2), F(0)), ZERO, ar=(F(1, 1024), F(1, 1024)), status=1)
for e, q, n in ((0, 1, 1), (1, 4, 3), (0, 5, 4), (1, 5, 2)):
    for s in ((F(1, 2), F(0)), (F(1, 2), F(3, 4)), (F(-3, 2), F(1, 2)), (F(5, 2), F(0)), (F(-1, 2), F(1))):
        emit('gamma', 0, q, n, s, ZERO, e=e, exact=('i' if e else '1') if s == (F(1, 2), F(0)) else '')
        emit('gamma', 0, q, n, s, ZERO, (F(1, 4096), F(1, 8192)), e=e)
    emit('epsilon', 0, q, n, (F(1, 2), F(3, 4)), ZERO, e=e, exact='i' if e else '1')
    for k in range(3):
        pole, zero = 1 + e + 2 * k, -e - 2 * k
        emit('gamma', 0, q, n, (F(pole), F(0)), ZERO, status=7, e=e)
        emit('gamma', 0, q, n, (F(pole), F(0)), ZERO, (F(1, 4096), F(1, 4096)), status=1, e=e)
        emit('gamma', 0, q, n, (F(pole) + F(1, 1024), F(0)), ZERO, e=e)
        emit('gamma', 0, q, n, (F(pole) - F(1, 1024), F(1, 1024)), ZERO, e=e)
        emit('epsilon', 0, q, n, (F(pole), F(0)), ZERO, e=e, exact='i' if e else '1')
        emit('epsilon', 0, q, n, (F(pole), F(0)), ZERO, (F(1, 4096), F(1, 4096)), e=e)
        emit('gamma', 0, q, n, (F(zero), F(0)), ZERO, e=e, exact='0')
        emit('gamma', 0, q, n, (F(zero), F(0)), ZERO, (F(1, 4096), F(1, 4096)), e=e)
        emit('gamma', 0, q, n, (F(zero) + F(1, 1024), F(0)), ZERO, e=e)
path = ROOT / 'tests/ref/vectors/t5-slice3/gamma.jsonl'
path.write_text(''.join(json.dumps(row, separators=(',', ':')) + '\n' for row in rows))
assert path.stat().st_size <= 400 * 1024
print(f'{len(rows)} vectors; {sum(len(r.get("samples", [])) for r in rows)} sample enclosures; '
      f'{path.stat().st_size} bytes')
