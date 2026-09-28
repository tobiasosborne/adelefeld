#!/usr/bin/env python3
"""Mutation testing for the reference (PLAN section 7: a test counts only if a wrong
implementation would fail it).

Each mutant is a small deliberate wrongness in one rule. For every mutant the test suite under
tests/ref/tests is run; the mutant is killed when at least one test fails or errors. The script
prints the number of mutants killed and surviving and lists the survivors.

Run:  python3 tests/ref/mutants.py
"""
import importlib
import os
import sys
import unittest
from fractions import Fraction
from math import floor, gcd

REF = os.path.dirname(os.path.abspath(__file__))
TESTS = os.path.join(REF, "tests")
for path in (REF, TESTS):
    if path not in sys.path:
        sys.path.insert(0, path)

from adfref import fball, membership, policies, recon  # noqa: E402
from adfref.fball import Fball  # noqa: E402
from adfref.rat import is_integer, qgcd, qlcm  # noqa: E402

TEST_MODULES = [
    "test_tables",
    "test_arithmetic",
    "test_canonical",
    "test_policies",
    "test_recon",
    "test_membership",
]


def _add_lcm(x, y):
    return Fball.from_center_radius(x.center + y.center, qlcm(x.radius, y.radius))


def _mul_drop_NM(x, y):
    a, N = x.center, x.radius
    b, M = y.center, y.radius
    return Fball.from_center_radius(a * b, qgcd(a * M, b * N))


def _canon_no_mod(A, H, d):
    if d == 0:
        raise ZeroDivisionError
    if H < 0:
        raise ValueError
    if d < 0:
        A, d = -A, -d
    if H == 0:
        g = gcd(abs(A), d)
        return A // g, 0, d // g
    g = gcd(gcd(abs(A), H), d)
    return A // g, H // g, d // g


def _canon_no_gcd(A, H, d):
    if d == 0:
        raise ZeroDivisionError
    if H < 0:
        raise ValueError
    if d < 0:
        A, d = -A, -d
    if H == 0:
        return A, 0, d
    return A % H, H, d


def _qgcd_largest(*xs):
    vals = [Fraction(x) for x in xs]
    vals = [v for v in vals if v != 0]
    if not vals:
        return Fraction(0)
    return max(vals, key=abs)


def _contains_drop_radius(x, y):
    M = y.radius
    if M == 0:
        return x.radius == 0 and x.center == y.center
    return is_integer((x.center - y.center) / M)


def _contains_drop_center(x, y):
    N, M = x.radius, y.radius
    if M == 0:
        return N == 0 and x.center == y.center
    if N == 0:
        return is_integer((x.center - y.center) / M)
    return is_integer(N / M)


def _overlaps_lcm(x, y):
    g = qlcm(x.radius, y.radius)
    if g == 0:
        return x.center == y.center
    return is_integer((x.center - y.center) / g)


def _equal_set_drop_int(x, y):
    return x.radius == y.radius


def _neg_identity(x):
    return Fball.from_center_radius(x.center, x.radius)


def _scale_center_only(x, q):
    return Fball.from_center_radius(Fraction(q) * x.center, x.radius)


def _scaled_add_wrong(x, y):
    if not isinstance(x, policies.ScaledBall) or not isinstance(y, policies.ScaledBall):
        return policies.scaled_add(x, y)
    return policies.ScaledBall(x.s + y.s, x.u + y.u, x.K)


def _scaled_add_ignore_context(x, y):
    """The G9 defect: the non-exact path uses x.K without checking x.K == y.K."""
    if not isinstance(x, policies.ScaledBall) or not isinstance(y, policies.ScaledBall):
        return policies.scaled_add(x, y)
    g = qgcd(x.s, y.s)
    A = int(x.s / g)
    B = int(y.s / g)
    return policies.ScaledBall(g, (A * x.u + B * y.u) % x.K, x.K)


def _scaled_mul_residue_sum(x, y):
    if not isinstance(x, policies.ScaledBall) or not isinstance(y, policies.ScaledBall):
        return policies.scaled_mul(x, y)
    return policies.ScaledBall(x.s * y.s, x.u + y.u, x.K)


def _scaled_mul_scale_sum(x, y):
    if not isinstance(x, policies.ScaledBall) or not isinstance(y, policies.ScaledBall):
        return policies.scaled_mul(x, y)
    return policies.ScaledBall(x.s + y.s, x.u * y.u, x.K)


def _cap_min(ball, C):
    C = Fraction(C)
    if ball.radius == 0:
        return ball
    return Fball.from_center_radius(ball.center, min(ball.radius, C))


def _membership_center_only(x, ball):
    return Fraction(x) == ball.center


def _recon_floor_min(ball, lo, hi):
    lo, hi = Fraction(lo), Fraction(hi)
    if lo > hi:
        return recon.Result(recon.NONE, [])
    a, N = ball.center, ball.radius
    if N == 0:
        return recon._one(a) if lo <= a <= hi else recon._none()
    kmin = floor((lo - a) / N)
    kmax = floor((hi - a) / N)
    if kmin > kmax:
        return recon._none()
    sols = [a + N * k for k in range(kmin, kmax + 1)]
    return recon.Result(recon.ONE if len(sols) == 1 else recon.SEVERAL, sols)


def _recon_kmin_zero(ball, lo, hi):
    lo, hi = Fraction(lo), Fraction(hi)
    if lo > hi:
        return recon.Result(recon.NONE, [])
    a, N = ball.center, ball.radius
    if N == 0:
        return recon._one(a) if lo <= a <= hi else recon._none()
    kmin = 0
    kmax = floor((hi - a) / N)
    if kmin > kmax:
        return recon._none()
    sols = [a + N * k for k in range(kmin, kmax + 1)]
    return recon.Result(recon.ONE if len(sols) == 1 else recon.SEVERAL, sols)


def _patch(module, attr, value, saved):
    saved.append((module, attr, getattr(module, attr)))
    setattr(module, attr, value)


MUTANTS = []


def mutant(name):
    def register(fn):
        MUTANTS.append((name, fn))
        return fn
    return register


@mutant("sum radius uses lcm instead of gcd")
def _m_add_lcm(saved):
    _patch(fball, "add", _add_lcm, saved)


@mutant("product radius drops the term N M")
def _m_mul_drop_NM(saved):
    _patch(fball, "mul", _mul_drop_NM, saved)


@mutant("canonical form does not reduce A modulo H")
def _m_canon_no_mod(saved):
    _patch(fball, "canon", _canon_no_mod, saved)


@mutant("canonical form does not divide by gcd(A,H,d)")
def _m_canon_no_gcd(saved):
    _patch(fball, "canon", _canon_no_gcd, saved)


@mutant("qgcd returns the largest argument")
def _m_qgcd_largest(saved):
    _patch(fball, "qgcd", _qgcd_largest, saved)
    _patch(policies, "qgcd", _qgcd_largest, saved)


@mutant("contains drops the radius condition N/M")
def _m_contains_drop_radius(saved):
    _patch(fball, "contains", _contains_drop_radius, saved)


@mutant("contains drops the centre condition (a-b)/M")
def _m_contains_drop_center(saved):
    _patch(fball, "contains", _contains_drop_center, saved)


@mutant("overlaps uses lcm instead of gcd")
def _m_overlaps_lcm(saved):
    _patch(fball, "overlaps", _overlaps_lcm, saved)


@mutant("equal_set tests only the radii")
def _m_equal_set_drop_int(saved):
    _patch(fball, "equal_set", _equal_set_drop_int, saved)


@mutant("negation forgets the sign of the centre")
def _m_neg_identity(saved):
    _patch(fball, "neg", _neg_identity, saved)


@mutant("scaling does not scale the radius")
def _m_scale_center_only(saved):
    _patch(fball, "scale", _scale_center_only, saved)


@mutant("scaled sum adds the scales and the residues")
def _m_scaled_add_wrong(saved):
    _patch(policies, "scaled_add", _scaled_add_wrong, saved)


@mutant("scaled sum ignores a differing context (G9)")
def _m_scaled_add_ignore_context(saved):
    _patch(policies, "scaled_add", _scaled_add_ignore_context, saved)


@mutant("scaled product adds the residues")
def _m_scaled_mul_residue_sum(saved):
    _patch(policies, "scaled_mul", _scaled_mul_residue_sum, saved)


@mutant("scaled product adds the scales")
def _m_scaled_mul_scale_sum(saved):
    _patch(policies, "scaled_mul", _scaled_mul_scale_sum, saved)


@mutant("absolute cap uses min(R, C) instead of gcd(R, C)")
def _m_cap_min(saved):
    _patch(policies, "absolute_cap", _cap_min, saved)


@mutant("membership tests only the exact centre")
def _m_membership_center_only(saved):
    _patch(membership, "rational_in_fball", _membership_center_only, saved)


@mutant("reconstruction uses floor for the lower bound")
def _m_recon_floor_min(saved):
    _patch(recon, "reconstruct", _recon_floor_min, saved)


@mutant("reconstruction ignores the lower bound")
def _m_recon_kmin_zero(saved):
    _patch(recon, "reconstruct", _recon_kmin_zero, saved)


def _build_suite():
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    for name in TEST_MODULES:
        module = importlib.import_module(name)
        suite.addTests(loader.loadTestsFromModule(module))
    return suite


def _run():
    stream = open(os.devnull, "w")
    try:
        return unittest.TextTestRunner(stream=stream, verbosity=0).run(_build_suite())
    finally:
        stream.close()


def main():
    # baseline: the unmutated suite must pass
    baseline = _run()
    print("baseline: %d tests, failures %d, errors %d"
          % (baseline.testsRun, len(baseline.failures), len(baseline.errors)))
    killed = 0
    survivors = []
    for name, apply in MUTANTS:
        saved = []
        apply(saved)
        try:
            result = _run()
        finally:
            for module, attr, value in reversed(saved):
                setattr(module, attr, value)
        if result.wasSuccessful():
            survivors.append(name)
        else:
            killed += 1
    print("mutants killed: %d" % killed)
    print("mutants survived: %d" % len(survivors))
    for name in survivors:
        print("  SURVIVED: %s" % name)
    return 0 if not survivors else 1


if __name__ == "__main__":
    sys.exit(main())
