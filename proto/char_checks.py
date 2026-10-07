#!/usr/bin/env python3
"""Exact character oracle for docs/api-3c.md; run under timeout 120.

Only FLINT labels/exponents are borrowed. Lowering independently searches for the
smallest modulus through which the exact values factor, then matches exact phases.
Gauss sums use mpmath at 60 digits and an independent python-flint sum at 256 bits.
The returned error is an EXACT rational L1 bound around the actual mpmath result,
computed from that certified rectangle. Hull comparisons use margin 1e-50 only.

Sources: refs/src/flint-3.0.1/acb_dirichlet.rst:328-380, acb.rst:6-18,637-643,663-668;
/usr/include/flint/dirichlet.h:51,68,84,110-136,139,162 (explicit brief exception).
[source pending: FLINT 3.0.1 dirichlet pairing and exponent implementation under refs/].
No adelefeld code is imported. The C evidence adapter is compiled and run by main;
its stdout is compared with this oracle, not used to construct oracle answers.
"""
from fractions import Fraction as F
from functools import lru_cache
from math import gcd, lcm
from pathlib import Path
import re
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
import flint
import mpmath as mp

ROOT = Path(__file__).resolve().parents[1]
ORACLE_MAX = 512                     # reference enumeration bound, not an API restriction
mp.mp.dps = 60
flint.ctx.threads = 1
MARGIN = mp.mpf('1e-50')
COUNTS = {}


def check(group, condition):
    if not condition:
        raise AssertionError(f'{group}: failed after {COUNTS.get(group, 0)} checks')
    COUNTS[group] = COUNTS.get(group, 0) + 1


def mpq(x):
    x = F(x)
    return mp.mpf(x.numerator) / x.denominator


def dyadic(m, e):
    return F(int(m)) * F(2) ** int(e)


def exact_mpf(x):
    sign, mantissa, exponent, _ = x._mpf_
    return dyadic(-mantissa if sign else mantissa, exponent)


def arb_interval(x):
    m = dyadic(*x.mid().man_exp())
    r = dyadic(*x.rad().man_exp())
    return m-r, m+r


def phase_acb(theta):
    theta = F(theta) % 1
    # Rational argument is enclosed before the transcendental evaluation.
    return (flint.acb(flint.arb(2*theta.numerator)/theta.denominator)).exp_pi_i()


def E(theta):
    theta = F(theta) % 1
    if theta.denominator in (1, 2, 4):
        return (mp.mpc(1), mp.mpc(0, 1), mp.mpc(-1), mp.mpc(0, -1))[int(theta*4)]
    return mp.expjpi(2*mpq(theta))


def validate(q, n):
    if q >= 2**64:
        raise OverflowError('UNSUPPORTED: q does not fit ulong')
    if q < 1 or n < 0 or gcd(n, q) != 1:
        raise ValueError('DOMAIN')
    return 1 if q == 1 else n % q


@lru_cache(None)
def raw_phases(q, n):
    n = validate(q, n)
    if q > ORACLE_MAX:
        raise OverflowError('reference enumeration limit, not a library status')
    if q == 1:
        return (F(0),)
    ch = flint.dirichlet_char(q, n)
    L = int(ch.group().exponent())
    return tuple(None if gcd(a, q) != 1 else F(int(ch.chi_exponent(a)), L) % 1
                 for a in range(q))


@lru_cache(None)
def lower(q, n):
    """Return primitive (C, label, parity, character_order); n may be unreduced.

    Exact equality is tested on units modulo q, not on non-units where extension
    by zero changes when the conductor is lowered. No floating-point matching.
    """
    n = validate(q, n)
    if n == 1:
        return 1, 1, 0, 1  # header:144-147, the principal label; also handles large q
    vals = raw_phases(q, n)
    units = [a for a in range(q) if vals[a] is not None]
    # Triviality on the kernel is necessary and sufficient for descent.
    C = next(d for d in range(1, q+1) if q % d == 0 and
             all(vals[a] == 0 for a in units if a % d == 1 % d))
    for label in range(1, C+1):
        if gcd(label, C) != 1:
            continue
        prim = raw_phases(C, label)
        if all(prim[a % C] == vals[a] for a in units):
            parity = int(2*prim[-1 % C])
            order = lcm(*(v.denominator for v in prim if v is not None))
            return C, label, parity, order
    raise AssertionError('no exact inducing label')


def chi_exponent(C, n, a):
    """k in [0,order), chi(a)=E(k/order), or None for zero; primitive input only."""
    c, label, _, order = lower(C, n)
    if (c, label) != (C, n):
        raise ValueError('expected a canonical primitive pair')
    theta = raw_phases(C, n)[a % C]
    return None if theta is None else int(theta*order)


def coset_data(C, n, c, N):
    """(base phase, image subgroup order); uses a compatible UNIT lift, not chi(c)."""
    _, _, _, order = lower(C, n)
    if N < 0 or (N == 0 and c not in (-1, 1)) or (N > 0 and gcd(c, N) != 1):
        raise ValueError('invalid unit coset')
    if N == 0:
        return F(chi_exponent(C, n, c), order), 1
    g = gcd(C, N)
    vals = raw_phases(C, n)
    a0 = next(a for a in range(C) if vals[a] is not None and (a-c) % g == 0)
    d = order
    for a, theta in enumerate(vals):
        if theta is not None and (a-1) % g == 0:
            d = gcd(d, int(theta*order))
    return vals[a0], order//d


def coset_values(C, n, c, N):
    """Exact set of phases in [0,1) on c U(N); N=0 accepts only c=+/-1."""
    b, m = coset_data(C, n, c, N)
    return frozenset((b+F(j, m)) % 1 for j in range(m))


def strict_coset(out, C, n, c, N):
    """Reference transaction only; models a mutable output slot, not C ball rounding."""
    vals = coset_values(C, n, c, N)
    if len(vals) != 1:
        return 'NOT_DETERMINED'
    out[0] = vals
    return 'OK'


def brute_coset(C, n, c, N):
    # Independent lifting through units modulo lcm(C,N), avoiding the gcd shortcut.
    if N == 0:
        return frozenset([raw_phases(C, n)[c % C]])
    L = lcm(C, N)
    return frozenset(raw_phases(C, n)[a % C] for a in range(L)
                     if gcd(a, L) == 1 and (a-c) % N == 0)


def coset_hull(C, n, c, N):
    b, m = coset_data(C, n, c, N)
    def distance(t):
        f = (m*(t-b)) % 1
        return min(f, 1-f)/m
    cs = lambda t: E(distance(t)).real
    return (-cs(F(1, 2)), cs(F(0)), -cs(F(3, 4)), cs(F(1, 4)))


def idele_values(C, n, x_inf, r, c, N, s):
    """Point real input; exact rational class coordinates, then numerical family member."""
    if x_inf == 0 or r <= 0:
        raise ValueError('not an idele')
    u = c if x_inf > 0 else -c
    t = abs(F(x_inf))/r
    return [mpq(t)**s*E(theta) for theta in sorted(coset_values(C, n, u, N))]


@lru_cache(None)
def gauss(C, n):
    """(mpmath complex at 60 digits, exact Fraction Euclidean error bound).

    The bound is the sum of coordinate distances to certified 256-bit endpoints;
    no correctness or rounding assumption about mpmath is used for certification.
    """
    _, _, _, order = lower(C, n)
    if lower(C, n)[:2] != (C, n):
        raise ValueError('primitive pair required')
    phases = [(F(k, order)+F(a, C)) % 1 for a in range(C)
              if (k := chi_exponent(C, n, a)) is not None]
    with mp.workdps(60):
        z = sum((E(t) for t in phases), mp.mpc(0))
    with flint.ctx.workprec(256):
        certified = sum((phase_acb(t) for t in phases), flint.acb(0))
        error = F(0)
        for x, coord in zip((z.real, z.imag), (certified.real, certified.imag)):
            lo, hi = arb_interval(coord)
            x = exact_mpf(x)
            error += max(abs(x-lo), abs(x-hi))
    return z, error


def primitive_pairs(bound):
    return [(C, n) for C in range(1, bound+1) for n in range(1, C+1)
            if gcd(C, n) == 1 and lower(C, n)[:2] == (C, n)]


def root_from_input(q, n):
    """Root number of the primitive character inducing the input, at 60 digits."""
    C, label, parity, _ = lower(q, n)
    return gauss(C, label)[0]/(mp.j**parity*mp.sqrt(C))


def gauss_sum_ball(C, n, p):
    """P4 exact endpoint additions and one final Q1 rounding; rational (mid,rad) pairs.

    Individual phase balls below model the phase API using certified FLINT values
    and the SAME specified midpoint/radius kernel, not a call into adelefeld.
    """
    from quotient3_checks import q1_radius, round_binary
    w = max(p, 2)+(C-1).bit_length()+8
    def rounded(lo, hi):
        center = (lo+hi)/2
        mid = round_binary(abs(center), w)
        if center < 0:
            mid = -mid
        return mid, q1_radius(max(mid-lo, hi-mid))
    order = lower(C, n)[3]
    lows, highs = [F(0), F(0)], [F(0), F(0)]
    with flint.ctx.workprec(w+64):
        for a in range(C):
            k = chi_exponent(C, n, a)
            if k is None:
                continue
            z = phase_acb(F(k, order)+F(a, C))
            for j, part in enumerate((z.real, z.imag)):
                mid, rad = rounded(*arb_interval(part))
                lows[j] += mid-rad
                highs[j] += mid+rad
    return tuple(rounded(lo, hi) for lo, hi in zip(lows, highs)), w


def check_sum_bound():
    for C, n in primitive_pairs(16):
        tau, error = gauss(C, n)
        for p in [2, 20, 53, 128]:
            ball, w = gauss_sum_ball(C, n, p)
            for (mid, rad), val in zip(ball, (tau.real, tau.imag)):
                check('sum_bound', rad <= 8*C*F(2)**(-w))
                check('sum_bound', mid-rad <= exact_mpf(val)+error and
                      mid+rad >= exact_mpf(val)-error)
                if p == 128:
                    check('sum_bound', rad < F(1, 2**60))
    check('sum_bound', gauss_sum_ball(1, 1, 128)[0] == ((F(1), F(0)), (F(0), F(0))))
    check('sum_bound', gauss_sum_ball(4, 3, 128)[0] == ((F(0), F(0)), (F(2), F(0))))


def check_lowering():
    examples = {(1, 0): (1, 1, 0, 1), (16, 9): (8, 5, 0, 2), (8, 7): (4, 3, 1, 2),
                (10, 3): (5, 3, 1, 4), (5, 7): (5, 2, 1, 4), (3, 1): (1, 1, 0, 1),
                (6, 5): (3, 2, 1, 2)}
    for inp, out in examples.items():
        check('constructors', lower(*inp) == out)
    check('constructors', lower(65536, 1) == (1, 1, 0, 1))
    for q, n, exc in [(0, 1, ValueError), (6, 2, ValueError), (6, 0, ValueError),
                      (2**64, 1, OverflowError)]:
        try:
            lower(q, n)
        except exc:
            check('constructors', True)
        else:
            check('constructors', False)
    for q in range(1, 81):
        for n in range(1, q+1):
            if gcd(q, n) != 1:
                continue
            C, m, e, order = lower(q, n)
            fch = flint.dirichlet_char(q, n)
            check('lowering', C == int(fch.conductor()))
            check('lowering', e == int(fch.parity()) and order == int(fch.order()))
            check('lowering', lower(C, m) == (C, m, e, order))
            check('lowering', all(raw_phases(q, n)[a] == raw_phases(C, m)[a % C]
                                 for a in range(q) if gcd(a, q) == 1))


def check_characters():
    with flint.ctx.workprec(256):
        for C, n in primitive_pairs(40):
            order = lower(C, n)[3]
            inv = 1 if C == 1 else pow(n, -1, C)
            ch = flint.dirichlet_char(C, n)
            for a in range(-C, C+1):
                k = chi_exponent(C, n, a)
                check('characters', (k is None) == (gcd(a, C) != 1))
                if k is not None:
                    check('characters', (k+chi_exponent(C, inv, a)) % order == 0)
                    check('characters', phase_acb(F(k, order)).overlaps(ch(a % C)))
                else:
                    check('characters', ch(a % C).is_zero())
            units = [a for a in range(C) if gcd(a, C) == 1]
            for a in units:
                for b in units:
                    check('characters', (chi_exponent(C, n, a)+chi_exponent(C, n, b) -
                                         chi_exponent(C, n, a*b)) % order == 0)


def check_cosets():
    for C, n in primitive_pairs(24):
        cases = [(1, 0), (-1, 0)] + [(c, N) for N in range(1, 13) for c in range(1, N+1)
                                               if gcd(c, N) == 1]
        for c, N in cases:
            vals = coset_values(C, n, c, N)
            check('cosets', vals == brute_coset(C, n, c, N))
            check('cosets', (len(vals) == 1) == (N == 0 or N % C == 0))
            if N % 4 == 2:
                check('cosets', vals == coset_values(C, n, c, N//2))
            hull = coset_hull(C, n, c, N)
            zs = [E(t) for t in vals]
            brute = (min(z.real for z in zs), max(z.real for z in zs),
                     min(z.imag for z in zs), max(z.imag for z in zs))
            for a, b in zip(hull, brute):
                check('hulls', abs(a-b) < MARGIN)
    # Large N is reduced modulo C; this test does not enumerate N residues.
    check('cosets', coset_values(5, 2, 1, 5*(2**2000)) == {F(0)})


def check_evaluation():
    # Ball-power enclosure against independent point evaluation, including complex s.
    with flint.ctx.workprec(256):
        for C, n, c, N in [(5, 2, 2, 5), (5, 2, 1, 1), (3, 2, 3, 4)]:
            vals = coset_values(C, n, c, N)
            rect = flint.acb(flint.arb(0, 1), flint.arb(0, 1))
            if len(vals) == 1:
                rect = phase_acb(next(iter(vals)))
            for sr, si in [(0, 0), (1, 0), (F(1, 2), 0), (0, 1), (1, 1)]:
                S = flint.acb(flint.arb(str(sr)), flint.arb(str(si)))
                box = flint.acb(flint.arb('2 +/- 0.5')) ** S * rect
                for t in [F(3, 2), F(7, 4), F(2), F(9, 4), F(5, 2)]:
                    for phase in vals:
                        point = flint.acb(flint.arb(t.numerator)/t.denominator) ** S * phase_acb(phase)
                        check('evaluation', box.overlaps(point))
    # Exact negative-real class map: (-2; 1*[1]) -> (2,[-1]).
    check('evaluation', idele_values(3, 2, -2, F(1), 1, 0, 1) == [-2])
    # Diagonal -1 has both signs, hence unit +1 and value 1.
    check('evaluation', idele_values(3, 2, -1, F(1), -1, 0, 1) == [1])
    check('evaluation', E(F(1, 4)) == mp.j)  # <1;[2 mod 5]>, (5,2), s=0
    for rat in [F(-7, 3), F(-1), F(1, 2), F(5)]:
        for s in [mp.mpc(0), mp.mpc(1), mp.mpc(1, 1)]:
            v = idele_values(5, 2, -2, F(3), 2, 5, s)
            scaled = idele_values(5, 2, -2*rat, 3*abs(rat), 2 if rat > 0 else -2, 5, s)
            check('evaluation', all(abs(a-b) < MARGIN for a, b in zip(v, scaled)))


def parse_dump_interval(s):
    m, e, r, rexp = (int(t, 16) for t in s.split())
    mid, rad = dyadic(m, e), dyadic(r, rexp)
    return mid-rad, mid+rad


def check_flint_c():
    # A fresh adapter avoids trusting a saved transcript. Both subprocesses are bounded.
    with tempfile.TemporaryDirectory(prefix='.oracle-', dir=ROOT/'lanes/d-char') as td:
        exe = str(Path(td)/'probe')
        cmd = ['cc', '-std=c11', '-O1', '-Wall', '-Wextra', '-Werror',
               str(ROOT/'lanes/d-char/flint_probe.c'), '-lflint', '-lmpfr', '-lgmp', '-o', exe]
        subprocess.run(cmd, check=True, timeout=30, capture_output=True)
        lines = subprocess.run([exe], check=True, timeout=30, capture_output=True, text=True).stdout.splitlines()
    check('flint_c', lines[0] == 'FLINT 3.0.1 3.0.1')
    check('flint_c', lines[1] == 'ABI 120 8 0 8 16 24')
    for line in lines[2:]:
        head, exponents, re_part, im_part = line.split('\t')
        q, n, C, m, e, order, L = map(int, head.split())
        check('flint_c', lower(q, n) == (C, m, e, order))
        for a, raw in enumerate(exponents.split(',')):
            k = chi_exponent(C, m, a)
            check('flint_c', (k is None and raw == '-') or
                  (k is not None and raw != '-' and F(k, order) == F(int(raw), L)))
        z, err = gauss(C, m)
        for x, field in zip((z.real, z.imag), (re_part, im_part)):
            lo, hi = parse_dump_interval(field)
            exact = exact_mpf(x)
            check('flint_c', exact+err >= lo and exact-err <= hi)
            check('flint_c', hi-lo < F(1, 2**200))


def check_gauss():
    for C, n in primitive_pairs(40):
        _, _, e, _ = lower(C, n)
        inv = 1 if C == 1 else pow(n, -1, C)
        tau, error = gauss(C, n)
        tc, ec = gauss(C, inv)
        W = root_from_input(C, n)
        Wc = tc/(mp.j**e*mp.sqrt(C))
        check('gauss', error < F(C, 10**50))
        check('gauss', abs(abs(tau)**2-C) < C*C*MARGIN)
        check('gauss', abs(tau*tc-(-1)**e*C) < C*C*MARGIN)
        check('gauss', abs(W*Wc-1) < C*MARGIN)
        # Independent positive/negative additive twists, zero and non-units included.
        order = lower(C, n)[3]
        for m in [-2, -1, 0, 1, 2]:
            twisted = sum((E(F(k, order)+F(m*a, C)) for a in range(C)
                           if (k := chi_exponent(C, n, a)) is not None), mp.mpc(0))
            k = chi_exponent(C, n, m)
            target = 0 if k is None else E(-F(k, order))*tau
            check('gauss', abs(twisted-target) < C*MARGIN)


def golden_ball(s):
    parts = re.fullmatch(r'\((.*?)\) \+ \((.*?)\)\*i', s).groups()
    return [tuple(F(t) for t in p.split(' +/- ')) if ' +/- ' in p else (F(p), F(0)) for p in parts]


def check_goldens():
    real_inputs = {(3, 2), (4, 3), (5, 4), (7, 6), (8, 3), (8, 5), (8, 7), (12, 11)}
    rows = 0
    for line in (ROOT/'tests/golden/gauss.tsv').read_text().splitlines():
        if line.startswith('#') or not line:
            continue
        text, expected = line.split('\t')
        q, n = map(int, re.search(r'q=(\d+), n=(\d+)', text).groups())
        C, m, e, _ = lower(q, n)
        tau, err = gauss(C, m)
        W = root_from_input(q, n)
        es, taus, ws = re.fullmatch(r'e=(\d) tau=(.*?) W=(.*)', expected).groups()
        check('goldens', int(es) == e)
        # Certified tau envelope fits each golden coordinate; W uses 60 digits + margin.
        for coord, (mid, rad) in zip((tau.real, tau.imag), golden_ball(taus)):
            check('goldens', abs(exact_mpf(coord)-mid)+err <= rad)
        for coord, (mid, rad) in zip((W.real, W.imag), golden_ball(ws)):
            check('goldens', abs(coord-mpq(mid))+C*MARGIN <= mpq(rad))
        if (q, n) in real_inputs:
            check('goldens', abs(W-1) < C*MARGIN)
        rows += 1
    check('goldens', rows == 17)


def check_faults_findings():
    # Explicit incorrect outputs, each rejected by a mathematical acceptance condition.
    check('faults_33', lower(16, 9)[:2] != (8, 9 % 8))
    check('faults_33', lower(5, 2)[2] != 2 % 2)
    check('faults_33', chi_exponent(5, 2, 5) is None)  # zero cannot be phase zero
    sentinel = [object()]
    before = sentinel[0]
    check('faults_33', strict_coset(sentinel, 5, 2, 1, 1) == 'NOT_DETERMINED' and sentinel[0] is before)
    check('faults_33', 2*E(F(1, 2)) != 2)  # dropped real sign
    vals = sorted(coset_values(5, 2, 1, 1))
    endpoints_min = min(E(vals[0]).real, E(vals[-1]).real)
    check('faults_33', endpoints_min > coset_hull(5, 2, 1, 1)[0]+MARGIN)
    tau, _ = gauss(3, 2)
    check('faults_34', abs(-tau-tau) > 1)  # negative finite sign
    t5, _ = gauss(5, 2)
    check('faults_34', abs(t5-gauss(5, 3)[0]) > 1)  # conjugated character
    check('faults_34', gauss(1, 1)[0] != 0)  # dropping the only C=1 term
    check('faults_34', abs(tau/mp.sqrt(3)-tau/(mp.j*mp.sqrt(3))) > 1)  # missing i^e
    check('faults_34', abs(gauss(4, 3)[0]/mp.sqrt(8)-gauss(4, 3)[0]/2) > mp.mpf('.1'))
    check('faults_34', E(F(2, 2)) != E(F(chi_exponent(5, 4, 2), 2)))  # raw exponent denominator
    check('findings', lower(8, 7) == (4, 3, 1, 2))
    check('findings', chi_exponent(3, 2, 3) is None and coset_values(3, 2, 3, 4) == {F(0), F(1, 2)})
    raw = flint.dirichlet_char(5, 4)
    check('findings', int(raw.chi_exponent(2)) == 2 and int(raw.order()) == 2 and
          int(raw.group().exponent()) == 4 and chi_exponent(5, 4, 2) == 1)


def main():
    print(f'python-flint {flint.__version__}, bundled FLINT {flint.__FLINT_VERSION__}, '
          f'mpmath {mp.__version__}; C adapter requires FLINT 3.0.1', flush=True)
    for fn in [check_lowering, check_characters, check_cosets, check_evaluation, check_flint_c,
               check_gauss, check_sum_bound, check_goldens, check_faults_findings]:
        old = dict(COUNTS)
        fn()
        for name, count in COUNTS.items():
            if count != old.get(name, 0):
                print(f'{name}: {count} checks', flush=True)
    print(f'TOTAL: {sum(COUNTS.values())} checks', flush=True)


if __name__ == '__main__':
    main()
