#!/usr/bin/env python3
"""Do the tests of proto/test_real_isolation.py bite? Each change below is made in a copy of
proto/real_isolation.py (the file itself is not touched), and the tests are run against the copy.
A change is caught if the run fails. Run from the repository root:
    timeout 170 python3 lanes/d-realroots/bite.py
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CHANGES = [
    ("the root at a midpoint is not looked for", "        if right[0] == 0:", "        if False:"),
    ("Descartes' test without the shift", "    return var(shift1(list(reversed(q))))",
     "    return var(list(reversed(q)))"),
    ("the root bound one bit too small", "    return hi\n\n\n# ----", "    return hi - 1\n\n\n# ----"),
    ("balls may touch (floor not strict)", "value(c, k) > floor", "value(c, k) >= floor"),
    ("one bit of accuracy less in the refinement",
     "if lo_clean and hi_clean and apart and accuracy(c, k) >= need:",
     "if lo_clean and hi_clean and apart and accuracy(c, k) >= need - 1:"),
    ("QIR accepts without the second sign", "                if s2 == -s_lo:\n", "                if True:\n"),
    ("the mirrored cells are shifted by one", "cells += [(-m - 1, k) for m, k in c]",
     "cells += [(-m, k) for m, k in c]"),
    ("a cell with v >= 2 is taken as isolating", "        if v == 1:", "        if v >= 1:"),
    ("a cell with v = 1 is dropped", "        if v == 0:\n            continue",
     "        if v <= 1 and k < 0:\n            continue"),
    ("the final tests are removed and the bound is small",
     "    if not ok:\n        return NOT_DETERMINED, 0, [], stats", "    pass"),
    ("bisection keeps the wrong half", "        if s == -s_lo:\n            c, k, hi_clean = 2 * c, k - 1, True",
     "        if s == s_lo:\n            c, k, hi_clean = 2 * c, k - 1, True"),
]


def main():
    src = open(os.path.join(ROOT, "proto", "real_isolation.py")).read()
    caught = 0
    for name, old, new in CHANGES:
        if src.count(old) != 1:
            print(f"NOT APPLIED ({src.count(old)} places): {name}")
            continue
        d = tempfile.mkdtemp()
        try:
            for fn in ("test_real_isolation.py", "solvers_checks.py"):
                shutil.copy(os.path.join(ROOT, "proto", fn), d)
            open(os.path.join(d, "real_isolation.py"), "w").write(src.replace(old, new))
            try:
                r = subprocess.run([sys.executable, "-m", "unittest", "test_real_isolation.py"], cwd=d,
                                   capture_output=True, text=True, timeout=60)
                failed = r.returncode != 0
                m = re.findall(r"^(?:FAIL|ERROR): (\w+)", r.stderr, re.M)
                what = ", ".join(sorted(set(m)))
            except subprocess.TimeoutExpired:
                failed, what = True, "no end in 60 s"
            caught += failed
            print(f"{'caught' if failed else 'SURVIVED'}: {name} [{what}]")
        finally:
            shutil.rmtree(d)
    print(f"{caught} of {len(CHANGES)} changes caught")
    return 0 if caught == len(CHANGES) else 1


if __name__ == "__main__":
    sys.exit(main())
