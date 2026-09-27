"""Canonical form of SPEC 4.1: one triple per set, invariants, edge cases.

For H > 0: 0 <= A < H and gcd(A, H, d) = 1. For H = 0: A/d in lowest terms.
"""
import os
import random
import sys
import unittest
from fractions import Fraction as F
from math import gcd

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from adfref import fball
from adfref.fball import Fball


class CanonicalInvariants(unittest.TestCase):
    def test_invariants_random(self):
        random.seed(31)
        for _ in range(2000):
            A = random.randint(-50, 50)
            H = random.randint(0, 50)
            d = random.randint(1, 30)
            b = Fball(A, H, d)
            self.assertGreaterEqual(b.H, 0)
            self.assertGreater(b.d, 0)
            self.assertEqual(gcd(gcd(abs(b.A), b.H), b.d), 1)
            if b.H > 0:
                self.assertTrue(0 <= b.A < b.H)
            else:
                self.assertEqual(gcd(abs(b.A), b.d), 1)
            # the canonical triple encodes the same set as the raw input
            self.assertEqual(b.radius, F(H, d))
            if b.radius == 0:
                self.assertEqual(b.center, F(A, d))
            else:
                self.assertEqual(((F(A, d) - b.center) / b.radius).denominator, 1)

    def test_zero_centre(self):
        self.assertEqual(Fball(0, 0, 5).as_tuple(), (0, 0, 1))
        self.assertEqual(Fball(0, 8, 4).as_tuple(), (0, 2, 1))

    def test_negative_centre(self):
        b = Fball(-3, 12, 1)
        self.assertEqual(b.as_tuple(), (9, 12, 1))
        self.assertEqual(b.center, F(9))

    def test_fractional_radius(self):
        b = Fball.from_center_radius(F(1, 2), F(3, 2))
        A, H, d = b.as_tuple()
        self.assertEqual(F(A, d), F(1, 2))
        self.assertEqual(F(H, d), F(3, 2))
        self.assertEqual(gcd(gcd(abs(A), H), d), 1)

    def test_shared_factors_centre_radius(self):
        # gcd(A, H, d) is removed, not only gcd(A, H)
        raw = Fball(6, 12, 2)
        self.assertEqual(raw.as_tuple(), (3, 6, 1))
        self.assertEqual(raw.center, F(3))
        self.assertEqual(raw.radius, F(6))


class CanonicalUniqueness(unittest.TestCase):
    def test_same_set_from_different_inputs(self):
        random.seed(32)
        for _ in range(1000):
            A = random.randint(-30, 30)
            H = random.randint(1, 30)
            d = random.randint(1, 12)
            base = Fball(A, H, d)
            k = random.randint(1, 7)
            shifted = Fball(A + k * H, H, d)
            scaled = Fball(A * k, H * k, d * k)
            self.assertEqual(base.as_tuple(), shifted.as_tuple())
            self.assertEqual(base.as_tuple(), scaled.as_tuple())

    def test_exact_same_set_from_different_inputs(self):
        b = Fball.from_center_radius(F(6, 8), F(0))
        self.assertEqual(b.as_tuple(), (3, 0, 4))
        self.assertEqual(Fball(12, 0, 16).as_tuple(), (3, 0, 4))
        self.assertEqual(Fball(-3, 0, -4).as_tuple(), (3, 0, 4))

    def test_no_two_canonical_triples_for_one_set(self):
        # brute force: equal_set on distinct canonical triples must be false
        triples = set()
        for A in range(-6, 7):
            for H in range(0, 7):
                for d in range(1, 5):
                    triples.add(Fball(A, H, d).as_tuple())
        seen = []
        for t in triples:
            b = Fball(*t)
            for c in seen:
                other = Fball(*c)
                if b.as_tuple() != other.as_tuple():
                    self.assertFalse(fball.equal_set(b, other), (b, other))
            seen.append(t)


if __name__ == "__main__":
    unittest.main()
