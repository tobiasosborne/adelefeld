"""lanes/m1-repair-dump/checks/mutprobe.py: compare the status of a text under the repaired library
and under a mutant library, through the review's bridge.c (public entry points only).

Usage: python3 -B mutprobe.py <path to the mutant bridge.so>

The texts are the candidates for the two surviving mutants of src/dump.c found by
`make mutate FILES=src/dump.c LIMIT=60`:
  938: the count L of an rfun is compared with max_items in the wrong pass;
  883: a complex ball of an sball with the keyword "c" fails to parse.
`sball` and `rfun` are bodies of a dump text in their own right (conventions 10.1), and only
adf_modctx_new_from_dump reads every body, so kind is -1 (adf_modctx_new_from_dump).

An acb is 8 tokens (arb = mid, exp, exp, rad), a ball of an sball is 5 tokens, an rfun term is
L, then L acb, then 3 acb.
"""
import ctypes as C
import sys
from pathlib import Path

REF = Path("lanes/m1-repair-dump/verify/bridge.so")
MUT = Path(sys.argv[1])
NAMES = {0: "OK", 7: "DOMAIN", 8: "UNSUPPORTED", 9: "PARSE", 10: "LIMIT"}


class Limits(C.Structure):
    _fields_ = [("max_len", C.c_size_t), ("max_exp10", C.c_long), ("max_prec", C.c_long),
                ("max_items", C.c_long)]


def load(path):
    lib = C.CDLL(str(path))
    lib.review_probe.argtypes = [C.c_char_p, C.c_size_t, C.c_int, C.c_size_t, C.c_void_p,
                                 C.POINTER(C.c_int)]
    return lib


ACB = "1 0 0 0 0 0 0 0"          # the complex ball 1 + 0 i, both exact
ACB_BAD_RAD = "1 0 0 -1 0 0 0 0"  # the same with the radius of the real part -1
BALL = "1 0 0 0 0"               # a ball of an sball: 5 tokens

CASES = []


def add(label, text, max_items=1048576):
    CASES.append((label, text, max_items))


# --- mutant 883: the complex ball of an sball with the keyword "c" ---
add("sball c, valid, no ball", "adf1 Q sball c " + ACB + " 0")
add("sball c, valid, one ball", "adf1 Q sball c " + ACB + " 1 " + BALL)
add("sball c, one token short", "adf1 Q sball c 1 0 0 0 0 0 0 0")
add("sball c, four tokens short", "adf1 Q sball c 1 0 0 0")
add("sball c, nothing after the keyword", "adf1 Q sball c")
add("sball c, the real ball is a word", "adf1 Q sball c zz 0 0 0 0 0 0 0 0")
add("sball c, the imaginary part is a word", "adf1 Q sball c 1 0 0 0 zz 0 0 0 0")
add("sball c, the real ball has a word in the middle", "adf1 Q sball c 1 zz 0 0 0 0 0 0 0")
add("sball c, a negative radius in the imaginary part", "adf1 Q sball c 1 0 0 0 0 0 0 -1 0")
add("sball r, valid", "adf1 Q sball r 1 0 0 0 0 " + BALL)
add("sball n, valid", "adf1 Q sball n 0")
add("sball n, one ball", "adf1 Q sball n 1 " + BALL)

# --- mutant 938: the count L of an rfun against max_items ---
add("rfun, no term", "adf1 Q rfun 0")
add("rfun, one term, L = 1", "adf1 Q rfun 1 1 " + ACB + " " + ACB * 3)
add("rfun, one term, L = 3, max_items = 2", "adf1 Q rfun 1 3 " + ACB * 3 + " " + ACB * 3, 2)
add("rfun, one term, L = 3, max_items = 3", "adf1 Q rfun 1 3 " + ACB * 3 + " " + ACB * 3, 3)
add("rfun, n = 2 over max_items = 1, both terms valid", "adf1 Q rfun 2 1 " + ACB + " " + ACB * 3
    + " 1 " + ACB + " " + ACB * 3, 1)
add("rfun, a bad radius before an L over max_items = 2",
    "adf1 Q rfun 1 3 " + ACB_BAD_RAD + " " + ACB * 2 + " " + ACB * 3, 2)
add("rfun, a bad radius before an L over max_items = 1048576",
    "adf1 Q rfun 1 3 " + ACB_BAD_RAD + " " + ACB * 2 + " " + ACB * 3)
add("rfun, L over max_items = 2 and the body is short", "adf1 Q rfun 1 3 " + ACB * 3, 2)
add("rfun, L over max_items = 2 and the body has a word", "adf1 Q rfun 1 3 " + ACB * 3 + " zz "
    + ACB * 3, 2)
add("rfun, L over max_items = 2 in the second term, the first one bad",
    "adf1 Q rfun 2 1 " + ACB_BAD_RAD + " " + ACB * 3 + " 3 " + ACB * 3 + " " + ACB * 3, 2)


def probe(lib, text, max_items):
    b = text.encode()
    trouble = C.c_int()
    v = Limits(1048576, 100000, 100000, max_items)
    st = lib.review_probe(b, len(b), -1, 0, C.byref(v), C.byref(trouble))
    return NAMES[st], trouble.value


ref, mut = load(REF), load(MUT)
diff = 0
for label, text, max_items in CASES:
    a, ta = probe(ref, text, max_items)
    c, tc = probe(mut, text, max_items)
    flag = "" if (a, ta) == (c, tc) else "  <-- DIFFERS"
    if flag:
        diff += 1
    print(f"max_items={max_items:<8d} {label:58s} ref={a:12s} mut={c:12s} trouble={ta},{tc}{flag}")
print(f"cases where the status differs: {diff}")
