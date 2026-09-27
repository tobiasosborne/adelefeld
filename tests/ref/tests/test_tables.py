"""Literal rows of the tables of SPEC.md sections 4.2, 4.3 and 4.4.

Each test states one row exactly as in the specification. The reference is the oracle;
these tests are meant to fail against a wrong implementation (PLAN section 7).
"""
import os
import sys
import unittest
from fractions import Fraction as F

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from adfref import fball
from adfref.fball import Fball


def B(a, N):
    return Fball.from_center_radius(F(a), F(N))


class Spec42Predicates(unittest.TestCase):
    """Rows of the predicate table of SPEC 4.2, with the radius-zero rules."""

    def test_equal_set_row(self):
        # N = M and (a - b)/N an integer
        self.assertTrue(fball.equal_set(B(3, 12), B(15, 12)))
        self.assertFalse(fball.equal_set(B(3, 12), B(9, 12)))
        self.assertFalse(fball.equal_set(B(3, 12), B(3, 24)))

    def test_overlaps_row(self):
        # (a - b)/gcd(N, M) an integer
        self.assertTrue(fball.overlaps(B(0, 2), B(0, 1)))
        self.assertFalse(fball.overlaps(B(0, 2), B(1, 2)))
        self.assertTrue(fball.overlaps(B(0, 2), B(1, 3)))

    def test_contains_row(self):
        # N/M an integer and (a - b)/M an integer
        self.assertTrue(fball.contains(B(0, 2), B(0, 1)))
        self.assertFalse(fball.contains(B(0, 1), B(0, 2)))
        self.assertFalse(fball.contains(B(1, 2), B(0, 2)))

    def test_radius_zero_is_a_point(self):
        self.assertTrue(fball.equal_set(B(1, 0), B(1, 0)))
        self.assertFalse(fball.equal_set(B(1, 0), B(2, 0)))
        self.assertTrue(fball.contains(B(7, 0), B(1, 3)))
        self.assertFalse(fball.contains(B(8, 0), B(1, 3)))
        self.assertFalse(fball.contains(B(1, 3), B(1, 0)))
        self.assertTrue(fball.contains(B(1, 0), B(1, 0)))
        self.assertTrue(fball.overlaps(B(7, 0), B(1, 3)))
        self.assertFalse(fball.overlaps(B(8, 0), B(1, 3)))
        self.assertFalse(fball.overlaps(B(1, 0), B(2, 0)))

    def test_overlap_is_not_transitive(self):
        self.assertTrue(fball.overlaps(B(0, 2), B(0, 1)))
        self.assertTrue(fball.overlaps(B(0, 1), B(1, 2)))
        self.assertFalse(fball.overlaps(B(0, 2), B(1, 2)))


class Spec43Rules(unittest.TestCase):
    """Rows of the arithmetic tables of SPEC 4.3."""

    def test_sum_formula(self):
        r = fball.add(B(3, 12), B(5, 18))
        self.assertEqual((r.center, r.radius), (F(2), F(6)))

    def test_product_formula(self):
        r = fball.mul(B(3, 12), B(5, 18))
        self.assertEqual((r.center, r.radius), (F(3), F(6)))

    def test_scale_by_exact_integer(self):
        r = fball.scale(B(5, 18), F(12))
        self.assertEqual((r.center, r.radius), (F(60), F(216)))

    def test_scale_by_exact_fraction(self):
        r = fball.scale(B(5, 18), F(1, 3))
        self.assertEqual((r.center, r.radius), (F(5, 3), F(6)))

    def test_fractional_radius_product(self):
        r = fball.mul(B(F(1, 2), 8), B(F(2, 3), 9))
        self.assertEqual((r.center, r.radius), (F(0), F(1, 6)))


class Spec44Policies(unittest.TestCase):
    """Rows of the policy table of SPEC 4.4."""

    def test_scaled_sum(self):
        from adfref.policies import ScaledBall, scaled_add

        x = ScaledBall(F(2), 3, 5)   # 2 * (3 + 5 Zhat)
        y = ScaledBall(F(3), 4, 5)   # 3 * (4 + 5 Zhat)
        r = scaled_add(x, y)
        # g = gcd(2,3) = 1, A = 2, B = 3, residue 2*3 + 3*4 = 18 mod 5 = 3
        self.assertEqual((r.s, r.u, r.K), (F(1), 3, 5))

    def test_scaled_product(self):
        from adfref.policies import ScaledBall, scaled_mul

        x = ScaledBall(F(2), 3, 5)
        y = ScaledBall(F(3), 4, 5)
        r = scaled_mul(x, y)
        # scale s t = 6, residue u v mod K = 12 mod 5 = 2
        self.assertEqual((r.s, r.u, r.K), (F(6), 2, 5))

    def test_fixed_radius_is_not_a_policy(self):
        tight = fball.mul(B(1, 2), B(F(1, 2), 0))
        self.assertEqual((tight.center, tight.radius), (F(1, 2), F(1)))
        bad = Fball.from_center_radius(F(1, 2), F(2))
        self.assertFalse(fball.contains(tight, bad))
        self.assertTrue(fball.contains_rational(tight, F(3, 2)))
        self.assertFalse(fball.contains_rational(bad, F(3, 2)))


if __name__ == "__main__":
    unittest.main()
