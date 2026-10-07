#!/usr/bin/env python3
"""Slice 4b. Indexing and transforms come from the unchanged design oracle."""
import json
import sys
from fractions import Fraction as Q
from math import lcm
from pathlib import Path
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'proto'))
import functions4_checks as o
from flint import acb, arb, ctx
ctx.prec = 320
OUT = Path('tests/ref/vectors/f4-slice4')
OUT.mkdir(parents=True, exist_ok=True)
LAYOUTS = [(1, 1), (2, 3), (3, 2), (4, 1), (1, 4), (6, 6), (12, 5)]
HUGE = Q((1 << 1999) + 7, 3)


def z(v):
    if isinstance(v, list):
        return v
    if not isinstance(v, acb):
        v = o.ball(v)
    return [str(v.real.mid().fmpq()), str(v.real.rad().fmpq()),
            str(v.imag.mid().fmpq()), str(v.imag.rad().fmpq())]


def values(D, M, uncertain=False, delta=False):
    return [[str(Q(1 if j == 1 else 0)), '0', '0', '0'] if delta else
            [str(Q((j*7) % 11-5, 8)), '1/64' if uncertain else '0',
             str(Q(j % 3-1, 4)), '1/128' if uncertain else '0'] for j in range(D*M)]


def balls(f):
    return [acb(arb(v[0], v[1]), arb(v[2], v[3])) for v in f]


def function(D, M, f):
    return dict(D=D, M=M, f=[z(v) for v in f])


def emit(name, records):
    p = OUT / (name + '.jsonl')
    p.write_text(''.join(json.dumps(r, separators=(',', ':')) + '\n' for r in records))
    print(name, len(records), p.stat().st_size)


inputs = []
def reference(D, M, f):
    data = function(D, M, f)
    if data not in inputs:
        inputs.append(data)
    return inputs.index(data)

unary, products, ideles, covariance, caps = [], [], [], [], []
for D, M in LAYOUTS:
    for uncertain in [False, True]:
        f = values(D, M, uncertain)
        x = reference(D, M, f)
        for op, qs in [('translate', [Q(0), Q(1, 2), Q(1, 3), Q(-7, 6), Q(5), HUGE]),
                       ('dilate', [Q(1), Q(-1), Q(2), Q(1, 2), Q(2, 3), Q(-3, 4), Q(5)])]:
            for q in qs:
                d, m, g = getattr(o, op)(D, M, f, q)
                unary.append(dict(op=op, x=x, q=str(q), y=function(d, m, g)))
        unary.append(dict(op='reflect', x=x, y=function(D, M, [f[-k % (D*M)] for k in range(D*M)])))
        unary.append(dict(op='conj', x=x, y=function(D, M,
                     [[v[0], v[1], str(-Q(v[2])), v[3]] for v in f])))
        for r in ([Q(1), Q(2), Q(1, 3), Q(6, 5)] if (D, M) == (2, 3) else []):
            for c, N in [(1, 0), (-1, 0), (1, D*M), (D*M-1 if D*M > 1 else 1, 2*D*M),
                         (1, 1), (1, 3)]:
                rec = dict(x=x, r=str(r), c=str(c), N=str(N))
                try:
                    rec['y'] = function(*o.dilate_idele(D, M, f, r, c, N))
                    rec['status'] = 'OK'
                except ValueError:
                    rec['status'] = 'NOT_DETERMINED'
                ideles.append(rec)
    for uncertain in [False, True]:
        d, m = LAYOUTS[(LAYOUTS.index((D, M))+1) % len(LAYOUTS)]
        f, g = values(D, M, uncertain), values(d, m, uncertain)
        D2, M2 = lcm(D, d), lcm(M, m)
        a, b = o.refine(D, M, f, D2, M2), o.refine(d, m, g, D2, M2)
        out = [o.ball(v)*o.ball(w) if not isinstance(v, list) and not isinstance(w, list) else
               (acb(0) if v == 0 or w == 0 else balls([v])[0]*balls([w])[0]) for v, w in zip(a, b)]
        products.append(dict(x=reference(D, M, f), w=reference(d, m, g),
                             a=function(D2, M2, a), b=function(D2, M2, b),
                             y=function(D2, M2, out), uncertain=int(uncertain)))
    for q in ([Q(-2, 3), Q(3, 2)] if (D, M) in [(2, 3), (4, 1)] else []):
        f = [Q(1) if j == 1 % (D*M) else Q(0) for j in range(D*M)]
        ctx.prec = 96
        left = o.ffun_transform(*o.dilate(D, M, f, q))
        right = o.dilate(*o.ffun_transform(D, M, f), 1/q)
        covariance.append(dict(x=reference(D, M, f), q=str(q), lhs=function(*left),
                               rhs=function(right[0], right[1], [o.ball(abs(q))*o.ball(v) for v in right[2]])))
    ctx.prec = 320
# Delta with D != M distinguishes the signs and support holes.
f = values(2, 3, delta=True)
for op, qs in [('translate', [Q(1, 2), Q(1, 3), Q(-7, 6)]),
               ('dilate', [Q(-1), Q(2, 3), Q(-3, 4)])]:
    for q in qs:
        unary.append(dict(op=op, x=reference(2, 3, f), q=str(q), y=function(*getattr(o, op)(2, 3, f, q))))
for op in ['reflect', 'conj']:
    g = [f[-k % 6] for k in range(6)] if op == 'reflect' else f
    unary.append(dict(op=op, x=reference(2, 3, f), y=function(2, 3, g)))
for r, c, N in [(Q(1), 1, 0), (Q(1), -1, 0), (Q(2, 3), 1, 3),
                (Q(2, 3), -1, 0), (Q(1), 1, 1)]:
    rec = dict(x=reference(2, 3, f), r=str(r), c=str(c), N=str(N))
    try:
        rec['y'] = function(*o.dilate_idele(2, 3, f, r, c, N))
        rec['status'] = 'OK'
    except ValueError:
        rec['status'] = 'NOT_DETERMINED'
    ideles.append(rec)
for q in [Q(1 << 1999), Q(1, 1 << 1999), Q(1048577), Q(0)]:
    caps.append(dict(op='dilate', q=str(q), status='DOMAIN' if q == 0 else 'LIMIT'))
caps.append(dict(op='translate', q=str(Q(1, 1048577)), status='LIMIT'))
for name, records in [('inputs', inputs), ('unary', unary), ('products', products), ('ideles', ideles),
                       ('covariance', covariance), ('caps', caps)]:
    emit(name, records)
total = sum(p.stat().st_size for p in OUT.glob('*.jsonl'))
assert total <= 400000, total
print('total', total)
