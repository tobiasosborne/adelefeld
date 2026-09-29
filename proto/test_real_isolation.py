#!/usr/bin/env python3
"""Tests of proto/real_isolation.py, the reference of docs/design/real-roots.md (issue adf-8di).

Run from the repository root:  timeout 170 python3 -m unittest proto/test_real_isolation.py -v
A table of times:              timeout 170 python3 proto/test_real_isolation.py --times

The oracle is not the module under test. It is `proto/solvers_checks.py`: the squarefree part over Q, the Sturm
chain of that file, exact signs with `fractions.Fraction` (`real_verify_complete`, the checker of solvers.md
P3.13(5)), and the accuracy by `acc_independent`. For planted roots the oracle is the list of the planted
rationals. The module under test uses Descartes' rule and no Sturm chain.
"""
import os
import random
import sys
import time
import unittest
from fractions import Fraction as F

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import solvers_checks as sc  # noqa: E402
import real_isolation as ri  # noqa: E402

PRECS = (2, 20, 53, 200)


def pair(e, c=1234567):
    """c (X - 2^e)(X - 2^e - 1), the family of docs/reviews/s2/review-real.md finding 1."""
    return sc.pmul([c], sc.pmul([-2 ** e, 1], [-2 ** e - 1, 1])), [F(2 ** e), F(2 ** e + 1)]


def thirds(e):
    """(3X - 3 2^e - 1)(3X - 3 2^e - 2): the roots 2^e + 1/3 and 2^e + 2/3, not dyadic."""
    return sc.pmul([-3 * 2 ** e - 1, 3], [-3 * 2 ** e - 2, 3]), [2 ** e + F(1, 3), 2 ** e + F(2, 3)]


def close(e):
    """(3X - 1)(3 2^e X - 2^e - 1): the roots 1/3 and 1/3 + 2^-e / 3."""
    return sc.pmul([-1, 3], [-2 ** e - 1, 3 * 2 ** e]), [F(1, 3), F(1, 3) + F(1, 3 * 2 ** e)]


def cluster(e, d):
    """prod_(i = 1..d) (3X - 3 2^e - i): d roots 2^e + i/3."""
    f, roots = [1], []
    for i in range(1, d + 1):
        f = sc.pmul(f, [-3 * 2 ** e - i, 3])
        roots.append(2 ** e + F(i, 3))
    return f, roots


def odd_bits(r):
    """The number of bits of the odd part of the numerator of a dyadic rational (0 for 0)."""
    m = abs(r.numerator)
    return (m // (m & -m)).bit_length() if m else 0


def planted_ok(balls, roots):
    """Each planted root in exactly one closed ball, each ball exactly one planted root."""
    return (all(sum(1 for lo, hi in balls if lo <= r <= hi) == 1 for r in roots) and
            all(sum(1 for r in roots if lo <= r <= hi) == 1 for lo, hi in balls))


def contract_ok(f, prec, res, expect=None):
    """The contract of adf_roots_real for the answer OK, by the oracle: the list passes the complete verifier
    (exact signs of P3.8, hi_i < lo_(i+1), number = the Sturm count recomputed from f) and the accuracy."""
    st, n, balls = res[0], res[1], res[2]
    if st != ri.OK or n != len(balls):
        return False
    if expect is not None and n != expect:
        return False
    return sc.real_verify_complete(f, balls, n) and sc.acc_independent(balls, prec)


class Pieces(unittest.TestCase):
    def test_var(self):
        self.assertEqual(ri.var([-1, 0, 0, 2, 0, -1]), 2)      # the example of sagraloff-mehlhorn, line 547
        self.assertEqual(ri.var([1, 2, 3]), 0)
        self.assertEqual(ri.var([0, 0, 5]), 0)
        self.assertEqual(ri.var([1, -1, 1, -1]), 3)

    def test_shift(self):
        for _ in range(200):
            q = [random.randint(-50, 50) for _ in range(random.randint(1, 8))]
            s = ri.shift1(q)
            for x in (-3, 0, 1, 7):
                self.assertEqual(sc.peval(s, x), sc.peval(q, x + 1))

    def test_var01_is_a_bound_with_parity(self):
        """Descartes' rule on (0, 1) against the Sturm count of the oracle: v >= m and v = m modulo 2, for
        squarefree polynomials with nonzero values at 0 and 1."""
        n = 0
        for _ in range(300):
            roots = [F(random.randint(-8, 24), 16) for _ in range(random.randint(1, 5))]
            f = [1]
            for r in set(roots):
                f = sc.pmul(f, [-r.numerator, r.denominator])
            if random.random() < 0.5:
                f = sc.pmul(f, [random.randint(1, 3), random.randint(-2, 2), random.randint(1, 3)])
            g = sc.squarefree_part(f)
            if sc.peval(g, 0) == 0 or sc.peval(g, 1) == 0:
                continue
            m = sc.roots_open(g, F(0), F(1))
            v = ri.var01(g)
            self.assertGreaterEqual(v, m)
            self.assertEqual((v - m) % 2, 0)
            n += 1
        self.assertGreater(n, 100)

    def test_root_bound(self):
        for name, f, _ in sc.REAL_CASES:
            g = sc.squarefree_part(f)
            if len(g) == 1:
                continue
            K = ri.root_bound_exp(g)
            self.assertEqual(sc.sturm_count(g), sc.roots_open(g, -F(2) ** K, F(2) ** K), name)
        g, roots = pair(1000)
        self.assertLessEqual(ri.root_bound_exp(g), 1003)
        # a bound that is tight matters for the cost: roots near 2^-3000 must not start at (0, 1)
        g = sc.pmul([-1, 3 * 2 ** 3000], [-2, 3 * 2 ** 3000])
        self.assertLessEqual(ri.root_bound_exp(g), -2997)

    def test_squarefree_part(self):
        for name, f, _ in sc.REAL_CASES:
            self.assertEqual(ri.squarefree_part(f), sc.squarefree_part(f), name)


class RealCases(unittest.TestCase):
    def test_real_cases(self):
        n = 0
        for name, f, expect in sc.REAL_CASES:
            for prec in PRECS:
                for refine in ("bisect", "qir"):
                    res = ri.real_roots(f, prec, refine=refine)
                    self.assertTrue(contract_ok(f, prec, res, expect), (name, prec, refine, res[:3]))
                    n += 1
        self.assertEqual(n, 18 * 4 * 2)

    def test_statuses(self):
        self.assertEqual(ri.real_roots([], 10)[0], ri.DOMAIN)
        self.assertEqual(ri.real_roots([0, 0], 10)[0], ri.DOMAIN)
        self.assertEqual(ri.real_roots([7], 10)[:3], (ri.OK, 0, []))
        # a precision below 2 is 2 (M1-D4)
        for p in (1, 0, -5):
            self.assertEqual(ri.real_roots([-2, 0, 1], p)[2], ri.real_roots([-2, 0, 1], 2)[2])

    def test_dyadic_roots_are_exact(self):
        roots = [F(0), F(1, 2), F(-3, 4), F(5, 8), F(1, 2 ** 30), F(1, 3), F(-7, 5), F(12345)]
        f = [1]
        for r in roots:
            f = sc.pmul(f, [-r.numerator, r.denominator])
        f = sc.pmul(f, f)                                      # every root double
        for prec in (2, 53):
            res = ri.real_roots(f, prec)
            self.assertTrue(contract_ok(f, prec, res, len(roots)))
            self.assertTrue(planted_ok(res[2], roots))
            # Proposition R4(3): a root m 2^t with m odd of at most max(prec, 2) + 1 bits is an exact ball
            # (12345 has 14 bits: exact at 53, and at 2 only if a point of the grid met it); 0 is exact
            exact = set(lo for lo, hi in res[2] if lo == hi)
            dyadic = set(r for r in roots if r.denominator & (r.denominator - 1) == 0)
            short = set(r for r in dyadic if odd_bits(r) <= max(prec, 2) + 1)
            self.assertTrue(short <= exact <= dyadic, (prec, exact))
            self.assertEqual(F(12345) in exact, prec == 53)

    def test_touching_cells(self):
        """1/3 and 2/3 are isolated by the cells (0, 1/2) and (1/2, 1), which touch at a point that is not a
        root; 1/4 and 3/4 with 1/2: cells that end at a root. The closed balls must be strictly apart."""
        for roots in ([F(1, 3), F(2, 3)], [F(1, 3), F(1, 2), F(2, 3)], [F(1, 2), F(5, 8), F(3, 4)],
                      [F(1, 2), F(9, 16), F(19, 32), F(5, 8)], [F(-1, 3), F(0), F(1, 3)]):
            f = [1]
            for r in roots:
                f = sc.pmul(f, [-r.numerator, r.denominator])
            for prec in (2, 20):
                for refine in ("bisect", "qir"):
                    res = ri.real_roots(f, prec, refine=refine)
                    self.assertTrue(contract_ok(f, prec, res, len(roots)), (roots, prec, res[:3]))
                    self.assertTrue(planted_ok(res[2], roots))

    def test_nested_when_prec_grows(self):
        """Design R4(4) (lane r-slice1): the balls for a larger prec lie in those for a smaller one; the floor of
        an item is static. Includes 191/3, 193/3 (accuracies that differ by one bit across 64)."""
        n = 0
        for roots in ([F(1, 3), F(2, 3)], [F(1, 3), F(1, 2), F(2, 3)], [F(191, 3), F(193, 3)],
                      [F(1021, 3), F(1025, 3), F(1027, 3)], [F(-1, 3), F(0), F(1, 3)]):
            f = [1]
            for r in roots:
                f = sc.pmul(f, [-r.numerator, r.denominator])
            for refine in ("bisect", "qir"):
                prev = None
                for prec in (2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 16, 20, 30, 53, 100):
                    res = ri.real_roots(f, prec, refine=refine)
                    self.assertTrue(contract_ok(f, prec, res, len(roots)))
                    if prev is not None:
                        for (lo0, hi0), (lo1, hi1) in zip(prev, res[2]):
                            self.assertTrue(lo0 <= lo1 and hi1 <= hi0, (roots, prec, refine))
                            n += 1
                    prev = res[2]
        self.assertGreater(n, 200)

    def test_planted_random(self):
        rnd = random.Random(20260929)
        pool = [F(-3), F(-1, 2), F(0), F(1, 3), F(1), F(1) + F(1, 2 ** 40), F(10) ** 30, -F(1, 10 ** 20),
                F(7, 5), F(-7, 5), F(1, 2 ** 30), F(2) ** 200, F(2) ** 200 + F(1, 3), F(1, 3) + F(1, 2 ** 90)]
        n = 0
        for _ in range(60):
            roots = rnd.sample(pool, rnd.randint(1, 5))
            f = [rnd.choice((1, 2, -3, 5))]
            for r in roots:
                for _ in range(rnd.choice((1, 1, 2, 3))):
                    f = sc.pmul(f, [-r.numerator, r.denominator])
            if rnd.random() < 0.4:
                f = sc.pmul(f, rnd.choice(([1, 0, 1], [1, 1, 1], [2, -1, 3])))
            for prec in (2, 20, 64):
                res = ri.real_roots(f, prec, refine=rnd.choice(("bisect", "qir")))
                self.assertTrue(contract_ok(f, prec, res, len(roots)), (roots, prec, res[:3]))
                self.assertTrue(planted_ok(res[2], roots), (roots, prec))
                n += 1
        self.assertEqual(n, 180)

    def test_random_integer_polynomials(self):
        rnd = random.Random(8)
        n = roots = 0
        for _ in range(150):
            f = [rnd.randint(-20, 20) for _ in range(rnd.randint(2, 9))]
            if not sc.ptrim(f):
                continue
            prec = rnd.choice((2, 30))
            res = ri.real_roots(f, prec)
            self.assertTrue(contract_ok(f, prec, res), (f, prec, res[:3]))
            n += 1
            roots += res[1]
        self.assertGreater(n, 140)
        self.assertGreater(roots, 100)


# (family, parameter): the largest time in seconds that the test accepts. The times measured are in
# docs/design/real-roots.md section 6; the limits are 20 times larger or more, because the machine is shared.
SLOW = [(pair, e) for e in (600, 1200, 1500, 1800, 3000)] + [(thirds, e) for e in (600, 1200, 1800, 3000)] + \
       [(close, e) for e in (600, 1200, 3000)]


class SlowFamily(unittest.TestCase):
    def test_slow_family(self):
        for fam, e in SLOW:
            f, roots = fam(e)
            for prec in (2, 53):
                t = time.perf_counter()
                res = ri.real_roots(f, prec)
                t = time.perf_counter() - t
                self.assertTrue(contract_ok(f, prec, res, 2), (fam.__name__, e, prec))
                self.assertTrue(planted_ok(res[2], roots), (fam.__name__, e, prec))
                self.assertLess(t, 10.0, (fam.__name__, e, prec))

    def test_cost_is_counted(self):
        """The bounds of the design (section 5): for the pair with non-dyadic roots the tree has at most
        2 (e + 4) + 3 nodes of each sign, and the bits of every end point are at most e + prec + 8."""
        for e in (100, 600, 3000):
            f, roots = thirds(e)
            res = ri.real_roots(f, 2)
            stats = res[3]
            self.assertLessEqual(stats["nodes"], 4 * (e + 4) + 6)
            self.assertGreaterEqual(stats["nodes"], e)          # the chain is there: no Newton step yet
            for lo, hi in res[2]:
                for x in (lo, hi):
                    self.assertLessEqual(x.numerator.bit_length() + x.denominator.bit_length(), e + 2 + 16)

    def test_cluster(self):
        for d, e in ((4, 300), (8, 300), (16, 300)):
            f, roots = cluster(e, d)
            res = ri.real_roots(f, 2)
            self.assertTrue(contract_ok(f, 2, res, d))
            self.assertTrue(planted_ok(res[2], roots))

    def test_qir_needs_fewer_evaluations(self):
        f = [-2, 0, 1]
        b = ri.real_roots(f, 2000, refine="bisect")
        q = ri.real_roots(f, 2000, refine="qir")
        self.assertTrue(contract_ok(f, 2000, b, 2))
        self.assertTrue(contract_ok(f, 2000, q, 2))
        self.assertGreater(b[3]["evaluations"], 3900)
        self.assertLess(q[3]["evaluations"], 200)


def times():
    print("family e prec refine seconds nodes evaluations largest_end_point_bits")
    rows = [(fam, e) for fam, e in SLOW] + [(lambda e, d=d: cluster(e, d), e) for d, e in ((8, 300), (16, 300))]
    for fam, e in rows:
        f, roots = fam(e)
        for prec, refine in ((2, "qir"), (53, "qir"), (2000, "qir"), (2000, "bisect")):
            t = time.perf_counter()
            res = ri.real_roots(f, prec, refine=refine)
            t = time.perf_counter() - t
            ok = contract_ok(f, prec, res, len(roots)) and planted_ok(res[2], roots)
            bits = max([max(x.numerator.bit_length(), x.denominator.bit_length()) for b in res[2] for x in b])
            name = "cluster" if fam.__name__ == "<lambda>" else fam.__name__
            print(f"{name} {e} {prec} {refine} {t:.4f} {res[3]['nodes']} {res[3]['evaluations']} {bits} "
                  f"{'ok' if ok else 'FAILED'}", flush=True)


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--times":
        times()
    else:
        unittest.main()
