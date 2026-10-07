#!/usr/bin/env python3
"""Vectors of lane f4-slice5 (slice 4f): E1 evaluation, partial-place evaluation, integrals and norms.

Oracle: proto/functions4_checks.py (evaluate, evaluate_partial, valuation, rvalue_ball, rterm_transform,
rterm_product). Second source for the finite index sets: rational samples of the ball, as the oracle's own
check_evaluation does (proto/functions4_checks.py:424-436), and the rule of E1 step 4 written out here for the
partial balls (evaluate_partial is the oracle's; step4 below is our transcription; both must agree).
Run: timeout 300 python3 -B lanes/f4-slice5/gen_vectors.py
Output: tests/ref/vectors/f4-slice5/{eval,local,sball,tensor,integral}.jsonl
"""
import json
import sys
from fractions import Fraction as Q
from math import gcd
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'proto'))
import functions4_checks as o  # noqa: E402
import sympy as sp  # noqa: E402
from flint import acb, ctx  # noqa: E402

ctx.prec = 400
OUT = ROOT / 'tests' / 'ref' / 'vectors' / 'f4-slice5'
DMS = [(1, 1), (2, 3), (3, 2), (4, 1), (1, 4), (6, 6), (12, 5)]
BIG = 2**1999 + 123456789


def mask(indices, L):
    return ''.join('1' if j in indices else '0' for j in range(L))


def canon(A, H, d):
    """tests/ref/adfref/fball.py canon, restated: d > 0; divide by gcd; reduce A into [0, H)."""
    if d < 0:
        A, d = -A, -d
    if H == 0:
        g = gcd(A, d)
        return A // g, 0, d // g
    g = gcd(gcd(A, H), d)
    A, H, d = A // g, H // g, d // g
    return A % H, H, d


def samples_check(D, M, A, H, d, indices, outside):
    """The oracle's own second source (functions4_checks.py:430-435): rational samples A + H n over d."""
    if abs(A) > 10**6:
        return
    samples = [Q(A + H * n, d) for n in range(d * D * M + 1)]
    seen = {int(D * x) % (D * M) for x in samples if (D * x).denominator == 1}
    assert indices == seen, (D, M, A, H, d)
    assert outside == any((D * x).denominator != 1 for x in samples), (D, M, A, H, d)


def balls():
    out = []
    for d in [1, 2, 3, 4, 5, 12]:
        for H in [0, 1, 2, 3, 6, 12]:
            for A in [-7, -1, 0, 1, 5, 11]:
                out.append((A, H, d))
    for d in [1, 12]:
        for H in [0, 12]:
            out.append((BIG, H, d))
            out.append((-BIG, H, d))
    # fractional radii through d (H/d), and a raw triple that is not canonical
    out += [(1, 1, 4), (3, 2, 8), (6, 4, 8), (10, 15, 5), (-9, 6, 12)]
    return out


def gen_eval():
    recs = []
    for D, M in DMS:
        f = (D, M, list(range(1, D * M + 1)))
        for A, H, d in balls():
            idx, outside = o.evaluate(f, (A, H, d))
            ci = o.evaluate(f, canon(A, H, d))
            assert ci == (idx, outside)           # a property of the set, not of the triple
            samples_check(D, M, A, H, d, set(idx), outside)
            recs.append({'D': D, 'M': M, 'A': str(A), 'H': str(H), 'd': str(d),
                         'm': mask(idx, D * M), 'o': outside})
    return recs


def gen_local():
    """Local-backend balls of the context with blocks 8, 9, 5 (K = 360): c + R Zhat with K/R, c K/R integers."""
    recs = []
    cases = [(Q(7), Q(12)), (Q(-3), Q(360)), (Q(0), Q(6)), (Q(11), Q(5)), (Q(1, 2), Q(3, 2)), (Q(5), Q(1)),
             (Q(-1, 4), Q(9, 4)), (Q(2), Q(72))]
    for D, M in DMS:
        f = (D, M, list(range(1, D * M + 1)))
        for c, R in cases:
            assert (360 / R).denominator == 1 and (c * 360 / R).denominator == 1
            den = (c.denominator * R.denominator) // gcd(c.denominator, R.denominator)
            A, H, d = canon(int(c * den), int(R * den), den)
            idx, outside = o.evaluate(f, (A, H, d))
            recs.append({'D': D, 'M': M, 'c': str(c), 'R': str(R), 'canon': [str(A), str(H), str(d)],
                         'm': mask(idx, D * M), 'o': outside})
    return recs


def step4(D, M, locals_):
    """E1 step 4 transcribed from docs/api-4.md:257-261 (our transcription, compared with the oracle)."""
    keep = set()
    for j in range(D * M):
        ok = True
        for p, a, e in locals_:
            m = o.valuation(M, p)
            t = m if e is None else min(e, m)
            if o.valuation(Q(j, D) - a, p) < t:
                ok = False
        if ok:
            keep.add(j)
    return keep


def gen_sball():
    recs = []
    tuples = {
        '23': [[(2, Q(1, 3), 1), (3, Q(1), 1)], [(2, Q(0), 2), (3, Q(2), 0)], [(2, Q(1, 2), -1), (3, Q(5), 2)],
               [(2, Q(1, 4), None), (3, Q(2, 3), None)], [(2, Q(3), 3), (3, Q(-1), None)], [(2, Q(0), None),
                                                                                           (3, Q(0), 1)],
               [(3, Q(1), 1)]],
        '5': [[(5, Q(1), 1)], [(5, Q(2, 5), 0)], [(5, Q(3), None)], [(5, Q(0), 2)], [(5, Q(1, 3), 1)]],
        '': [[]],
    }
    for D, M in DMS:
        f = (D, M, list(range(1, D * M + 1)))
        for key, lst in tuples.items():
            for locals_ in lst:
                idx, outside = o.evaluate_partial(f, locals_)
                assert set(idx) == step4(D, M, locals_) and outside
                recs.append({'D': D, 'M': M, 'loc': [[p, str(a), e] for p, a, e in locals_],
                             'm': mask(idx, D * M)})
    # projections of adeles to {2, 3}: the sball set contains the adele set; equal when the primes of D M and
    # of the radius are 2 and 3 only
    agree = 0
    for D, M in DMS:
        f = (D, M, list(range(1, D * M + 1)))
        for A, H, d in [(1, 6, 1), (5, 12, 1), (1, 0, 3), (7, 4, 3), (1, 30, 2), (2, 5, 1), (0, 1, 1)]:
            A, H, d = canon(A, H, d)
            loc = []
            for p in (2, 3):
                if H == 0:
                    loc.append((p, Q(A, d), None))
                else:
                    loc.append((p, Q(A, d), o.valuation(H, p) - o.valuation(d, p)))
            idx, _ = o.evaluate_partial(f, loc)
            aidx, aout = o.evaluate(f, (A, H, d))
            assert set(aidx) <= set(idx)
            agree += set(aidx) == set(idx)
            recs.append({'D': D, 'M': M, 'proj': [str(A), str(H), str(d)], 'm': mask(idx, D * M),
                         'am': mask(aidx, D * M), 'ao': aout})
    print('sball projections: index sets equal to the adele sets in', agree, 'of', 7 * len(DMS))
    return recs


R6 = sp.Rational
TERMS = {
    'gauss': [([1], 1, 0, 0)],
    'base': [([1, -2, 3], R6(6, 5) + sp.I / 5, R6(7, 10) - sp.I / 3, -R6(1, 5))],
    'odd': [([0, 1], 1, 0, 0)],
    'two': [([2, 0, 1], R6(1, 2), 1, 0), ([1], 2, -R6(1, 2), R6(1, 3))],
    'drift': [([1], 1, 4, 0)],
}


def num(z):
    z = sp.nsimplify(z)
    return [str(sp.re(z)), str(sp.im(z))]


def term_json(t):
    P, A, B, C = t
    return {'P': [num(c) for c in P], 'A': num(A), 'B': num(B), 'C': num(C)}


def bstr(b):
    return [b.real.str(60, radius=True), b.imag.str(60, radius=True)]


def phi_value(terms, x):
    return sum((o.rvalue_ball(t, x) for t in terms), acb(0))


def phi_integral(terms):
    return sum((o.rvalue_ball(o.rterm_transform(*t), 0) for t in terms), acb(0))


def phi_norm2(terms):
    s = acb(0)
    for a in terms:
        for b in terms:
            P, A, B, C = b
            conj = [sp.conjugate(c) for c in P], sp.conjugate(A), sp.conjugate(B), sp.conjugate(C)
            s += o.rvalue_ball(o.rterm_transform(*o.rterm_product(a, conj)), 0)
    return s


def fvalues(D, M, family):
    L = D * M
    if family == 'int':
        return [Q(j + 1) for j in range(L)]
    return [(Q(j * j % 7 - 3), Q(j % 3)) for j in range(L)]


def gen_tensor():
    recs = []
    xs = [Q(0), Q(1, 4), Q(-2, 3), Q(3)]
    for name, terms in TERMS.items():
        for x in xs:
            recs.append({'k': 'val', 'phi': name, 'terms': [term_json(t) for t in terms], 'x': str(x),
                         'v': bstr(phi_value(terms, x))})
        recs.append({'k': 'int', 'phi': name, 'terms': [term_json(t) for t in terms],
                     'i': bstr(phi_integral(terms)), 'n': bstr(phi_norm2(terms))})
    return recs


def gen_integral():
    recs = []
    for D, M in DMS:
        for fam in ['int', 'cplx']:
            vals = fvalues(D, M, fam)
            if fam == 'int':
                integ = (sum(vals) / M, Q(0))
                norm = sum(v * v for v in vals) / M
                vj = [[str(v), '0'] for v in vals]
            else:
                integ = (sum(v[0] for v in vals) / M, sum(v[1] for v in vals) / M)
                norm = sum(v[0] ** 2 + v[1] ** 2 for v in vals) / M
                vj = [[str(a), str(b)] for a, b in vals]
            recs.append({'D': D, 'M': M, 'f': vj, 'i': [str(integ[0]), str(integ[1])], 'n': str(norm)})
    # the example of faults_44: (2, 3), [1..6]: integral 7, norm 91/3 (functions4_checks.py:604-605)
    assert recs[2]['i'] == ['7', '0'] and recs[2]['n'] == '91/3'
    return recs


def write(name, recs):
    OUT.mkdir(parents=True, exist_ok=True)
    with open(OUT / name, 'w') as fh:
        for r in recs:
            fh.write(json.dumps(r, separators=(',', ':')) + '\n')
    print(name, len(recs), 'records', (OUT / name).stat().st_size, 'bytes')


def main():
    write('eval.jsonl', gen_eval())
    write('local.jsonl', gen_local())
    write('sball.jsonl', gen_sball())
    write('tensor.jsonl', gen_tensor())
    write('integral.jsonl', gen_integral())


if __name__ == '__main__':
    main()
