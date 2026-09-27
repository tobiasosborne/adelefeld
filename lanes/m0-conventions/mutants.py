#!/usr/bin/env python3
"""Mutation check of proto/text_grammar.py: each mutant changes one rule of docs/conventions.md; the test suite
proto/test_text_grammar.py (a fast subset, failfast) must fail on every mutant.

Run from the repository root:  python3 lanes/m0-conventions/mutants.py [first] [last]
"""
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "proto", "text_grammar.py")
TEST = os.path.join(ROOT, "proto", "test_text_grammar.py")

MUTANTS = [
    ("ties away from zero instead of to even (9.5)",
     "return round(Fraction(y) / u) * u", "return math.floor(Fraction(y) / u + Fraction(1, 2)) * u"),
    ("radius rounded down (9.5)", "return math.ceil(Fraction(E) / unit) * unit",
     "return math.floor(Fraction(E) / unit) * unit"),
    ("positional range starts at -5 (9.5 fmt)", "if -4 <= X <= 20:", "if -5 <= X <= 20:"),
    ("positional range ends at 21 (9.5 fmt)", "if -4 <= X <= 20:", "if -4 <= X <= 21:"),
    ("midpoint digit below the radius dropped (9.5 step 3)", "q = max(q, _floor_log10(rad) - k + 1)",
     "q = max(q, _floor_log10(rad) - k + 2)"),
    ("unit modulus halved when even (5.6)", "if N % 4 == 2:", "if N % 2 == 0:"),
    ("unit residue range 0..N-1 in the printed normal form (CV-16)",
     "    c %= N\n    if c == 0:\n        c = N\n    return (c, N)\n\n\ndef _is_prime",
     "    c %= N\n    return (c, N)\n\n\ndef _is_prime"),
    ("unit residue range 0..N in the dump loader (CV-16)", "elif N < 0 or not 1 <= c <= N", "elif N < 0 or not 0 <= c <= N"),
    ("piece midpoint must be < 1 (5.10)", "if not (0 <= mid <= 1):", "if not (0 <= mid < 1):"),
    ("excludes-zero test not strict (9.3)", "return lo > 0 or hi < 0", "return lo >= 0 or hi <= 0"),
    ("finite centre reduced into (0, N] (5.2)", "a = a - N * math.floor(a / N)", "a = a - N * math.ceil(a / N)"),
    ("length limit off by one (8.4)", "if len(data) > limits.max_len:", "if len(data) >= limits.max_len:"),
    ("exponent limit off by one (8.4)", "int(e) > limits.max_exp10:", "int(e) >= limits.max_exp10:"),
    ("radius mantissa bound 31 bits instead of 30 (10.2)", "rm >= MAG_LIMIT:", "rm >= 2 * MAG_LIMIT:"),
    ("class real part only non-negative (5.7)", "if _arb_sign(node[1]) != 1:", "if _arb_sign(node[1]) == -1:"),
    ("local centre reduced modulo p^(j+1) (5.8)", "t = (a * pow(b, -1, pj)) % pj", "t = (a * pow(b, -1, pj)) % (pj * p)"),
    ("constrained printing not repeated (9.5)", "        if t == text:\n", "        if True:\n"),
    ("constrained printing keeps n midpoint digits (9.5)", "nk = n + k - 2", "nk = n"),
    ("dump version 2 accepted (10.2)", 'if ver != "1":', 'if ver not in ("1", "2"):'),
    ("unit coset printed as stored, not normal (5.6)", "c, N = ucoset_normal(u)", "c, N = u"),
    ("primality not checked (9.3)", "return bool(flint.fmpz(p).is_prime())", "return True"),
    ("mod 0 kept as a ball (9.3)", "return fmt_rat(a) if N == 0 else", "return fmt_rat(a) if N < 0 else"),
    ("idele scale 0 accepted (9.3)", "if scale <= 0:", "if scale < 0:"),
    ("NUL accepted (8.2)", "if not (0x20 <= c <= 0x7E or c in (0x09, 0x0A, 0x0D)):",
     "if not (0x20 <= c <= 0x7E or c in (0x00, 0x09, 0x0A, 0x0D)):"),
    ("order of checks: DOMAIN before LIMIT (8.5)",
     "        _check_limits(tree, limits)\n        _check_unsupported(tree)\n        return _build_and_print(type_name, tree)",
     "        _build_and_print(type_name, tree)\n        _check_limits(tree, limits)\n        _check_unsupported(tree)\n"
     "        return _build_and_print(type_name, tree)"),
    # part B (version 0.2)
    ("dump field other than Q accepted (10.1, D8)", 'if f.group(1) != "Q":', 'if f.group(1) not in ("Q", "K"):'),
    ("archimedean count not checked (10.1, seams R3)", "    if len(arch) != 1:\n        _domain()",
     "    if len(arch) < 1:\n        _domain()"),
    ("phases over B - 1 roots only (6.1)", "for k in range(B)}", "for k in range(max(B - 1, 1))}"),
    ("raw local: gcd condition reinstated (5.3, CV-55)", "    g = gcd(gcd(A, H), d)\n    return (A // g, H // g, d // g)",
     "    _v_G(A, H, d)\n    return (A, H, d)"),
    ("local denominator 0 accepted (5.3)", "    if d < 1:\n        _domain()\n    for r, m in zip(res, q):",
     "    for r, m in zip(res, q):"),
    ("complex ball under inf read as real (9.2)", 'if self.peek("("):\n                return ("C", self.complex())',
     'if False:\n                return ("C", self.complex())'),
]

SUBSET = ["test_text_grammar.TestGoldenVectors.test_types", "test_text_grammar.TestGoldenVectors.test_dump",
          "test_text_grammar.TestGoldenVectors.test_realball_print",
          "test_text_grammar.TestGoldenVectors.test_realball_read", "test_text_grammar.TestGoldenVectors.test_dispatch",
          "test_text_grammar.TestGoldenVectors.test_psi_phases",
          "test_text_grammar.TestRealPrinting", "test_text_grammar.TestCrossChecks"]


def run(first, last):
    src = open(SRC).read()
    killed = survived = 0
    for idx in range(first, min(last, len(MUTANTS) - 1) + 1):
        name, old, new = MUTANTS[idx]
        if src.count(old) != 1:
            print("[%2d] NOT APPLICABLE (pattern found %d times): %s" % (idx, src.count(old), name))
            survived += 1
            continue
        tmp = tempfile.mkdtemp()
        try:
            os.mkdir(os.path.join(tmp, "proto"))
            os.symlink(os.path.join(ROOT, "tests"), os.path.join(tmp, "tests"))
            with open(os.path.join(tmp, "proto", "text_grammar.py"), "w") as f:
                f.write(src.replace(old, new))
            shutil.copy(TEST, os.path.join(tmp, "proto", "test_text_grammar.py"))
            try:
                r = subprocess.run([sys.executable, "-m", "unittest", "-f"] + SUBSET,
                                   cwd=os.path.join(tmp, "proto"), capture_output=True, text=True, timeout=90)
            except subprocess.TimeoutExpired:
                killed += 1
                print("[%2d] killed (timeout 90 s; the unmutated subset runs in about 25 s): %s" % (idx, name))
                continue
            if r.returncode != 0:
                killed += 1
                print("[%2d] killed:   %s" % (idx, name))
            else:
                survived += 1
                print("[%2d] SURVIVED: %s" % (idx, name))
        finally:
            shutil.rmtree(tmp)
    print("killed %d, survived %d" % (killed, survived))


if __name__ == "__main__":
    a = int(sys.argv[1]) if len(sys.argv) > 1 else 0
    b = int(sys.argv[2]) if len(sys.argv) > 2 else len(MUTANTS) - 1
    run(a, b)
