#!/usr/bin/env python3
"""Milestone 4 design oracle; no adelefeld C code is imported.

Finite indexing and phases use Fraction. ffun_transform returns certified acb
balls (python-flint's installed FLINT, not a claim about the C library version).
Real closure uses exact SymPy algebra. Poisson and quadrature use 60 decimal
digits with relative/absolute comparison margin 1e-45; those are numerical
checks, NOT interval certificates. Lattice tails are analysis.md Lemma 6.
Sources: refs/src/flint-3.0.1/arb.rst:6-12, acb.rst:6-9,590-615.
Run: timeout 120 python3 -B proto/functions4_checks.py
"""
from dataclasses import dataclass
from fractions import Fraction as Q
from functools import lru_cache
from math import comb, gcd, lcm
from pathlib import Path
import sys
sys.dont_write_bytecode = True

import mpmath as mp
import sympy as sp
import flint
from flint import acb, arb, ctx

sys.path.insert(0, str(Path(__file__).resolve().parent))
import analysis_checks as analysis
import text_grammar as grammar

mp.mp.dps = 60
ctx.prec = 212
MARGIN = mp.mpf('1e-45')
COUNTS = {}
X = sp.Symbol('x', real=True)
PI, I = sp.pi, sp.I


def check(group, condition):
    COUNTS[group] = COUNTS.get(group, 0) + 1
    if not condition:
        raise AssertionError(f'{group}: check {COUNTS[group]}')


def close(group, actual, expected, margin=MARGIN):
    check(group, abs(actual - expected) <= margin * max(1, abs(expected)))


def mpq(q):
    q = Q(q)
    return mp.mpf(q.numerator) / q.denominator


def smp(z):
    return _smp_cached(sp.sympify(z), mp.mp.dps)


@lru_cache(maxsize=4096)
def _smp_cached(z, digits):
    return mp.mpc(str(sp.re(z).evalf(65)), str(sp.im(z).evalf(65)))


def ball(z):
    """Exact rationals and symbolic expressions -> enclosure, not a decimal point."""
    if isinstance(z, acb):
        return z
    if isinstance(z, Q):
        return acb(z.numerator) / z.denominator
    z = sp.sympify(z)
    if z == I:
        return acb(0, 1)
    if z == PI:
        return acb(arb.pi())
    if z.is_Rational:
        return acb(int(z.p)) / int(z.q)
    if z.is_Add:
        return sum((ball(v) for v in z.args), acb(0))
    if z.is_Mul:
        out = acb(1)
        for v in z.args:
            out *= ball(v)
        return out
    if z.is_Pow and z.exp.is_Integer:
        return ball(z.base) ** int(z.exp)
    if z.is_Pow and z.exp == sp.Rational(-1, 2):
        return 1 / ball(z.base).sqrt()
    if z.is_Pow and z.exp == sp.Rational(1, 2):
        return ball(z.base).sqrt()
    raise TypeError(f'unsupported exact expression {z}')


def phase(q):
    """Exact reduced cyclotomic angle in Q/Z; never float(j*k/L)."""
    return Q(q) % 1


def phase_ball(q):
    q = phase(q)
    cardinal = {Q(0): acb(1), Q(1, 4): acb(0, 1),
                Q(1, 2): acb(-1), Q(3, 4): acb(0, -1)}
    return cardinal[q] if q in cardinal else (2 * ball(q)).exp_pi_i()


def ffun_transform(D, M, f):
    """Return (M,D,g), g[k] enclosing (1/M) sum f[j] E(-jk/(DM))."""
    assert D >= 1 and M >= 1 and len(f) == D * M
    return M, D, [sum((ball(v) * phase_ball(Q(-j*k, D*M))
                      for j, v in enumerate(f)), acb(0)) / M for k in range(D*M)]


def point(D, M, f, x):
    t = D * Q(x)
    return 0 if t.denominator != 1 else f[t.numerator % (D*M)]


def refine(D, M, f, D2, M2):
    """Exact zero extension and repetition, with D|D2 and M|M2."""
    assert D2 % D == 0 and M2 % M == 0 and len(f) == D*M
    r = D2 // D
    return [0 if k % r else f[(k // r) % (D*M)] for k in range(D2*M2)]


def combine(a, b, product=False):
    D, M = lcm(a[0], b[0]), lcm(a[1], b[1])
    u, v = refine(*a, D, M), refine(*b, D, M)
    return D, M, [x*y if product else x+y for x, y in zip(u, v)]


def translate(D, M, f, q):
    """T_q f(x)=f(x-q). No floating conversion of q."""
    q = Q(q)
    D2 = lcm(D, q.denominator)
    g = refine(D, M, f, D2, M)
    shift = int(D2*q)
    return D2, M, [g[(k-shift) % (D2*M)] for k in range(D2*M)]


def dilate(D, M, f, q):
    """D_q f(x)=f(q*x); deliberately nonminimal but exact layout."""
    q = Q(q)
    if not q:
        raise ValueError('DOMAIN: zero dilation is excluded')
    s, t = abs(q.numerator), q.denominator
    sign = 1 if q > 0 else -1
    return s*D, t*M, [0 if k % t else f[(sign*(k//t)) % (D*M)]
                      for k in range(s*t*D*M)]


def unit_residues(L, c, N):
    """Image of c U(N) in (Z/L)^*. N=0 is the exact unit +/-1."""
    if N == 0:
        assert c in (-1, 1)
        return {c % L}
    assert gcd(c, N) == 1
    g = gcd(L, N)
    return {u for u in range(L) if gcd(u, L) == 1 and (u-c) % g == 0}


def dilate_idele(D, M, f, r, c, N):
    assert Q(r) > 0
    residues = unit_residues(D*M, c, N)
    if len(residues) != 1:
        raise ValueError('NOT_DETERMINED: unit does not fix array indices')
    u = next(iter(residues))
    return dilate(D, M, [f[(j*u) % (D*M)] for j in range(D*M)], r)


def evaluate(f, finite_ball):
    """Return (frozenset(indices met), outside_support) for triple (A,H,d)."""
    D, M, values = f
    A, H, d = finite_ball
    assert H >= 0 and d > 0 and len(values) == D*M
    g = gcd(H*D, M*d*D)
    indices = frozenset(j for j in range(D*M) if (A*D-j*d) % g == 0)
    outside = (D*A) % d != 0 or (D*H) % d != 0
    return indices, outside


def valuation(q, p):
    q = Q(q)
    if not q:
        return mp.inf
    n, d, v = abs(q.numerator), q.denominator, 0
    while n % p == 0:
        n //= p
        v += 1
    while d % p == 0:
        d //= p
        v -= 1
    return v


def evaluate_partial(f, locals_):
    """locals_ entries (p, centre, exponent); exponent=None denotes exact local point.

    This evaluates all adelic completions, not a default at absent places.
    """
    D, M, _ = f
    indices = {j for j in range(D*M) if all(
        valuation(Q(j, D)-a, p) >= (valuation(M, p) if e is None else min(e, valuation(M, p)))
        for p, a, e in locals_)}
    return frozenset(indices), True  # An absent prime always allows a point off support.


def pexpr(P, x=X):
    return sum((sp.sympify(c)*x**j for j, c in enumerate(P)), sp.S(0))


def coeffs(expr):
    p = sp.Poly(sp.expand(expr), X)
    return [] if p.is_zero else [p.nth(j) for j in range(p.degree()+1)]


def rterm_translate(P, A, B, C, q):
    q = sp.Rational(q)
    return coeffs(pexpr(P, X-q)), A, B+2*PI*A*q, C-B*q-PI*A*q*q


def rterm_dilate(P, A, B, C, h):
    h = sp.Rational(h)
    if not h:
        raise ValueError('DOMAIN')
    return coeffs(pexpr(P, h*X)), A*h*h, B*h, C


def rterm_product(a, b):
    P, A, B, C = a
    Qp, aA, aB, aC = b
    return coeffs(pexpr(P)*pexpr(Qp)), A+aA, B+aB, C+aC


def rterm_derivative(P, A, B, C):
    p = pexpr(P)
    return coeffs(sp.diff(p, X)+(B-2*PI*A*X)*p), A, B, C


def rterm_transform(P, A, B, C):
    """Exact symbolic new (P,A,B,C), positive real Fourier kernel.

    A must lie in Re(A)>0. sqrt is the branch positive on positive reals.
    The prefactor A**(-1/2) belongs to the new polynomial, not exp(C).
    """
    A, B, C = map(sp.sympify, (A, B, C))
    z = sp.Symbol('z')
    H, total = sp.S(1), sp.S(0)
    for p in P:
        total += p*H
        H = sp.expand(sp.diff(H, z)+z*H/(2*PI*A))
    return (coeffs(total.subs(z, B+2*PI*I*X)/sp.sqrt(A)),
            1/A, I*B/A, C+B*B/(4*PI*A))


def rvalue(term, x):
    P, A, B, C = term
    return sum(smp(c)*x**j for j, c in enumerate(P))*mp.exp(-mp.pi*smp(A)*x*x+smp(B)*x+smp(C))


def rvalue_ball(term, x):
    P, A, B, C = term
    x = ball(x)
    p = acb(0)
    for c in reversed(P):
        p = p*x+ball(c)
    return p*(-ball(PI)*ball(A)*x*x+ball(B)*x+ball(C)).exp()


def lattice_tail(term, a, h, N):
    P, A, B, C = term
    # Preflight the Lemma 6 prefix search. The oracle deliberately uses a smaller
    # limit than production D1; do not spend millions of iterations near Re(A)=0.
    alpha = mp.pi*smp(A).real*mpq(h)**2
    beta = abs((mpq(h)*(smp(B)-2*mp.pi*smp(A)*mpq(a))).real)
    if alpha <= 0:
        raise ValueError('DOMAIN')
    K = int(mp.floor(N))+4096
    if mp.exp(max(0, len(P)-1)/K-alpha*(2*K+1)+beta) > mp.mpf('.5'):
        raise ValueError('LIMIT: oracle ratio prefix exceeds 4096')
    return analysis.lattice_bound([smp(c) for c in P], smp(A), smp(B), smp(C), mpq(a), mpq(h), N)


def lattice_tail_ball(term, a, h, N):
    """Lemma 6 with acb/arb enclosures; small fixed cases, budget checked by numerical oracle first."""
    lattice_tail(term, a, h, N)
    P, A, B, C = term
    a, h = ball(a), ball(h)
    A, B, C = ball(A), ball(B), ball(C)
    alpha = (arb.pi()*(A*h*h).real).lower()
    beta = abs((h*(B-2*ball(PI)*A*a)).real).upper()
    gamma = (C+B*a-ball(PI)*A*a*a).real.upper()
    assert alpha > 0
    q = [sum((ball(P[k])*comb(k, j)*a**(k-j)*h**j for k in range(j, len(P))), acb(0))
         for j in range(len(P))]
    return 2*gamma.exp()*sum((abs(v).upper()*analysis.ball_series_bound(j, alpha, N, beta)
                              for j, v in enumerate(q)), arb(0))


def poisson_sides_ball(tensor, N):
    """Certified independent truncations for the fixed exact symbolic test inputs."""
    terms, (D, M, f) = tensor
    nl, nr = N
    fv = list(map(ball, f))
    g = ffun_transform(D, M, f)[2]
    hats = [rterm_transform(*t) for t in terms]
    lv = sum((fv[j]*rvalue_ball(t, Q(j, D)+M*n) for j in range(D*M)
              for n in range(-nl, nl+1) for t in terms), acb(0))
    rv = sum((g[n % (D*M)]*rvalue_ball(t, Q(n, M)) for n in range(-nr, nr+1) for t in hats), acb(0))
    bl = sum((abs(fv[j]).upper()*lattice_tail_ball(t, Q(j, D), M, nl)
              for j in range(D*M) for t in terms), arb(0)).upper()
    br = (max(abs(v).upper() for v in g)*sum((lattice_tail_ball(t, 0, Q(1, M), nr)
                                           for t in hats), arb(0))).upper()
    # arb(mid,rad) adds the positive error to each real coordinate.
    return lv+acb(arb(0, bl), arb(0, bl)), rv+acb(arb(0, br), arb(0, br))


@dataclass(frozen=True)
class Estimate:
    value: object
    tail: object
    numerical_margin: object


def poisson_sides(tensor, N):
    """tensor=(real_terms,(D,M,values)); N=(N_left,N_right) or a common integer.

    Returns two independently accumulated estimates. Tail is the mathematical
    Lemma 6 bound evaluated numerically; numerical_margin is a comparison margin,
    not a certified floating point error bound. Production must use arb instead.
    """
    terms, (D, M, f) = tensor
    nl, nr = (N, N) if isinstance(N, int) else N
    fv = [smp(v) for v in f]
    g = [sum(fv[j]*mp.exp(2j*mp.pi*mpq(phase(Q(-j*k, D*M)))) for j in range(D*M))/M
         for k in range(D*M)]
    hats = [rterm_transform(*term) for term in terms]
    # Compile coefficients once; expensive symbolic conversion is outside lattice loops.
    def compile_term(term):
        P, A, B, C = term
        p, a, b, c = [smp(v) for v in P], smp(A), smp(B), smp(C)
        return lambda x: sum(v*x**j for j, v in enumerate(p))*mp.exp(-mp.pi*a*x*x+b*x+c)
    left_f, right_f = list(map(compile_term, terms)), list(map(compile_term, hats))
    lv = [fv[j]*fun(mpq(Q(j, D)+M*n)) for j in range(D*M)
          for n in range(-nl, nl+1) for fun in left_f]
    rv = [g[n % (D*M)]*fun(mpq(Q(n, M))) for n in range(-nr, nr+1) for fun in right_f]
    bl = sum(abs(fv[j])*lattice_tail(t, Q(j, D), M, nl) for j in range(D*M) for t in terms)
    br = max(map(abs, g))*sum(lattice_tail(t, 0, Q(1, M), nr) for t in hats)
    return (Estimate(sum(lv), bl, MARGIN*(1+sum(map(abs, lv)))),
            Estimate(sum(rv), br, MARGIN*(1+sum(map(abs, rv)))))


def choose_cutoffs(tensor, bits, cap=4096):
    """Independent geometric searches; explicit returned cutoffs, no use of equality."""
    nl = nr = 0
    goal = mp.power(2, -bits)/8
    while True:
        left, right = poisson_sides(tensor, (nl, nr))
        if left.tail <= goal and right.tail <= goal:
            return nl, nr
        if left.tail > goal:
            nl = max(1, 2*nl)
        if right.tail > goal:
            nr = max(1, 2*nr)
        if max(nl, nr) > cap:
            raise ValueError('LIMIT')


def check_finite():
    for D, M in [(1, 1), (2, 3), (3, 2), (2, 5), (4, 3)]:
        f = [j*j-2+I*(3*j+1) for j in range(D*M)]
        for d, m in [(D, M), (2*D, 3*M)]:
            g = refine(D, M, f, d, m)
            for k in range(d*m):
                check('finite_algebra', g[k] == point(D, M, f, Q(k, d)))
        for q in [Q(-2, 3), Q(1, D), Q(5, 4), Q(0)]:
            g = translate(D, M, f, q)
            for k in range(g[0]*g[1]):
                check('finite_algebra', g[2][k] == point(D, M, f, Q(k, g[0])-q))
        for q in [Q(-2, 3), Q(3, 2), Q(-1), Q(1)]:
            g = dilate(D, M, f, q)
            for k in range(g[0]*g[1]):
                check('finite_algebra', g[2][k] == point(D, M, f, q*Q(k, g[0])))
        g = ffun_transform(D, M, f)
        back = ffun_transform(*g)
        for k in range(D*M):
            check('finite_transform', back[2][k].contains(ball(f[-k % (D*M)])))
            expected = analysis.finite_transform([smp(v) for v in f], D, M)[k]
            # Check numerical reference against a margin, not acb midpoint containment.
            delta = g[2][k]-acb(str(expected.real), str(expected.imag))
            check('finite_transform', abs(delta) < arb('1e-45'))
        check('finite_transform', g[2][0].contains(ball(sum(f)/M)))
        n1 = sum((ball(sp.expand_complex(v*sp.conjugate(v))) for v in f), acb(0))/M
        n2 = sum((v*v.conjugate() for v in g[2]), acb(0))/D
        check('finite_transform', (n1-n2).contains(0))
        for q in [Q(-2, 3), Q(3, 2)]:
            dilated = dilate(D, M, f, q)
            lhs = ffun_transform(*dilated)
            rhs = dilate(*g, 1/q)
            check('dilation_covariance', lhs[:2] == rhs[:2])
            for u, v in zip(lhs[2], rhs[2]):
                check('dilation_covariance', (u-ball(abs(q))*ball(v)).contains(0))
            check('dilation_covariance',
                  sp.expand(sum(dilated[2])/sp.Integer(dilated[1])-sp.Rational(abs(q))*sum(f)/M) == 0)
    a, b = (2, 3, list(range(6))), (3, 2, list(range(6, 12)))
    for product in [False, True]:
        D, M, f = combine(a, b, product)
        check('finite_algebra', (D, M) == (6, 6))
        for k in range(D*M):
            u, v = point(*a, Q(k, D)), point(*b, Q(k, D))
            check('finite_algebra', f[k] == (u*v if product else u+v))
    for L in range(1, 17):
        for N in range(1, 13):
            mod = lcm(L, N)
            lifts = {x % L for x in range(mod) if gcd(x, mod) == 1 and (x-1) % N == 0}
            check('idele_indices', unit_residues(L, 1, N) == lifts)
    check('idele_indices', unit_residues(6, 1, 3) == {1})
    check('idele_indices', unit_residues(6, 1, 1) == {1, 5})
    check('idele_indices', dilate_idele(*a, Q(2, 3), 1, 3) == dilate(*a, Q(2, 3)))
    finite = 2, 5, list(range(1, 11))
    for r in [Q(1), Q(2, 3), Q(3, 2)]:
        D2, M2, values = dilate_idele(*finite, r, 3, 10)
        # Integer lift of the unit residue, coprime to every denominator involved.
        lift = next(v for v in range(3, 100, 10) if gcd(v, r.denominator*finite[0]) == 1)
        for k in range(D2*M2):
            check('idele_indices', values[k] == point(*finite, r*lift*Q(k, D2)))


def check_evaluation():
    for D, M in [(1, 1), (2, 3), (3, 2)]:
        f = D, M, list(range(1, D*M+1))
        for d in range(1, 5):
            for H in range(5):
                for A in range(5):
                    indices, outside = evaluate(f, (A, H, d))
                    # Independent rational samples exhaust all integer residues after clearing denominators.
                    samples = [Q(A+H*n, d) for n in range(d*D*M+1)]
                    seen = {int(D*x) % (D*M) for x in samples if (D*x).denominator == 1}
                    check('evaluation', indices == seen)
                    check('evaluation', outside == any((D*x).denominator != 1 for x in samples))
    f = 2, 3, [1, 2, 3, 4, 5, 6]
    check('evaluation', evaluate(f, (0, 1, 1)) == (frozenset({0, 2, 4}), False))
    check('evaluation', evaluate(f, (0, 1, 4)) == (frozenset(range(6)), True))
    check('evaluation', evaluate(f, (1, 0, 5)) == (frozenset(), True))
    check('evaluation', evaluate_partial(f, [(3, Q(1), 1)]) == (frozenset({2, 5}), True))
    check('evaluation', evaluate_partial(f, []) == (frozenset(range(6)), True))


def check_real():
    base = [sp.S(1), -2, 3], sp.Rational(6, 5)+I/5, sp.Rational(7, 10)-I/3, -sp.Rational(1, 5)
    P, A, B, C = base
    # Compare expanded exponents, not exponentially simplified numerical identities.
    for q in [sp.Rational(-2, 3), sp.Rational(1, 4)]:
        p, a, b, c = rterm_translate(*base, q)
        check('real_closure', sp.expand(pexpr(p)-pexpr(P, X-q)) == 0)
        check('real_closure', sp.expand(-PI*a*X**2+b*X+c-(-PI*A*(X-q)**2+B*(X-q)+C)) == 0)
        p, a, b, c = rterm_dilate(*base, q)
        check('real_closure', sp.expand(pexpr(p)-pexpr(P, q*X)) == 0)
        check('real_closure', sp.expand(-PI*a*X**2+b*X+c-(-PI*A*(q*X)**2+B*q*X+C)) == 0)
    p, a, b, c = rterm_product(base, base)
    check('real_closure', sp.expand(pexpr(p)-pexpr(P)**2) == 0)
    check('real_closure', (a, b, c) == (2*A, 2*B, 2*C))
    p, a, b, c = rterm_derivative(*base)
    check('real_closure', sp.expand(pexpr(p)-sp.diff(pexpr(P), X)-(B-2*PI*A*X)*pexpr(P)) == 0)
    shifted = rterm_translate([1], 1, 0, 0, sp.Rational(1, 3))
    for term in [base, shifted, ([0, 1], 1, 0, 0), ([], 1, 0, 0)]:
        hat = rterm_transform(*term)
        for y in [mp.mpf('-0.4'), mp.mpf('0.2')]:
            direct = mp.quad(lambda x: rvalue(term, x)*mp.exp(2j*mp.pi*x*y), [-mp.inf, -1, 0, 1, mp.inf])
            close('real_transform', rvalue(hat, y), direct)
            twice = rterm_transform(*hat)
            close('real_transform', rvalue(twice, y), rvalue(term, -y))
        integral = mp.quad(lambda x: rvalue(term, x), [-mp.inf, -1, 0, 1, mp.inf])
        close('integral_norm', rvalue(hat, 0), integral)
    check('real_transform', rterm_transform([0, 1], 1, 0, 0)[0] == [0, I])
    for a in [1+2*I, 1-2*I, sp.Rational(1, 100)+3*I]:
        check('real_transform', (ball(a).sqrt()*ball(1/a).sqrt()).contains(1))
        check('real_transform', ball(a).sqrt().real > 0)
    # Direct positive integrand supplies an independent norm check.
    norm = mp.quad(lambda x: abs(rvalue(base, x))**2, [-mp.inf, -1, 0, 1, mp.inf])
    conj = [sp.conjugate(c) for c in P], sp.conjugate(A), sp.conjugate(B), sp.conjugate(C)
    close('integral_norm', rvalue(rterm_transform(*rterm_product(base, conj)), 0), norm)


def check_goldens():
    root = Path(__file__).resolve().parents[1]
    for kind, count in [('ffun', 18), ('rfun', 19)]:
        rows = [line.split('\t') for line in (root/'tests/golden'/f'{kind}.tsv').read_text().splitlines()
                if line and not line.startswith('#')]
        check('goldens', len(rows) == count)
        for raw, expected in rows:
            try:
                got = grammar.canonical(kind, raw.encode())
            except grammar.TextError as e:
                got = '!'+e.status
            check('goldens', got == expected)
            if kind == 'ffun' and not got.startswith('!'):
                tree = grammar._syntax(kind, got)
                D, M = int(tree[1]), int(tree[2])
                rectangles = [grammar._complex(z) for z in tree[3][1]]
                # Two opposite joint corner assignments challenge uncertain coefficient enclosures.
                f = [acb(arb(str(a), str(r)), arb(str(b), str(s))) for (a, r), (b, s) in rectangles]
                g = ffun_transform(D, M, f)[2]
                for sign in [-1, 1]:
                    exact = [sp.Rational(a+sign*r)+I*sp.Rational(b-sign*s)
                             for (a, r), (b, s) in rectangles]
                    point_g = ffun_transform(D, M, exact)[2]
                    for u, v in zip(g, point_g):
                        check('golden_transforms', u.overlaps(v))


def check_tails_poisson():
    terms = [([1, sp.Rational(1, 3)+I/5], sp.Rational(4, 5), sp.Rational(7, 5), 0)]
    tensor = terms, (2, 3, [1, 2+I, 0, -1, 3, I])
    for N in [(0, 0), (1, 3), (2, 9)]:
        left, right = poisson_sides(tensor, N)
        check('poisson', abs(left.value-right.value) <= left.tail+right.tail+
              left.numerical_margin+right.numerical_margin)
    for bits in [30, 80, 120]:
        cuts = choose_cutoffs(tensor, bits)
        left, right = poisson_sides(tensor, cuts)
        check('poisson', left.tail <= mp.power(2, -bits)/8)
        check('poisson', right.tail <= mp.power(2, -bits)/8)
        close('poisson', left.value, right.value, mp.power(2, -bits)/4)
        print(f'  width target 2^-{bits}: N_left={cuts[0]}, N_right={cuts[1]}')
    for term in terms+[rterm_translate([1], 1, 0, 0, sp.Rational(1, 3))]:
        for a, h in [(Q(0), Q(1)), (Q(2, 3), Q(-2, 5))]:
            for N in [0, 1, 3]:
                observed = sum(abs(rvalue(term, mpq(a+h*n))) for n in range(-35, 36) if abs(n)>N)
                check('tails', observed <= lattice_tail(term, a, h, N)*(1+MARGIN))
    # Nonzero parameter uncertainty prevents arbitrary output width reduction.
    theta = sum(mp.exp(-mp.pi*n*n) for n in range(-20, 21))
    check('findings', theta > 1)
    print('  fixed coefficient interval [1,2]: theta value width >=', mp.nstr(theta, 18))
    # A small but positive Re(A) makes a fixed cutoff insufficient, not a domain error.
    slow = ([([1], sp.Rational(1, 10**8), 0, 0)], (1, 1, [1]))
    check('tails', mp.exp(-mp.pi*4097**2/10**8) > mp.mpf('1e-20'))
    try:
        lattice_tail(slow[0][0], 0, 1, 4096)
    except ValueError as error:
        check('tails', str(error).startswith('LIMIT'))
    else:
        check('tails', False)


def check_precision():
    old = ctx.prec
    widths = []
    try:
        for prec in [64, 128, 212]:
            ctx.prec = prec
            f = ffun_transform(2, 3, [0, 1, 0, 0, 0, 0])[2]
            check('precision', f[1].imag < 0)
            widths.append(f[1].rad())
            check('precision', f[1].rad() < arb(2)**(-prec+12))
        check('precision', widths[1] < widths[0])
        check('precision', widths[2] < widths[1])
    finally:
        ctx.prec = old
    # Real polynomial/shifted input, D!=M; each side recomputed at each precision.
    tensor = ([rterm_translate([1, sp.Rational(1, 5)], 1, 0, 0, sp.Rational(1, 3))],
              (2, 3, [1, 2, 0, -1, 3, 1]))
    diameters = []
    try:
        for prec, bits in [(80, 30), (144, 70), (212, 110)]:
            ctx.prec = prec
            cuts = choose_cutoffs(tensor, bits+10)
            left, right = poisson_sides_ball(tensor, cuts)
            check('certified_poisson', left.overlaps(right))
            diameter = max(2*z.real.rad() for z in [left, right])
            diameter = max(diameter, *(2*z.imag.rad() for z in [left, right]))
            check('certified_poisson', diameter < arb(2)**(-bits))
            diameters.append(diameter)
        check('certified_poisson', diameters[1] < diameters[0])
        check('certified_poisson', diameters[2] < diameters[1])
    finally:
        ctx.prec = old


def check_faults():
    delta = 2, 3, [0, 1, 0, 0, 0, 0]
    a, b = (2, 3, list(range(6))), (3, 2, list(range(6)))
    check('faults_41', lcm(a[0], b[0]) != max(a[0], b[0]))
    check('faults_41', lcm(a[1], b[1]) != max(a[1], b[1]))
    check('faults_41', translate(*delta, Q(1, 2)) != translate(*delta, Q(-1, 2)))
    check('faults_41', refine(*delta, 4, 3)[1] == 0)
    check('faults_41', dilate(*delta, Q(2, 3))[2] != dilate(*delta, Q(3, 2))[2])
    check('faults_41', unit_residues(6, 1, 1) != {1})
    g = ffun_transform(*delta)[2]
    check('faults_42', g[1].imag < 0)
    check('faults_42', not g[0].contains(ball(Q(1, 2))))
    check('faults_42', ffun_transform(*ffun_transform(*delta))[2][5].contains(1))
    check('faults_42', ffun_transform(*delta)[:2] == (3, 2))
    check('faults_42', not g[1].overlaps(ffun_transform(*delta)[2][2]))
    check('faults_42', (sum(v*v.conjugate() for v in g)/2).contains(ball(Q(1, 3))))
    shifted = rterm_translate([1], 1, 0, 0, sp.Rational(1, 3))
    hat = rterm_transform(*shifted)
    check('faults_43', rvalue(hat, mp.mpf('0.25')).imag > 0)
    check('faults_43', rterm_transform([0, 1], 1, 0, 0)[0] != [0, -I])
    check('faults_43', shifted[3] == -PI/9)
    check('faults_43', rterm_dilate([1], 1, 2, 0, -2)[2] == -4)
    check('faults_43', rterm_product(([1], 1, 0, 0), ([1], 1, 0, 0))[1] == 2)
    check('faults_43', abs(rvalue(rterm_transform([1], 4, 0, 0), 0)-mp.mpf('.5')) < MARGIN)
    f = 2, 3, [1, 2, 3, 4, 5, 6]
    check('faults_44', evaluate(f, (0, 1, 1))[0] != {0})
    check('faults_44', evaluate(f, (0, 1, 4))[1])
    check('faults_44', evaluate(f, (1, 0, 5)) == (frozenset(), True))
    check('faults_44', evaluate_partial(f, [(3, Q(1), 1)])[1])
    check('faults_44', Q(sum(f[2]), 3) == 7)
    check('faults_44', Q(sum(v*v for v in f[2]), 3) == Q(91, 3))
    slow = ([1], sp.Rational(1, 5), 0, 0)
    observed = 2*sum(mp.exp(-mp.pi*n*n/5) for n in range(1, 30))
    check('faults_45', observed > lattice_tail(slow, 0, 1, 0)/2)
    t = [rterm_translate([1], 1, 0, 0, sp.Rational(1, 3))], delta
    l, r = poisson_sides(t, (4, 32))
    wrong_sign = poisson_sides(([rterm_dilate(*t[0][0], -1)], delta), (4, 32))[1].value
    check('faults_45', abs(l.value-wrong_sign) > mp.mpf('.01'))
    principal = poisson_sides(([([1], 1, 0, 0)], (1, 1, [1])), (4, 8))
    check('faults_45', abs(principal[0].value-(principal[1].value-1)) > mp.mpf('.9'))
    hat = rterm_transform(*t[0][0])
    wrong_spacing = sum(mp.exp(-2j*mp.pi*n/6)/3*rvalue(hat, mpq(Q(n, 2))) for n in range(-32, 33))
    check('faults_45', abs(l.value-wrong_spacing) > mp.mpf('.01'))
    tail_after_one = 2*sum(mp.exp(-mp.pi*n*n/5) for n in range(2, 30))
    check('faults_45', tail_after_one > lattice_tail(slow, 0, 1, 2))
    shifted = rterm_translate([1], 1, 0, 0, 1)
    beta_dropped = 2*mp.exp(-mp.pi)*analysis.series_bound(0, mp.pi, 0)
    check('faults_45', abs(rvalue(shifted, 1)) > beta_dropped)
    check('resource_examples', 1024**2 <= 2**20 < 1025**2 and 1025 < 2**62)


def main():
    print('python-flint', flint.__version__, 'FLINT', getattr(flint, '__FLINT_VERSION__', 'unreported'))
    print('Exact phases/indices; acb at 212 bits; mpmath at 60 digits, numerical margin 1e-45')
    for fn in [check_finite, check_evaluation, check_real, check_goldens,
               check_tails_poisson, check_precision, check_faults]:
        before = COUNTS.copy()
        fn()
        for group, count in COUNTS.items():
            if count != before.get(group, 0):
                print(f'{group}: {count-before.get(group, 0)} checks', flush=True)
    print(f'TOTAL: {sum(COUNTS.values())} checks')


if __name__ == '__main__':
    main()
