"""Rational reconstruction from a full adelic ball and a real interval (SPEC 9.2, first bullet)."""
import os
import random
import sys
import unittest
from fractions import Fraction as F

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from adfref import recon
from adfref.fball import Fball


def B(a, N):
    return Fball.from_center_radius(F(a), F(N))


class FullBallReconstruction(unittest.TestCase):
    def test_several_candidates(self):
        # 1 + 3 Z meets [0, 10] in {1, 4, 7, 10}
        r = recon.reconstruct(B(1, 3), F(0), F(10))
        self.assertEqual(r.status, recon.SEVERAL)
        self.assertEqual(r.solutions, [F(1), F(4), F(7), F(10)])

    def test_one_candidate(self):
        r = recon.reconstruct(B(1, 3), F(1), F(3, 2))
        self.assertEqual(r.status, recon.ONE)
        self.assertEqual(r.solutions, [F(1)])

    def test_no_candidate(self):
        r = recon.reconstruct(B(1, 3), F(2), F(5, 2))
        self.assertEqual(r.status, recon.NONE)
        self.assertEqual(r.solutions, [])

    def test_endpoint_included(self):
        r = recon.reconstruct(B(1, 3), F(4), F(4))
        self.assertEqual(r.status, recon.ONE)
        self.assertEqual(r.solutions, [F(4)])

    def test_exact_ball(self):
        r = recon.reconstruct(B(5, 0), F(4), F(6))
        self.assertEqual(r.status, recon.ONE)
        self.assertEqual(r.solutions, [F(5)])
        r = recon.reconstruct(B(5, 0), F(6), F(7))
        self.assertEqual(r.status, recon.NONE)

    def test_interval_shorter_than_radius_has_at_most_one(self):
        random.seed(51)
        for _ in range(300):
            a = F(random.randint(-20, 20), random.randint(1, 5))
            N = F(random.randint(1, 10), random.randint(1, 4))
            lo = F(random.randint(-20, 20), random.randint(1, 5))
            hi = lo + N * F(random.randint(0, 9), 10)
            if hi - lo >= N:
                continue
            r = recon.reconstruct(B(a, N), lo, hi)
            self.assertLessEqual(len(r.solutions), 1)

    def test_all_returned_candidates_are_in_the_ball(self):
        from adfref.membership import rational_in_fball

        random.seed(52)
        for _ in range(300):
            a = F(random.randint(-20, 20), random.randint(1, 5))
            N = F(random.randint(0, 10), random.randint(1, 4))
            lo = F(random.randint(-30, 30), random.randint(1, 5))
            hi = lo + F(random.randint(0, 30), random.randint(1, 5))
            ball = B(a, N)
            r = recon.reconstruct(ball, lo, hi)
            for x in r.solutions:
                self.assertTrue(rational_in_fball(x, ball))
                self.assertTrue(lo <= x <= hi)


if __name__ == "__main__":
    unittest.main()
