#!/usr/bin/env python3
"""Exact decimal printer applied to the installed FLINT probe's dyadic fields."""
import sys
from pathlib import Path
from fractions import Fraction as F

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[3] / "proto"))
import text_grammar as tg

fields = dict(line.split("=", 1) for line in (HERE / "flint_probe.out").read_text().splitlines()
              if line.startswith("read_decimal_"))
out = []
for key, value in fields.items():
    mm, me, rm, re = (int(x, 16) for x in value.split())
    printed = tg.print_real(F(mm) * F(2) ** me, F(rm) * F(2) ** re)
    out.append(printed)
    print(key, "dump=" + value, "printed=" + printed)
assert out == ["1 +/- 0.14", "1 +/- 0.15"]
print("C_read_print_fixed_point_failures=2")
