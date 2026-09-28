"""Finite balls (A + H Zhat)/d, canonical, with the tight rules of SPEC 4.3.

Canonical form (SPEC 4.1, D2):
  H > 0:  0 <= A < H and gcd(A, H, d) = 1
  H = 0:  A/d in lowest terms
The set is the centre a = A/d plus the radius N = H/d times Zhat. Radius 0 is the single point.

The arithmetic is the tight rule of docs/proofs/precision.md, Propositions 1 and 2. The set
predicates and the three-valued comparison are Proposition 3 (and the radius-zero rules).
"""
import enum
from fractions import Fraction
from math import gcd

from .rat import is_integer, qgcd


class Comparison(enum.IntEnum):
    """Point comparison returns int with the fixed values, not a status (docs/conventions.md 2.1, closure C1):
    ADF_CMP_EQUAL = 0 (both exact and equal), ADF_CMP_DIFFERENT = 1 (disjoint), ADF_CMP_UNDECIDED = 2."""
    EQUAL = 0
    DIFFERENT = 1
    UNDECIDED = 2


EQUAL = Comparison.EQUAL
DIFFERENT = Comparison.DIFFERENT
UNDECIDED = Comparison.UNDECIDED


def canon(A, H, d):
    """Canonical triple of the set (A + H Zhat)/d. Raises on d = 0 or H < 0.

    Uniqueness: let (A, H, d) and (A', H', d') be canonical for one set. If H = H' = 0 both are
    the same rational in lowest terms. Otherwise H/d = H'/d' = n/m in lowest terms, so d = m e,
    d' = m e', H = n e, H' = n e', and gcd(A, H, d) = gcd(A, e). Equality of the sets gives
    A e' = A' e (mod n e e'); reducing modulo e gives e | e', and symmetrically e' | e, so e = e'.
    Then A = A' because both lie in [0, n e).
    """
    if not isinstance(A, int) or not isinstance(H, int) or not isinstance(d, int):
        raise TypeError("A, H, d must be integers")
    if d == 0:
        raise ZeroDivisionError("denominator d must not be 0")
    if H < 0:
        raise ValueError("radius H must not be negative")
    if d < 0:
        A, d = -A, -d
    if H == 0:
        g = gcd(abs(A), d)
        return A // g, 0, d // g
    g = gcd(gcd(abs(A), H), d)
    A, H, d = A // g, H // g, d // g
    A %= H
    return A, H, d


class Fball:
    """The finite ball (A + H Zhat)/d in canonical form."""

    __slots__ = ("A", "H", "d")

    def __init__(self, A, H, d=1):
        self.A, self.H, self.d = canon(A, H, d)

    @classmethod
    def from_center_radius(cls, c, R):
        """Ball with centre c and radius R, R a non-negative rational."""
        c = Fraction(c)
        R = Fraction(R)
        if R < 0:
            raise ValueError("radius must not be negative")
        if R == 0:
            return cls(c.numerator, 0, c.denominator)
        den = c.denominator * R.denominator // gcd(c.denominator, R.denominator)
        return cls(int(c * den), int(R * den), den)

    @property
    def center(self):
        return Fraction(self.A, self.d)

    @property
    def radius(self):
        return Fraction(self.H, self.d)

    @property
    def is_exact(self):
        return self.H == 0

    def as_tuple(self):
        return self.A, self.H, self.d

    def __eq__(self, other):
        return isinstance(other, Fball) and self.as_tuple() == other.as_tuple()

    def __hash__(self):
        return hash(self.as_tuple())

    def __repr__(self):
        return "Fball(%d, %d, %d)" % self.as_tuple()


def add(x, y):
    """Tight sum: (a + N Zhat) + (b + M Zhat) = (a + b) + gcd(N, M) Zhat (Proposition 1)."""
    return Fball.from_center_radius(x.center + y.center, qgcd(x.radius, y.radius))


def neg(x):
    """Negation: -(a + N Zhat) = -a + N Zhat (Zhat is symmetric)."""
    return Fball.from_center_radius(-x.center, x.radius)


def sub(x, y):
    """Tight difference, via negation and the tight sum."""
    return add(x, neg(y))


def mul(x, y):
    """Tight product radius gcd(a M, b N, N M) (Proposition 2)."""
    a, N = x.center, x.radius
    b, M = y.center, y.radius
    return Fball.from_center_radius(a * b, qgcd(a * M, b * N, N * M))


def scale(x, q):
    """Multiply by an exact rational q: q (a + N Zhat) = q a + |q| N Zhat (for q != 0)."""
    q = Fraction(q)
    return Fball.from_center_radius(q * x.center, abs(q) * x.radius)


def contains_rational(ball, x):
    """The rational x lies in the ball (ball arithmetic; membership.py is independent)."""
    x = Fraction(x)
    if ball.radius == 0:
        return x == ball.center
    return is_integer((x - ball.center) / ball.radius)


def equal_set(x, y):
    """SPEC 4.2: N = M and (a - b)/N an integer; radius zero is a point."""
    if x.radius == 0 or y.radius == 0:
        return x.radius == y.radius and x.center == y.center
    return x.radius == y.radius and is_integer((x.center - y.center) / x.radius)


def overlaps(x, y):
    """SPEC 4.2: (a - b)/gcd(N, M) an integer."""
    g = qgcd(x.radius, y.radius)
    if g == 0:
        return x.center == y.center
    return is_integer((x.center - y.center) / g)


def contains(x, y):
    """SPEC 4.2: is the ball x inside the ball y? N/M and (a - b)/M integers."""
    N, M = x.radius, y.radius
    if M == 0:
        return N == 0 and x.center == y.center
    if N == 0:
        return is_integer((x.center - y.center) / M)
    return is_integer(N / M) and is_integer((x.center - y.center) / M)


def compare(x, y):
    """Three-valued comparison of the two unknown points, as int codes: EQUAL = 0, DIFFERENT = 1,
    UNDECIDED = 2 (docs/conventions.md 2.1, closure C1)."""
    if x.radius == 0 and y.radius == 0:
        return EQUAL if x.center == y.center else DIFFERENT
    if not overlaps(x, y):
        return DIFFERENT
    return UNDECIDED
