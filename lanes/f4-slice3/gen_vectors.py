#!/usr/bin/env python3
"""Vectors of lane f4-slice3 (slice 4e: transform, derivative, integral, norm2 of adf_rfun) from the oracle
proto/functions4_checks.py.

Run from the repository root:  timeout 600 python3 -B lanes/f4-slice3/gen_vectors.py
Writes tests/ref/vectors/f4-slice3/transform.jsonl (one record per function) and shifted.jsonl.

Sources in the oracle (read before use):
- rterm_transform (proto/functions4_checks.py:235-248): exact SymPy (P', 1/A, iB/A, C + B^2/(4 pi A)), the
  polynomial with the factor A^(-1/2) (principal root); converted to balls by the oracle's ball()
  (functions4_checks.py:61-87, python-flint at 400 bits) and printed with 60 digits.
- rterm_derivative (functions4_checks.py:230-232): exact, written as (a + b pi) + i (c + d pi).
- rterm_product (functions4_checks.py:224-228) and rvalue_ball (:256-262): the integral is the transform at 0
  (check_real, :469), the norm the integral of phi conj(phi) over every ordered pair of terms (:474-477).
- Second source, numerical and not a certificate: mp.quad of phi, |phi|^2 and phi(x) exp(2 pi i x y) at 60
  digits, as check_real does (:463-476); the oracle's margin is 1e-45.
Input numbers are [re, im] or [re, im, e] (radius 2^e on both parts), as lanes/f4-slice2/gen_vectors.py;
a member of a ball input is its midpoint or one of its two extreme corners (the same corner in every ball).
"""
import json
import random
import sys
from fractions import Fraction as Q
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'proto'))
import mpmath as mp                                  # noqa: E402
import sympy as sp                                   # noqa: E402
from flint import acb, ctx                           # noqa: E402
import functions4_checks as F                        # noqa: E402

ctx.prec = 400
mp.mp.dps = 60
OUT = ROOT / 'tests/ref/vectors/f4-slice3'
rng = random.Random(4005)
PI, I = sp.pi, sp.I
DIG = 60


def s(q):
    q = sp.Rational(q)
    return str(q.p) if q.q == 1 else '%d/%d' % (q.p, q.q)


def parts(e):
    """exact sympy number a + b pi (a, b Gaussian rationals) -> [re a, re b, im a, im b]"""
    e = sp.expand(sp.sympify(e))
    p = sp.Poly(e, PI)
    if p.degree() > 1:
        raise ValueError(e)
    a, b = p.coeff_monomial(1), p.coeff_monomial(PI)
    return [s(sp.re(a)), s(sp.re(b)), s(sp.im(a)), s(sp.im(b))]


def bstr(z):
    """an oracle value (sympy expression or acb) -> [re, im] arb texts with DIG digits"""
    b = z if isinstance(z, acb) else F.ball(z)
    return [b.real.str(DIG, radius=True), b.imag.str(DIG, radius=True)]


def cnum(z):
    return sp.Rational(z[0]) + I * sp.Rational(z[1])


def jnum(z):
    return [s(z[0]), s(z[1])] + ([] if z[2] is None else [z[2]])


def jterm(t):
    return {'P': [jnum(c) for c in t['P']], 'A': jnum(t['A']), 'B': jnum(t['B']), 'C': jnum(t['C'])}


def is_exact(t):
    return all(z[2] is None for z in t['P'] + [t['A'], t['B'], t['C']])


def members(t):
    out = []
    for sign in (0, -1, 1):
        def pick(z):
            if z[2] is None or sign == 0:
                return cnum(z)
            r = sp.Rational(2) ** z[2]
            return sp.Rational(z[0]) + sign * r + I * (sp.Rational(z[1]) - sign * r)
        out.append(([pick(c) for c in t['P']], pick(t['A']), pick(t['B']), pick(t['C'])))
        if is_exact(t):
            break
    return out


def dy(lo=-8, hi=8, den=(1, 2, 4, 8)):
    return Q(rng.randint(lo, hi), rng.choice(den))


def rand_term(kind):
    deg = rng.randint(-1, 6) if kind != 'big' else 6
    P = []
    for j in range(deg + 1):
        re, im = dy(), (dy() if rng.random() < 0.5 else Q(0))
        if j == deg and re == 0 and im == 0:
            re = Q(1)
        rad = rng.choice([None, None, -12, -20]) if kind == 'ball' else None
        P.append((re, im, rad))
    if kind == 'boundary':
        A = (Q(1, 10 ** 30), Q(rng.choice([0, 3, -2])), None)
    elif kind == 'complex':
        A = rng.choice([(Q(6, 5), Q(1, 5), None), (Q(1), Q(2), None), (Q(1), Q(-2), None),
                        (Q(1, 100), Q(3), None), (Q(1, 2), Q(-7, 4), None)])
    elif kind == 'ball':
        A = (Q(rng.randint(2, 12), 4), dy(-6, 6), -30)
    else:
        A = (Q(rng.randint(1, 12), 4), Q(0), None)
    B = (dy(-4, 4), dy(-4, 4) if rng.random() < 0.5 else Q(0), None)
    C = (dy(-2, 2), dy(-2, 2) if rng.random() < 0.5 else Q(0), None)
    return {'P': P, 'A': A, 'B': B, 'C': C}


def oracle_conj(t):
    P, A, B, C = t
    return [sp.conjugate(c) for c in P], sp.conjugate(A), sp.conjugate(B), sp.conjugate(C)


def integral_ball(fm):
    """the oracle's integral of a member function (a list of exact terms): the transform at 0"""
    v = acb(0)
    for t in fm:
        if t[0]:
            v += F.rvalue_ball(F.rterm_transform(*t), F.Q(0))
    return v


def norm_ball(fm):
    """every ordered pair (k, l) of terms: the integral of t_k conj(t_l)"""
    v = acb(0)
    for a in fm:
        for b in fm:
            prod = F.rterm_product(a, oracle_conj(b))
            if prod[0]:
                v += F.rvalue_ball(F.rterm_transform(*prod), F.Q(0))
    return v


def mval(fm, x):
    return sum((F.rvalue(t, x) for t in fm), mp.mpc(0))


def quad(f):
    return mp.quad(f, [-mp.inf, -1, 0, 1, mp.inf])


YS = [Q(0), Q(1, 4), Q(-2, 5), Q(3, 2)]


def record(f, label):
    mem = [members(t) for t in f]
    nm = max([len(m) for m in mem] + [1])
    fms = [[m[min(k, len(m) - 1)] for m in mem] for k in range(nm)]
    rec = {'label': label, 'x': [jterm(t) for t in f], 'exact_in': all(is_exact(t) for t in f)}
    hat = []
    for m in mem:
        hm = []
        for P, A, B, C in m:
            Pn, An, Bn, Cn = F.rterm_transform(P, A, B, C)
            hm.append({'P': [bstr(c) for c in Pn], 'A': bstr(An), 'B': bstr(Bn), 'C': bstr(Cn)})
        hat.append(hm)
    rec['hat'] = hat
    rec['deriv'] = [[dict(zip('PABC', ([parts(c) for c in d[0]],) + tuple(parts(v) for v in d[1:])))
                     for d in (F.rterm_derivative(*mm) for mm in m)] for m in mem]
    rec['refl'] = [[{'P': [parts(c * (-1) ** j) for j, c in enumerate(P)], 'A': parts(A), 'B': parts(-B),
                     'C': parts(C)} for P, A, B, C in m] for m in mem]
    rec['integral'] = [bstr(integral_ball(fm)) for fm in fms]
    rec['norm2'] = [bstr(norm_ball(fm)) for fm in fms]
    rec['values'] = [{'y': s(y), 'members': [bstr(sum((F.rvalue_ball(F.rterm_transform(*t), F.Q(y))
                                                       for t in fm if t[0]), acb(0))) for fm in fms]}
                     for y in YS]
    # the second, numerical source: only where mp.quad is reliable (exact inputs, Re(A) >= 1/4, small degree)
    if rec['exact_in'] and f and all(Q(t['A'][0]) >= Q(1, 4) and len(t['P']) <= 5 for t in f):
        fm = fms[0]
        qi = quad(lambda x: mval(fm, x))
        qn = quad(lambda x: abs(mval(fm, x)) ** 2)
        rec['quad_integral'] = [mp.nstr(qi.real, 50), mp.nstr(qi.imag, 50)]
        rec['quad_norm2'] = mp.nstr(qn, 50)
        rec['quad_hat'] = []
        for y in (Q(1, 5), Q(-2, 5)):
            yy = F.mpq(y)
            qh = quad(lambda x: mval(fm, x) * mp.exp(2j * mp.pi * x * yy))
            rec['quad_hat'].append({'y': s(y), 'v': [mp.nstr(qh.real, 50), mp.nstr(qh.imag, 50)]})
    return rec


def zero_c():
    return (Q(0), Q(0), None)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    funs = []
    gauss = {'P': [(Q(1), Q(0), None)], 'A': (Q(1), Q(0), None), 'B': zero_c(), 'C': zero_c()}
    funs.append(([gauss], 'gauss'))
    funs.append(([dict(gauss, P=[zero_c(), (Q(1), Q(0), None)])], 'odd gauss'))
    funs.append(([dict(gauss, A=(Q(4), Q(0), None))], 'A = 4 (amplitude 1/2)'))
    funs.append(([dict(gauss, P=[])], 'zero polynomial'))
    funs.append(([], 'zero function'))
    funs.append(([dict(gauss, A=(Q(1), Q(2), None)), dict(gauss, A=(Q(1), Q(-2), None))],
                 'both root quadrants'))
    kinds = ['exact', 'exact', 'exact', 'complex', 'complex', 'ball', 'boundary', 'big']
    while len(funs) < 60:
        n = rng.choice([1, 1, 2, 2, 3])
        f = [rand_term(rng.choice(kinds)) for _ in range(n)]
        funs.append((f, 'random'))
    recs = [record(f, label) for f, label in funs]
    with open(OUT / 'transform.jsonl', 'w') as fh:
        for r in recs:
            fh.write(json.dumps(r, separators=(',', ':')) + '\n')
    # the shifted Gaussian of PLAN 4.3 (api-4.md R1 step 3): exp(-pi (x - 1/3)^2), exact in pi form
    shifted = F.rterm_translate([1], 1, 0, 0, sp.Rational(1, 3))
    hat = F.rterm_transform(*shifted)
    v = F.rvalue_ball(hat, F.Q(Q(1, 4)))
    assert v.imag > 0
    exact = sp.exp(-PI / 16) * (sp.cos(PI / 6) + I * sp.sin(PI / 6))      # exp(-pi y^2) E(y/3) at y = 1/4
    assert abs(complex(sp.N(exact, 30)) - complex(F.rvalue(hat, mp.mpf('0.25')))) < 1e-25
    sh = {'term': {'P': [parts(c) for c in shifted[0]], 'A': parts(shifted[1]), 'B': parts(shifted[2]),
                   'C': parts(shifted[3])},
          'hat': {'P': [bstr(c) for c in hat[0]], 'A': bstr(hat[1]), 'B': bstr(hat[2]), 'C': bstr(hat[3])},
          'y': '1/4', 'value': bstr(v)}
    with open(OUT / 'shifted.jsonl', 'w') as fh:
        fh.write(json.dumps(sh, separators=(',', ':')) + '\n')
    for name in ('transform', 'shifted'):
        print(name, (OUT / (name + '.jsonl')).stat().st_size, 'bytes')
    print('records', len(recs), 'exact', sum(r['exact_in'] for r in recs), 'quadrature',
          sum('quad_norm2' in r for r in recs))


if __name__ == '__main__':
    main()
