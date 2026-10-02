#!/usr/bin/env python3
"""Exact finite-ring oracle for 1F.5. No library imports and no series implementation.

For each integral input a+p^r Z_p, exhaust b modulo p^H, H=r+1, and bucket b^n modulo p^r.
H exceeds the predicted root exponent E=r-v_p(n)-(n-1)j, j=v_p(a)/n. Thus the set equality at H checks every
root residue and includes a distance-p^E witness. It checks ALL input residue classes at r.
Scaling a unit ball by p^(n*t) adds t to its root exponent and comparison precision, also for t<0.
This finite enumeration is independent of exp/log. Infinite set equality is proved in R2.
"""
import json
from math import gcd
from fractions import Fraction
from pathlib import Path

OUT = Path('tests/ref/vectors/f-slice8/balls.jsonl')


def vp(n, p):
    s = 0
    while n % p == 0:
        n //= p
        s += 1
    return s


def generate():
    rows = []
    witnesses = residues = 0
    for p in (2, 3, 5, 7):
        c = 2 if p == 2 else 1
        for n in range(1, 13):
            s = vp(n, p)
            for r in range(1, (5 if p == 2 else 3) + 1):
                mod, H = p**r, r+1
                buckets = [[] for _ in range(mod)]
                for b in range(p**H):
                    buckets[pow(b, n, mod)].append(b)
                    residues += 1
                for a in range(mod):
                    m = vp(a, p) if a else 0
                    unit = a // p**m
                    relative = r-m
                    status = 0
                    ids, bs = [], []
                    j = m//n
                    E = j+relative-s
                    if n == 1:
                        E = r
                        ids, bs = [0], [unit]
                    elif a == 0 or relative < c+s:
                        status = 1
                    elif m % n or not buckets[a]:
                        status = 7
                    else:
                        branches = {}
                        for b in buckets[a]:
                            identifier = (b//p**j) % (4 if p == 2 else p)
                            branches.setdefault(identifier, set()).add((b % p**E)//p**j)
                        assert len(branches) == gcd(n, 2 if p == 2 else p-1)
                        for identifier in sorted(branches):
                            assert len(branches[identifier]) == 1
                            ids.append(identifier)
                            bs.append(next(iter(branches[identifier])))
                        actual = set(buckets[a])
                        centres = {b*p**j for b in bs}
                        expected = {b for b in range(p**H) if b % p**E in centres}
                        assert actual == expected
                        for b in centres:
                            assert b in actual and b+p**E in actual
                            witnesses += 1
                    rows.append(dict(p=p, n=n, a=unit, m=m, M=r, s=status, E=E, ids=ids, bs=bs))
                    if a == 1 and status == 0 and n > 1:
                        for shift in (-1, 1):
                            rows.append(dict(p=p, n=n, a=a, m=n*shift, M=n*shift+r,
                                             s=0, E=shift+E, ids=ids, bs=bs))
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(''.join(json.dumps(row, separators=(',', ':'))+'\n' for row in rows))
    assert OUT.stat().st_size < 1_000_000
    print(f'{len(rows)} rows; {residues} exhaustive residues; {witnesses} distance witnesses; '
          f'{OUT.stat().st_size} bytes')


def exact_cases():
    rows = []
    for p in (2, 3, 5, 7):
        for n in range(2, 13):
            N = 4
            H = N+vp(n, p)
            P = p**H
            roots = {}
            for b in range(P):
                if b % p:
                    key = pow(b, n, P)
                    ident = b % (4 if p == 2 else p)
                    group = roots.setdefault(key, {})
                    if ident in group:
                        assert group[ident] == b % p**N
                    group[ident] = b % p**N
            inputs = {Fraction(a, d) for a in range(-11, 12) if a and a % p
                      for d in (1, 2, 3) if d % p}
            for q in sorted(inputs):
                qmod = (q.numerator*pow(q.denominator, -1, P)) % P
                branches = roots.get(qmod, {})
                ids = sorted(branches)
                assert not ids or len(ids) == gcd(n, 2 if p == 2 else p-1)
                rationals = {}
                # Small rational-root oracle by exhaustive INTEGER powers, without fmpz_root.
                for a in range(-11, 12):
                    for d in range(1, 4):
                        b = Fraction(a, d)
                        if b**n == q:
                            ident = (b.numerator*pow(b.denominator, -1, 4 if p == 2 else p))
                            rationals[ident % (4 if p == 2 else p)] = b
                rows.append(dict(p=p, n=n, an=q.numerator, ad=q.denominator, N=N,
                                 s=0 if ids else 7, ids=ids, bs=[branches[i] for i in ids],
                                 qn=[rationals[i].numerator if i in rationals else 0 for i in ids],
                                 qd=[rationals[i].denominator if i in rationals else 0 for i in ids]))
    path = OUT.with_name('exact.jsonl')
    path.write_text(''.join(json.dumps(row, separators=(',', ':'))+'\n' for row in rows))
    assert path.stat().st_size+OUT.stat().st_size < 1_000_000
    print(f'{len(rows)} exact rational inputs; output N=4, power comparison H=4+v_p(n); '
          f'{path.stat().st_size} bytes')


if __name__ == '__main__':
    generate()
    exact_cases()
