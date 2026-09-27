"""Rational reconstruction from a full adelic ball and a real interval (SPEC 9.2, first bullet).

For N > 0 the ball (a + N Zhat) meets Q in the arithmetic progression a + N Z (precision.md,
Lemma 1). Intersecting with the real interval [lo, hi] gives finitely many candidates. The result
status is one of "none", "one", "several".
"""
from fractions import Fraction
from math import ceil, floor

NONE = "none"
ONE = "one"
SEVERAL = "several"


class Result:
    __slots__ = ("status", "solutions")

    def __init__(self, status, solutions):
        self.status = status
        self.solutions = list(solutions)

    def __eq__(self, other):
        return (isinstance(other, Result) and self.status == other.status
                and self.solutions == other.solutions)

    def __repr__(self):
        return "Result(%r, %r)" % (self.status, self.solutions)


def _none():
    return Result(NONE, [])


def _one(x):
    return Result(ONE, [x])


def reconstruct(ball, lo, hi):
    """Candidates x in (a + N Zhat) meet [lo, hi]. N = 0: only the exact centre a is a candidate."""
    lo = Fraction(lo)
    hi = Fraction(hi)
    if lo > hi:
        return _none()
    a, N = ball.center, ball.radius
    if N == 0:
        if lo <= a <= hi:
            return _one(a)
        return _none()
    kmin = ceil((lo - a) / N)
    kmax = floor((hi - a) / N)
    if kmin > kmax:
        return _none()
    solutions = [a + N * k for k in range(kmin, kmax + 1)]
    if len(solutions) == 1:
        return Result(ONE, solutions)
    return Result(SEVERAL, solutions)
