#!/usr/bin/env python3
"""Vectors of lane f4-slice2 (slice 4d: adf_rfun) from the oracle proto/functions4_checks.py.

Run from the repository root:  timeout 300 python3 -B lanes/f4-slice2/gen_vectors.py
Writes tests/ref/vectors/f4-slice2/{closure,values,texts}.jsonl.

Exact closure results come from the oracle's rterm_translate, rterm_dilate, rterm_product
(proto/functions4_checks.py:212-228, SymPy, exact); sums, reflection and conjugation, which the oracle
has no function for, are written here from docs/api-4.md section 5 (concatenation; odd coefficients
and B change sign; field conjugation). Values phi(x) come from the oracle's rvalue_ball
(proto/functions4_checks.py:256-262, python-flint acb at 400 bits). Texts come from the reference
parser proto/text_grammar.py (canonical). Every exact number is written as a rational string;
a number of a closure result is (re0 + re1 pi) + i (im0 + im1 pi), the four parts as strings.
"""
import json
import random
import sys
from fractions import Fraction as Q
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'proto'))
import sympy as sp                                   # noqa: E402
from flint import acb, arb, ctx                      # noqa: E402
import functions4_checks as F                        # noqa: E402
import text_grammar as G                             # noqa: E402

ctx.prec = 400
OUT = ROOT / 'tests/ref/vectors/f4-slice2'
rng = random.Random(4004)
PI, I = sp.pi, sp.I


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


def cnum(z):
    """input complex number (re, im, radius exponent or None) -> sympy midpoint"""
    return sp.Rational(z[0]) + I * sp.Rational(z[1])


def jnum(z):
    return [s(z[0]), s(z[1])] + ([] if z[2] is None else [z[2]])


def jterm(t):
    return {'P': [jnum(c) for c in t['P']], 'A': jnum(t['A']), 'B': jnum(t['B']), 'C': jnum(t['C'])}


def members(t):
    """exact members of a term with ball inputs: the midpoint, all lower corners, all upper corners"""
    out = []
    for sign in (0, -1, 1):
        def pick(z):
            if z[2] is None or sign == 0:
                return cnum(z)
            r = sp.Rational(2) ** z[2]
            return sp.Rational(z[0]) + sign * r + I * (sp.Rational(z[1]) - sign * r)
        out.append(([pick(c) for c in t['P']], pick(t['A']), pick(t['B']), pick(t['C'])))
        if all(z[2] is None for z in t['P'] + [t['A'], t['B'], t['C']]):
            break
    return out


def exact_term(P, A, B, C):
    return {'P': [parts(c) for c in P], 'A': parts(A), 'B': parts(B), 'C': parts(C)}


def is_exact(t):
    return all(z[2] is None for z in t['P'] + [t['A'], t['B'], t['C']])


def dy(lo=-8, hi=8, den=(1, 2, 4, 8)):
    return Q(rng.randint(lo, hi), rng.choice(den))


def rand_term(kind):
    deg = rng.randint(0, 6)
    P = []
    for j in range(deg + 1):
        re, im = dy(), dy() if rng.random() < 0.5 else Q(0)
        if j == deg and re == 0 and im == 0:
            re = Q(1)
        rad = rng.choice([None, None, None, -10, -20]) if kind == 'ball' else None
        P.append((re, im, rad))
    if kind == 'ball' and rng.random() < 0.5:
        P.append((Q(0), Q(0), -12))          # an inexact small coefficient, kept (conventions 5.12)
    if kind == 'boundary':
        A = (Q(1, 10 ** 30), Q(rng.randint(-3, 3)), None)
    elif kind == 'nondyadic':
        A = (Q(6, 5), Q(1, 5), None)
    elif rng.random() < 0.5:
        A = (Q(rng.randint(1, 12), 4), Q(0), None)
    else:
        A = (Q(rng.randint(1, 12), 4), dy(-6, 6), -30 if kind == 'ball' else None)
    B = (dy(-4, 4), dy(-4, 4) if rng.random() < 0.5 else Q(0), None)
    C = (dy(-2, 2), dy(-2, 2) if rng.random() < 0.5 else Q(0), None)
    if kind == 'nondyadic':
        B = (Q(7, 10), Q(-1, 3), None)
        C = (Q(-1, 5), Q(0), None)
    return {'P': P, 'A': A, 'B': B, 'C': C}


def rand_fun(nterms, kinds=('exact', 'exact', 'ball', 'nondyadic', 'boundary')):
    return [rand_term(rng.choice(kinds)) for _ in range(nterms)]


BIG_Q = Q(rng.getrandbits(2000) | (1 << 1999), (rng.getrandbits(1000) | (1 << 999) | 1))


def closure():
    recs = []
    funs = [rand_fun(rng.randint(1, 3)) for _ in range(8)]
    funs.append([{'P': [], 'A': (Q(1), Q(0), None), 'B': (Q(0), Q(0), None), 'C': (Q(0), Q(0), None)}])
    funs.append([])
    # the shifted Gaussian of PLAN 4.3 as an input too
    funs.append([{'P': [(Q(1), Q(0), None)], 'A': (Q(1), Q(0), None), 'B': (Q(0), Q(0), None),
                  'C': (Q(0), Q(0), None)}])
    for f in funs:
        for q in [Q(0), Q(1, 3), Q(-7, 2), BIG_Q]:
            if q == BIG_Q and (sum(len(t["P"]) for t in f) > 4 or len(f) > 2):
                continue
            res = []
            for t in f:
                res.append([exact_term(*F.rterm_translate(*m, sp.Rational(q.numerator, q.denominator)))
                            for m in members(t)])
            recs.append({'op': 'translate', 'x': [jterm(t) for t in f], 'q': s(q), 'members': res,
                         'exact_in': all(is_exact(t) for t in f)})
        for h in [Q(1), Q(-1), Q(2, 3), Q(-5), BIG_Q]:
            if h == BIG_Q and (sum(len(t["P"]) for t in f) > 4 or len(f) > 2):
                continue
            res = [[exact_term(*F.rterm_dilate(*m, sp.Rational(h.numerator, h.denominator)))
                    for m in members(t)] for t in f]
            recs.append({'op': 'dilate', 'x': [jterm(t) for t in f], 'q': s(h), 'members': res,
                         'exact_in': all(is_exact(t) for t in f)})
        for op in ('reflect', 'conj'):
            res = []
            for t in f:
                ms = []
                for P, A, B, C in members(t):
                    if op == 'reflect':
                        ms.append(exact_term([c * (-1) ** j for j, c in enumerate(P)], A, -B, C))
                    else:
                        ms.append(exact_term([sp.conjugate(c) for c in P], sp.conjugate(A), sp.conjugate(B),
                                             sp.conjugate(C)))
                res.append(ms)
            recs.append({'op': op, 'x': [jterm(t) for t in f], 'members': res,
                         'exact_in': all(is_exact(t) for t in f)})
    for _ in range(12):
        f, g = rand_fun(rng.randint(0, 3)), rand_fun(rng.randint(0, 3))
        # sum: the terms of x, then those of y
        res = [[exact_term(*m) for m in members(t)] for t in f + g]
        recs.append({'op': 'add', 'x': [jterm(t) for t in f], 'y': [jterm(t) for t in g], 'members': res,
                     'exact_in': all(is_exact(t) for t in f + g)})
        # product: pairs (i, j) in lexicographic order; members pair the same corner choice
        res = []
        for a in f:
            for b in g:
                ma, mb = members(a), members(b)
                res.append([exact_term(*F.rterm_product(ma[min(k, len(ma) - 1)], mb[min(k, len(mb) - 1)]))
                            for k in range(max(len(ma), len(mb)))])
        recs.append({'op': 'mul', 'x': [jterm(t) for t in f], 'y': [jterm(t) for t in g], 'members': res,
                     'exact_in': all(is_exact(t) for t in f + g)})
    return recs


def flint_q(q):
    return acb(q.numerator) / q.denominator


def ref_value(term_members, x):
    """sum over terms of the oracle's rvalue_ball at the exact rational x, as two arb strings"""
    v = acb(0)
    for P, A, B, C in term_members:
        v += F.rvalue_ball((P, A, B, C), F.Q(x))
    return [v.real.str(40, radius=True), v.imag.str(40, radius=True)]


def values():
    recs = []
    funs = [rand_fun(rng.randint(1, 3)) for _ in range(11)]
    shifted = F.rterm_translate([1], 1, 0, 0, sp.Rational(1, 3))
    funs.append('shifted')
    funs.append([])
    for f in funs:
        if f == 'shifted':
            # exp(-pi (x - 1/3)^2) = exp(-pi x^2 + (2 pi/3) x - pi/9): given in exact pi form
            xs = [Q(1, 3), Q(-1, 3), Q(0), Q(1), Q(2, 3)]
            vals = [ref_value([shifted], x) for x in xs]
            recs.append({'fun': 'shifted', 'term': exact_term(*shifted),
                         'points': [{'x': s(x), 'members': [v]} for x, v in zip(xs, vals)]})
            continue
        mem = [members(t) for t in f]
        nm = max([len(m) for m in mem] + [1])
        pts = []
        xs = [Q(k, 4) for k in range(-12, 13)] + [Q(1, 3), Q(-2, 3), Q(5, 7), Q(-7, 5), Q(3), Q(-3),
                                                  Q(1, 1000), Q(-9, 4), Q(11, 3), Q(-1, 10), Q(7, 2)]
        for x in xs:
            pts.append({'x': s(x), 'members': [ref_value([m[min(k, len(m) - 1)] for m in mem], x)
                                                 for k in range(nm)]})
        for c, e in [(Q(1, 2), -6), (Q(-1), -4), (Q(0), -8), (Q(2), -3)]:
            r = Q(1, 2 ** -e)
            samples = [c - r, c + r / 3, c + r]
            pts.append({'x': s(c), 'xrad': e,
                        'samples': [[ref_value([m[min(k, len(m) - 1)] for m in mem], y) for k in range(nm)]
                                    for y in samples]})
        recs.append({'fun': [jterm(t) for t in f], 'points': pts})
    return recs


def num_text(q, rad=None):
    """a decimal text of the dyadic q (exact) with an optional dyadic radius"""
    def dec(v):
        v = Q(v)
        if v.denominator == 1:
            return str(v.numerator)
        k = 0
        while (v * 10 ** k).denominator != 1:
            k += 1
        n = v * 10 ** k
        sign = '-' if n < 0 else ''
        digits = str(abs(n.numerator)).rjust(k + 1, '0')
        return sign + digits[:-k] + '.' + digits[-k:]
    return dec(q) + ('' if rad is None else ' +/- ' + dec(rad))


def ctext(re, im, rre=None, rim=None):
    return '(%s) + (%s)*i' % (num_text(re, rre), num_text(im, rim))


def rand_text():
    terms = []
    for _ in range(rng.randint(0, 3)):
        deg = rng.randint(-1, 4)
        P = [ctext(dy(), dy(), rng.choice([None, None, Q(1, 4)])) for _ in range(deg + 1)]
        for _ in range(rng.choice([0, 0, 1, 2])):
            P.append(rng.choice([ctext(0, 0), ctext(0, 0, Q(1, 8)), ctext(0, 0, None, Q(1, 16))]))
        are = rng.choice([Q(1), Q(1, 2), Q(3, 4), Q(0), Q(-1), Q(5, 2), Q(2), Q(1, 8)])
        arad = rng.choice([None, None, None, Q(1, 16), Q(1, 2)])
        A = ctext(are, dy(), arad)
        terms.append('term(P=[%s], A=%s, B=%s, C=%s)' % (', '.join(P), A, ctext(dy(), dy()), ctext(dy(), 0)))
    t = 'rfun(%s)' % ', '.join(terms)
    r = rng.random()
    if r < 0.08:
        t = t.replace('A=', 'Z=', 1)
    elif r < 0.14:
        t = t[:-1]
    elif r < 0.2:
        t = t.replace(' ', '   ').replace('(', ' ( ')
    elif r < 0.24:
        t = t.replace('(1)', '(1e100001)', 1)
    return t


def texts():
    recs = []
    seen = set()
    while len(recs) < 160:
        t = rand_text()
        if t in seen:
            continue
        seen.add(t)
        try:
            out = G.canonical('rfun', t.encode())
        except G.TextError as e:
            out = '!' + e.status
        recs.append({'input': t, 'expected': out})
    # max_items: a coefficient list and a term list at and above a limit of 3
    three = 'term(P=[%s], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)' % ', '.join([ctext(1, 0)] * 3)
    four = three.replace('P=[', 'P=[(1) + (0)*i, ', 1)
    for t, ok in [('rfun(%s)' % three, True), ('rfun(%s)' % four, False),
                  ('rfun(%s)' % ', '.join([three] * 3), True), ('rfun(%s)' % ', '.join([three] * 4), False)]:
        lim = G.Limits(max_items=3)
        try:
            out = G.canonical('rfun', t.encode(), lim)
        except G.TextError as e:
            out = '!' + e.status
        assert (out[0] != '!') == ok, out
        recs.append({'input': t, 'expected': out, 'max_items': 3})
    return recs


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    for name, recs in [('closure', closure()), ('values', values()), ('texts', texts())]:
        with open(OUT / (name + '.jsonl'), 'w') as f:
            for r in recs:
                f.write(json.dumps(r, separators=(',', ':')) + '\n')
        print(name, len(recs), (OUT / (name + '.jsonl')).stat().st_size, 'bytes')


if __name__ == '__main__':
    main()
