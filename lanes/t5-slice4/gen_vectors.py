#!/usr/bin/env python3
"""Vectors of slice 5d (lane t5-slice4): timeout 300 python3 -B lanes/t5-slice4/gen_vectors.py

Writes tests/ref/vectors/t5-slice4/continue.jsonl from the oracle proto/tate_checks.py (docs/api-5.md sections 4,
6, 7; analysis P11, P12, P13, L14, P15). No adelefeld code is imported. Records, one JSON object per line:

  value:    q, n (raw Conrey pair of tests/golden/gauss.tsv, or (1, 1)), C, label, e; s = [re, im] exact
            rationals; I = I_chi(s) and L = Lambda_chi(s) = C^((s+e)/2) I_chi(s) at 60 digits as [re, im] balls
            "mid +/- rad" (rad = 1e-50 max(1, |value|)); If, Lf = the same from python-flint at 256 bits (arb
            strings); zero = 1 at a trivial zero of L (s = -e - 2k, s != 0 for C = 1), where Lambda is finite and
            nonzero although Gamma(z) has a pole.
            Away from the trivial zeros: mpmath Lambda = (C/pi)^z Gamma(z) L(s, chi) with the oracle's Hurwitz
            l_reference, and FLINT's dirichlet_char(C, label).l_function (or acb zeta) completed by hand with the
            same factor (acb_dirichlet.rst:567-569; for zeta acb_dirichlet.rst:98-100). At a trivial zero
            s0 = -e - 2k (z0 = -k) the product Gamma(z) L(s) is infinity times 0 and is not evaluated: mpmath uses
            the limit 2 (-1)^k L'(s0)/k! (Gamma(z) has residue (-1)^k/k! at -k [source pending: residues of
            Gamma; a numerical reference only], z - z0 = (s - s0)/2, L(s0) = 0) with L' from mpmath's Hurwitz
            derivative; FLINT gives W_chi Lambda(1 - s0, conj(chi)) by the functional equation of analysis P13
            with FLINT's L at 1 - s0 and the oracle's root_ball. The two sources are asserted to agree to
            1e-45 max(1, |Lambda|).
  ball:     q, n, s, rad (exact dyadic radii): the corners (the two real end points when the imaginary radius is 0)
            with their Lambda and I references (mpmath, and FLINT for the pole-free rectangle near 1 of the
            oracle's poles group). Width floors (N-D23, api-5.md T4.3) and enclosure over the rectangle.
  pole:     q, n, s, rad: the oracle's pole_status for the exact dyadic ball (DOMAIN, NOT_DETERMINED, OK).
  residue:  which = "one" ((s - 1) Lambda(s) at s = 1 + 2^-k) or "zero" (s Lambda(s) at s = 2^-k), k = 10, 20, 40,
            for zeta: the product at 60 digits (SPEC 8: (s - 1) Lambda -> 1, s Lambda -> -1).
  cutoff:   the oracle's continuation(chi, s, bits) cutoffs N, R and Taylor degrees (Lambda, P15 searches) at
            s = 1/2, 3/8 + 3i/4, -3/2 for C = 1, 4, 5 and bits 20, 53, 80; bound = the upper bound of its omitted
            regions and quadrature remainders E_n + E_t + both remainders (added to both radii, api-5.md:218).
  side:     the two Mellin integrals of P13 step 3 separately, at 60 digits by mpmath's incomplete Gamma (the
            oracle's numeric_piece with R = infinity, N = 60; a numerical reference only, P15 [source pending]):
            fwd = integral_1^inf V_chi(t) t^(z-1) dt, dual = W_chi integral_1^inf V_conj(chi)(t) t^(z'-1) dt.
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
from flint import acb, arb, ctx  # noqa: E402
import flint  # noqa: E402

mp.mp.dps = 60
OUT = os.path.join(ROOT, 'tests', 'ref', 'vectors', 't5-slice4', 'continue.jsonl')
H40 = F(1, 2**40)


def golden_pairs():
    pairs = [(1, 1)]
    with open(os.path.join(ROOT, 'tests', 'golden', 'gauss.tsv')) as fh:
        for line in fh:
            if line.startswith('char('):
                head = line.split(')')[0]
                pairs.append((int(head.split('q=')[1].split(',')[0]), int(head.split('n=')[1].split(',')[0])))
    return pairs


def qstr(x):
    x = F(x)
    return f'{x.numerator}/{x.denominator}'


def mpball(v):
    v = mp.mpc(v)
    rad = mp.mpf('1e-50')*max(1, abs(v))
    return [mp.nstr(v.real, 62, strip_zeros=False) + ' +/- ' + mp.nstr(rad, 3),
            mp.nstr(v.imag, 62, strip_zeros=False) + ' +/- ' + mp.nstr(rad, 3)]


def arbstr(x):
    return x.str(70, radius=True)


def cpoint(s):
    return mp.mpc(T.mpq(s[0]), T.mpq(s[1]))


def trivial_zero(chi, s):
    """s = -e - 2k with k >= 0, excluding the pole s = 0 of C = 1."""
    if s[1] != 0 or s[0].denominator != 1:
        return None
    k2 = -chi.e - s[0]
    if k2 < 0 or k2 % 2:
        return None
    if chi.C == 1 and s[0] == 0:
        return None
    return int(k2 // 2)


def lprime(chi, s0):
    """L'(s0, chi) from L(s) = C^-s sum_a chi(a) zeta(s, a/C) (acb_dirichlet.rst:524-534)."""
    if chi.C == 1:
        return mp.zeta(s0, 1, 1)
    C = chi.C
    S = sum(chi(a)*mp.zeta(s0, mp.mpf(a)/C) for a in range(1, C+1))
    D = sum(chi(a)*mp.zeta(s0, mp.mpf(a)/C, 1) for a in range(1, C+1))
    return mp.mpf(C)**(-s0)*(D - mp.log(C)*S)


def lambda_mp(chi, s):
    """Lambda_chi(s) at 60 digits (see the module docstring)."""
    k = trivial_zero(chi, s)
    t = cpoint(s)
    z = (t + chi.e)/2
    if k is None:
        return (chi.C/mp.pi)**z*mp.gamma(z)*T.l_reference(chi, t)
    lim = 2*(-1)**k*lprime(chi, t)/mp.factorial(k)
    return (chi.C/mp.pi)**z*lim


def flint_l(chi, s):
    if chi.C == 1:
        return s.zeta()
    return flint.dirichlet_char(chi.C, chi.label).l_function(s)


def lambda_flint(chi, s):
    """FLINT's L completed by hand; at a trivial zero by the functional equation (analysis P13)."""
    with ctx.workprec(256):
        sb = acb(arb(s[0].numerator)/s[0].denominator, arb(s[1].numerator)/s[1].denominator)
        if trivial_zero(chi, s) is None:
            z = (sb + chi.e)/2
            return (arb(chi.C)/arb.pi())**z*z.gamma()*flint_l(chi, sb)
        cc = chi.conj()
        u = 1 - sb
        z = (u + cc.e)/2
        return T.root_ball(chi)*(arb(chi.C)/arb.pi())**z*z.gamma()*flint_l(cc, u)


def cfactor(chi, s, inverse):
    """C^(-z) (inverse) or C^z, z = (s + e)/2, at 60 digits."""
    z = (cpoint(s) + chi.e)/2
    return mp.mpf(chi.C)**(-z if inverse else z)


def value_record(q, n, chi, s):
    lam = lambda_mp(chi, s)
    fl = lambda_flint(chi, s)
    wm = T.numeric(fl)
    assert abs(lam - wm) <= T.MARGIN*max(1, abs(lam)), (q, n, s, lam, wm)
    assert T.width(fl) < F(1, 10**40), (q, n, s)
    with ctx.workprec(256):
        zb = (acb(arb(s[0].numerator)/s[0].denominator, arb(s[1].numerator)/s[1].denominator) + chi.e)/2
        ifl = fl*arb(chi.C)**(-zb)
    iv = lam*cfactor(chi, s, True)
    k = trivial_zero(chi, s)
    if k is not None:
        assert abs(lam) > mp.mpf('1e-10'), (q, n, s)        # finite and nonzero at a trivial zero
    return {'kind': 'value', 'q': q, 'n': n, 'C': chi.C, 'label': chi.label, 'e': chi.e,
            's': [qstr(s[0]), qstr(s[1])], 'zero': 0 if k is None else 1,
            'I': mpball(iv), 'L': mpball(lam),
            'If': [arbstr(ifl.real), arbstr(ifl.imag)], 'Lf': [arbstr(fl.real), arbstr(fl.imag)]}


def ball_record(q, n, chi, s, rad, with_flint=False):
    pts = []
    if rad[1] == 0:
        corners = [(s[0] - rad[0], s[1]), (s[0] + rad[0], s[1])]
    else:
        corners = [(s[0] + a*rad[0], s[1] + b*rad[1]) for a in (-1, 1) for b in (-1, 1)]
    for c in corners:
        lam = lambda_mp(chi, c)
        rec = {'s': [qstr(c[0]), qstr(c[1])], 'L': mpball(lam), 'I': mpball(lam*cfactor(chi, c, True))}
        if with_flint:
            fl = lambda_flint(chi, c)
            assert abs(lam - T.numeric(fl)) <= T.MARGIN*max(1, abs(lam))
            rec['Lf'] = [arbstr(fl.real), arbstr(fl.imag)]
        pts.append(rec)
    return {'kind': 'ball', 'q': q, 'n': n, 's': [qstr(s[0]), qstr(s[1])], 'rad': [qstr(rad[0]), qstr(rad[1])],
            'points': pts}


def exact_ball(s, rad):
    def one(m, r):
        b = arb(m.numerator)/m.denominator
        return b + arb(0, arb(r.numerator)/r.denominator) if r else b
    with ctx.workprec(4096):
        return acb(one(s[0], rad[0]), one(s[1], rad[1]))


def main():
    pairs = golden_pairs()
    assert len(pairs) == 18, pairs
    pts = [(F(1, 2), F(0)), (F(1, 2), F(14)), (F(3, 8), F(3, 4)), (F(-1), F(0)), (F(-2), F(0)), (F(-3), F(0)),
           (F(-7, 2), F(0)), (F(2), F(3)), (1 - H40, F(0)), (H40, F(0)), (F(1), F(1, 2**10))]
    records = []
    for q, n in pairs:
        chi = T.character(q, n)
        extra = [(F(-2), F(0)), (F(-4), F(0))] if chi.C == 1 else \
            [(F(0), F(0)), (F(-2), F(0)), (F(-4), F(0))] if chi.e == 0 else [(F(-1), F(0)), (F(-3), F(0))]
        mine = list(pts)
        for s in extra:
            if s not in mine:
                mine.append(s)
        for s in mine:
            records.append(value_record(q, n, chi, s))
    zeta, c52 = T.character(1), T.character(5, 2)
    # input radii: N-D23 floors at bits 80, enclosure at bits 40 (oracle's input_radii group: 3/8 + 3i/4)
    for (q, n), chi in (((1, 1), zeta), ((5, 2), c52), ((4, 3), T.character(4, 3))):
        records.append(ball_record(q, n, chi, (F(3, 8), F(3, 4)), (F(1, 2**60), F(1, 2**60))))
        records.append(ball_record(q, n, chi, (F(-3, 2), F(0)), (F(1, 2**60), F(0))))
    # the width witness on the continuation side: zeta on [-1/4, -1/8] (end values differ by more than 2^-4)
    records.append(ball_record(1, 1, zeta, (F(-3, 16), F(0)), (F(1, 16), F(0))))
    # the pole-free rectangle near 1 (the oracle's poles group): 1 + 2^-40 +/- 2^-200
    records.append(ball_record(1, 1, zeta, (1 + H40, F(0)), (F(1, 2**200), F(0)), with_flint=True))
    # pole statuses for exact dyadic balls
    e10 = F(1, 2**10)
    e20 = F(1, 2**20)
    balls = [((F(0), F(0)), (F(0), F(0))), ((F(1), F(0)), (F(0), F(0))),
             ((F(1), F(0)), (e10, F(0))), ((F(0), F(0)), (e10, F(0))), ((F(1), e10), (F(0), F(0))),
             ((1 - e10, F(0)), (e10, F(0))), ((e10, F(0)), (e10, F(0))), ((F(1), e10), (F(0), e10)),
             ((F(0), F(3)), (F(0), F(3))), ((F(-1), F(0)), (F(3), F(1))), ((F(1, 2), F(0)), (F(1, 2), F(0))),
             ((F(-1, 8), F(0)), (e20, F(0))), ((F(17, 16), F(0)), (e20, e20)), ((F(1, 2), F(3)), (e20, e20)),
             ((F(1), F(1, 8)), (e20, e20))]
    for (q, n) in ((1, 1), (5, 2), (4, 3)):
        chi = T.character(q, n)
        for s, rad in balls:
            records.append({'kind': 'pole', 'q': q, 'n': n, 's': [qstr(s[0]), qstr(s[1])],
                            'rad': [qstr(rad[0]), qstr(rad[1])], 'status': T.pole_status(chi, exact_ball(s, rad))})
    # residues of zeta's completion
    for k in (10, 20, 40):
        h = F(1, 2**k)
        records.append({'kind': 'residue', 'which': 'one', 'k': k, 's': [qstr(1 + h), '0/1'],
                        'value': mpball(T.mpq(h)*lambda_mp(zeta, (1 + h, F(0))))})
        records.append({'kind': 'residue', 'which': 'zero', 'k': k, 's': [qstr(h), '0/1'],
                        'value': mpball(T.mpq(h)*lambda_mp(zeta, (h, F(0))))})
    # cutoffs of the oracle's continuation (Lambda) at points of Re(s) < 1
    for (q, n) in ((1, 1), (4, 3), (5, 2)):
        chi = T.character(q, n)
        for s in ((F(1, 2), F(0)), (F(3, 8), F(3, 4)), (F(-3, 2), F(0))):
            sv = s[0] if s[1] == 0 else acb(arb(s[0].numerator)/s[0].denominator,
                                            arb(s[1].numerator)/s[1].denominator)
            for bits in (20, 53, 80):
                r = T.continuation(chi, sv, bits)
                records.append({'kind': 'cutoff', 'q': q, 'n': n, 's': [qstr(s[0]), qstr(s[1])], 'bits': bits,
                                'N': r.N, 'R': r.R, 'degrees': [list(d) for d in r.degrees],
                                'coefficients': r.work,
                                'bound': mp.nstr(T.mpq(T.endpoints(r.tail + r.quadrature)[1]), 20)})
    # the two sides separately (independence test)
    for (q, n) in ((1, 1), (5, 2), (4, 3)):
        chi = T.character(q, n)
        for s in ((F(3, 8), F(3, 4)), (F(-3, 2), F(0))):
            t = cpoint(s)
            fwd = T.numeric_piece(chi, (t + chi.e)/2, 60)
            dual = T.root_number(chi)*T.numeric_piece(chi.conj(), (1 - t + chi.e)/2, 60)
            poles = (1/(t - 1) - 1/t) if chi.C == 1 else 0
            assert abs(fwd + dual + poles - lambda_mp(chi, s)) <= mp.mpf('1e-40')
            records.append({'kind': 'side', 'q': q, 'n': n, 's': [qstr(s[0]), qstr(s[1])],
                            'fwd': mpball(fwd), 'dual': mpball(dual)})
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, 'w') as fh:
        for r in records:
            fh.write(json.dumps(r, separators=(',', ':')) + '\n')
    counts = {}
    for r in records:
        counts[r['kind']] = counts.get(r['kind'], 0) + 1
    print('wrote', OUT, counts, os.path.getsize(OUT), 'bytes')
    print('mpmath', mp.__version__, 'python-flint', flint.__version__, 'FLINT', flint.__FLINT_VERSION__)


if __name__ == '__main__':
    main()
