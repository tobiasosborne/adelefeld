#!/usr/bin/env python3
"""Vectors of slice 5e (lane t5-slice5): timeout 300 python3 -B lanes/t5-slice5/gen_vectors.py

Writes tests/ref/vectors/t5-slice5/fe.jsonl, the references of the functional equation command (docs/api-5.md
section 5, analysis P13: Lambda(s, chi) = W_chi Lambda(1 - s, conj(chi)), docs/proofs/analysis.md:574). No
adelefeld code is imported; the oracle proto/tate_checks.py supplies the character descriptor (the lowering of a
raw Conrey pair, conj) and the mpmath Hurwitz L. One JSON object per line:

  value: q, n (the raw pair of tests/golden/gauss.tsv), C, label, e (the primitive character it lowers to);
         s = [re, im] exact rationals "p/q";
         L, Lf: Lambda(s, chi) = (C/pi)^z Gamma(z) L(s, chi), z = (s + e)/2 (conventions 6.4), from mpmath at 60
                digits ("mid +/- rad", rad = 1e-50 max(1, |value|)) and from python-flint at 256 bits (arb text);
         B, Bf: Lambda(1 - s, conj(chi)), the same two sources;
         zero: 1 if s is a trivial zero of L(., chi), 2 if 1 - s is one of L(., conj chi), else 0.
  ball:  q, n, C, label, e; s = the centre, rad = [2^-60, 2^-60] (exact); corners: the four corners
         s +/- 2^-60 +/- 2^-60 i with L and Lf at each.

The FLINT value is FLINT's L completed by hand. FLINT's normalisation is quoted from
refs/src/flint-3.0.1/acb_dirichlet.rst: ":: void acb_dirichlet_zeta ... Computes \\zeta(s)" (:98-100);
"L(s,\\chi) = q^{-s}\\sum_{k=1}^q \\chi(k) \\,\\zeta\\!\\left(s,\\frac kq\\right)" (:524-534);
":: void acb_dirichlet_l ... Computes L(s,\\chi) using a default choice of algorithm" (:567-569); python-flint's
dirichlet_char(C, label).l_function and acb_series.dirichlet_l call these. The Conrey label is the one the oracle's
lowering returns [source pending: universal FLINT Conrey phase/label identification, inherited from api-3c].

At a trivial zero s0 = -e - 2k (z0 = -k; s0 != 0 for C = 1) the product Gamma(z) L(s) is infinity times 0. Neither
source uses the functional equation there (that would make the test circular): Lambda = (C/pi)^z L(s)/rgamma(z),
and both L(s) and rgamma(z(s)) have a simple zero at s0, so the quotient is L'(s0)/(d/ds rgamma(z(s)))(s0)
= L'(s0)/(rgamma'(z0)/2). FLINT: the coefficients of t of acb_series.dirichlet_l(s0 + t) and of
acb_series.rgamma(z0 + t/2). mpmath: L'(s0) from the derivative of the Hurwitz formula above (mp.zeta(s, a, 1))
and rgamma'(z0) by mp.diff. The two sources are asserted to agree to 1e-45 max(1, |Lambda|), and the functional
equation is asserted numerically with the oracle's W (a check of the vectors, not a premise of the test).
"""
from fractions import Fraction as F
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, 'proto'))
sys.dont_write_bytecode = True

import mpmath as mp  # noqa: E402
import tate_checks as T  # noqa: E402
from flint import acb, arb, acb_series, ctx, dirichlet_char  # noqa: E402

mp.mp.dps = 60
OUT = os.path.join(ROOT, 'tests', 'ref', 'vectors', 't5-slice5', 'fe.jsonl')
POINTS = [(F(3, 8), F(3, 4)), (F(1, 2), F(0)), (F(1, 2), F(14)), (F(-1), F(0)), (F(2), F(3)), (F(1, 4), F(1))]
H60 = F(1, 2**60)


def golden_pairs():
    pairs = []
    with open(os.path.join(ROOT, 'tests', 'golden', 'gauss.tsv')) as fh:
        for line in fh:
            if line.startswith('char('):
                head = line.split(')')[0]
                pairs.append((int(head.split('q=')[1].split(',')[0]), int(head.split('n=')[1].split(',')[0])))
    assert len(pairs) == 17, pairs
    return pairs


def qstr(x):
    x = F(x)
    return f'{x.numerator}/{x.denominator}'


def mpball(v):
    v = mp.mpc(v)
    rad = mp.mpf('1e-50')*max(1, abs(v))
    return [mp.nstr(v.real, 55, strip_zeros=False) + ' +/- ' + mp.nstr(rad, 3),
            mp.nstr(v.imag, 55, strip_zeros=False) + ' +/- ' + mp.nstr(rad, 3)]


def arbstr(z):
    return [z.real.str(55, radius=True), z.imag.str(55, radius=True)]


def trivial_zero(chi, s):
    """k with s = -e - 2k, k >= 0, excluding the pole s = 0 of C = 1; else None."""
    if s[1] != 0 or s[0].denominator != 1:
        return None
    k2 = -chi.e - s[0]
    if k2 < 0 or k2 % 2 or (chi.C == 1 and s[0] == 0):
        return None
    return int(k2 // 2)


def lprime(chi, s0):
    """L'(s0, chi) from the Hurwitz formula of acb_dirichlet.rst:524-534, differentiated in s."""
    if chi.C == 1:
        return mp.zeta(s0, 1, 1)
    C = chi.C
    S = sum(chi(a)*mp.zeta(s0, mp.mpf(a)/C) for a in range(1, C+1))
    D = sum(chi(a)*mp.zeta(s0, mp.mpf(a)/C, 1) for a in range(1, C+1))
    return mp.mpf(C)**(-s0)*(D - mp.log(C)*S)


def lambda_mp(chi, s):
    t = mp.mpc(T.mpq(s[0]), T.mpq(s[1]))
    z = (t + chi.e)/2
    if trivial_zero(chi, s) is None:
        return (chi.C/mp.pi)**z*mp.gamma(z)*T.l_reference(chi, t)
    return (chi.C/mp.pi)**z*lprime(chi, t)/(mp.diff(mp.rgamma, z)/2)


def lambda_flint(chi, s):
    with ctx.workprec(256):
        sb = acb(arb(s[0].numerator)/s[0].denominator, arb(s[1].numerator)/s[1].denominator)
        z = (sb + chi.e)/2
        G = dirichlet_char(chi.C, chi.label if chi.C > 1 else 1)
        fac = (arb(chi.C)/arb.pi())**z
        if trivial_zero(chi, s) is None:
            return fac*z.gamma()*G.l_function(sb)
        Ls = acb_series.dirichlet_l(acb_series([sb, acb(1)], 3), G)
        Rg = acb_series([z, acb(1)/2], 3).rgamma()
        return fac*Ls.coeffs()[1]/Rg.coeffs()[1]


def check_pair(chi, s, mpv, flv, what):
    wm = T.numeric(flv)
    assert abs(mpv - wm) <= T.MARGIN*max(1, abs(mpv)), (what, chi.C, chi.label, s, mpv, wm)
    assert T.width(flv) < F(1, 10**40), (what, chi.C, s)


def value_record(q, n, chi, s):
    cc = chi.conj()
    u = (1 - s[0], -s[1])
    L, Lf = lambda_mp(chi, s), lambda_flint(chi, s)
    B, Bf = lambda_mp(cc, u), lambda_flint(cc, u)
    check_pair(chi, s, L, Lf, 'L')
    check_pair(cc, u, B, Bf, 'B')
    W = T.root_number(chi)
    assert abs(L - W*B) <= T.MARGIN*max(1, abs(L)), ('fe', q, n, s)
    zero = 1 if trivial_zero(chi, s) is not None else 2 if trivial_zero(cc, u) is not None else 0
    return {'kind': 'value', 'q': q, 'n': n, 'C': chi.C, 'label': chi.label, 'e': chi.e,
            's': [qstr(s[0]), qstr(s[1])], 'L': mpball(L), 'Lf': arbstr(Lf), 'B': mpball(B), 'Bf': arbstr(Bf),
            'zero': zero}


def ball_record(q, n, chi, c):
    corners = []
    for dx in (-H60, H60):
        for dy in (-H60, H60):
            p = (c[0] + dx, c[1] + dy)
            L, Lf = lambda_mp(chi, p), lambda_flint(chi, p)
            check_pair(chi, p, L, Lf, 'corner')
            corners.append({'s': [qstr(p[0]), qstr(p[1])], 'L': mpball(L), 'Lf': arbstr(Lf)})
    return {'kind': 'ball', 'q': q, 'n': n, 'C': chi.C, 'label': chi.label, 'e': chi.e,
            's': [qstr(c[0]), qstr(c[1])], 'rad': [qstr(H60), qstr(H60)], 'corners': corners}


def main():
    recs = []
    for q, n in golden_pairs():
        chi = T.character(q, n)
        pts = POINTS + [(F(-chi.e - 2), F(0)), (F(3 + chi.e), F(0))]
        for s in pts:
            recs.append(value_record(q, n, chi, s))
        recs.append(ball_record(q, n, chi, POINTS[0]))
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, 'w') as fh:
        for r in recs:
            fh.write(json.dumps(r, separators=(',', ':')) + '\n')
    nv = sum(r['kind'] == 'value' for r in recs)
    nz = sum(r['kind'] == 'value' and r['zero'] > 0 for r in recs)
    print(f'{OUT}: {len(recs)} records ({nv} value, {nz} at a trivial zero of one side, {len(recs) - nv} ball), '
          f'{os.path.getsize(OUT)} bytes')


if __name__ == '__main__':
    main()
