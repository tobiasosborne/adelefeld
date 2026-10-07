"""m4-review1: own models of real test functions sum P(x) exp(-pi A x^2 + B x + C) (api-4.md:18-21).
Exact rational closure algebra where no pi enters; mpmath at 80 digits otherwise. The Fourier transform with
the kernel exp(+2 pi i x y) (derived from conventions 6.1:840-847 and tate-poonen notes.txt:693-700,733-740:
conj(psi_inf(xy)) = conj(E(-xy)) = E(+xy)) in closed form by completing the square:
  int (t + mu)^j exp(-a t^2) dt = sum_k binom(j, 2k) mu^(j-2k) Gamma(k+1/2) a^(-k-1/2), a = pi A, mu = b/(2a),
  int x^j exp(-a x^2 + b x) dx = exp(b^2/(4a)) * that,  b = B + 2 pi i y,
with a^(1/2) the principal root (Re > 0 for Re a > 0: the integral is analytic in a on Re a > 0 and positive
for real a). This is not the H_j recurrence of api-4.md R1."""
import random
from fractions import Fraction as Fr
from math import comb
import mpmath
from mpmath import mpf, mpc
from m import *

mpmath.mp.dps = 80


def F2m(q):
    return mpf(q.numerator) / q.denominator


def C2m(z):
    return mpc(F2m(z[0]), F2m(z[1]))


def cmul(a, b):
    return (a[0] * b[0] - a[1] * b[1], a[0] * b[1] + a[1] * b[0])


def cadd(a, b):
    return (a[0] + b[0], a[1] + b[1])


def polymul(P, Q):
    if not P or not Q:
        return []
    out = [(Fr(0), Fr(0))] * (len(P) + len(Q) - 1)
    for i, a in enumerate(P):
        for j, b in enumerate(Q):
            out[i + j] = cadd(out[i + j], cmul(a, b))
    return norm(out)


def norm(P):
    P = list(P)
    while P and P[-1] == (0, 0):
        P.pop()
    return P


def shift(P, q):
    """P(x - q) exactly."""
    out = [(Fr(0), Fr(0))] * len(P)
    for j, c in enumerate(P):
        for k in range(j + 1):
            w = comb(j, k) * (-q) ** (j - k)
            out[k] = cadd(out[k], (c[0] * w, c[1] * w))
    return norm(out)


def ev_term(P, A, B, C, x):
    """exact params (complex Fractions), x mpmath number."""
    v = mpc(0)
    for c in reversed(P):
        v = v * x + C2m(c)
    return v * mpmath.exp(-mpmath.pi * C2m(A) * x * x + C2m(B) * x + C2m(C))


def ev(terms, x):
    return sum((ev_term(P, A, B, C, x) for (P, A, B, C) in terms if P), mpc(0))


def ft_term(P, A, B, C, y):
    a = mpmath.pi * C2m(A)
    b = C2m(B) + 2j * mpmath.pi * y
    mu = b / (2 * a)
    sa = mpmath.sqrt(a)
    tot = mpc(0)
    for j, c in enumerate(P):
        s = mpc(0)
        for k in range(j // 2 + 1):
            s += comb(j, 2 * k) * mu ** (j - 2 * k) * mpmath.gamma(k + mpf(1) / 2) / (a ** k * sa)
        tot += C2m(c) * s
    return tot * mpmath.exp(b * b / (4 * a) + C2m(C))


def ft(terms, y):
    return sum((ft_term(P, A, B, C, y) for (P, A, B, C) in terms if P), mpc(0))


def has_num(ball, v, tol):
    """ball = (mid, rad) Fractions, v mpf. 1 inside, 0 certainly outside (beyond tol), 2 undecided."""
    m, r = ball
    if m is None:
        return 0
    d = abs(v - F2m(m))
    R = F2m(r)
    if d + tol <= R:
        return 1
    if d - tol > R:
        return 0
    return 2


def has_c(ball, z, tol):
    a = has_num(ball[0], mpmath.re(z), tol)
    b = has_num(ball[1], mpmath.im(z), tol)
    if a == 0 or b == 0:
        return 0
    return 1 if a == b == 1 else 2


def has_exact(ball, z):
    return acb_has(ball, z)


def quad_line(f, W, c=0, pieces=None):
    pts = mpmath.linspace(c - W, c + W, pieces or int(4 * W) + 2)
    return mpmath.quad(f, pts)
