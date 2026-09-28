"""Reference for adf_adele and adf_cadele (work package 1.3).

Written from the public header include/adelefeld/adele.h and from docs/SPEC.md 4.1, 4.3, 4.5,
docs/conventions.md 5.5 and docs/proofs/precision.md Propositions 1, 2 and 6, not from the C
implementation. The finite coordinate is adfref.fball (exact sets). The archimedean coordinate
is an exact closed interval with rational end points (fractions.Fraction) for adf_adele, and a
pair of such intervals (real box, imaginary box) for adf_cadele.

Meaning (docs/SPEC.md 4.1). An adele is the set I x F of the ring R x A_f: a real interval and a
finite ball. A complex adele is Z x F of C x A_f: a complex box and a finite ball. The two
coordinates are independent, so arithmetic is componentwise:

    (I ; F) + (J ; G) = (I + J ; F + G)
    (I ; F) - (J ; G) = (I - J ; F - G)
    (I ; F) * (J ; G) = (I * J ; F * G)
    -(I ; F)          = (-I ; -F)
    (I ; F) + q       = (I + q ; F + q)          q an exact rational
    (I ; F) * q       = (I * q ; F * q)
    (I ; F) / q       = (I / q ; F / q)          q != 0

The finite rules are precision.md Proposition 1 (line 27), Proposition 2 (line 34) and
Proposition 6(2) (line 106). The real interval rules are the standard ones: the image of an
interval under +, -, * is again an interval, and the interval hull below contains the image. For
adf_cadele the product of two complex boxes is not a box; the generator records point witnesses
only for it, since an enclosing box is not unique.

The generator produces, for each vector record: the operands, the exact result set of the finite
part (a canonical triple), and exact rational witnesses. A witness names a rational point in each
input (for the real coordinate and for the finite ball) and the exact image point. A C
implementation satisfies the vector when its output contains every image point, in each
coordinate. For the real coordinate of adf_adele the generator also records the interval hull of
the true result, which an enclosing interval must contain.
"""

from fractions import Fraction
import random

from . import fball as _fball
from .fball import Fball


# ----------------------------------------------------------------- intervals

class Interval:
    """A closed real interval [lo, hi] with exact rational end points."""

    __slots__ = ("lo", "hi")

    def __init__(self, lo, hi):
        lo = Fraction(lo)
        hi = Fraction(hi)
        if lo > hi:
            raise ValueError("empty interval: lo > hi")
        self.lo = lo
        self.hi = hi

    @classmethod
    def point(cls, x):
        return cls(x, x)

    def is_point(self):
        return self.lo == self.hi

    def contains(self, x):
        return self.lo <= Fraction(x) <= self.hi

    def add(self, other):
        return Interval(self.lo + other.lo, self.hi + other.hi)

    def sub(self, other):
        return Interval(self.lo - other.hi, self.hi - other.lo)

    def neg(self):
        return Interval(-self.hi, -self.lo)

    def mul(self, other):
        cs = (self.lo * other.lo, self.lo * other.hi,
              self.hi * other.lo, self.hi * other.hi)
        return Interval(min(cs), max(cs))

    def add_rat(self, q):
        q = Fraction(q)
        return Interval(self.lo + q, self.hi + q)

    def mul_rat(self, q):
        q = Fraction(q)
        if q >= 0:
            return Interval(self.lo * q, self.hi * q)
        return Interval(self.hi * q, self.lo * q)

    def div_rat(self, q):
        q = Fraction(q)
        if q == 0:
            raise ZeroDivisionError("division by the exact zero")
        return self.mul_rat(1 / q)

    def sample(self, rng):
        """A rational point of the interval, chosen for the witness."""
        if self.lo == self.hi:
            return self.lo
        # An integer, or a fraction with a small denominator, at or between the ends.
        den = rng.choice((1, 2, 3, 5, 7))
        num = rng.randint(0, den)
        x = self.lo + (self.hi - self.lo) * Fraction(num, den)
        if rng.random() < 0.15:
            x = rng.choice((self.lo, self.hi))
        return x

    def __eq__(self, other):
        return isinstance(other, Interval) and self.lo == other.lo and self.hi == other.hi

    def __repr__(self):
        return "Interval(%s, %s)" % (self.lo, self.hi)


class CBox:
    """A complex box: real part in re, imaginary part in im, both rational intervals."""

    __slots__ = ("re", "im")

    def __init__(self, re, im):
        self.re = re if isinstance(re, Interval) else Interval.point(re)
        self.im = im if isinstance(im, Interval) else Interval.point(im)

    @classmethod
    def point(cls, z):
        """A complex point given as a pair (re, im) of rationals."""
        return cls(Interval.point(z[0]), Interval.point(z[1]))

    def add(self, other):
        return CBox(self.re.add(other.re), self.im.add(other.im))

    def sub(self, other):
        return CBox(self.re.sub(other.re), self.im.sub(other.im))

    def neg(self):
        return CBox(self.re.neg(), self.im.neg())

    def mul(self, other):
        # (a + b i)(c + d i) = (a c - b d) + (a d + b c) i, each product an interval.
        re = self.re.mul(other.re).sub(self.im.mul(other.im))
        im = self.re.mul(other.im).add(self.im.mul(other.re))
        return CBox(re, im)

    def add_rat(self, q):
        return CBox(self.re.add_rat(q), self.im)

    def mul_rat(self, q):
        return CBox(self.re.mul_rat(q), self.im.mul_rat(q))

    def div_rat(self, q):
        return CBox(self.re.div_rat(q), self.im.div_rat(q))

    def sample(self, rng):
        return (self.re.sample(rng), self.im.sample(rng))

    def contains(self, z):
        return self.re.contains(z[0]) and self.im.contains(z[1])


# ------------------------------------------------------------------- adeles

def _point_of_ball(ball, rng):
    """A rational point of the finite ball a + N Zhat: (A + k H)/d for an integer k (k = 0 when
    H = 0, where every k gives the same point)."""
    if ball.H == 0:
        return Fraction(ball.A, ball.d)
    k = rng.randint(-4, 4)
    return Fraction(ball.A + k * ball.H, ball.d)


class Adele:
    """An adele: a real interval and a finite ball, independent (docs/SPEC.md 4.1)."""

    __slots__ = ("re", "fin")

    def __init__(self, re, fin):
        self.re = re if isinstance(re, Interval) else Interval(re, re)
        self.fin = fin if isinstance(fin, Fball) else Fball(fin, 0, 1)

    @classmethod
    def of_rat(cls, q):
        q = Fraction(q)
        return cls(Interval.point(q), Fball(q.numerator, 0, q.denominator))

    def add(self, other):
        return Adele(self.re.add(other.re), _fball.add(self.fin, other.fin))

    def sub(self, other):
        return Adele(self.re.sub(other.re), _fball.sub(self.fin, other.fin))

    def mul(self, other):
        return Adele(self.re.mul(other.re), _fball.mul(self.fin, other.fin))

    def neg(self):
        return Adele(self.re.neg(), _fball.neg(self.fin))

    def add_rat(self, q):
        q = Fraction(q)
        return Adele(self.re.add_rat(q), _fball.add(self.fin, Fball(q.numerator, 0, q.denominator)))

    def mul_rat(self, q):
        q = Fraction(q)
        return Adele(self.re.mul_rat(q), _fball.scale(self.fin, q))

    def div_rat(self, q):
        q = Fraction(q)
        if q == 0:
            raise ZeroDivisionError("division by the exact zero")
        return Adele(self.re.div_rat(q), _fball.scale(self.fin, 1 / q))


class Cadele:
    """A complex adele: a complex box and a finite ball (docs/SPEC.md 4.1)."""

    __slots__ = ("inf", "fin")

    def __init__(self, inf, fin):
        self.inf = inf if isinstance(inf, CBox) else CBox(Interval.point(inf), Interval.point(0))
        self.fin = fin if isinstance(fin, Fball) else Fball(fin, 0, 1)

    @classmethod
    def of_rat(cls, q):
        q = Fraction(q)
        return cls(CBox(Interval.point(q), Interval.point(0)), Fball(q.numerator, 0, q.denominator))

    def add(self, other):
        return Cadele(self.inf.add(other.inf), _fball.add(self.fin, other.fin))

    def sub(self, other):
        return Cadele(self.inf.sub(other.inf), _fball.sub(self.fin, other.fin))

    def mul(self, other):
        return Cadele(self.inf.mul(other.inf), _fball.mul(self.fin, other.fin))

    def neg(self):
        return Cadele(self.inf.neg(), _fball.neg(self.fin))

    def add_rat(self, q):
        q = Fraction(q)
        return Cadele(self.inf.add_rat(q), _fball.add(self.fin, Fball(q.numerator, 0, q.denominator)))

    def mul_rat(self, q):
        q = Fraction(q)
        return Cadele(self.inf.mul_rat(q), _fball.scale(self.fin, q))

    def div_rat(self, q):
        q = Fraction(q)
        if q == 0:
            raise ZeroDivisionError("division by the exact zero")
        return Cadele(self.inf.div_rat(q), _fball.scale(self.fin, 1 / q))


# ------------------------------------------------------------- exactness check

def dyadic_exact(q, prec):
    """True when arb_set_fmpq(q, prec) is exact: q is dyadic and its odd mantissa fits in prec
    bits (docs/SPEC.md 4.1; arb.rst:167, "rounded to prec bits"). The exact zero is exact for
    every prec."""
    q = Fraction(q)
    if q == 0:
        return True
    n = abs(q.numerator)
    d = q.denominator
    # d must be a power of two.
    if d & (d - 1) != 0:
        return False
    return n.bit_length() <= prec
