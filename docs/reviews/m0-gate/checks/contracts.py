#!/usr/bin/env python3
"""Read-only adversarial checks; assertions confirm the reported counterexamples.

Run from the repository root with python3 -B. No production file is modified.
"""
import json
import sys
from fractions import Fraction as F
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
sys.path[:0] = [str(ROOT / "proto"), str(ROOT / "tests/ref")]
import text_grammar as tg
from adfref.fball import Fball, contains_rational
from adfref.policies import ScaledBall, absolute_cap, scaled_add, scaled_mul
from adfref.recon import reconstruct


emitted = 0


def emit(name, **values):
    global emitted
    emitted += 1
    print(json.dumps(dict(check=name, **values), sort_keys=True))


# Mismatched contexts: the documented precondition is not checked by scaled_add.
x, y = ScaledBall(1, 0, 2), ScaledBall(1, 0, 3)
bad = scaled_add(x, y)
assert contains_rational(x.to_fball(), -2) and contains_rational(y.to_fball(), 3)
assert not contains_rational(bad.to_fball(), 1)
emit("scaled_add_context", K_left=2, K_right=3, true_sum=1, result=str(bad), contains=False)
try:
    scaled_mul(x, y)
except ValueError:
    emit("scaled_mul_context", rejects=True)
else:
    raise AssertionError("mul unexpectedly accepted")

# The independently chosen cap and closed-interval conventions are implemented.
caps = 0
for q in [F(-7, 3), F(0), F(1, 3), F(2)]:
    for cap in [F(1, 5), F(1), F(6)]:
        b = Fball.from_center_radius(q, 0)
        assert absolute_cap(b, cap) == b
        caps += 1
recons = 0
for a in [F(-1, 3), F(0), F(5, 2)]:
    for radius in [F(0), F(1, 2), F(3)]:
        b = Fball.from_center_radius(a, radius)
        for lo, hi in [(a, a), (a - 1, a), (a, a + 1)]:
            assert a in reconstruct(b, lo, hi).solutions
            recons += 1
emit("reference_decisions", exact_cap_cases=caps, endpoint_cases=recons)

# The dump validator forgets the max_items limit on contexts, including nested ones.
for s in ["adf1 Q modctx 6 2 2 3", "adf1 Q fball l 1 6 2 2 3 0 0",
          "adf1 Q scaled s 1 1 0 6 2 2 3",
          "adf1 Q qclass pieces 1 1 1 -1 0 0 l 2 6 2 2 3 0 0"]:
    result = tg.dump_roundtrip(s, tg.Limits(max_items=1))
    assert result == s
    emit("missing_context_limit", input=s, max_items=1, expected="!LIMIT", actual=result)

# Each piece satisfies the storage predicate, but no single supplied ctx can load both.
mixed = "adf1 Q qclass pieces 2 1 1 -2 0 0 l 1 2 1 2 0 1 3 -2 0 0 l 1 3 1 3 0"
assert tg.dump_roundtrip(mixed) == mixed
emit("mixed_context_dump", input=mixed, contexts=[2, 3], accepted=True)

# A local raw piece whose canonical denominator is 1 but stored d is 2.
raw = "adf1 Q qclass pieces 1 1 1 -1 0 0 l 2 6 2 2 3 0 0"
assert tg.dump_roundtrip(raw) == raw
emit("raw_piece_predicate", input=raw, stored_d=2, canonical_d=1, accepted=True)

# acb_poly trims a zero top coefficient, whereas text accepts/preserves it.
z = "(0) + (0)*i"
o = "(1) + (0)*i"
for coeffs in [z, o + ", " + z]:
    text = "rfun(term(P=[" + coeffs + "], A=" + o + ", B=" + z + ", C=" + z + "))"
    assert tg.canonical("rfun", text) == text
    emit("rfun_zero_top", input=text, accepted=True)

# The claimed term-count minimality is not part of the proved inequalities.
emit("log_count_name", p=3, v=1, n=5, prescribed_terms=5, sufficient_terms=4,
     note="k-v_3(k) >= 5 for every k >= 5; at k=5 the floor-log bound is only 4")

# G has two Boolean parses unless parentheses enclose the two H cases.
common, positive_case, exact_case = False, False, True
emit("G_parentheses", conventional_precedence=(common and positive_case) or exact_case,
     intended=common and (positive_case or exact_case), witness="A=1,H=0,d=-1")

# A large-radius quotient piece is admitted; the midpoint alone does not limit excess.
qtext = "union((0.99 +/- 100 ; 0 mod 1)) + Q"
qout = tg.canonical("qclass", qtext)
emit("quotient_midpoint_print", input=qtext, output=qout, reread=tg.canonical("qclass", qout))

print(f"contract_records={emitted}")
