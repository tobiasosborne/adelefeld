"""Precision policies of SPEC 4.4: scaled residue and absolute cap.

The tight result is the oracle. After every operation the tight value must be inside the value of
each other policy. The conversion from a tight ball to the scaled form reports loss.
"""
import os
import random
import sys
import unittest
from fractions import Fraction as F

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from adfref import fball, policies
from adfref.fball import Fball
from adfref.policies import ScaledBall, Exact
from adfref.rat import qgcd


def B(a, N):
    return Fball.from_center_radius(F(a), F(N))


class Conversion(unittest.TestCase):
    def test_loss_reported(self):
        tight = B(3, 12)
        value, lost = policies.convert_from_tight(tight, 6)
        self.assertTrue(lost)
        self.assertEqual((value.s, value.u, value.K), (F(1), 3, 6))
        self.assertEqual(value.radius, F(6))
        self.assertTrue(fball.contains(tight, value.to_fball()))

    def test_no_loss_reported(self):
        tight = B(3, 6)
        value, lost = policies.convert_from_tight(tight, 6)
        self.assertFalse(lost)
        self.assertEqual((value.s, value.u, value.K), (F(1), 3, 6))
        self.assertTrue(fball.contains(tight, value.to_fball()))

    def test_conversion_never_gains(self):
        random.seed(41)
        for _ in range(500):
            a = F(random.randint(-9, 9), random.randint(1, 5))
            N = F(random.randint(0, 9), random.randint(1, 5))
            K = random.randint(1, 12)
            tight = B(a, N)
            value, lost = policies.convert_from_tight(tight, K)
            self.assertTrue(fball.contains(tight, value.to_fball()))
            newr = value.radius
            if N == 0:
                self.assertFalse(lost)
            else:
                self.assertEqual((N / newr).denominator, 1)  # new radius divides N
                self.assertEqual(lost, newr != N)

    def test_exact_stays_exact(self):
        tight = B(5, 0)
        value, lost = policies.convert_from_tight(tight, 6)
        self.assertIsInstance(value, Exact)
        self.assertFalse(lost)
        self.assertEqual(value.q, F(5))


class PolicyComparison(unittest.TestCase):
    def setUp(self):
        self.x = B(3, 12)
        self.y = B(5, 18)
        self.z = B(F(1, 2), 8)
        self.w = B(F(2, 3), 9)
        self.C = F(5)

    def eval_tight(self):
        out = []
        v = fball.add(self.x, self.y); out.append(v)
        v = fball.scale(v, F(7, 5)); out.append(v)
        v = fball.mul(v, self.z); out.append(v)
        v = fball.sub(v, self.w); out.append(v)
        v = fball.neg(v); out.append(v)
        return out

    def eval_cap(self):
        out = []
        v = fball.add(self.x, self.y)
        v = policies.absolute_cap(v, self.C); out.append(v)
        v = fball.scale(v, F(7, 5))
        v = policies.absolute_cap(v, self.C); out.append(v)
        v = fball.mul(v, self.z)
        v = policies.absolute_cap(v, self.C); out.append(v)
        v = fball.sub(v, self.w)
        v = policies.absolute_cap(v, self.C); out.append(v)
        v = fball.neg(v)
        v = policies.absolute_cap(v, self.C); out.append(v)
        return out

    def eval_scaled(self, K):
        out = []
        xs, _ = policies.convert_from_tight(self.x, K)
        ys, _ = policies.convert_from_tight(self.y, K)
        zs, _ = policies.convert_from_tight(self.z, K)
        ws, _ = policies.convert_from_tight(self.w, K)
        v = policies.scaled_add(xs, ys); out.append(v)
        v = policies.scaled_scale(v, F(7, 5)); out.append(v)
        v = policies.scaled_mul(v, zs); out.append(v)
        v = policies.scaled_sub(v, ws); out.append(v)
        v = policies.scaled_neg(v); out.append(v)
        return out

    def test_tight_inside_cap_after_every_operation(self):
        for t, c in zip(self.eval_tight(), self.eval_cap()):
            self.assertTrue(fball.contains(t, c), (t, c))

    def test_tight_inside_scaled_after_every_operation(self):
        for K in (1, 2, 3, 6, 12):
            for t, s in zip(self.eval_tight(), self.eval_scaled(K)):
                self.assertTrue(fball.contains(t, s.to_fball()), (K, t, s))


class IntendedLoss(unittest.TestCase):
    def test_cap_loss(self):
        # gcd(20, 3) = 1, so a radius of 20 becomes 1: the result is coarser
        capped = policies.absolute_cap(B(0, 20), F(3))
        self.assertEqual(capped.radius, F(1))

    def test_cap_keeps_a_finer_radius(self):
        # gcd(2, 6) = 2, equal to the radius
        capped = policies.absolute_cap(B(0, 2), F(6))
        self.assertEqual(capped.radius, F(2))

    def test_cap_keeps_an_exact_value_exact(self):
        # SPEC 4.4 literally says gcd(R, C); gcd(0, C) = C would coarsen a point. The reference
        # keeps an exact value exact. This is a convention the C code must share.
        capped = policies.absolute_cap(B(5, 0), F(3))
        self.assertTrue(capped.is_exact)
        self.assertEqual(capped.center, F(5))

    def test_fixed_radius_policy_fails(self):
        tight = fball.mul(B(1, 2), B(F(1, 2), 0))
        fixed = B(F(1, 2), 2)
        self.assertFalse(fball.contains(tight, fixed))
        self.assertTrue(fball.contains_rational(tight, F(3, 2)))
        self.assertFalse(fball.contains_rational(fixed, F(3, 2)))


if __name__ == "__main__":
    unittest.main()
