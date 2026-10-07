#!/usr/bin/env python3
"""Milestone 5 oracle; timeout 120 python3 proto/tate_checks.py.

Exact phases and cyclotomic reductions, 60-digit mpmath references (relative
comparison margin 1e-45), and independently accumulated FLINT ball quadrature.
No adelefeld implementation is imported. No L/zeta/incomplete-Gamma evaluation
is used by continuation(). The optional FLINT reference uses the installed
python-flint version, which is printed; it is not necessarily FLINT 3.0.1.

Project definitions: analysis.md P9-P15; docs/api-5.md T1-T5 proves additions.
External contracts: refs/src/flint-3.0.1/arb.rst:6-12, acb.rst:6-17,893-902;
acb_dirichlet.rst:129-133,524-534,567-569. Upper incomplete Gamma is used only
as a numerical reference, never as an enclosure premise (source still pending).
"""
from dataclasses import dataclass
from fractions import Fraction as F
from functools import lru_cache
from math import gcd, lcm
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
import mpmath as mp
import sympy as sy
try:
    import flint
    from flint import acb, arb, ctx
    ctx.threads = 1
except ImportError:
    flint = None

import analysis_checks as analysis

mp.mp.dps = 60
MARGIN = mp.mpf('1e-45')
COUNTS = {}
ROOT = Path(__file__).resolve().parents[1]


def check(group, condition):
    if not condition:
        raise AssertionError(f'{group}: check {COUNTS.get(group, 0) + 1}')
    COUNTS[group] = COUNTS.get(group, 0) + 1


def close(group, x, y, margin=MARGIN):
    check(group, abs(x-y) <= margin*max(1, abs(y)))


def mpq(x):
    x = F(x)
    return mp.mpf(x.numerator)/x.denominator


def phase(x):
    x = F(x) % 1
    if x.denominator in (1, 2, 4):
        return (mp.mpc(1), mp.mpc(0, 1), mp.mpc(-1), mp.mpc(0, -1))[int(4*x)]
    return mp.expjpi(2*mpq(x))


def dyadic(x):
    sign, m, e, _ = x._mpf_
    return F(-m if sign else m)*F(2)**e


def endpoints(x):
    m, e = x.mid().man_exp()
    r, k = x.rad().man_exp()
    m, r = F(int(m))*F(2)**int(e), F(int(r))*F(2)**int(k)
    return m-r, m+r


def rb(x):
    if isinstance(x, arb):
        return x
    if isinstance(x, mp.mpf):
        x = dyadic(x)
    if isinstance(x, F):
        return arb(x.numerator)/x.denominator
    return arb(x)


def cb(x):
    if isinstance(x, acb):
        return x
    if isinstance(x, mp.mpc):
        return acb(rb(x.real), rb(x.imag))
    if isinstance(x, F):
        return acb(rb(x))
    return acb(x)


def phase_ball(q):
    return cb(2*(F(q) % 1)).exp_pi_i()


def width(z):
    return max(2*endpoints(z.real.rad())[1], 2*endpoints(z.imag.rad())[1])


def numeric(z):
    return mp.mpc(mpq(sum(endpoints(z.real))/2), mpq(sum(endpoints(z.imag))/2))


@dataclass(frozen=True)
class Character:
    """Primitive finite character: exact angles in Q/Z; None means zero.

    This is an oracle descriptor, not a proposed library value type. The
    constructor from a Conrey pair lowers exactly through char_checks.
    """
    C: int
    phases: tuple
    label: int = 0

    @property
    def e(self):
        return int(2*self.phases[-1 % self.C])

    def __call__(self, n):
        q = self.phases[n % self.C]
        return mp.mpc(0) if q is None else phase(q)

    def ball(self, n):
        q = self.phases[n % self.C]
        return acb(0) if q is None else phase_ball(q)

    def conj(self):
        return Character(self.C, tuple(None if q is None else -q % 1 for q in self.phases),
                         pow(self.label, -1, self.C) if self.C > 1 and self.label else self.label)


@lru_cache(None)
def character(q, n=1):
    if flint is not None:
        import char_checks as chars
        C, label, _, _ = chars.lower(q, n)
        return Character(C, chars.raw_phases(C, label), label)
    # Explicit small tables permit the numerical and exact checks without FLINT.
    tables = {(1, 1): (0,), (3, 2): (None, 0, F(1, 2)),
              (4, 3): (None, 0, None, F(1, 2)),
              (5, 2): (None, 0, F(1, 4), F(3, 4), F(1, 2)),
              (5, 4): (None, 0, F(1, 2), F(1, 2), 0)}
    table = tables[(q, n)]
    return Character(q, tuple(None if x is None else F(x) for x in table), n)


def gauss(chi, negative=False):
    return sum((chi(u)*phase(F((-1 if negative else 1)*u, chi.C))
                for u in range(chi.C)), mp.mpc(0))


def root_number(chi):
    return gauss(chi)/(mp.j**chi.e*mp.sqrt(chi.C))


def root_ball(chi):
    tau = sum((chi.ball(u)*phase_ball(F(u, chi.C)) for u in range(chi.C)), acb(0))
    return tau/(acb(0, 1)**chi.e*arb(chi.C).sqrt())


def local_exponent(p, chi_p):
    q, a = chi_p.C, 0
    while q > 1 and q % p == 0:
        q //= p
        a += 1
    if q != 1:
        raise ValueError('DOMAIN: unit conductor is not a power of p')
    return a


def local_integral(p, alpha, chi_p, s):
    """P9 standard vector: eta_0^-1 on units if ramified; 1_Zp otherwise.

    chi_p is eta_0 itself, not the global chi. alpha is a nonzero complex value.
    This returns the meromorphic closed form; exact poles raise ValueError.
    """
    if not alpha:
        raise ValueError('DOMAIN: alpha=0')
    if local_exponent(p, chi_p):
        return mp.mpc(1)
    d = 1-alpha*mp.power(p, -s)
    if not d:
        raise ValueError('DOMAIN: exact pole')
    return 1/d


def local_gamma(p, alpha, chi_p, s):
    a = local_exponent(p, chi_p)
    if a:
        return alpha**a*mp.power(p, -a*s)*gauss(chi_p.conj(), negative=True)
    return (1-alpha*mp.power(p, -s))/(1-mp.power(p, s-1)/alpha)


def local_epsilon(p, alpha, chi_p, s):
    return local_gamma(p, alpha, chi_p, s) if local_exponent(p, chi_p) else mp.mpc(1)


def real_integral(e, s):
    if e not in (0, 1):
        raise ValueError('DOMAIN: parity')
    return mp.pi**(-(s+e)/2)*mp.gamma((s+e)/2)


def real_gamma(e, s):
    return mp.j**e*mp.pi**(s-mp.mpf('.5'))*mp.gamma((1-s+e)/2)*mp.rgamma((s+e)/2)


def exact_gauss(chi, negative=False):
    """Exact polynomial in a cyclotomic root, reduced modulo Phi_m."""
    phases = [(q+F((-1 if negative else 1)*u, chi.C)) % 1
              for u, q in enumerate(chi.phases) if q is not None]
    m = lcm(*(q.denominator for q in phases))
    x = sy.Symbol('X')
    poly = sum(x**int(m*q) for q in phases)
    return m, sy.rem(poly, sy.cyclotomic_poly(m, x), x)


def exact_local(p, chi_p):
    """Rational expressions in A=eta(p), T=p^-s and an exact cyclotomic G."""
    A, T, X = sy.symbols('A T X')
    a = local_exponent(p, chi_p)
    if not a:
        L = 1/(1-A*T)
        gamma = (1-A*T)/(1-1/(p*A*T))
        return L, gamma, sy.Integer(1), 1
    m, G = exact_gauss(chi_p.conj(), negative=True)
    return sy.Integer(1), A**a*T**a*G, A**a*T**a*G, m


def exact_euler_factor(chi, p):
    """(m, expression): X is a primitive m-th root, T=p^-s, all data exact."""
    q = chi.phases[p % chi.C]
    if q is None:
        return 1, sy.Integer(1)
    X, T = sy.symbols('X T')
    return q.denominator, 1/(1-X**q.numerator*T)


def l_reference(chi, s):
    """Independent mpmath Hurwitz reference; no artificial offset at s=1."""
    if chi.C == 1:
        return mp.zeta(s)
    if s == 1:
        # [source pending: Hurwitz constant term -digamma(a); numerical reference only]
        return -sum(chi(a)*mp.digamma(mp.mpf(a)/chi.C)
                    for a in range(1, chi.C+1))/chi.C
    return chi.C**(-s)*sum(chi(a)*mp.zeta(s, mp.mpf(a)/chi.C)
                          for a in range(1, chi.C+1))


def global_value(chi, s):
    """Completed Lambda=C^((s+e)/2) I; 60-digit numerical reference only.

    Use continuation at removable Gamma-times-L singularities. The reference
    product deliberately does not pretend that 0*infinity is an evaluation.
    """
    s = mp.mpc(s)
    return (chi.C/mp.pi)**((s+chi.e)/2)*mp.gamma((s+chi.e)/2)*l_reference(chi, s)


def flint_reference(chi, s):
    s = cb(s)
    return (arb(chi.C)/arb.pi())**((s+chi.e)/2)*((s+chi.e)/2).gamma()*\
        flint.dirichlet_char(chi.C, chi.label).l_function(s)


def series_bound(e, c, N):
    """Outward L6 bound; use lower(c), including the entire prefix."""
    c = rb(endpoints(c)[0])
    return analysis.ball_series_bound(e, c, N)


def integral_bound(r, b, R):
    """L14 with exact rational endpoints; monotonic for t>=R>=1."""
    r, b = endpoints(rb(r))[1], endpoints(rb(b))[0]
    R = F(R)
    rp = max(r, 0)
    if b >= 2*rp/R:
        return 2*rb(R)**rb(r)*(-rb(b)*rb(R)).exp()/rb(b)
    R0 = 2*rp/b
    peak = min(max(r/b, R), R0)
    return rb(R0-R)*rb(peak)**rb(r)*(-rb(b)*rb(peak)).exp()+\
        2*rb(R0)**rb(r)*(-rb(b)*rb(R0)).exp()/rb(b)


def tail_bound(chi, z, N, R, omit_n=True, omit_t=True):
    a = arb.pi()/chi.C
    result = arb(0)
    if omit_n:
        result += a.exp()*series_bound(chi.e, a, N)*integral_bound(z.real-1, a, 1)
    if omit_t:
        result += a.exp()*series_bound(chi.e, a, 0)*integral_bound(z.real-1, a, R)
    return result.upper()


def taylor_piece(chi, z, N, R, target):
    """Own quadrature, T3: dyadic panels, Taylor recurrence, geometric remainder.

    The complex disk is used only to bound coefficients. The finite polynomial
    and its integral use acb. No gamma_upper, zeta or l_function call here.
    """
    a, total, error, work, degrees = arb.pi()/chi.C, acb(0), arb(0), 0, []
    panels = max(1, (R-1).bit_length())
    lo = 1
    while lo < R:
        hi = min(2*lo, R)
        m, h = F(lo+hi, 2), F(hi-lo, 2)
        d, q = m/2, 2*h/m
        r = z.real-1
        powers = max((rb(m-d)**r).upper(), (rb(m+d)**r).upper())
        imag = abs(z.imag).upper()
        B = powers*(arb.pi()*imag/2).exp()*sum(
            (arb(n)**chi.e*(-a*n*n*rb(m-d)).exp() for n in range(1, N+1)), arb(0))
        J = 0
        err = 2*rb(h)*B*rb(q)**(J+1)/(1-rb(q))
        while not err <= target/panels:
            J += 1
            if J > 4096:
                raise OverflowError('oracle Taylor degree cap')
            err = 2*rb(h)*B*rb(q)**(J+1)/(1-rb(q))
        value = acb(0)
        for n in range(1, N+1):
            if chi.phases[n % chi.C] is None:
                continue
            b = a*n*n
            prev, current = acb(0), (-b*rb(m)).exp()*cb(m)**(z-1)
            integral = 2*cb(h)*current
            for k in range(J):
                nxt = ((z-1-b*rb(m)-k)*current-b*prev)/(rb(m)*(k+1))
                prev, current = current, nxt
                if (k+1) % 2 == 0:
                    integral += 2*cb(h)*current*cb(h)**(k+1)/(k+2)
            value += chi.ball(n)*n**chi.e*integral
            work += J+1
        total += value
        error += err
        degrees.append(J)
        lo = hi
    return total, error.upper(), work, tuple(degrees)


@dataclass
class Enclosure:
    ball: object
    N: int
    R: int
    tail: object
    quadrature: object
    work: int
    degrees: tuple


def pole_status(chi, s):
    """Global pole geometry for a finite rectangle; no computed zero tests."""
    if not s.is_finite():
        return 'DOMAIN'
    if chi.C > 1:
        return 'OK'
    if s == 0 or s == 1:
        return 'DOMAIN'
    if s.contains(0) or s.contains(1):
        return 'NOT_DETERMINED'
    return 'OK'


def continuation(chi, s, bits, *, _prec=None):
    """Certified completed Lambda by P13 splitting; returns Enclosure.

    For pole/width failures raises ValueError(status); no partial result.
    Without python-flint the certification path is unavailable and raises
    RuntimeError; the numerical reference functions still work.
    """
    if flint is None:
        raise RuntimeError('python-flint required for certified continuation')
    if not 0 <= bits <= 2048:
        raise ValueError('oracle bits cap (not proposed API cap)')
    w = _prec or max(96, bits+64)
    with ctx.workprec(w):
        s = cb(s)
        status = pole_status(chi, s)
        if status != 'OK':
            raise ValueError(status)
        z, zp = (s+chi.e)/2, (1-s+chi.e)/2
        target = arb(2)**(-bits)
        N = 0
        while True:
            en = sum((tail_bound(chi, v, N, 1, omit_t=False) for v in (z, zp)), arb(0))
            if en <= target/32:
                break
            N = 1 if N == 0 else N*2
            if N > 2048:
                raise OverflowError('oracle series cap')
        R = 1
        while True:
            et = sum((tail_bound(chi, v, 0, R, omit_n=False) for v in (z, zp)), arb(0))
            if et <= target/32:
                break
            R *= 2
            if R > 2**20:
                raise OverflowError('oracle integration cutoff cap')
        left, el, wl, dl = taylor_piece(chi, z, N, R, target/64)
        right, er, wr, dr = taylor_piece(chi.conj(), zp, N, R, target/64)
        value = left+root_ball(chi)*right
        if chi.C == 1:
            value += 1/(s-1)-1/s
        error = (en+et+el+er).upper()
        value += acb(arb(0, error), arb(0, error))
        if not value.is_finite() or width(value) > F(1, 2**bits):
            if _prec is None and s.is_exact():
                return continuation(chi, s, bits, _prec=2*w)
            raise ValueError('NOT_DETERMINED: target width')
        return Enclosure(value, N, R, (en+et).upper(), (el+er).upper(), wl+wr, (dl, dr))


def functional_equation_sides(chi, s, bits=80):
    """Two separate quadratures. No reuse, intersection, or copying a side."""
    with ctx.workprec(max(128, bits+64)):
        s = cb(s)
        left = continuation(chi, s, bits+4).ball
        right = root_ball(chi)*continuation(chi.conj(), 1-s, bits+4).ball
        return left, right


def numeric_piece(chi, z, N, R=mp.inf, scale=None):
    a = mp.pi/chi.C if scale is None else scale
    return sum(chi(n)*n**chi.e*(a*n*n)**(-z)*mp.gammainc(z, a*n*n, a*n*n*R)
               for n in range(1, N+1))


def norm_one_split(chi, s, N=100):
    """Literal P12 split at idele norm 1, independent of P13's balanced split."""
    e, C = chi.e, chi.C
    first = numeric_piece(chi, (s+e)/2, N, scale=mp.pi)
    second = mp.j**(-e)*gauss(chi)*mp.mpf(C)**(-1-e)*numeric_piece(
        chi.conj(), (1-s+e)/2, N, scale=mp.pi/(C*C))
    poles = 1/(s-1)-1/s if C == 1 else 0
    return first+second+poles


def euler_bound(sigma, P):
    """T1 logarithmic omitted-prime bound, valid uniformly for |chi(p)|<=1."""
    sigma, P = mp.mpf(sigma), mp.mpf(P)
    return P**(1-sigma)/((sigma-1)*(1-P**(-sigma)))


def check_local():
    group = 'local_exact'
    A, T = sy.symbols('A T')
    for p in (2, 3, 5, 7):
        L, g, eps, _ = exact_local(p, character(1))
        check(group, sy.cancel(L-(1-A*T)**-1) == 0)
        dual = 1/(1-1/(p*A*T))
        check(group, sy.cancel(g*L/dual-1) == 0 and eps == 1)
    for p, pair in ((2, (4, 3)), (3, (3, 2)), (5, (5, 2)), (5, (5, 4))):
        chi = character(*pair)
        L, g, eps, order = exact_local(p, chi)
        check(group, L == 1 and g == eps)
        a = local_exponent(p, chi)
        q = chi.C
        x = sy.Symbol('X')
        # Exact inverse-vector unit average, and the deliberately wrong same-character vector.
        angles = [t for t in chi.phases if t is not None]
        check(group, all((t-t) % 1 == 0 for t in angles))
        m = lcm(*(t.denominator for t in angles))
        square_avg = sy.rem(sum(x**int(m*((2*t) % 1)) for t in angles), sy.cyclotomic_poly(m, x), x)
        if pair == (5, 2):
            check(group, square_avg == 0)
        # Exact finite transform identity at every residue, in a common cyclotomic ring.
        m = lcm(q, *(t.denominator for t in angles))
        modulus = sy.cyclotomic_poly(m, x)
        G = sum(x**int(m*((-t-F(u, q)) % 1)) for u, t in enumerate(chi.phases) if t is not None)
        for v in range(q):
            actual = sum(x**int(m*((-t-F(u*v, q)) % 1))
                         for u, t in enumerate(chi.phases) if t is not None)
            expected = 0 if gcd(v, q) > 1 else x**int(m*chi.phases[v])*G
            check(group, sy.rem(actual-expected, modulus, x) == 0)
        s, alpha = mp.mpc('1.3', '.4'), phase(F(1, 4))
        close('local_numeric', local_integral(p, alpha, chi, s), 1)
        close('local_numeric', local_gamma(p, alpha, chi, s),
              analysis.local_gamma(p, alpha, chi, a, s))
        close('local_numeric', local_epsilon(p, alpha, chi, s), local_gamma(p, alpha, chi, s))
        # Actual multiplicative shell transform, not a second call to the gamma formula.
        f = [mp.conj(chi(u)) for u in range(q)]
        transformed = analysis.finite_transform(f, 1, q)
        lhs = analysis.local_mellin(transformed, p, a, 0, 1/alpha, chi.conj(), a, 1-s)
        close('local_numeric', lhs, local_gamma(p, alpha, chi, s))
    for p in (2, 3, 7):
        s, alpha = mp.mpc('2.4', '.3'), phase(F(1, 3))
        partial = sum(alpha**k*mp.power(p, -k*s) for k in range(90))
        close('local_numeric', local_integral(p, alpha, character(1), s), partial)
        close('local_numeric', local_epsilon(p, alpha, character(1), s), 1)
    close('local_numeric', local_integral(2, 1, character(1), 2), mp.mpf(4)/3)
    close('local_numeric', real_integral(0, 2), 1/mp.pi)
    for pair in ((1, 1), (3, 2), (5, 2), (5, 4)):
        chi = character(*pair)
        for p in (2, 3, 5, 7):
            m, expression = exact_euler_factor(chi, p)
            X = sy.Symbol('X')
            expected = 1 if chi.phases[p % chi.C] is None else 1/(1-X**int(
                m*chi.phases[p % chi.C])*T)
            check('euler_exact', sy.cancel(expression-expected) == 0)
            if chi.phases[p % chi.C] is not None:
                check('euler_exact', sy.rem(X**m-1, sy.cyclotomic_poly(m, X), X) == 0)
    for alpha, s in ((1, 0), (2, 1), (F(1, 2), -1)):
        try:
            local_integral(2, mpq(alpha), character(1), s)
        except ValueError as ex:
            check('local_poles', str(ex) == 'DOMAIN: exact pole')
        else:
            check('local_poles', False)
    for p in (2, 3, 5):
        for k in (-1, 0, 1):
            with mp.workdps(90):
                pole = 2*mp.j*mp.pi*k/mp.log(p)
                displacement = mp.mpf('1e-30')
                value = local_integral(p, 1, character(1), pole+displacement)
                close('local_poles', displacement*value, 1/mp.log(p), mp.mpf('1e-27'))
        close('local_poles', local_gamma(p, 1, character(1), 0), 0)
        close('local_poles', local_epsilon(p, 1, character(1), 1), 1)


def check_real():
    for e in (0, 1):
        for s in (mp.mpc('1.3', '.4'), mp.mpf(2)):
            value = 2*mp.quad(lambda x: x**(s+e-1)*mp.exp(-mp.pi*x*x), [0, 1, mp.inf])
            close('real', value, real_integral(e, s))
            if e == 1 and s == 2:
                try:
                    real_gamma(e, s)
                except ValueError:
                    check('real', True)  # gamma_odd has a genuine pole at 2
                else:
                    check('real', False)
            else:
                close('real', real_gamma(e, s)*real_integral(e, s),
                      mp.j**e*real_integral(e, 1-s))
        for k in range(3):
            h = mp.mpf('1e-30')
            s = -e-2*k+h
            close('real', h*real_integral(e, s), 2*(-1)**k*mp.pi**k/mp.factorial(k), mp.mpf('1e-27'))
        close('real', real_gamma(e, -e), 0)


def check_globals():
    pairs = [(1, 1), (3, 2), (4, 3), (5, 2), (5, 4)]
    if flint:
        pairs += [(9, 2), (15, 2), (16, 3)]
    for pair in pairs:
        chi = character(*pair)
        for s in (mp.mpc('2.4', '.7'), mp.mpf(2)):
            value = global_value(chi, s)
            close('global_numeric', norm_one_split(chi, s, 128)*chi.C**((s+chi.e)/2), value)
            P = 31
            prod = mp.fprod(1/(1-chi(p)*mp.power(p, -s)) for p in list(sy.primerange(2, P+1)))
            bound = abs(prod)*mp.expm1(euler_bound(s.real, P))
            check('euler', abs(l_reference(chi, s)-prod) < bound)
            if flint:
                with ctx.workprec(256):
                    ref = flint_reference(chi, s)
                    close('global_flint', numeric(ref), value)
    close('global_numeric', global_value(character(1), 2), mp.pi/6)
    close('global_numeric', l_reference(character(4, 3), 1), mp.pi/4)
    close('global_numeric', global_value(character(4, 3), 1), 1)
    # Exact integer cutoff sufficient for a logarithmic tail goal 1/100 at sigma=2.
    check('euler', euler_bound(2, 201) <= mp.mpf('.01'))
    check('euler', len(list(sy.primerange(2, 202))) == 46)


def check_bounds():
    for r, b, R in ((0, 1, 1), (4, 1, 1), (2, 1, 4), (-2, 2, 1)):
        exact = mp.gammainc(r+1, b*R, mp.inf)/mp.mpf(b)**(r+1)
        bound = analysis.integral_bound(mp.mpf(r), mp.mpf(b), mp.mpf(R))
        check('bounds', exact <= bound)
    for C, e in ((1, 0), (4, 1), (5, 1), (15, 0)):
        a, N, R = mp.pi/C, 2, 3
        for r in (mp.mpf('-.7'), mp.mpf(4)):
            n_tail = sum(n**e*(a*n*n)**(-r-1)*mp.gammainc(r+1, a*n*n, mp.inf)
                         for n in range(N+1, 60))
            t_tail = sum(n**e*(a*n*n)**(-r-1)*mp.gammainc(r+1, a*n*n*R, mp.inf)
                         for n in range(1, 60))
            check('bounds', n_tail < mp.exp(a)*analysis.series_bound(e, a, N)*
                  analysis.integral_bound(r, a, mp.mpf(1)))
            check('bounds', t_tail < mp.exp(a)*analysis.series_bound(e, a, 0)*
                  analysis.integral_bound(r, a, mp.mpf(R)))
    # Midpoint P15 independently checked; T3 is the production proposal.
    chi, z, R, N = character(5, 2), mp.mpc('.3', '1.2'), 4, 6
    a = mp.pi/chi.C
    U = [max(1, mp.mpf(R)**(z.real-1-j)) for j in range(3)]
    M2 = sum(n**chi.e*mp.exp(-a*n*n)*(a*a*n**4*U[0]+2*a*n*n*abs(z-1)*U[1]+
             abs((z-1)*(z-2))*U[2]) for n in range(1, N+1))
    errors = []
    for K in (16, 32, 64):
        h = mp.mpf(R-1)/K
        value = h*sum(sum(chi(n)*n**chi.e*mp.exp(-a*n*n*(1+(k+mp.mpf('.5'))*h))
                         for n in range(1, N+1))*(1+(k+mp.mpf('.5'))*h)**(z-1) for k in range(K))
        errors.append(abs(value-numeric_piece(chi, z, N, R)))
        check('midpoint', errors[-1] <= (R-1)*h*h*M2/24)
    check('midpoint', errors[2] < errors[1] < errors[0])


def check_certificates():
    # Check the new quadrature itself against a different finite-integral algorithm.
    # This numerical check is separate from the ball proof and does not certify mpmath.
    with ctx.workprec(256):
        for pair in ((1, 1), (4, 3), (5, 2)):
            chi = character(*pair)
            for z in (mp.mpc('-.75', '.5'), mp.mpc('2.25', '-1.25')):
                for R in (1, 2, 8):
                    value, err, _, _ = taylor_piece(chi, cb(z), 6, R, arb(2)**-60)
                    reference = numeric_piece(chi, z, 6, R)
                    numerical_error = abs(numeric(value)-reference)
                    budget = mpq(endpoints(err)[1])+mpq(width(value))+MARGIN*max(1, abs(reference))
                    check('taylor', numerical_error <= budget)
                    check('taylor', err <= arb(2)**-60)
    for pair in ((1, 1), (5, 2)):
        chi, s, widths = character(*pair), mp.mpc('2.25', '.5'), []
        for bits in (32, 80):
            result = continuation(chi, s, bits)
            with ctx.workprec(256):
                uncompleted = result.ball*arb(chi.C)**(-(cb(s)+chi.e)/2)
                reference = flint_reference(chi, s)*arb(chi.C)**(-(cb(s)+chi.e)/2)
                check('halfplane_width', uncompleted.overlaps(reference))
                widths.append(width(uncompleted))
        check('halfplane_width', widths[1] < widths[0])
    for pair in ((1, 1), (4, 3), (5, 2), (5, 4), (15, 2)):
        chi = character(*pair)
        s = mp.mpc('.375', '.75')
        previous = None
        for bits in (32, 64, 100):
            result = continuation(chi, s, bits)
            with ctx.workprec(256):
                check('continuation', result.ball.overlaps(flint_reference(chi, s)))
            check('continuation', width(result.ball) <= F(1, 2**bits))
            check('continuation', result.tail+result.quadrature <= arb(2)**(-bits)/8)
            if previous is not None:
                check('continuation', width(result.ball) < previous)
            previous = width(result.ball)
        print(f'  certificate C={chi.C}: bits=100 N={result.N} R={result.R} '
              f'degree<={max(result.degrees[0]+result.degrees[1])} coefficients={result.work}', flush=True)
    # Away from the center strip, at a removable Gamma*L zero, and on a nonzero-radius input.
    for pair, s in (((5, 2), acb(-1)), ((5, 4), acb(0)), ((1, 1), acb(-2)),
                    ((4, 3), acb(1)), ((5, 2), acb('-2.25', '1.5'))):
        chi = character(*pair)
        result = continuation(chi, s, 70)
        check('continuation', result.ball.is_finite())
        if pair == (4, 3):
            check('continuation', result.ball.contains(1))
        else:
            with ctx.workprec(192):
                ref = root_ball(chi)*flint_reference(chi.conj(), 1-s)
                check('continuation', result.ball.overlaps(ref))
    with ctx.workprec(192):
        uncertain = acb(arb('0.375 +/- 0.000000000000000000000001'), arb('.75'))
        result = continuation(character(5, 2), uncertain, 50)
        for side in endpoints(uncertain.real):
            check('input_radii', result.ball.overlaps(flint_reference(character(5, 2), acb(rb(side), '.75'))))
        try:
            continuation(character(5, 2), acb(arb('0.375 +/- 0.1'), '.75'), 60)
        except ValueError as ex:
            check('input_radii', str(ex).startswith('NOT_DETERMINED'))
        else:
            check('input_radii', False)


def check_poles_fe():
    chi = character(1)
    for s, expected in ((acb(0), 'DOMAIN'), (acb(1), 'DOMAIN'),
                        (acb(arb('1 +/- 0.001')), 'NOT_DETERMINED'),
                        (acb(arb('0 +/- 0.001')), 'NOT_DETERMINED'),
                        (acb(1, '0.001'), 'OK')):
        check('poles', pole_status(chi, s) == expected)
    with ctx.workprec(256):
        h = arb(2)**-40
        for s, displacement, residue in ((acb(h), acb(h), -1), (1+acb(h), acb(h), 1)):
            result = continuation(chi, s, 80)
            regular = result.ball*displacement
            check('poles', abs(regular-residue) < arb(2)**-35)
        # Pole-free rectangle near one: the residue term propagates its large sensitivity.
        near = acb(arb(1)+h+arb(0, arb(2)**-200))
        result = continuation(chi, near, 70)
        check('poles', result.ball.is_finite())
        for endpoint in endpoints(near.real):
            check('poles', result.ball.overlaps(flint_reference(chi, cb(endpoint))))
    for pair in ((1, 1), (4, 3), (5, 2), (5, 4), (15, 2)):
        chi = character(*pair)
        s = mp.mpc('.375', '.75')
        left, right = functional_equation_sides(chi, s)
        check('functional_equation', left.overlaps(right) and left.is_finite() and right.is_finite())
        check('functional_equation', width(left) <= F(1, 2**80) and width(right) <= F(1, 2**80))
        with ctx.workprec(256):
            check('functional_equation', left.overlaps(flint_reference(chi, s)))
            check('functional_equation', right.overlaps(flint_reference(chi, s)))


def check_goldens_composite():
    import char_checks as chars
    count = 0
    for line in (ROOT/'tests/golden/gauss.tsv').read_text().splitlines():
        if not line or line.startswith('#'):
            continue
        q, n = map(int, re.search(r'q=(\d+), n=(\d+)', line).groups())
        chi = character(q, n)
        expected = line.split(' W=')[1]
        with ctx.workprec(256):
            value = root_ball(chi)
            for coordinate, (mid, rad) in zip((value.real, value.imag), chars.golden_ball(expected)):
                lo, hi = endpoints(coordinate)
                check('goldens', mid-rad <= lo <= hi <= mid+rad)
        count += 1
    check('goldens', count == 17)
    for pair in ((3, 2), (4, 3), (5, 4), (7, 6), (8, 3), (8, 5), (8, 7), (12, 11)):
        chi = character(*pair)
        close('goldens', root_number(chi), 1)
    check('goldens', character(8, 7).C == 4)
    print('  raw golden (8,7) -> primitive (4,3); tau=2i, W=1', flush=True)
    # CRT unit restrictions and the off-prime uniformizer factor, C=15.
    chi = character(15, 2)
    parts = []
    for p, q in ((3, 3), (5, 5)):
        co = chi.C//q
        phases = tuple(None if gcd(u, q) > 1 else chi.phases[(1+co*((u-1)*pow(co, -1, q) % q)) % chi.C]
                       for u in range(q))
        parts.append((p, Character(q, phases)))
    for u in range(1, 15):
        close('composite', chi(u), parts[0][1](u)*parts[1][1](u))
    s = mp.mpc('.35', '1.7')
    product = mp.j**chi.e
    for p, ch in parts:
        alpha = mp.fprod(other(p) for op, other in parts if op != p)
        product *= local_gamma(p, alpha, ch.conj(), s)
    close('composite', product, mp.j**(-chi.e)*gauss(chi)*15**(-s))


def check_poisson_dilation():
    import functions4_checks as f4
    with ctx.workprec(256):
        for pair in ((1, 1), (4, 3), (5, 2), (5, 4)):
            chi = character(*pair)
            vals = [0 if q is None else sy.exp(2*sy.pi*sy.I*sy.Rational(q.numerator, q.denominator))
                    for q in chi.phases]
            for t in (F(1, 2), F(2)):
                P = [0, sy.Rational(t.numerator, t.denominator)] if chi.e else [1]
                term = (P, sy.Rational(t.numerator**2, t.denominator**2), 0, 0)
                left, right = f4.poisson_sides_ball(([term], (1, chi.C, vals)), (4, 32))
                check('poisson_dilation', left.overlaps(right))
                value = mpq(t)**chi.e*sum(chi(n)*n**chi.e*mp.exp(-mp.pi*(mpq(t)*n)**2)
                                        for n in range(-40, 41))
                margin = rb(MARGIN*max(1, abs(value)))
                reference = cb(value)+acb(arb(0, margin), arb(0, margin))
                check('poisson_dilation', left.overlaps(reference))
                check('poisson_dilation', right.overlaps(reference))
                check('poisson_dilation', width(left) < F(1, 10**6) and width(right) < F(1, 10**6))
        # Both factors change for an idele whose finite scale is nontrivial.
        # Reuse the independently summed example, including norm |x_inf|/r.
        before = sum(analysis.COUNTS.values())
        analysis.check_poisson_right_and_idele()
        inherited = sum(analysis.COUNTS.values())-before
        check('poisson_dilation', inherited == 4)
        COUNTS['inherited_analysis'] = COUNTS.get('inherited_analysis', 0)+inherited
        print(f'  inherited analysis.check_poisson_right_and_idele: {inherited} assertions', flush=True)


def check_findings_witnesses():
    lower, upper = mp.mpf('1.125'), mp.mpf('1.25')
    difference = abs(global_value(character(1), lower)-global_value(character(1), upper))
    check('width_witness', difference > 1)
    print('  zeta completion at 1.125 versus 1.25: separation='+mp.nstr(difference, 24), flush=True)
    for func, value in ((lambda s: real_integral(1, s), -1), (lambda s: real_gamma(1, s), 2)):
        try:
            func(value)
        except ValueError:
            check('pole_qualification', True)
        else:
            check('pole_qualification', False)


def check_independence():
    """Exercise reference independence and two-call data flow, not just equality."""
    chi = character(5, 2)
    originals = {name: globals()[name] for name in ('global_value', 'l_reference',
                                                  'numeric_piece', 'flint_reference')}
    mp_originals = {name: getattr(mp, name) for name in ('zeta', 'gamma', 'gammainc', 'dirichlet')}

    def forbidden(*args, **kwargs):
        raise AssertionError('reference function reached from continuation')

    try:
        for name in originals:
            globals()[name] = forbidden
        for name in mp_originals:
            setattr(mp, name, forbidden)
        result = continuation(chi, F(3, 8), 40)
        check('independence', result.ball.is_finite())
        left, right = functional_equation_sides(chi, F(3, 8), 40)
        check('independence', left.overlaps(right))
    finally:
        globals().update(originals)
        for name, function in mp_originals.items():
            setattr(mp, name, function)
    original = globals()['continuation']
    calls = []

    def sentinel(ch, s, bits):
        calls.append((ch, s))
        return Enclosure(acb(len(calls)), 0, 1, arb(0), arb(0), 0, ())

    try:
        globals()['continuation'] = sentinel
        left, right = functional_equation_sides(chi, F(3, 8), 40)
        check('independence', len(calls) == 2 and calls[0][0] == chi and calls[1][0] == chi.conj())
        check('independence', calls[0][1] == cb(F(3, 8)) and calls[1][1] == cb(F(5, 8)))
        check('independence', left == 1 and not right.overlaps(root_ball(chi)))
    finally:
        globals()['continuation'] = original


def check_faults():
    """Six distinguishable wrong formulas per work package; numerical witnesses."""
    chi, s = character(5, 2), mp.mpc('.375', '.75')
    # 5.1: inverse omitted, wrong sign, alpha dropped, a dropped, parity dropped, measure doubled.
    eta, alpha = chi, phase(F(1, 4))
    g = local_gamma(5, alpha, eta, s)
    correct = [mp.mpc(1), gauss(eta.conj(), True), g,
               local_gamma(2, alpha, character(4, 3), s), real_integral(1, 2), mp.mpc(1)]
    wrong = [sum(eta(u)**2 for u in range(1, 5))/4, gauss(eta.conj()),
             local_gamma(5, 1, eta, s), alpha*2**(-s)*gauss(character(4, 3), True),
             real_integral(0, 2), mp.mpc(2)]
    for a, b in zip(correct, wrong):
        check('faults_51', abs(a-b) > mp.mpf('1e-5'))
    # 5.2: conductor, parity, conjugation, C^-s in Hurwitz, factor two, xi normalization.
    s2 = mp.mpc('2.4', '.7')
    value = global_value(chi, s2)
    wrong = [value/5**((s2+chi.e)/2), real_integral(0, s2)*5**(s2/2)*l_reference(chi, s2),
             global_value(chi.conj(), s2), value*5**s2, 2*value]
    for b in wrong:
        check('faults_52', abs(value-b) > mp.mpf('1e-5'))
    zeta = global_value(character(1), s2)
    check('faults_52', abs(zeta-s2*(s2-1)*zeta/2) > mp.mpf('1e-5'))
    # 5.3: pole sign, zero mode, wrong dual exponent, missing L14 factor, no tail, no radius.
    zeta = global_value(character(1), s)
    check('faults_53', abs(zeta-(zeta+2/s)) > 1)
    check('faults_53', abs(zeta-(zeta+1/(s-1))) > 1)
    wrong_dual = numeric_piece(chi, (s+chi.e)/2, 24)+root_number(chi)*numeric_piece(
        chi.conj(), (s+chi.e)/2, 24)
    check('faults_53', abs(global_value(chi, s)-wrong_dual) > mp.mpf('1e-3'))
    r, b = mp.mpf(4), mp.mpf(1)
    check('faults_53', mp.gammainc(r+1, b, mp.inf)/b**(r+1) > mp.exp(-b)/b)
    check('faults_53', abs(numeric_piece(chi, (s+chi.e)/2, 1)-numeric_piece(
        chi, (s+chi.e)/2, 24)) > mp.mpf('.001'))
    check('faults_53', abs(global_value(character(1), mp.mpf('1.125'))-
                          global_value(character(1), mp.mpf('1.25'))) > 1)
    # 5.4: conjugation, W inverse, sign, missing conductor, wrong 1-s, copied fake sides.
    v, W = global_value(chi, s), root_number(chi)
    wrong = [W*global_value(chi, 1-s), global_value(chi.conj(), 1-s)/W,
             -W*global_value(chi.conj(), 1-s), v/5**((s+chi.e)/2),
             W*global_value(chi.conj(), -s), mp.mpc(2)]
    for b in wrong:
        check('faults_54', abs(v-b) > mp.mpf('1e-4'))


def main():
    print('mpmath', mp.__version__, 'dps=60 comparison margin=1e-45', flush=True)
    print('python-flint', flint.__version__ if flint else 'unavailable',
          'FLINT', getattr(flint, '__FLINT_VERSION__', 'unavailable'), flush=True)
    groups = [check_local, check_real, check_globals, check_bounds, check_faults, check_findings_witnesses]
    if flint:
        groups += [check_certificates, check_poles_fe, check_goldens_composite,
                   check_poisson_dilation, check_independence]
    else:
        print('SKIP certified ball, FLINT reference, Conrey golden and tensor groups', flush=True)
    for fn in groups:
        before = dict(COUNTS)
        fn()
        for name, count in COUNTS.items():
            if count != before.get(name, 0):
                print(f'{name}: {count} checks', flush=True)
    print(f'TOTAL {sum(COUNTS.values())} checks', flush=True)


if __name__ == '__main__':
    main()
