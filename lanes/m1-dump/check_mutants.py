#!/usr/bin/env python3
"""Apply single mutants of src/dump.c by hand and run test programs against each (lane m1-dump).

Used to confirm that a test added after a mutation run kills the mutant it was written for. Each
mutant is (line, old text, new text); the mutated file is compiled into the scratch directory given as
the first argument and linked before build/libadelefeld.a, so that its definitions win. With --san the
mutant and the tests are compiled with -fsanitize=address,undefined (the library must then be built
with `make SAN=1`). Run from the repository root:

    make build/test_dump build/test_dump_ctx            (or: make SAN=1 ...)
    python3 lanes/m1-dump/check_mutants.py <scratch dir> [--san] [name ...]
"""
import os
import subprocess
import sys

MUTANTS = {
    # run 2 survivors, killed by the ffun tests of stage_edges_and_piece_order
    "902": (902, "st->mode == DP_LIMITS && dp_over_items(dm, st->lim)",
            "st->mode != DP_LIMITS && dp_over_items(dm, st->lim)"),
    "894": (894, "fmpz_cmp_ui(M, (ulong) c->left) > 0", "fmpz_cmp_ui(M, (ulong) c->left) > 1"),
    # run 1 survivors of the kind drop_call, judged with the sanitizers
    "560": (560, "flint_free(d->q);", ""),
    "1373": (1373, "dp_sb_room(b, n);", ""),
}


def main():
    scratch = sys.argv[1]
    san = "--san" in sys.argv[2:]
    names = [a for a in sys.argv[2:] if a != "--san"] or list(MUTANTS)
    flags = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"] if san else []
    src = open("src/dump.c").read().split("\n")
    for name in names:
        ln, old, new = MUTANTS[name]
        lines = list(src)
        assert old in lines[ln - 1], (name, lines[ln - 1])
        lines[ln - 1] = lines[ln - 1].replace(old, new)
        c = os.path.join(scratch, "mut_%s.c" % name)
        o = os.path.join(scratch, "mut_%s.o" % name)
        with open(c, "w") as f:
            f.write("\n".join(lines))
        subprocess.check_call(["cc", "-Iinclude", "-std=c11", "-O2", "-g"] + flags + ["-c", c, "-o", o])
        for test in ("test_dump", "test_dump_ctx"):
            t = os.path.join(scratch, "%s_%s" % (test, name))
            subprocess.check_call(["cc", "-Iinclude", "-Itests", "-std=c11", "-O2", "-g"] + flags +
                                  ["tests/%s.c" % test, o, "build/support/golden.o", "build/support/jsonl.o",
                                   "build/libadelefeld.a", "-lflint", "-lgmp", "-lm", "-o", t])
            r = subprocess.run([t], capture_output=True, text=True)
            fails = [l for l in r.stdout.splitlines() if l.startswith("FAIL tests")]
            san_msg = [l for l in r.stderr.splitlines() if "Sanitizer" in l or "runtime error" in l]
            print("mutant %s, %s: exit %d, %d failed checks, %s" % (
                name, test, r.returncode, len(fails), san_msg[0].strip() if san_msg else "no sanitizer message"))


if __name__ == "__main__":
    main()
