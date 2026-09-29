#!/usr/bin/env python3
"""Unit tests of tests/ref/adfref/modctx_ref.py (lane m1-modctx).

Run from the repository root:

    python3 lanes/m1-modctx/test_modctx_ref.py

The cases are computed by hand from docs/conventions.md 5.14 and docs/proofs/policies.md
Definition 16 and Lemma 17, not from the C code.
"""
import os
import sys
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tests", "ref"))

from adfref import modctx_ref as m  # noqa: E402


class TestPrimality(unittest.TestCase):
    def test_small(self):
        self.assertFalse(m.is_prime(0))
        self.assertFalse(m.is_prime(1))
        self.assertTrue(m.is_prime(2))
        self.assertTrue(m.is_prime(3))
        self.assertFalse(m.is_prime(4))
        self.assertTrue(m.is_prime(2 ** 61 - 1))          # Mersenne prime
        self.assertFalse(m.is_prime(2 ** 61 - 1 - 2))     # even
        self.assertFalse(m.is_prime(3215031751))          # = 151 * 751 * 28351

    def test_primes_upto(self):
        self.assertEqual(m.primes_upto(30), [2, 3, 5, 7, 11, 13, 17, 19, 23, 29])
        self.assertEqual(m.primes_upto(1), [])
        self.assertEqual(m.primes_upto(2), [2])


class TestBlocks(unittest.TestCase):
    def test_empty(self):
        self.assertEqual(m.new_blocks([]), ("OK", 1, []))
        self.assertEqual(m.new_prime_powers([], []), ("OK", 1, []))

    def test_valid(self):
        status, K, blocks = m.new_blocks([2, 3, 5])
        self.assertEqual((status, K, blocks), ("OK", 30, [2, 3, 5]))

    def test_zero_block(self):
        self.assertEqual(m.new_blocks([0]), ("DOMAIN", None, None))

    def test_one_block(self):
        self.assertEqual(m.new_blocks([2, 1]), ("DOMAIN", None, None))

    def test_not_coprime(self):
        self.assertEqual(m.new_blocks([4, 6]), ("DOMAIN", None, None))
        self.assertEqual(m.new_blocks([2, 9, 15]), ("DOMAIN", None, None))

    def test_order_kept(self):
        status, K, blocks = m.new_blocks([5, 2])
        self.assertEqual((status, K, blocks), ("OK", 10, [5, 2]))

    def test_prime_powers(self):
        status, K, blocks = m.new_prime_powers([2, 3], [3, 2])
        self.assertEqual((status, K, blocks), ("OK", 72, [8, 9]))

    def test_exponent_zero(self):
        self.assertEqual(m.new_prime_powers([2], [0]), ("DOMAIN", None, None))

    def test_composite_prime(self):
        self.assertEqual(m.new_prime_powers([4], [2]), ("DOMAIN", None, None))

    def test_repeated_prime(self):
        self.assertEqual(m.new_prime_powers([2, 2], [1, 2]), ("DOMAIN", None, None))

    def test_power_overflow(self):
        self.assertEqual(m.new_prime_powers([2], [64]), ("UNSUPPORTED", None, None))
        # 3^40 = 12157665459056928801 < 2^64, 3^41 >= 2^64
        self.assertEqual(m.new_prime_powers([3], [40]),
                         ("OK", 12157665459056928801, [12157665459056928801]))
        self.assertEqual(m.new_prime_powers([3], [41]), ("UNSUPPORTED", None, None))

    def test_fmpz(self):
        self.assertEqual(m.new_fmpz(1), ("OK", 1, []))
        self.assertEqual(m.new_fmpz(7), ("OK", 7, [7]))
        self.assertEqual(m.new_fmpz(0), ("DOMAIN", None, None))
        self.assertEqual(m.new_fmpz(-5), ("DOMAIN", None, None))
        self.assertEqual(m.new_fmpz(1 << 64), ("OK", 1 << 64, []))
        self.assertEqual(m.new_fmpz((1 << 64) - 1), ("OK", (1 << 64) - 1, [(1 << 64) - 1]))

    def test_factorial(self):
        self.assertEqual(m.new_factorial(0), ("OK", 1, []))
        self.assertEqual(m.new_factorial(1), ("OK", 1, []))
        status, K, blocks = m.new_factorial(5)
        self.assertEqual((status, K, blocks), ("OK", 120, [8, 3, 5]))
        # 5! = 120 = 2^3 * 3 * 5; the blocks are the prime powers of n! in increasing p
        status, K, blocks = m.new_factorial(10)
        self.assertEqual((status, K, blocks), ("OK", 3628800, [2 ** 8, 3 ** 4, 25, 7]))
        # v_2(65!) = 63 keeps 2^63 as a word, v_2(66!) = 64 does not
        self.assertEqual(m.new_factorial(65)[0], "OK")
        self.assertEqual(m.new_factorial(66), ("UNSUPPORTED", None, None))
        self.assertEqual(m.new_factorial(2 ** 40), ("UNSUPPORTED", None, None))

    def test_factorial_blocks_are_prime_powers(self):
        status, K, blocks = m.new_factorial(20)
        self.assertEqual(status, "OK")
        self.assertEqual(K, 2432902008176640000)
        self.assertEqual([b for b in blocks], [2 ** 18, 3 ** 8, 5 ** 4, 7 ** 2, 11, 13, 17, 19])
        prod = 1
        for b in blocks:
            prod *= b
        self.assertEqual(prod, K)

    def test_primorial(self):
        self.assertEqual(m.new_primorial_pow(0, 3), ("OK", 1, []))
        self.assertEqual(m.new_primorial_pow(1, 3), ("OK", 1, []))
        self.assertEqual(m.new_primorial_pow(10 ** 9, 0), ("OK", 1, []))
        status, K, blocks = m.new_primorial_pow(10, 2)
        self.assertEqual((status, K, blocks), ("OK", (2 * 3 * 5 * 7) ** 2, [4, 9, 25, 49]))
        status, K, blocks = m.new_primorial_pow(13, 1)
        self.assertEqual((status, K, blocks), ("OK", 30030, [2, 3, 5, 7, 11, 13]))
        # e = 63 keeps 2^63 and overflows at 3
        self.assertEqual(m.new_primorial_pow(2, 63), ("OK", 2 ** 63, [2 ** 63]))
        self.assertEqual(m.new_primorial_pow(3, 63), ("UNSUPPORTED", None, None))
        self.assertEqual(m.new_primorial_pow(2, 64), ("UNSUPPORTED", None, None))
        self.assertEqual(m.new_primorial_pow(2 ** 32, 1), ("UNSUPPORTED", None, None))


class TestConversion(unittest.TestCase):
    def test_reduce_positive(self):
        self.assertEqual(m.reduce(23, [6, 35, 11]), [5, 23, 1])

    def test_reduce_negative(self):
        # -1 is q_i - 1 modulo each block (conventions 5.3: 0 <= res[i] < q_i)
        self.assertEqual(m.reduce(-1, [6, 35, 11]), [5, 34, 10])

    def test_reduce_no_blocks(self):
        self.assertEqual(m.reduce(23, []), [])
        self.assertEqual(m.combine([], []), 0)

    def test_roundtrip(self):
        blocks = [8, 9, 5, 7]
        K = 8 * 9 * 5 * 7
        for a in [0, 1, 2519, 2520, 2521, -1, -2520, 10 ** 100]:
            A = m.combine(m.reduce(a, blocks), blocks)
            self.assertEqual(A, a % K)
            self.assertTrue(0 <= A < K)

    def test_combine_is_crt(self):
        # x = 2 mod 8, 3 mod 9, 4 mod 5, 6 mod 7: solved by hand (1434 = 2 + 8*179)
        self.assertEqual(m.combine([2, 3, 4, 6], [8, 9, 5, 7]), 1434)

    def test_reduces_match_definition(self):
        blocks = [6, 35, 11]
        a = 12345678901234567890123456789
        self.assertEqual(m.reduce(a, blocks), [a % 6, a % 35, a % 11])
        self.assertEqual(m.combine(m.reduce(a, blocks), blocks), a % (6 * 35 * 11))


if __name__ == "__main__":
    unittest.main()
