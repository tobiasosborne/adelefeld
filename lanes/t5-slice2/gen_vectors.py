#!/usr/bin/env python3
"""Vectors of slice 5c (lane t5-slice2): timeout 300 python3 -B lanes/t5-slice2/gen_vectors.py

Writes tests/ref/vectors/t5-slice2/tate.jsonl from the oracle proto/tate_checks.py (docs/api-5.md; analysis
P11, P13). Records, one JSON object per line:
  vector:  q, n (raw Conrey pair of tests/golden/gauss.tsv, or (1, 1)), C, label, e (the oracle's lowering,
           char_checks.lower), phases (f[j] = chi(j) for j in [0, C) as exact "a/b" in [0, 1), null on nonunits).
  value:   q, n, C, e, s = [re, im] exact rationals, rad = [re_rad, im_rad] exact dyadic radii ("0": exact),
           points = the 60-digit references I_chi(t) = pi^-z Gamma(z) L(t, chi), z = (t + e)/2, at the midpoint
           and, for a ball, at its four corners, each [re, im] as "mid +/- rad" (rad = 1e-50 max(1, |I|));
           flint = the same values by python-flint's dirichlet_char(C, label).l_function completed by hand with
           pi^-z Gamma(z) at 256 bits (a second source; acb_dirichlet.rst:567-569), as arb strings.
           The two sources are checked against each other here (margin 1e-45 max(1, |I|)).
  cutoff:  the oracle's continuation(chi, s, bits) cutoffs N, R and its Taylor degrees (Lambda, P15 search):
           s = 2 at bits 20, 53, 80 (C = 1, 4, 5); s = 2, 3, 9/8, 6 at bits 0, 4, 12 (C = 1, 4, 5, 16).
  piece:   T3 (api-5.md:286-300) as the oracle's check_certificates: sum_(n<=6) chi(n) n^e integral_1^R
           exp(-pi n^2 t/C) t^(z-1) dt by mpmath's incomplete Gamma (the oracle's numeric_piece; a numerical
           reference only, P15 [source pending]) for z in -3/4 + i/2, 9/4 - 5i/4, R in 1, 2, 8.
  witness: zeta on the hull s = [1.125, 1.25]: the two end values, separation > 1 (N-D23, api-5.md T4.3).
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
OUT = os.path.join(ROOT, 'tests', 'ref', 'vectors', 't-slice2', 'tate.jsonl')


def golden_pairs():
    pairs = [(1, 1)]
    with open(os.path.join(ROOT, 'tests', 'golden', 'gauss.tsv')) as fh:
        for line in fh:
            if line.startswith('char('):
                head = line.split(')')[0]
                q = int(head.split('q=')[1].split(',')[0])
                n = int(head.split('n=')[1].split(',')[0])
                pairs.append((q, n))
    return pairs


def qstr(x):
    x = F(x)
    return f'{x.numerator}/{x.denominator}'


def mpball(v):
    rad = mp.mpf('1e-50')*max(1, abs(v))
    return [mp.nstr(v.real, 62, strip_zeros=False) + ' +/- ' + mp.nstr(rad, 3),
            mp.nstr(v.imag, 62, strip_zeros=False) + ' +/- ' + mp.nstr(rad, 3)]


def I_ref(chi, t):
    """60-digit reference I_chi(t) = pi^-z Gamma(z) L(t, chi), analysis P11; mpmath Hurwitz for C > 1."""
    t = mp.mpc(t)
    z = (t + chi.e)/2
    return mp.pi**(-z)*mp.gamma(z)*T.l_reference(chi, t)


def I_flint(chi, t):
    """python-flint L value completed by hand: pi^-z Gamma(z) L(s, chi) (acb_dirichlet.rst:567-569)."""
    with ctx.workprec(256):
        s = acb(arb(t.real), arb(t.imag)) if isinstance(t, mp.mpc) else acb(t)
        z = (s + chi.e)/2
        if chi.C == 1:
            L = s.zeta()
        else:
            L = flint.dirichlet_char(chi.C, chi.label).l_function(s)
        return arb.pi()**(-z)*z.gamma()*L


def arbstr(x):
    return x.str(70, radius=True)


def main():
    pairs = golden_pairs()
    assert len(pairs) == 18, pairs
    svals = [(F(2), F(0)), (F(3), F(0)), (F(9, 8), F(0)), (F(3, 2), F(14)), (F(2), F(3)), (F(10001, 10000), F(0))]
    balls = [((F(2), F(3)), (F(1, 2**70), F(1, 2**70))), ((F(3, 2), F(0)), (F(1, 2**60), F(0)))]
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    records = []
    for q, n in pairs:
        chi = T.character(q, n)
        records.append({'kind': 'vector', 'q': q, 'n': n, 'C': chi.C, 'label': chi.label, 'e': chi.e,
                        'phases': [None if p is None else qstr(p) for p in chi.phases]})
        for s, rad in [(s, (F(0), F(0))) for s in svals] + balls:
            mid = mp.mpc(T.mpq(s[0]), T.mpq(s[1]))
            pts = [mid]
            if rad != (F(0), F(0)):
                for a in (-1, 1):
                    for b in (-1, 1):
                        pts.append(mp.mpc(T.mpq(s[0] + a*rad[0]), T.mpq(s[1] + b*rad[1])))
            refs, fl = [], []
            for t in pts:
                v = I_ref(chi, t)
                with ctx.workprec(256):
                    w = I_flint(chi, t)
                    wm = mp.mpc(T.mpq(sum(T.endpoints(w.real))/2), T.mpq(sum(T.endpoints(w.imag))/2))
                assert abs(v - wm) <= T.MARGIN*max(1, abs(v)), (q, n, t, v, wm)
                assert T.width(w) < F(1, 10**40)
                refs.append(mpball(v))
                fl.append([arbstr(w.real), arbstr(w.imag)])
            records.append({'kind': 'value', 'q': q, 'n': n, 'C': chi.C, 'e': chi.e,
                            's': [qstr(s[0]), qstr(s[1])], 'rad': [qstr(rad[0]), qstr(rad[1])],
                            'points': refs, 'flint': fl})
    cuts = [((q, n), F(2), bits) for q, n in ((1, 1), (4, 3), (5, 2)) for bits in (20, 53, 80)]
    cuts += [((q, n), s, bits) for q, n in ((1, 1), (4, 3), (5, 2), (16, 3)) for s in (F(2), F(3), F(9, 8), F(6))
             for bits in (0, 4, 12)]
    for (q, n), s, bits in cuts:
        chi = T.character(q, n)
        r = T.continuation(chi, s, bits)
        records.append({'kind': 'cutoff', 'q': q, 'n': n, 's': qstr(s), 'bits': bits, 'N': r.N, 'R': r.R,
                        'degrees': [list(d) for d in r.degrees], 'coefficients': r.work})
    for q, n in ((1, 1), (4, 3), (5, 2)):
        chi = T.character(q, n)
        for z in ((F(-3, 4), F(1, 2)), (F(9, 4), F(-5, 4))):
            for R in (1, 2, 8):
                v = T.numeric_piece(chi, mp.mpc(T.mpq(z[0]), T.mpq(z[1])), 6, R)
                records.append({'kind': 'piece', 'q': q, 'n': n, 'z': [qstr(z[0]), qstr(z[1])], 'N': 6, 'R': R,
                                'value': mpball(v)})
    zeta = T.character(1)
    lo, hi = I_ref(zeta, mp.mpf('1.125')), I_ref(zeta, mp.mpf('1.25'))
    assert abs(lo - hi) > 1
    records.append({'kind': 'witness', 'q': 1, 'n': 1, 's': ['19/16', '0/1'], 'rad': ['1/16', '0/1'],
                    'points': [mpball(lo), mpball(hi)], 'separation': mp.nstr(abs(lo - hi), 20)})
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
