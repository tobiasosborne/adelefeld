"""Enclosure by enumeration, tightness witnesses, and the three-valued comparison.

Enclosure: for small radii, every sum/product of members from a window of rationals lies in the
result. Tightness: the result radius is the gcd of the differences actually attained, using the
witness pairs of docs/proofs/precision.md, Propositions 1 and 2.
"""
import os
import random
import sys
import unittest
from fractions import Fraction as F

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from adfref import fball
from adfref.fball import Fball
from adfref.rat import qgcd


def B(a, N):
    return Fball.from_center_radius(F(a), F(N))


def members(ball, k=6):
    a, N = ball.center, ball.radius
    if N == 0:
        return [a]
    return [a + N * i for i in range(-k, k + 1)]


class EnclosureEnumeration(unittest.TestCase):
    def test_sum_enclosure(self):
        random.seed(11)
        n = 0
        for _ in range(300):
            a = F(random.randint(-9, 9), random.randint(1, 5))
            b = F(random.randint(-9, 9), random.randint(1, 5))
            N = F(random.randint(0, 9), random.randint(1, 4))
            M = F(random.randint(0, 9), random.randint(1, 4))
            x, y = B(a, N), B(b, M)
            r = fball.add(x, y)
            for s in members(x, 4):
                for t in members(y, 4):
                    self.assertTrue(fball.contains_rational(r, s + t), (a, N, b, M, s, t))
                    n += 1
        self.assertGreater(n, 1000)

    def test_product_enclosure(self):
        random.seed(12)
        n = 0
        for _ in range(300):
            a = F(random.randint(-6, 6), random.randint(1, 4))
            b = F(random.randint(-6, 6), random.randint(1, 4))
            N = F(random.randint(0, 6), random.randint(1, 3))
            M = F(random.randint(0, 6), random.randint(1, 3))
            x, y = B(a, N), B(b, M)
            r = fball.mul(x, y)
            for s in members(x, 4):
                for t in members(y, 4):
                    self.assertTrue(fball.contains_rational(r, s * t), (a, N, b, M, s, t))
                    n += 1
        self.assertGreater(n, 1000)

    def test_scale_enclosure(self):
        random.seed(13)
        for _ in range(200):
            a = F(random.randint(-9, 9), random.randint(1, 5))
            N = F(random.randint(0, 9), random.randint(1, 4))
            q = F(random.randint(-9, 9), random.randint(1, 5))
            x = B(a, N)
            r = fball.scale(x, q)
            for s in members(x, 4):
                self.assertTrue(fball.contains_rational(r, q * s))


class TightnessWitnesses(unittest.TestCase):
    def test_sum_tightness_witnesses(self):
        random.seed(21)
        for _ in range(300):
            a = F(random.randint(-9, 9), random.randint(1, 5))
            b = F(random.randint(-9, 9), random.randint(1, 5))
            N = F(random.randint(0, 9), random.randint(1, 4))
            M = F(random.randint(0, 9), random.randint(1, 4))
            x, y = B(a, N), B(b, M)
            r = fball.add(x, y)
            diffs = [(a + N * i) + (b + M * j) - (a + b)
                     for i, j in [(0, 0), (1, 0), (0, 1), (1, 1)]]
            self.assertEqual(qgcd(*diffs), r.radius)

    def test_product_tightness_witnesses(self):
        random.seed(22)
        for _ in range(300):
            a = F(random.randint(-9, 9), random.randint(1, 5))
            b = F(random.randint(-9, 9), random.randint(1, 5))
            N = F(random.randint(0, 9), random.randint(1, 4))
            M = F(random.randint(0, 9), random.randint(1, 4))
            x, y = B(a, N), B(b, M)
            r = fball.mul(x, y)
            diffs = [(a + N * i) * (b + M * j) - a * b
                     for i, j in [(0, 0), (0, 1), (1, 0), (1, 1)]]
            self.assertEqual(qgcd(*diffs), r.radius)

    def test_product_tightness_needs_NM(self):
        # witnesses with a = b = 0 force the term N M
        x, y = B(0, 2), B(0, 3)
        r = fball.mul(x, y)
        self.assertEqual(r.radius, F(6))
        self.assertEqual(r.center, F(0))

    def test_no_smaller_ball_contains_the_products(self):
        # a ball with a radius that is a proper multiple of the tight radius (a strictly smaller
        # set) misses one of the witnesses of the proof
        x, y = B(1, 2), B(1, 3)
        r = fball.mul(x, y)
        self.assertEqual(r.radius, qgcd(1 * 3, 1 * 2, 2 * 3))  # gcd(3,2,6) = 1
        for mult in (2, 3, 4, 6):
            bad = Fball.from_center_radius(r.center, r.radius * mult)
            self.assertFalse(fball.contains(r, bad), mult)


class NegationAndDifference(unittest.TestCase):
    def test_negation(self):
        self.assertEqual(fball.neg(B(3, 12)).as_tuple(), (9, 12, 1))
        self.assertEqual(fball.neg(B(0, 0)).as_tuple(), (0, 0, 1))
        self.assertEqual(fball.neg(B(F(1, 3), 0)).center, F(-1, 3))
        self.assertEqual(fball.neg(B(F(1, 3), 0)).radius, F(0))

    def test_difference(self):
        r = fball.sub(B(3, 12), B(5, 18))
        self.assertEqual((r.center, r.radius), (F(4), F(6)))

    def test_negation_is_an_involution(self):
        random.seed(23)
        for _ in range(200):
            a = F(random.randint(-9, 9), random.randint(1, 5))
            N = F(random.randint(0, 9), random.randint(1, 4))
            x = B(a, N)
            self.assertTrue(fball.equal_set(fball.neg(fball.neg(x)), x))

    def test_correlations_are_not_kept(self):
        # SPEC 4.3: x - x on a ball is a ball around 0, not 0
        x = B(5, 6)
        d = fball.sub(x, x)
        self.assertEqual((d.center, d.radius), (F(0), F(6)))
        self.assertFalse(fball.equal_set(d, B(0, 0)))


class ThreeValuedComparison(unittest.TestCase):
    def test_certainly_equal(self):
        self.assertEqual(fball.compare(B(1, 0), B(1, 0)), fball.EQUAL)

    def test_certainly_different(self):
        self.assertEqual(fball.compare(B(1, 0), B(2, 0)), fball.DIFFERENT)
        self.assertEqual(fball.compare(B(0, 2), B(1, 2)), fball.DIFFERENT)
        self.assertEqual(fball.compare(B(1, 0), B(2, 2)), fball.DIFFERENT)

    def test_undecided(self):
        self.assertEqual(fball.compare(B(1, 0), B(1, 2)), fball.UNDECIDED)
        self.assertEqual(fball.compare(B(0, 2), B(0, 4)), fball.UNDECIDED)
        self.assertEqual(fball.compare(B(0, 2), B(2, 4)), fball.UNDECIDED)


if __name__ == "__main__":
    unittest.main()
