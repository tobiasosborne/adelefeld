#!/usr/bin/env python3
"""Closure regressions; only the public reference entry points are called."""
from fractions import Fraction as F
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "proto"))
sys.path.insert(0, str(ROOT / "tests/ref"))
import text_grammar as tg
from adfref import policies as p

checks = 0


def equal(actual, expected):
    global checks
    assert actual == expected, (actual, expected)
    checks += 1


two = b"adf1 Q qclass pieces 2 1 1 -2 0 0 l 1 2 1 2 0 1 3 -2 0 0 l 1 3 1 3 0"
same = b"adf1 Q qclass pieces 2 1 1 -2 0 0 l 1 2 1 2 0 1 3 -2 0 0 l 1 2 1 2 0"
cases = [
    (two, [(2, (2,)), (3, (3,))]),
    (same, [(2, (2,)), (2, (2,))]),
    (b"adf1 Q scaled x -1 2 6 2 2 3", [(6, (2, 3))]),
    (b"adf1 Q scaled x 0 1 1 0", [(1, ())]),
    (b"adf1 Q scaled s 1 1 0 10000000000000000 0", [(2 ** 64, ())]),
    (b"adf1 Q adele 1 1 0 0 0 l 2 6 2 2 3 0 1", [(6, (2, 3))]),
    (b"adf1 Q cadele 1 1 0 0 0 0 0 0 0 l 2 6 2 2 3 0 1", [(6, (2, 3))]),
    (b"adf1 Q qclass lift 1 1 0 0 0 l 2 6 2 2 3 0 1", [(6, (2, 3))]),
    (b"adf1 Q modctx 6 2 2 3", [(6, (2, 3))]),
    (b"adf1 Q rat 1 2", []),
]
for text, desc in cases:
    equal(tg.dump_contexts(text), desc)
    equal(tg.dump_roundtrip(text), text.decode())
    equal(tg.dump_load_check(text, desc), "OK")
    equal(tg.dump_load_check(text, desc + [(1, ())]), "!DOMAIN")
    if desc:
        equal(tg.dump_load_check(text, desc[:-1]), "!DOMAIN")
        equal(tg.dump_load_check(text, [None] + desc[1:]), "!DOMAIN")
equal(tg.dump_load_check(two, [(3, (3,)), (2, (2,))]), "!DOMAIN")
equal(tg.dump_load_check(cases[2][0], [(6, (3, 2))]), "!DOMAIN")
print("dump_occurrence_binding_checks=%d failures=0" % checks)

before = checks
for text in (
        b"adf1 Q modctx 6 2 2 3",
        b"adf1 Q fball l 1 6 2 2 3 0 0",
        b"adf1 Q scaled s 1 1 0 6 2 2 3",
        b"adf1 Q scaled x 0 1 6 2 2 3",
        b"adf1 Q qclass pieces 1 1 1 -1 0 0 l 2 6 2 2 3 0 0",
        b"adf1 Q qclass pieces 1 1 1 -1 0 0 l 0 7 2 2 3 0 0"):
    equal(tg.dump_roundtrip(text, tg.Limits(max_items=1)), "!LIMIT")
    equal(tg.dump_load_check(text, [], tg.Limits(max_items=1)), "!LIMIT")
print("nested_context_limit_checks=%d failures=0" % (checks - before))

before = checks
for op in (p.scaled_add, p.scaled_sub, p.scaled_mul):
    try:
        op(p.ScaledBall(1, 0, 2), p.ScaledBall(1, 0, 3))
    except ValueError:
        checks += 1
    else:
        raise AssertionError("mismatched contexts accepted")
for K in range(1, 11):
    for J in range(1, 11):
        from math import lcm
        L = lcm(K, J)
        for u in (0, K - 1):
            x, y = p.ScaledBall(F(2, 3), u, K), p.ScaledBall(F(3, 5), J - 1, J)
            xl, lostx = p.convert_from_tight(x.to_fball(), L)
            yl, losty = p.convert_from_tight(y.to_fball(), L)
            equal((lostx, losty), (False, False))
            equal(xl.to_fball(), x.to_fball())
            equal(yl.to_fball(), y.to_fball())
            result = p.scaled_add(xl, yl).to_fball()
            # Independent rational gcd and centre reduction, with no reference sum operation.
            a, b = x.radius, y.radius
            den = lcm(a.denominator, b.denominator)
            from math import gcd
            radius = F(gcd(int(a * den), int(b * den)), den)
            centre = (x.s * x.u + y.s * y.u) % radius
            equal((result.center, result.radius), (centre, radius))
witness = p.ScaledBall(1, 1, 2)
converted, lost = p.convert_from_tight(witness.to_fball(), 1)
equal(lost, True)
equal(converted.K, 1)
equal((converted.to_fball().center, converted.radius), (F(0), F(1)))
print("scaled_mismatch_and_common_context_checks=%d failures=0" % (checks - before))
print("C5_witness=1+2Zhat target_K=1 result=Zhat lost=1")
print("reference_limitations=context_pointer_identity, borrowing, C_aliasing, exact_operand_contexts")
print("total_checks=%d failures=0" % checks)
