"""Precision policies of SPEC 4.4: scaled residues and the absolute cap.

Scaled residue context: one integer K >= 1; a value is s (u + K Zhat), s a positive rational,
0 <= u < K, so its radius is s K. The sum and product are Proposition 5, with g = gcd(s, t),
A = s/g, B = t/g:

    sum:      scale g,   residue (A u + B v) mod K      (tight)
    product:  scale s t, residue (u v) mod K            (encloses the tight radius s t K gcd(u, v, K))

`convert_from_tight` is Proposition 6(1): s = gcd(a, R/K), u = (a/s) mod K, and it reports when the
radius s K is strictly finer than R (a loss of precision). Exact values stay exact (no loss).

Absolute cap: a cap C is given; after a tight operation the radius R is replaced by gcd(R, C),
which divides R and is therefore coarser or equal. An exact value is kept exact.
"""
from fractions import Fraction

from . import fball as _fball
from .fball import Fball
from .rat import qgcd


class ScaledBall:
    """The set s (u + K Zhat) in a scaled-residue context."""

    __slots__ = ("s", "u", "K")

    def __init__(self, s, u, K):
        s = Fraction(s)
        if s <= 0:
            raise ValueError("scale s must be positive")
        if K < 1:
            raise ValueError("modulus K must be at least 1")
        self.s = s
        self.u = int(u) % int(K)
        self.K = int(K)

    @property
    def radius(self):
        return self.s * self.K

    def to_fball(self):
        return Fball.from_center_radius(self.s * self.u, self.radius)

    def __eq__(self, other):
        return (isinstance(other, ScaledBall) and self.s == other.s
                and self.u == other.u and self.K == other.K)

    def __repr__(self):
        return "ScaledBall(%s, %d, %d)" % (self.s, self.u, self.K)


class Exact:
    """An exact rational inside a policy (radius 0)."""

    __slots__ = ("q",)

    def __init__(self, q):
        self.q = Fraction(q)

    @property
    def radius(self):
        return Fraction(0)

    def to_fball(self):
        return Fball.from_center_radius(self.q, 0)

    def __eq__(self, other):
        return isinstance(other, Exact) and self.q == other.q

    def __repr__(self):
        return "Exact(%s)" % (self.q,)


def convert_from_tight(ball, K):
    """Return (value, lost). Proposition 6(1); lost is True when the radius becomes strictly finer."""
    K = int(K)
    if K < 1:
        raise ValueError("modulus K must be at least 1")
    a, R = ball.center, ball.radius
    if R == 0:
        return Exact(a), False
    s = qgcd(a, R / K)
    u = int(a / s) % K
    lost = (s * K != R)
    return ScaledBall(s, u, K), lost


def _context(x, y):
    K = x.K if isinstance(x, ScaledBall) else y.K
    if isinstance(x, ScaledBall) and isinstance(y, ScaledBall) and x.K != y.K:
        raise ValueError("scaled values must share the modulus K")
    return K


def scaled_add(x, y):
    """Proposition 5(1); exact operands fall back through the tight sum. The two operands must share
    the context K; otherwise a ValueError is raised (the documented precondition of SPEC 4.4)."""
    if isinstance(x, Exact) and isinstance(y, Exact):
        return Exact(x.q + y.q)
    if isinstance(x, Exact) or isinstance(y, Exact):
        K = _context(x, y)
        tight = _fball.add(x.to_fball(), y.to_fball())
        value, _ = convert_from_tight(tight, K)
        return value
    g = qgcd(x.s, y.s)
    K = _context(x, y)
    A = int(x.s / g)
    B = int(y.s / g)
    return ScaledBall(g, (A * x.u + B * y.u) % K, K)


def scaled_neg(x):
    if isinstance(x, Exact):
        return Exact(-x.q)
    return ScaledBall(x.s, (-x.u) % x.K, x.K)


def scaled_sub(x, y):
    return scaled_add(x, scaled_neg(y))


def scaled_mul(x, y):
    """Proposition 5(2); an exact factor uses Proposition 6(2)."""
    if isinstance(x, Exact) and isinstance(y, Exact):
        return Exact(x.q * y.q)
    if isinstance(x, Exact) or isinstance(y, Exact):
        q = x.q if isinstance(x, Exact) else y.q
        other = y if isinstance(x, Exact) else x
        if q == 0:
            return Exact(0)
        sign = 1 if q > 0 else -1
        return ScaledBall(abs(q) * other.s, (sign * other.u) % other.K, other.K)
    if x.K != y.K:
        raise ValueError("scaled values must share the modulus K")
    return ScaledBall(x.s * y.s, (x.u * y.u) % x.K, x.K)


def scaled_scale(x, q):
    """Exact scalar times a policy value; Proposition 6(2)."""
    q = Fraction(q)
    if q == 0:
        return Exact(0)
    if isinstance(x, Exact):
        return Exact(q * x.q)
    sign = 1 if q > 0 else -1
    return ScaledBall(abs(q) * x.s, (sign * x.u) % x.K, x.K)


def absolute_cap(ball, C):
    """Replace the radius R by gcd(R, C). Exact values are kept exact."""
    C = Fraction(C)
    if C <= 0:
        raise ValueError("cap C must be positive")
    if ball.radius == 0:
        return ball
    return Fball.from_center_radius(ball.center, qgcd(ball.radius, C))
