#!/usr/bin/env python3
"""judge.py: build and judge single mutants, for the entries of tools/mutate/equivalent.txt.

    python3 lanes/m1-repair-tools/judge.py SPEC...     (at most 5 specs per call)

SPEC is `FILE:LINE:KIND[:OLD@@NEW]`, e.g. `src/adele.c:242:swap_args` or
`src/recon.c:130:cmp:>=@@>`; OLD and NEW are needed only when the line holds more than one mutant of
that kind.  Each mutant is built and judged exactly as tools/mutate/mutate.py does it
(check_mutant, `make -s -j2 check`), one after the other, and the status is printed:
`survived` means every test passed with the mutant, which is what an equivalent mutant does.
"""
import os
import shutil
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tools", "mutate"))
import mutate  # noqa: E402


def select(spec):
    parts = spec.split(":", 3)
    path, line, kind = parts[0], int(parts[1]), parts[2]
    change = parts[3] if len(parts) > 3 else None
    with open(os.path.join(ROOT, path)) as fh:
        text = fh.read()
    found = [m for m in mutate.mutants_of(path, text, ROOT) if m.line == line and m.kind == kind]
    if change is not None:
        old, _, new = change.partition("@@")
        occ = None
        if "#" in new and new.rsplit("#", 1)[1].isdigit():
            new, occ = new.rsplit("#", 1)
            occ = int(occ)
        found = [m for m in found if m.old == old and m.new == new]
        if occ is not None:
            found = sorted(found, key=lambda m: m.start)[occ - 1: occ]
    if len(found) != 1:
        raise SystemExit("%s: %d mutants match: %s" % (spec, len(found), [str(m) for m in found]))
    return found[0]


def main(argv):
    if not argv or len(argv) > 5:
        raise SystemExit(__doc__)
    scratch = os.path.join(ROOT, "build", "judge-%d" % os.getpid())
    os.makedirs(scratch, exist_ok=True)
    try:
        for n, spec in enumerate(argv):
            m = select(spec)
            start = time.time()
            mutate.check_mutant(m, ROOT, scratch, mutate.COPY_ENTRIES, "make -s -j2 check", 300, n)
            print("%-9s %5.1f s  %s\n          key: %s" % (m.status, time.time() - start, m, m.key))
            if m.status != "survived":
                print("          detail: %s" % m.detail)
            sys.stdout.flush()
    finally:
        shutil.rmtree(scratch, ignore_errors=True)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
