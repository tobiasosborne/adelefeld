"""Exact oracle for 1F.7. No FLINT and no C evaluator is used.

Definition 1 and Lemma 5, docs/proofs/functions.md:20-27,117-138, give the series and tail bound.
Own proof of the reference count: for k >= 1, v(x^k/k!) >= k*w-(k-1)/(p-1).
This bound increases with k on the domain. Stop before the first degree for which it is
at least N+3; every later term, including either parity subsequence, lies in p^(N+3) Z_p.
The ultrametric inequality and completeness bound the whole tail. Sum Fractions exactly,
then reduce only the final rational modulo p^N. This uses more terms than the C count.

Enumeration: p=2,3,5,7, all domain balls with c<=M<=c+2, and all their representatives
modulo p^(c+3). Values are computed at H=2*(c+2)+1-v_p(2), at least E+1 for every hull.
Finite enumeration is evidence, not a replacement for F12's proof for every point in Q_p.

Hensel roots: start with r^2=-1 mod p and lift r -> r+t*p^j, where
 t=-(r*r+1)/p^j * (2*r)^(-1) mod p. Expansion proves the next digit, and 2*r is a unit.
For exp(i*x) tests at requested N take H=N+2. The true root differs by p^H;
its product with x differs by p^(H+v(x)), so Proposition 10 makes the exp values agree
modulo p^N. Division by 2*i loses no digits at p=5,13. The hyperbolic identity uses
exp at N+v_p(2), so dividing the sum/difference by 2 leaves N absolute digits.
The identities follow by selecting even/odd terms of exp; convergence permits regrouping
(Lemma 3, functions.md:56). These are own proofs, not citations from memory.

Run with --generate to write the fixtures, or without it for independent oracle checks.
"""
from fractions import Fraction
from pathlib import Path
import json
import random
import sys

from lfunc_checks import c_of, val, residue, lb_exact, lb_ball, lb_value, domain_status, exp_point

FUNCS = ('sin', 'cos', 'sinh', 'cosh')
DEST = Path('tests/ref/vectors/f-slice7')


def point(f, p, x, N):
    x = Fraction(x)
    if N <= 0:
        return 0
    odd = f in ('sin', 'sinh')
    if not x:
        return 0 if odd else 1
    w = val(x, p)
    assert w >= c_of(p)
    term, total, k = Fraction(1), Fraction(0), 0
    while k == 0 or k*w*(p-1)-(k-1) < (N+3)*(p-1):
        if k % 2 == int(odd):
            sign = -1 if f in ('sin', 'cos') and (k//2) % 2 else 1
            total += sign * term
        k += 1
        term *= x / k
    return residue(total, p, N)


def exponent(f, x):
    M = x['N']
    return 2*M-(x['p'] == 2) if f in ('cos', 'cosh') and not x['un'] else M


def result(f, x, N):
    st = domain_status('exp', x)
    if st != 'OK':
        return st, None
    a, p = lb_value(x), x['p']
    if x['exact'] and a == 0:
        return 'OK', lb_exact(p, int(f in ('cos', 'cosh')))
    K = N if x['exact'] else min(N, exponent(f, x))
    return 'OK', lb_ball(p, point(f, p, a, K), K)


def root_i(p, H):
    r = next(r for r in range(1, p) if (r*r+1) % p == 0)
    mod = p
    for _ in range(1, H):
        t = (-(r*r+1)//mod * pow(2*r, -1, p)) % p
        r += t*mod
        mod *= p
    assert (r*r+1) % mod == 0
    return r


def case(f, p, a, M, N):
    x = lb_exact(p, a) if M is None else lb_ball(p, a, M)
    st, y = result(f, x, N)
    return dict(f=f, x=x, N=N, status=st, y=y)


def generate():
    DEST.mkdir(parents=True, exist_ok=True)
    cases = []
    for p in (2, 3, 5, 7, 13):
        c = c_of(p)
        for f in FUNCS:
            for a in (0, 1, p, p**c, -p**c, Fraction(p**c, p+1), p**(c+2)):
                for M in (None, c-1, c, c+2):
                    for N in (-2, 0, 1, c+1, 8):
                        cases.append(case(f, p, a, M, N))
    rng = random.Random(107)
    for j in range(1200):
        p = rng.choice((2, 3, 5, 7, 13))
        a = Fraction(rng.randrange(-500, 501)*p**rng.randrange(c_of(p), c_of(p)+3),
                     rng.choice([d for d in range(1, 25) if d % p]))
        M = None if j % 2 else rng.randrange(c_of(p), 9)
        cases.append(case(FUNCS[j % 4], p, a, M, rng.randrange(-2, 33)))
    p = 2**64-59
    for f in FUNCS:
        for N in (1, 2, 8, 20, 80, 200):
            for a in (p, Fraction(-p, 3)):
                cases.append(case(f, p, a, None, N))
    # Thousand-bit signed rational numerators and denominators, still short enough for exact arithmetic.
    for p in (2, 3, 5, 7):
        d = 2**1001+1
        while d % p == 0:
            d += 2
        for f in FUNCS:
            cases.append(case(f, p, Fraction(-(2**1100+7)*p**c_of(p), d), None, 24))
    grids = []
    for p in (2, 3, 5, 7):
        c = c_of(p)
        H = 2*(c+2)+1-(p == 2)
        for t in range(0, p**(c+3), p**c):
            grids.append(dict(p=p, t=t, H=H, values=[point(f, p, t, H) for f in FUNCS]))
    roots = [dict(p=p, N=N, H=N+2, i=root_i(p, N+2)) for p in (5, 13) for N in (2, 8, 20, 32)]
    for name, rows in (('cases', cases), ('points', grids), ('roots', roots)):
        path = DEST / (name+'.jsonl')
        path.write_text(''.join(json.dumps(r, separators=(',', ':'))+'\n' for r in rows))
        print(f'{name}: {len(rows)} rows, {path.stat().st_size} bytes')
    assert sum(p.stat().st_size for p in DEST.glob('*.jsonl')) < 1000000


def selftest():
    assert point('cos', 2, 4, 4) == 9
    assert point('sin', 3, 3, 2) == 3
    assert val(point('cos', 3, 3, 5)-1, 3) == 2
    identities = 0
    for p in (2, 3, 5, 7, 13):
        for t in range(-5, 6):
            x = Fraction(t*p**c_of(p), p+1)
            for N in (2, 8, 16):
                e = exp_point(p, x, N+(p == 2))
                en = exp_point(p, -x, N+(p == 2))
                assert point('sinh', p, x, N) == residue(Fraction(e-en, 2), p, N)
                assert point('cosh', p, x, N) == residue(Fraction(e+en, 2), p, N)
                identities += 2
                if p in (5, 13):
                    i = root_i(p, N+2)
                    ep = exp_point(p, i*x, N)
                    em = exp_point(p, -i*x, N)
                    assert point('sin', p, x, N) == residue(Fraction(ep-em, 2*i), p, N)
                    assert point('cos', p, x, N) == residue(Fraction(ep+em, 2), p, N)
                    identities += 2
    print(f'oracle: 3 independent truncations, {identities} identity checks, 0 failures')


if __name__ == '__main__':
    selftest()
    if '--generate' in sys.argv:
        generate()
