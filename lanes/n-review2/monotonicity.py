"""Exact counterexample for conventions 9.5. No files outside this lane are written."""
from fractions import Fraction as F
import importlib.util

spec = importlib.util.spec_from_file_location("reference", "proto/text_grammar.py")
ref = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ref)
mid, rad = F(793, 512), F(741, 512)
print(f"mid={mid} rad={rad} exact_lower={mid-rad} digits=1")
expected = [(F(2), F(19, 10)), (F(3, 2), F(3, 2))]
for k in range(2, 6):
    text, loops, (m, r) = ref.print_real_detail(mid, rad, 1, k)
    print(f"k={k} text={text!r} loops={loops} lower={m-r} satisfies={m>r}")
    if k < 4:
        assert (m, r) == expected[k - 2]
assert expected[0][0] > expected[0][1]
assert expected[1][0] == expected[1][1]
print("checks=4 failures=0")
