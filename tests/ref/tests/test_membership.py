"""Membership of a rational in a finite ball, from the definition (SPEC 4.3 / precision.md Lemma 1).

The definition does not use the ball arithmetic: x lies in (A + H Zhat)/d with H > 0 exactly when
(x d - A)/H is an integer, i.e. has no prime in its denominator. H = 0 is the single point A/d.
"""
import os
import random
import sys
import unittest
from fractions import Fraction as F

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from adfref import fball
from adfref.fball import Fball
from adfref.membership import rational_in_fball


def B(a, N):
    return Fball.from_center_radius(F(a), F(N))


class MembershipDefinition(unittest.TestCase):
    def test_spec_example_residue_is_not_membership(self):
        # 1/5 is 5 mod 6 as a residue, but 1/5 is not in the adelic ball 5 + 6 Zhat
        self.assertFalse(rational_in_fball(F(1, 5), B(5, 6)))
        self.assertTrue(rational_in_fball(F(5), B(5, 6)))

    def test_progression(self):
        self.assertTrue(rational_in_fball(F(7), B(1, 3)))
        self.assertFalse(rational_in_fball(F(8), B(1, 3)))
        self.assertTrue(rational_in_fball(F(1, 3), B(F(1, 3), 0)))
        self.assertFalse(rational_in_fball(F(1, 2), B(F(1, 3), 0)))

    def test_fractional_radius(self):
        # 0 mod 1/2 contains 0 and 1/2 but not 1/4
        ball = B(0, F(1, 2))
        self.assertTrue(rational_in_fball(F(0), ball))
        self.assertTrue(rational_in_fball(F(1, 2), ball))
        self.assertFalse(rational_in_fball(F(1, 4), ball))

    def test_negatives(self):
        ball = B(F(-1, 2), F(3, 2))
        for k in range(-6, 7):
            self.assertTrue(rational_in_fball(F(-1, 2) + F(3, 2) * k, ball))

    def test_matches_ball_arithmetic(self):
        random.seed(61)
        for _ in range(2000):
            a = F(random.randint(-20, 20), random.randint(1, 6))
            N = F(random.randint(0, 12), random.randint(1, 5))
            x = F(random.randint(-40, 40), random.randint(1, 8))
            ball = B(a, N)
            self.assertEqual(rational_in_fball(x, ball),
                             fball.contains_rational(ball, x), (a, N, x))

    def test_definition_does_not_use_gcd(self):
        # (A + H Zhat)/d with H > 0 and gcd(A, H, d) = 1: numerator of (x d - A)/H is the test
        b = B(3, 12)
        self.assertEqual(rational_in_fball(F(15), b), True)
        self.assertEqual(rational_in_fball(F(16), b), False)


if __name__ == "__main__":
    unittest.main()
