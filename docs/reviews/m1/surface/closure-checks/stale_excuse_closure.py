#!/usr/bin/env python3
"""Run the two scaled mutants whose old line number excuses had gone stale."""
import os
from pathlib import Path
import shutil
import sys

sys.path.insert(0, "tools/mutate")
import mutate  # noqa: E402

eq = mutate.read_equivalent("tools/mutate/equivalent.txt")
source = Path("src/scaled.c").read_text()
picked = [m for m in mutate.mutants_of("src/scaled.c", source, ".")
          if (m.kind == "swap_args" and m.old == "fmpz_mul(t, d, K)")
          or (m.kind == "drop_call" and m.old == "fmpq_zero(x->s);")]
assert len(picked) == 2
scratch = os.path.abspath("build/review_mutate_closure")
bad = 0
try:
    for n, mutant in enumerate(picked):
        mutate.check_mutant(mutant, os.path.abspath("."), scratch, mutate.COPY_ENTRIES,
                            "make -s -j2 check", 170, n)
        good = mutant.status == "survived" and mutant.key in eq
        print(f"{mutant.kind} {mutant.old}: status {mutant.status}, text key in excuses "
              f"{mutant.key in eq}, {'PASS' if good else 'FAIL'}")
        bad += not good
finally:
    shutil.rmtree(scratch, ignore_errors=True)
print(f"stale_excuse_closure: {len(picked)} mutants, {bad} failures")
raise SystemExit(bool(bad))
