#!/usr/bin/env python3
"""Tests of proto/text_grammar.py, the reference parser and printer of docs/conventions.md sections 8 to 10.

Run from the repository root:  python3 -m unittest proto/test_text_grammar.py -v
The golden vectors in tests/golden/ are read with the file format of docs/conventions.md 11.1.
"""
import os
import random
import sys
import unittest
from fractions import Fraction

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tests", "ref"))

import text_grammar as tg  # noqa: E402

GOLDEN = os.path.join(ROOT, "tests", "golden")
TYPES = ["rat", "fball", "adele", "cadele", "ucoset", "idele", "idclass", "lball", "sball", "qclass", "ffun",
         "rfun", "char"]
STATUS_NAMES = {"OK", "NOT_DETERMINED", "UNIT_NOT_CERTIFIED", "NEEDS_SPLIT", "NOT_UNIQUE", "NO_SOLUTION",
                "NOT_UNIT", "DOMAIN", "UNSUPPORTED", "PARSE", "LIMIT"}


# ---------------------------------------------------------------------------------------------------------------
# golden file reader (docs/conventions.md 11.1); deliberately independent of text_grammar.py

def unescape(s):
    out = bytearray()
    i = 0
    while i < len(s):
        c = s[i]
        if c != "\\":
            out += c.encode("ascii")
            i += 1
            continue
        if i + 1 >= len(s):
            raise ValueError("dangling backslash in " + repr(s))
        e = s[i + 1]
        if e == "\\":
            out.append(0x5C)
            i += 2
        elif e == "t":
            out.append(0x09)
            i += 2
        elif e == "n":
            out.append(0x0A)
            i += 2
        elif e == "r":
            out.append(0x0D)
            i += 2
        elif e == "x":
            h = s[i + 2:i + 4]
            if len(h) != 2 or any(ch not in "0123456789abcdefABCDEF" for ch in h):
                raise ValueError("bad \\x escape in " + repr(s))
            out.append(int(h, 16))
            i += 4
        else:
            raise ValueError("unknown escape \\" + e + " in " + repr(s))
    return bytes(out)


def decode_input(field):
    if field.startswith("@gen:"):
        parts = field[5:].split("|")
        if len(parts) != 4:
            raise ValueError("bad @gen: " + field)
        prefix, unit, count, suffix = parts
        return unescape(prefix) + unescape(unit) * int(count) + unescape(suffix)
    return unescape(field)


def read_golden(name):
    path = os.path.join(GOLDEN, name + ".tsv")
    vectors = []
    with open(path, "rb") as f:
        raw = f.read()
    raw.decode("ascii")  # the file itself must be ASCII
    for lineno, line in enumerate(raw.decode("ascii").split("\n"), 1):
        if line == "" or line.startswith("#"):
            continue
        if line.count("\t") != 1:
            raise ValueError("%s:%d: not exactly one TAB" % (path, lineno))
        inp, exp = line.split("\t")
        vectors.append((lineno, inp, decode_input(inp), exp))
    return vectors


def is_status(exp):
    return exp.startswith("!")


# ---------------------------------------------------------------------------------------------------------------

class TestGoldenFiles(unittest.TestCase):
    """The golden files themselves: format, statuses, coverage of the three classes, and the required examples."""

    def test_files_exist_and_parse(self):
        for name in TYPES + ["dispatch", "realball_read", "realball_print", "dump", "psi_phases", "gauss"]:
            vs = read_golden(name)
            self.assertGreater(len(vs), 5, name)
            for lineno, inp, data, exp in vs:
                if is_status(exp):
                    self.assertIn(exp[1:], STATUS_NAMES, "%s:%d" % (name, lineno))

    def test_three_classes_per_type(self):
        for name in TYPES:
            vs = read_golden(name)
            same = [v for v in vs if not is_status(v[3]) and v[2] == v[3].encode()]
            changed = [v for v in vs if not is_status(v[3]) and v[2] != v[3].encode()]
            bad = [v for v in vs if is_status(v[3])]
            self.assertTrue(same and changed and bad, name)

    def test_hostile_inputs_present(self):
        allv = []
        for name in TYPES:
            allv += [v[2] for v in read_golden(name)]
        self.assertTrue(any(b"\x00" in d for d in allv), "embedded NUL")
        self.assertTrue(any(any(c >= 0x80 for c in d) for d in allv), "non-ASCII")
        self.assertTrue(any(len(d) > 1048576 for d in allv), "overlong")
        self.assertTrue(any(b"1e100001" in d for d in allv), "huge exponent")
        self.assertTrue(any(d.startswith(b"(((") for d in allv), "deeply nested")
        self.assertTrue(any(b"007" in d for d in allv), "leading zeros")
        self.assertTrue(any(b"mod -6" in d for d in allv), "negative modulus")
        self.assertTrue(any(b"1/0" in d for d in allv), "zero denominator")

    def test_spec_and_plan_examples_present(self):
        want = {
            "rat": ["7/3"],
            "adele": ["(3.14159 +/- 1e-5 ; 5/3 mod 6)"],
            "fball": ["(* ; 2 mod 6)", "3 mod 12", "5 mod 18", "2 mod 6", "3 mod 6", "60 mod 216", "5/3 mod 6",
                      "1/2 mod 8", "2/3 mod 9", "0 mod 1/6"],
            "lball": ["[p=5: 3 + O(5^4)]"],
            "idele": ["(2.5 +/- 1e-9 ; 3/2 * [5 mod 36])"],
            "idclass": ["<1.25 +/- 1e-30 ; [5 mod 36]>"],
            "ucoset": ["[5 mod 6]", "[2 mod 3]"],
        }
        for name, examples in want.items():
            inputs = {v[1] for v in read_golden(name)}
            for e in examples:
                self.assertIn(e, inputs, name)

    def test_unescape(self):
        self.assertEqual(unescape(r"a\x00b\t\\"), b"a\x00b\t\\")
        with self.assertRaises(ValueError):
            unescape(r"\q")


class TestGoldenVectors(unittest.TestCase):

    def check_file(self, name, fn):
        failures = []
        for lineno, inp, data, exp in read_golden(name):
            got = fn(data)
            if got != exp:
                failures.append("%s.tsv:%d %r: expected %r, got %r" % (name, lineno, inp[:80], exp, got))
        self.assertEqual(failures, [], "\n".join(failures))

    def test_types(self):
        for name in TYPES:
            with self.subTest(type=name):
                self.check_file(name, lambda d, name=name: tg.canonical(name, d))

    def test_fixed_points(self):
        """Every expected output is canonical: it reads back to itself and classifies as its type."""
        failures = []
        for name in TYPES:
            for lineno, inp, data, exp in read_golden(name):
                if is_status(exp):
                    continue
                again = tg.canonical(name, exp.encode())
                cls = tg.classify(exp.encode())
                if again != exp or cls != name:
                    failures.append("%s:%d %r -> %r, %r" % (name, lineno, exp, again, cls))
        self.assertEqual(failures, [], "\n".join(failures))

    def test_dispatch(self):
        self.check_file("dispatch", tg.classify)

    def test_realball_read(self):
        def f(d):
            try:
                lo, hi = tg.read_real(d)
            except tg.TextError as e:
                return "!" + e.status
            return "%s %s" % (lo, hi)
        self.check_file("realball_read", f)

    def test_realball_print(self):
        def f(d):
            mid, rad, digits = d.decode().split()
            return tg.print_real(Fraction(mid), Fraction(rad), int(digits))
        self.check_file("realball_print", f)

    def test_realball_print_vectors_are_c_representable(self):
        """docs/conventions.md 11.3(4): midpoints with at most 128 bits, radius mantissa below 2^30."""
        for lineno, inp, data, exp in read_golden("realball_print"):
            mid, rad, _ = data.decode().split()
            for q, bits in ((Fraction(mid), 128), (Fraction(rad), 30)):
                den = q.denominator
                self.assertEqual(den & (den - 1), 0, "not dyadic: line %d" % lineno)
                m = abs(q.numerator)
                while m and m % 2 == 0:
                    m //= 2
                if bits == 128:
                    # 2^1000 is allowed: its odd mantissa is 1
                    self.assertLess(m, 2 ** bits, "line %d" % lineno)
                else:
                    self.assertLess(m, 2 ** bits, "line %d" % lineno)

    def test_dump(self):
        self.check_file("dump", tg.dump_roundtrip)

    def test_psi_phases(self):
        self.check_file("psi_phases", tg.psi_phases)

    def test_psi_phases_definition(self):
        """The phase set equals psi_f of sampled points of the ball, computed from the p-primary fractional parts
        of analysis.md Definition 1 (independent of text_grammar.psi_phases)."""
        rng = random.Random(8)
        for lineno, inp, data, exp in read_golden("psi_phases"):
            if is_status(exp):
                continue
            text = tg.canonical("fball", data)[4:-1]          # "a" or "a mod N"
            a = Fraction(text.split(" mod ")[0])
            N = Fraction(text.split(" mod ")[1]) if " mod " in text else Fraction(0)
            angles = {Fraction(t) for t in exp.split()}
            seen = set()
            for _ in range(200):
                x = a + N * rng.randrange(-50, 50)            # rational points of the ball: a + N Z
                seen.add(sum_fp(x) % 1)
            self.assertEqual(seen, angles, inp)

    def test_gauss(self):
        """tests/golden/gauss.tsv: the balls contain tau and W recomputed at 90 digits, |tau|^2 = C, and the
        texts are fixed points of the complex-ball printer."""
        import flint
        import mpmath
        from math import gcd as igcd
        mpmath.mp.dps = 90
        for lineno, inp, data, exp in read_golden("gauss"):
            canon = tg.canonical("char", data)
            q = int(canon.split("q=")[1].split(",")[0])
            n = int(canon.split("n=")[1].split(",")[0])
            parts = dict(item.split("=", 1) for item in exp.split(" ", 1)[:1])
            e = int(parts["e"])
            tau_text = exp.split(" tau=")[1].split(" W=")[0]
            w_text = exp.split(" W=")[1]
            if q == 1:
                tau = mpmath.mpc(1)
                self.assertEqual(e, 0)
            else:
                c = flint.dirichlet_char(q, n)
                self.assertEqual(int(c.parity()), e)
                ex = int(flint.dirichlet_group(q).exponent())
                tau = mpmath.mpc(0)
                for x in range(1, q + 1):
                    if igcd(x, q) == 1:
                        t = Fraction(int(c.chi_exponent(x)), ex) + Fraction(x, q)
                        tau += mpmath.expjpi(2 * mpmath.mpf(t.numerator) / t.denominator)
            w = tau / (mpmath.mpc(0, 1) ** e * mpmath.sqrt(q))
            for text, z in ((tau_text, tau), (w_text, w)):
                re_t, im_t = text[1:-3].split(") + (")
                for part, val in ((re_t, z.real), (im_t, z.imag)):
                    lo, hi = tg.read_real(part.encode())
                    v = Fraction(mpmath.nstr(val, 80))
                    self.assertTrue(lo - Fraction(1, 10 ** 70) <= v <= hi + Fraction(1, 10 ** 70), (inp, part))
                    self.assertLess(hi - lo, Fraction(1, 10 ** 18))
                    self.assertEqual(tg.print_real((lo + hi) / 2, (hi - lo) / 2, 20), part)
            self.assertLess(abs(abs(tau) ** 2 - q), mpmath.mpf(10) ** -80)


class TestGateFindings(unittest.TestCase):
    """Regressions for the milestone-0 gate review (docs/reviews/m0-gate/review.md) applied to the reference."""

    def test_g8_context_block_counts_have_limits(self):
        """G8: max_items bounds the block count of every context occurrence, before semantic checks."""
        lim = tg.Limits(max_items=1)
        for s in ["adf1 Q modctx 6 2 2 3",
                  "adf1 Q fball l 1 6 2 2 3 0 0",
                  "adf1 Q scaled s 1 1 0 6 2 2 3",
                  "adf1 Q qclass pieces 1 1 1 -1 0 0 l 2 6 2 2 3 0 0"]:
            self.assertEqual(tg.dump_roundtrip(s.encode(), lim), "!LIMIT", s)
        # every count limit precedes every semantic error (8.5 stage 4 before stage 6)
        self.assertEqual(tg.dump_roundtrip(b"adf1 Q modctx 7 2 2 3", lim), "!LIMIT")
        self.assertEqual(tg.dump_roundtrip(b"adf1 Q modctx 7 2 2 3"), "!DOMAIN")
        self.assertEqual(tg.dump_roundtrip(b"adf1 Q modctx 6 2 2 3", tg.Limits(max_items=2)),
                         "adf1 Q modctx 6 2 2 3")

    def test_g3_context_occurrences_and_bindings(self):
        """G3: one binding per context occurrence, in dump traversal order (conventions 10.2)."""
        two = "adf1 Q qclass pieces 2 1 1 -2 0 0 l 1 2 1 2 0 1 3 -2 0 0 l 1 3 1 3 0"
        self.assertEqual(tg.dump_contexts(two.encode()), [(2, (2,)), (3, (3,))])
        self.assertEqual(tg.dump_load_check(two.encode(), [(2, (2,)), (3, (3,))]), "OK")
        for binds in ([], [(2, (2,))], [(2, (2,)), (2, (2,))], [(3, (3,)), (2, (2,))],
                      [(2, (2,)), (3, (3,), None)], [(2, (2,)), None], [(2, (2,)), (3, (3,)), (2, (2,))]):
            self.assertNotEqual(tg.dump_load_check(two.encode(), binds), "OK", repr(binds))
        # nested and single occurrences, including a scaled context with no blocks
        self.assertEqual(tg.dump_contexts(b"adf1 Q adele 1 1 0 0 0 l 2 6 2 2 3 0 1"), [(6, (2, 3))])
        self.assertEqual(tg.dump_load_check(b"adf1 Q adele 1 1 0 0 0 l 2 6 2 2 3 0 1", [(6, (2, 3))]), "OK")
        self.assertEqual(tg.dump_load_check(b"adf1 Q adele 1 1 0 0 0 l 2 6 2 2 3 0 1", []), "!DOMAIN")
        self.assertEqual(tg.dump_contexts(b"adf1 Q scaled x 1 2 1 0"), [(1, ())])
        self.assertEqual(tg.dump_load_check(b"adf1 Q scaled x 1 2 1 0", [(1, ())]), "OK")
        self.assertEqual(tg.dump_load_check(b"adf1 Q scaled x 1 2 1 0", [(2, (2,))]), "!DOMAIN")
        self.assertEqual(tg.dump_contexts(b"adf1 Q qclass lift 1 1 -1 0 0 g 1 3 3"), [])
        self.assertEqual(tg.dump_load_check(b"adf1 Q qclass lift 1 1 -1 0 0 g 1 3 3", []), "OK")
        self.assertEqual(tg.dump_load_check(b"adf1 Q qclass lift 1 1 -1 0 0 g 1 3 3", [(1, ())]), "!DOMAIN")
        # an invalid dump reports its own status before any binding rule
        self.assertEqual(tg.dump_load_check(b"adf1 Q rat 1 0", [(1, ())]), "!DOMAIN")
        self.assertEqual(tg.dump_contexts(b"adf2 Q rat 1 1"), "!UNSUPPORTED")

    def test_g7_polynomial_normalisation(self):
        """G7: P has length >= 0 and no exact-zero last coefficient; trailing exact zeros are removed on
        input of the value form; a ball merely containing zero is not trimmed."""
        z, o, ball = "(0) + (0)*i", "(1) + (0)*i", "(0 +/- 0.25) + (0)*i"

        def term(p):
            return "rfun(term(P=[%s], A=%s, B=%s, C=%s))" % (p, o, z, z)

        self.assertEqual(tg.canonical("rfun", term(z).encode()), term(""))
        self.assertEqual(tg.canonical("rfun", term("").encode()), term(""))
        self.assertEqual(tg.canonical("rfun", term(o + ", " + z).encode()), term(o))
        self.assertEqual(tg.canonical("rfun", term(z + ", " + o + ", " + z).encode()), term(z + ", " + o))
        self.assertEqual(tg.canonical("rfun", term(o + ", " + ball).encode()), term(o + ", " + ball))

    def test_g4_c_value_text_round_trips_may_change(self):
        """G4: 1 +/- 0.13 prints the exactly representable ball 1 +/- 1/8, but the arb fields that a C parser
        stores for 1 +/- 0.13 and its rereading (checks/flint_probe.c at prec 128) print one step wider on every
        pass. Each text still encloses the exact interval of the first text (conventions 9.6, version 0.3)."""
        self.assertEqual(tg.print_real(Fraction(1), Fraction(1, 8), 20), "1 +/- 0.13")
        t1 = tg.print_real(Fraction(1), Fraction(0x10a3d70b) * Fraction(2) ** -31, 20)
        t2 = tg.print_real(Fraction(1), Fraction(0x23d70a3f) * Fraction(2) ** -32, 20)
        self.assertEqual(t1, "1 +/- 0.14")
        self.assertEqual(t2, "1 +/- 0.15")
        for t in ("1 +/- 0.13", t1, t2):
            lo, hi = tg.read_real(t.encode())
            self.assertLessEqual(lo, Fraction(87, 100), t)
            self.assertGreaterEqual(hi, Fraction(113, 100), t)


class TestStatus(unittest.TestCase):
    def test_values(self):
        self.assertEqual(tg.STATUS, {"OK": 0, "NOT_DETERMINED": 1, "UNIT_NOT_CERTIFIED": 2, "NEEDS_SPLIT": 3,
                                     "NOT_UNIQUE": 4, "NO_SOLUTION": 5, "NOT_UNIT": 6, "DOMAIN": 7,
                                     "UNSUPPORTED": 8, "PARSE": 9, "LIMIT": 10})

    def test_combine(self):
        self.assertEqual(tg.combine([]), ("OK", None))
        self.assertEqual(tg.combine([(3, "NOT_DETERMINED"), (0, "DOMAIN"), (5, "DOMAIN")]), ("DOMAIN", 0))
        self.assertEqual(tg.combine([(2, "OK"), (3, "NOT_DETERMINED")]), ("NOT_DETERMINED", 3))


def sum_fp(x):
    """Sum over p of the p-primary fractional parts {x}_p of a rational x (analysis.md Definition 1)."""
    x = Fraction(x)
    total = Fraction(0)
    den = x.denominator
    p = 2
    while den > 1:
        if den % p == 0:
            k = 0
            while den % p == 0:
                den //= p
                k += 1
            pk = p ** k
            rest = x.denominator // pk
            # {x}_p = t / p^k with t = numerator * rest^-1 mod p^k
            t = (x.numerator * pow(rest, -1, pk)) % pk
            total += Fraction(t, pk)
        p += 1
    return total


def random_dyadic(rng, maxbits=60, maxexp=80):
    m = rng.randrange(0, 2 ** rng.randrange(1, maxbits))
    e = rng.randrange(-maxexp, maxexp)
    return Fraction(m) * Fraction(2) ** e


class TestRealPrinting(unittest.TestCase):
    """Properties of docs/conventions.md 9.5 on random dyadic balls (as an arb would hold them)."""

    def test_enclosure_and_idempotence(self):
        rng = random.Random(20260927)
        for _ in range(4000):
            mid = random_dyadic(rng) * rng.choice([1, -1])
            rad = random_dyadic(rng, maxbits=30) if rng.random() < 0.8 else Fraction(0)
            n = rng.choice([1, 2, 3, 5, 20])
            s = tg.print_real(mid, rad, n)
            lo, hi = tg.read_real(s.encode())
            self.assertLessEqual(lo, mid - rad, s)
            self.assertGreaterEqual(hi, mid + rad, s)
            m2, r2 = (lo + hi) / 2, (hi - lo) / 2
            self.assertEqual(tg.print_real(m2, r2, n), s, (mid, rad, n))

    def test_loop_count_small(self):
        rng = random.Random(7)
        worst = 0
        for _ in range(4000):
            mid = random_dyadic(rng) * rng.choice([1, -1])
            rad = random_dyadic(rng, maxbits=30)
            _, loops, _ = tg.print_real_detail(mid, rad, rng.choice([1, 2, 3, 20]), 2)
            worst = max(worst, loops)
        self.assertLessEqual(worst, 3)

    def test_constrained(self):
        rng = random.Random(11)
        for _ in range(3000):
            mid = random_dyadic(rng, maxbits=40, maxexp=20)
            if mid == 0:
                continue
            frac = Fraction(rng.randrange(1, 10 ** 6), 10 ** 6)
            rad = abs(mid) * (1 - frac / rng.choice([1, 10, 1000, 10 ** 6]))
            rad = Fraction(int(rad * 2 ** 40), 2 ** 40)
            if rad >= abs(mid):
                continue
            mid = mid * rng.choice([1, -1])
            n = rng.choice([2, 5, 20])
            s = tg.print_real(mid, rad, n, cond="nonzero")
            lo, hi = tg.read_real(s.encode())
            self.assertTrue(lo > 0 or hi < 0, s)
            self.assertLessEqual(lo, mid - rad)
            self.assertGreaterEqual(hi, mid + rad)
            self.assertEqual(tg.print_real((lo + hi) / 2, (hi - lo) / 2, n, cond="nonzero"), s, (mid, rad))
            if mid > 0:
                s2 = tg.print_real(mid, rad, n, cond="positive")
                lo2, _ = tg.read_real(s2.encode())
                self.assertGreater(lo2, 0)

    def test_fmt(self):
        cases = {Fraction(0): "0", Fraction(5, 2): "2.5", Fraction(1, 10000): "0.0001", Fraction(1, 100000): "1e-5",
                 Fraction(1250): "1250", Fraction(10 ** 20): "100000000000000000000", Fraction(10 ** 21): "1e21",
                 Fraction(3, 2 * 10 ** 30): "1.5e-30", Fraction(-3, 4): "-0.75", Fraction(3 * 10 ** 21): "3e21"}
        for x, s in cases.items():
            self.assertEqual(tg.fmt_decimal(x), s)


class TestCrossChecks(unittest.TestCase):

    def test_fball_against_tests_ref(self):
        """The canonical finite part agrees with canon() of the Python reference of lane m1-pyref."""
        from adfref.fball import canon
        rng = random.Random(3)
        for _ in range(3000):
            a = Fraction(rng.randrange(-500, 500), rng.randrange(1, 40))
            N = Fraction(rng.randrange(0, 60), rng.randrange(1, 40))
            text = "%s mod %s" % (a, N)
            got = tg.canonical("fball", text.encode())
            d = a.denominator * N.denominator
            A, H, dd = canon(int(a * d), int(N * d), d)
            ca, cN = Fraction(A, dd), Fraction(H, dd)
            want = "(* ; %s)" % (ca if H == 0 else "%s mod %s" % (ca, cN))
            self.assertEqual(got, want, text)

    def test_lball_centre_definition(self):
        """The printed centre is in the ball, in [0, p^N), and has a p-power denominator (conventions 5.8)."""
        rng = random.Random(5)
        for _ in range(2000):
            p = rng.choice([2, 3, 5, 7, 11])
            x = Fraction(rng.randrange(-400, 400), rng.randrange(1, 300))
            N = rng.randrange(-4, 6)
            got = tg.canonical("lball", ("[p=%d: %s + O(%d^%d)]" % (p, x, p, N)).encode())
            centre = Fraction(got.split(": ")[1].split(" + ")[0])
            self.assertTrue(0 <= centre < Fraction(p) ** N, got)
            den = centre.denominator
            while den % p == 0:
                den //= p
            self.assertEqual(den, 1, got)
            diff = (x - centre) / Fraction(p) ** N
            self.assertEqual(diff.denominator % p != 0, True, (x, got))


class TestFuzz(unittest.TestCase):
    """Deterministic mutation fuzzing: every byte string gives a string result, never an exception, and every
    accepted input prints to a fixed point of the same type."""

    def test_mutations(self):
        rng = random.Random(99)
        seeds = []
        for name in TYPES:
            seeds += [(name, v[2]) for v in read_golden(name) if len(v[2]) < 400]
        alphabet = b" ()[]<>{};,:*+=^/-.e0123456789modpORCiQunfftrchaDMPABqns\x00\x80\t"
        for _ in range(6000):
            name, data = rng.choice(seeds)
            b = bytearray(data)
            for _ in range(rng.randrange(1, 4)):
                op = rng.randrange(3)
                pos = rng.randrange(len(b) + 1)
                if op == 0 or not b:
                    b[pos:pos] = bytes([rng.choice(alphabet)])
                elif op == 1:
                    del b[min(pos, len(b) - 1)]
                else:
                    b[min(pos, len(b) - 1)] = rng.choice(alphabet)
            b = bytes(b)
            target = rng.choice(TYPES)
            out = tg.canonical(target, b)
            self.assertIsInstance(out, str)
            if not out.startswith("!"):
                self.assertEqual(tg.canonical(target, out.encode()), out, (target, b))
                self.assertEqual(tg.classify(out.encode()), target, (target, b))
            cls = tg.classify(b)
            self.assertTrue(cls.startswith("!") or cls in TYPES)
            if not out.startswith("!"):
                self.assertEqual(cls, target, b)

    def test_random_bytes(self):
        rng = random.Random(1234)
        for _ in range(3000):
            b = bytes(rng.randrange(256) for _ in range(rng.randrange(0, 40)))
            for t in TYPES:
                self.assertIsInstance(tg.canonical(t, b), str)
            self.assertIsInstance(tg.dump_roundtrip(b), str)

    def test_dump_mutations(self):
        rng = random.Random(4321)
        seeds = [v[2] for v in read_golden("dump") if len(v[2]) < 400]
        alphabet = b" 0123456789abcdef-xgbnlrc"
        for _ in range(4000):
            b = bytearray(rng.choice(seeds))
            pos = rng.randrange(len(b))
            b[pos] = rng.choice(alphabet)
            b = bytes(b)
            out = tg.dump_roundtrip(b)
            self.assertIsInstance(out, str)
            if not out.startswith("!"):
                self.assertEqual(out.encode(), b)


if __name__ == "__main__":
    unittest.main()
