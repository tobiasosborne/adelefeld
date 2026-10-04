#!/usr/bin/env python3
"""f-review14: check the f-slice12 fixture rows against the true Hilbert formula
(Definition 4) and against the fixture oracle proto/symbol_checks.py.

The oracle searches primitive triples mod p^2 (16 at 2) with the RAW rational coefficients,
not with coefficients reduced to valuation 0 or 1. This checks whether that matters.
"""
import json
from fractions import Fraction
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
V = ROOT / 'tests/ref/vectors/f-slice12'


def unit_class(q, p):
    q = Fraction(q)
    a, b = q.numerator, q.denominator
    va = vb = 0
    while a % p == 0:
        a //= p
        va += 1
    while b % p == 0:
        b //= p
        vb += 1
    M = 8 if p == 2 else p
    u = a * pow(b, -1, M) % M
    return (va - vb), u


def primitive_solution(a, b, p):
    M = 16 if p == 2 else p * p
    squares = {(x * x % M, x % p != 0) for x in range(M)}
    for x2, xu in squares:
        for y2, yu in squares:
            for z2, zu in squares:
                if (xu or yu or zu) and (a * x2 + b * y2 - z2) % M == 0:
                    return 1
    return -1


def oracle(a, b, p):
    """As in proto/symbol_checks.py: the VALUATION PARITY only, so the coefficients
    searched have valuation 0 or 1, which is what Y6 step 1 reduces to."""
    if p == 0:
        return -1 if a < 0 and b < 0 else 1
    va, u = unit_class(a, p)
    vb, w = unit_class(b, p)
    return primitive_solution(p ** (va % 2) * u, p ** (vb % 2) * w, p)


def eps(u):   # (u-1)/2 mod 2
    return ((u - 1) // 2) % 2


def om(u):    # (u^2-1)/8 mod 2
    return ((u * u - 1) // 8) % 2


def formula(a, b, p):
    """Definition 4, catalogue.md:63-78."""
    if p == 0:
        return -1 if a < 0 and b < 0 else 1
    va, u = unit_class(a, p)
    vb, w = unit_class(b, p)
    alpha, beta = va % 2, vb % 2
    if p == 2:
        e = eps(u) * eps(w) + alpha * om(w) + beta * om(u)
        return -1 if e % 2 else 1
    t = 1
    if alpha and beta and p % 4 == 3:
        t = -1
    lu = 1 if pow(u, (p - 1) // 2, p) == 1 else -1
    lw = 1 if pow(w, (p - 1) // 2, p) == 1 else -1
    return t * (lu if beta else 1) * (lw if alpha else 1)


def report(name):
    bad = []
    out_of_range = 0
    rows = [json.loads(l) for l in (V / name).open()]
    for i, r in enumerate(rows):
        p = int(r['p'])
        a, b = Fraction(r['a']), Fraction(r['b'])
        o = oracle(a, b, p)
        f = formula(a, b, p)
        if o != int(r['value']) or f != int(r['value']):
            bad.append((i + 1, r, o, f))
        if p:
            va, _ = unit_class(a, p)
            vb, _ = unit_class(b, p)
            if va not in (0, 1) or vb not in (0, 1):
                out_of_range += 1
    print(f'{name}: {len(rows)} rows, oracle/formula disagree with the fixture: {len(bad)}, '
          f'rows with a valuation outside {{0,1}}: {out_of_range}')
    for row, r, o, f in bad[:20]:
        print('   row', row, r, 'oracle', o, 'formula', f)
    return bad


if __name__ == '__main__':
    for n in ('hilbert.jsonl', 'hilbert_finite.jsonl'):
        if (V / n).exists():
            report(n)