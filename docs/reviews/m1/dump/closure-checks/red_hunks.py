"""Prepare isolated reversions of the R1/R2/R4 repairs and the R3 harness repair.

The selected regression functions are included unchanged from tests/. No repository source is edited.
R2 restores the old semantic function from the repair lane's preserved source, plus the old arb walk.
"""
from pathlib import Path
import re

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[4]
scratch = HERE / "red"
scratch.mkdir(exist_ok=True)
source = (ROOT / "src/dump.c").read_text()
old = (ROOT / "lanes/m1-repair-dump/checks/red/dump_before.c").read_text()
cap = "    if (st->mode == DP_WORDS && x->k > (size_t) ADF_MODCTX_MAX_BLOCKS)\n"
cap += "        return dp_fail(st, ADF_UNSUPPORTED);\n"
assert source.count(cap) == 1
variants = {"R1": source.replace(cap, "")}
limit = "    if (st->mode == DP_LIMITS && st->arb_exp_limit\n"
limit += "        && (dp_abs_over(a->e, ADF_DUMP_QCLASS_EXP_MAX) || "
limit += "dp_abs_over(a->re, ADF_DUMP_QCLASS_EXP_MAX)))\n"
limit += "        return dp_fail(st, ADF_LIMIT);\n"
assert source.count(limit) == 1
start = "static int\ndp_q_piece("
end = "static int\ndp_w_qclass("
piece = old[old.index(start):old.index(end)]
r2 = source[:source.index(start)] + piece + source[source.index(end):]
variants["R2"] = r2.replace(limit, "")
alphabet = "(b < 0x20 && b != 0x09 && b != 0x0a && b != 0x0d) || b > 0x7e"
assert source.count(alphabet) == 1
variants["R4"] = source.replace(alphabet, "b < 0x20 || b > 0x7e")
for name, contents in variants.items():
    (scratch / (name + ".c")).write_text(contents)
    testfile = "test_dump.c" if name == "R4" else "test_dump_limits.c"
    test = {"R1": "block_cap_before_the_predicate_of_the_context",
            "R2": "qclass_exponent_bound_is_a_limit_of_stage_four",
            "R4": "header_before_the_whitespace_of_the_body"}[name]
    code = '#define ADF_TEST_NO_MAIN\n#include "tests/' + testfile + '"\n'
    code += 'int main(void) { adf_test_current = "' + test + '";\n'
    code += 'adf_test_fn_' + test + '();\n'
    code += 'printf("selected_tests=1 checks=%lu failures=%lu\\n", adf_test_checks, adf_test_failures);\n'
    code += 'flint_cleanup_master(); return adf_test_failures != 0; }\n'
    (scratch / (name + "_test.c")).write_text(code)

fuzz = (ROOT / "tests/fuzz/fuzz_dump.c").read_text()
fuzz = fuzz.replace('"../../src/', '"src/')
fuzz = fuzz.replace("size_t tl = 0;", "size_t tl;")
pattern = r"char \* t = (adf_\w+_dump_str\(&tl, x\));\n\s+same_bytes\(t, tl, s, n\);"
fuzz, replacements = re.subn(pattern, r"same_bytes(\1, tl, s, n);", fuzz)
assert replacements == 5
(scratch / "R3_fuzz.c").write_text(fuzz)
driver = (HERE / "fuzz_driver.c").read_text().replace('"tests/fuzz/fuzz_dump.c"', '"R3_fuzz.c"')
(scratch / "R3_test.c").write_text(driver)
print("isolated_library_reversions=3 selected_regression_functions=3 harness_reversions=1 length_sites=5")
