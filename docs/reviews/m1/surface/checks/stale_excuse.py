#!/usr/bin/env python3
"""stale_excuse.py: run, with mutate.py's own functions, two mutants of src/scaled.c that
tools/mutate/equivalent.txt excuses under line numbers that no longer hold them (lines 209 and
60, now 201 and 54), and judge them with the tool's own rule (mutate.py:845-854).

    python3 docs/reviews/m1/surface/checks/stale_excuse.py      (from the repository root)

Expected by the commit message of ce1fb15 ("excused mutants of scaled.c ... taken into
equivalent.txt"): both are reported 'excused'.  The script prints what the tool reports."""
import os, shutil, sys, time
sys.dont_write_bytecode = True
sys.path.insert(0, "tools/mutate")
import mutate

root = os.path.abspath(".")
scratch = os.path.abspath("build/review_mutate")
os.makedirs(scratch, exist_ok=True)
eq = mutate.read_equivalent("tools/mutate/equivalent.txt")
text = open("src/scaled.c").read()
pick = [m for m in mutate.mutants_of("src/scaled.c", text, ".")
        if (m.kind == "swap_args" and m.old == "fmpz_mul(t, d, K)")
        or (m.kind == "drop_call" and m.old == "fmpq_zero(x->s);")]
print("excuse entries: %s, %s" % (("src/scaled.c", 209, "swap_args") in eq, ("src/scaled.c", 60, "drop_call") in eq))
for n, m in enumerate(pick):
    t = time.time()
    mutate.check_mutant(m, root, scratch, mutate.COPY_ENTRIES, "make -s -j2 check", 170, n)
    status = m.status
    if status == "survived" and m.key in eq:
        status = "excused"
    print("%s: tests say %s; the tool reports %s (key %s:%d:%s in equivalent.txt: %s) [%.0f s]"
          % (m, m.status, status.upper(), m.key[0], m.key[1], m.key[2], m.key in eq, time.time() - t))
shutil.rmtree(scratch, ignore_errors=True)
