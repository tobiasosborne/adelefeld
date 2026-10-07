#!/usr/bin/env python3
"""Vectors of lane f4-slice6 (slice 4g): Poisson summation with certified tails, docs/api-4.md section 7, P1.

Oracle: proto/functions4_checks.py: poisson_sides_ball (certified independent truncations, python-flint balls at
600 bits),
poisson_sides and choose_cutoffs (the cutoff search through 0, 1, 2, 4, ... at 60 digits), lattice_tail (the
Lemma 6 bound B_phi(a, h, N) of docs/proofs/analysis.md:213-259), rterm_transform, ffun_transform.
Run: timeout 300 python3 -B lanes/f4-slice6/gen_vectors.py
Output: tests/ref/vectors/f4-slice6/poisson.jsonl, one record per tensor:
  terms (rational P, A, B, C as [re, im] strings), D, M, f ([re, im] rational strings),
  left, right (60-digit certified balls of the two sums, computed independently at 2^-460 tails),
  cuts: {bits: [NL, NR]} for bits 20, 53, 80, 128 (choose_cutoffs),
  tl: per left lattice (j, term) the bound B_phi(j/D, M, N) at N in 0, 1, 2, 4, 8, 16 (20 digits);
  tr: per transformed term the bound B_phihat(0, 1/M, N) at the same N; fmax: max_k |g_k| (25 digits).
And one record "theta" with sum_n exp(-pi n^2) at 60 digits (the witness of api-4.md section 10).
"""
import json
import sys
from fractions import Fraction as Q
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'proto'))
import functions4_checks as o  # noqa: E402
import mpmath as mp  # noqa: E402
import sympy as sp  # noqa: E402
from flint import acb, arb, ctx  # noqa: E402

I = sp.I
R = sp.Rational
OUT = ROOT / 'tests' / 'ref' / 'vectors' / 'f4-slice6'
DMS = [(1, 1), (2, 3), (3, 2), (4, 1), (6, 6)]
NS = [0, 1, 2, 4, 8, 16]
BITS = [20, 53, 80, 128]

PHIS = {
    'gauss': [([1], 1, 0, 0)],
    'x1': [([0, 1], 1, 0, 0)],
    'x2': [([0, 0, 1], 1, 0, 0)],
    'shift': [([1], 1, R(7, 10), 0)],
    'complexA': [([1, R(1, 3) + I / 5], R(6, 5) + I / 5, R(7, 10) - I / 3, R(-1, 5))],
    'terms': [([1], 1, 0, 0), ([1, R(1, 3) + I / 5], R(4, 5), R(7, 5), 0)],
    'deg4': [([1, -1, R(1, 2), 0, R(1, 3)], R(3, 2), 0, R(1, 10))],
}


def fam(D, M, kind):
    L = D * M
    if kind == 0:
        return [sp.Integer(j + 1) for j in range(L)]
    if kind == 1:
        return [sp.Integer((j * j) % 7 - 3) + I * (j % 3) for j in range(L)]
    return [sp.Integer(1) if j == 1 else sp.Integer(0) for j in range(L)]  # delta_1: support smaller


def rq(z):
    z = sp.nsimplify(z)
    re, im = sp.re(z), sp.im(z)
    assert re.is_Rational and im.is_Rational, z
    return [str(re), str(im)]


def term_json(t):
    P, A, B, C = t
    return {'P': [rq(c) for c in P], 'A': rq(A), 'B': rq(B), 'C': rq(C)}


def bstr(x):
    return x.str(60, radius=True)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    recs = []
    for (D, M) in DMS:
        for name, terms in PHIS.items():
            kinds = [0, 1] if (D, M) in [(2, 3), (6, 6)] else [0]
            if (D, M) == (2, 3) and name == 'shift':
                kinds.append(2)
            for kind in kinds:
                f = fam(D, M, kind)
                tensor = (terms, (D, M, f))
                ctx.prec = 600
                big = o.choose_cutoffs(tensor, 460)
                left, right = o.poisson_sides_ball(tensor, big)
                assert left.overlaps(right)
                assert left.real.rad() < arb(10) ** -60 and right.real.rad() < arb(10) ** -60
                cuts = {str(b): list(o.choose_cutoffs(tensor, b)) for b in BITS}
                tl = []
                for j in range(D * M):
                    for k, t in enumerate(terms):
                        tl.append([j, k, [mp.nstr(o.lattice_tail(t, Q(j, D), M, n), 20) for n in NS]])
                hats = [o.rterm_transform(*t) for t in terms]
                tr = [[mp.nstr(o.lattice_tail(h, 0, Q(1, M), n), 20) for n in NS] for h in hats]
                fv = [o.smp(v) for v in f]
                g = [sum(fv[j] * mp.exp(2j * mp.pi * o.mpq(o.phase(Q(-j * k, D * M)))) for j in range(D * M)) / M
                     for k in range(D * M)]
                recs.append({'k': 'pois', 'phi': name, 'fam': kind, 'terms': [term_json(t) for t in terms],
                             'D': D, 'M': M, 'f': [rq(v) for v in f],
                             'left': [bstr(left.real), bstr(left.imag)],
                             'right': [bstr(right.real), bstr(right.imag)],
                             'big': list(big), 'cuts': cuts, 'tl': tl, 'tr': tr,
                             'fmax': mp.nstr(max(map(abs, g)), 25)})
                print(D, M, name, kind, big, cuts, flush=True)
    mp.mp.dps = 70
    theta = sum(mp.exp(-mp.pi * n * n) for n in range(-30, 31))
    recs.append({'k': 'theta', 'v': mp.nstr(theta, 60)})
    with open(OUT / 'poisson.jsonl', 'w') as fh:
        for r in recs:
            fh.write(json.dumps(r, separators=(',', ':')) + '\n')
    print(len(recs), 'records')


if __name__ == '__main__':
    main()
