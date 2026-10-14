#!/usr/bin/env python3
"""Vectors of lane t5-repair1: timeout 300 python3 -B lanes/t5-repair1/gen_vectors.py

Writes tests/ref/vectors/t5-repair1/jbound.jsonl. An evaluation of one attempt of the balanced split for zeta
(C = 1, e = 0) written for this lane from the statements, with mpmath at 60 digits; no adelefeld code and no
proto/ oracle is imported. Sources, read before this file:
  - docs/proofs/analysis.md:215-221 (Lemma 6, S_j(c, T), the ratio rho_j(c, K) = exp(j/K - c(2K + 1)) and the
    prefix terms before the first K with rho <= 1/2);
  - docs/proofs/analysis.md:630-639 (Lemma 14, both branches of J_bound);
  - docs/proofs/analysis.md:652-661 (Proposition 15, E_sum and E_integral);
  - docs/api-5.md:196-220 (the searches N = 0, 1, 2, 4, ... and R = 1, 2, 4, ... to epsilon/32, the panels
    [2^k, 2^(k+1)], the majorant B, the panel error 2 h B q^(J+1)/(1 - q) <= epsilon/(64 panels));
  - docs/api-5b.md:146-148 (decision 5, the work units).
One record per (s, bits): N, R, K, the panel degrees of both sides, the total bound E_n + E_t + both quadrature
remainders that tt_attempt adds to both radii, the work charged, and the branch of L14 taken at every search
step. The generator evaluates the same quantities for two mutants of src/tate.c and records which of them the
record tells apart (a changed N, R, degree, work or a bound off by more than 10^-6 relative):
  m247: tt_jbound's second branch with exp(-(R0 - R)) for exp(-b R0) (lane t5-slice4, survivor 247);
  m551: the upper bound of Re(z') dropped, so r2 = 0 - 1 = -1 (lane t5-slice2, survivor 551).
Every comparison the C code decides (a branch, a search step, a degree, rho <= 1/2) is asserted to hold with a
relative margin of 10^-9, so that the 64-bit balls of the C code decide it the same way.
"""
from fractions import Fraction as F
import json
import os

import mpmath as mp

mp.mp.dps = 60
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
OUT = os.path.join(ROOT, 'tests', 'ref', 'vectors', 't5-repair1', 'jbound.jsonl')
MARGIN = mp.mpf('1e-9')


def decide(x, y):
    """x <= y, asserted to be far from equality."""
    assert abs(x - y) > MARGIN * max(abs(x), abs(y)), (x, y)
    return x <= y


def s_series(j, c, N, steps):
    """Lemma 6 with beta = 0 as tt_series runs it: one step per K, the last included."""
    K = N + 1
    prefix = mp.mpf(0)
    while True:
        steps[0] += 1
        rho = mp.exp(mp.mpf(j) / K - c * (2 * K + 1))
        term = mp.mpf(K) ** j * mp.exp(-c * K * K)
        if decide(rho, mp.mpf(1) / 2):
            return prefix + term / (1 - rho)
        prefix += term
        K += 1


def jbound(r, b, R, mut, branches):
    """Lemma 14 (analysis.md:630-639). mut = 'm247' replants survivor 247 of t5-slice4."""
    rp = 2 * max(r, 0)
    if decide(rp, b * R) or rp == 0:                      # b R >= 2 max(r, 0)
        branches.append(1)
        return 2 * mp.mpf(R) ** r * mp.exp(-b * R) / b
    branches.append(2)
    assert r > 0
    R0 = 2 * r / b
    ts = min(max(r / b, mp.mpf(R)), R0)
    first = (R0 - R) * ts ** r * mp.exp(-b * ts)
    expo = -(R0 - R) if mut == 'm247' else -b * R0
    return first + 2 * R0 ** r * mp.exp(expo) / b


def attempt(s, bits, mut=None):
    """One attempt of tt_attempt for zeta (C = 1, e = 0, mode Lambda) at a real dyadic s."""
    C, e = 1, 0
    eps = mp.mpf(2) ** (-bits)
    a0 = mp.pi / C
    ea0 = mp.exp(a0)
    z = (s + e) / 2
    zd = (1 - s + e) / 2
    r1 = mpq(z - 1)
    r2 = mp.mpf(-1) if mut == 'm551' else mpq(zd - 1)
    tg = eps / 32
    work = C + C * C + 2 * (e + 1) ** 2 - e
    l6 = [0]
    nbr, rbr = [], []
    N = 0
    while True:
        work += 1
        S = s_series(e, a0, N, l6)
        br = []
        En = ea0 * S * (jbound(r1, a0, 1, mut, br) + jbound(r2, a0, 1, mut, br))
        nbr.append(br)
        if decide(En, tg):
            break
        N = 2 * N if N else 1
    S0 = s_series(e, a0, 0, l6)
    R = 1
    while True:
        work += 1
        br = []
        Et = ea0 * S0 * (jbound(r1, a0, R, mut, br) + jbound(r2, a0, R, mut, br))
        rbr.append(br)
        if decide(Et, tg):
            break
        R *= 2
    work += l6[0] + 2 * N
    K = 0
    while 2 ** K < R:
        K += 1
    tp = eps / 64 / K if K else None
    degs, qerr = [], mp.mpf(0)
    for zz in (z, zd):                                    # the T3 majorant uses the true Re(z') - 1
        r = mpq(zz - 1)
        deg = []
        for k in range(K):
            x = mp.mpf(3) * mp.mpf(2) ** (k - 2)          # m - d
            P = max(x ** r, (3 * x) ** r)
            B = sum(mp.mpf(n) ** e * mp.exp(-a0 * n * n * x) for n in range(1, N + 1)) * P
            err = 6 * mp.mpf(2) ** (k - 1) * B * mp.mpf(2) / 3
            J = 0
            if N == 0:
                err = mp.mpf(0)
            while not (N == 0 or decide(err, tp)):
                J += 1
                err *= mp.mpf(2) / 3
            deg.append(J)
            qerr += err
            work += (J + 1) * N                           # every coefficient of zeta is nonzero
        degs.append(deg)
    return {'N': N, 'R': R, 'K': K, 'degrees': degs, 'bound': En + Et + qerr, 'En': En, 'Et': Et,
            'work': work, 'nbr': nbr, 'rbr': rbr}


def mpq(x):
    return mp.mpf(x.numerator) / x.denominator


def differs(a, b):
    return (a['N'], a['R'], a['degrees'], a['work']) != (b['N'], b['R'], b['degrees'], b['work']) or \
        abs(a['bound'] - b['bound']) > mp.mpf('1e-6') * a['bound']


def qstr(x):
    x = F(x)
    return str(x.numerator) if x.denominator == 1 else '%d/%d' % (x.numerator, x.denominator)


def main():
    records = []
    for s in (F(41, 2), F(9, 1), F(-39, 2), F(-8, 1), F(2, 1)):
        for bits in (4, 20, 53, 80):
            a = attempt(s, bits)
            kills = [m for m in ('m247', 'm551') if differs(a, attempt(s, bits, m))]
            records.append({'kind': 'jbound', 'q': 1, 'n': 1, 's': qstr(s), 'bits': bits, 'N': a['N'],
                            'R': a['R'], 'K': a['K'], 'degrees': a['degrees'],
                            'bound': mp.nstr(a['bound'], 25), 'En': mp.nstr(a['En'], 25),
                            'Et': mp.nstr(a['Et'], 25), 'work': a['work'],
                            'nbranches': a['nbr'], 'rbranches': a['rbr'], 'kills': kills})
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, 'w') as f:
        for r in records:
            f.write(json.dumps(r, separators=(',', ':')) + '\n')
    for r in records:
        print(r['s'], r['bits'], 'N', r['N'], 'R', r['R'], 'K', r['K'], 'work', r['work'], 'bound', r['bound'],
              'nbr', r['nbranches'][-1], 'rbr', r['rbranches'], 'kills', r['kills'])
    for m in ('m247', 'm551'):
        assert any(m in r['kills'] for r in records), m
    print(len(records), 'records ->', OUT)


if __name__ == '__main__':
    main()
