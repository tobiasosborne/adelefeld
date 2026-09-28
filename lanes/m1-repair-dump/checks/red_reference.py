"""Red run for the reference: the three new tests of proto/test_text_grammar.py against
proto/text_grammar.py with the repairs of R1, R2 and R4 taken back out.  Run from the repository
root:  python3 -B lanes/m1-repair-dump/checks/red_reference.py"""
import importlib.util
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "proto"))
sys.path.insert(0, str(ROOT / "tests" / "ref"))

src = (ROOT / "proto" / "text_grammar.py").read_text()

# R4: the alphabet of stage 2 rejects TAB, LF and CR again
new = """    for c in s:
        if not (0x20 <= c <= 0x7E or c in (0x09, 0x0A, 0x0D)):
            raise TextError("PARSE")"""
old = """    for c in s:
        if not 0x20 <= c <= 0x7E:
            raise TextError("PARSE")"""
assert new in src
src = src.replace(new, old)

# R2: the bound of M1-D9 leaves stage 4 and runs in stage 6 again
new = """    if kind == "qclass":
        # stage 4 (decision M1-D9): the binary exponents of the real ball of every piece, on the
        # digit strings, before any semantic check
        for a in _qclass_arbs(node[2]):
            if abs(a[2]) > QCLASS_EXP_MAX or abs(a[4]) > QCLASS_EXP_MAX:
                raise TextError("LIMIT")
    # stage 5: at most MODCTX_MAX_BLOCKS blocks per context (decision M1-D5), decided from the
    # count of the text alone, before the predicate of 5.14
    for ctx in _ctx_occurrences(node):
        if len(ctx[2]) > MODCTX_MAX_BLOCKS:
            raise TextError("UNSUPPORTED")
"""
assert new in src
src = src.replace(new, "")
new = """    if abs(me) > QCLASS_EXP_MAX or abs(re_) > QCLASS_EXP_MAX:"""
old = """    if abs(me) > QCLASS_EXP_MAX or abs(re_) > QCLASS_EXP_MAX:"""
assert new in src

path = pathlib.Path("/tmp/text_grammar_before_repair.py")
path.write_text(src)
spec = importlib.util.spec_from_file_location("text_grammar", path)
before = importlib.util.module_from_spec(spec)
sys.modules["text_grammar"] = before
spec.loader.exec_module(before)

import test_text_grammar as tt  # noqa: E402  (uses sys.modules["text_grammar"])

suite = unittest.TestSuite()
for name in ("test_r1_block_cap_of_m1d5", "test_r2_qclass_exponent_bound_is_a_limit_of_stage_four",
             "test_r4_alphabet_before_the_header"):
    suite.addTest(tt.TestDumpReviewFindings(name))
print("== the reference as it was before the repairs of R1, R2 and R4")
res = unittest.TextTestRunner(verbosity=2).run(suite)
print("failed checks: %d" % (len(res.failures) + len(res.errors)))
