#!/usr/bin/env python3
"""lanes/s2-slice3/bite.py: item 5 of the brief of lane s2-slice3: the tests bite. Six changes of src/roots.c, one
at a time, each in a scratch copy of the tree under build/bite3/ (Makefile, include, src, tests); for each the copy
is built and tests/test_roots_real.c is run under timeout 120, with an address-space limit of 4 GiB (a change that
hands a polynomial with a multiple root to arb_fmpz_poly_complex_roots makes FLINT raise its precision without end).
A change is caught when the test program exits with a status other than 0 (a failed check, an abort, or the
timeout). No `make mutate`.

Run from the repository root:  python3 lanes/s2-slice3/bite.py [index ...] >> lanes/s2-slice3/redgreen.log
(without an index every change is run; with indices, those only, so that one call stays below 3 minutes)."""
import os
import resource
import shutil
import subprocess
import sys

CHANGES = [
    ("the squarefree part not taken (f used for g in adf_roots_real)",
     "    normalise(g, &reduced, f);\n    d = fmpz_poly_degree(g);\n",
     "    fmpz_poly_set(g, f);\n    reduced = 0;\n    d = fmpz_poly_degree(g);\n"),
    ("the sign test at one end point accepting a zero (g(lo) g(hi) <= 0)",
     "    return sl * sh < 0;\n",
     "    return sl * sh <= 0;\n"),
    ("the disjointness test removed",
     "            ok = arf_cmp(hi_prev, lo) < 0;      /* hi_(i-1) < lo_i */\n",
     "            ok = 1;\n"),
    ("the comparison of the two counts removed",
     "    if (m != count)\n        return ADF_NOT_DETERMINED;"
     "              /* step 6: the number of intervals is n */\n",
     "    (void) count;\n"),
    ("the accuracy measured before the widening instead of after",
     "        ok = real_accurate(out + i, prec);\n",
     "        ok = real_accurate(in + i, prec);\n"),
    ("verify_complete reading the count from the list",
     "    c = fmpz_poly_degree(h) > 0 ? fmpz_poly_num_real_roots(h) : 0;\n",
     "    c = L->count;\n"),
]


def limit():
    resource.setrlimit(resource.RLIMIT_AS, (4 << 30, 4 << 30))


def main():
    root = os.getcwd()
    src = open(os.path.join(root, "src/roots.c")).read()
    work = os.path.join(root, "build", "bite3")
    which = [int(a) for a in sys.argv[1:]] or list(range(len(CHANGES)))
    caught = 0
    for k in which:
        name, old, new = CHANGES[k]
        assert src.count(old) == 1, name
        shutil.rmtree(work, ignore_errors=True)
        os.makedirs(work)
        for d in ("include", "src", "tests"):
            shutil.copytree(os.path.join(root, d), os.path.join(work, d))
        shutil.copy(os.path.join(root, "Makefile"), work)
        with open(os.path.join(work, "src/roots.c"), "w") as fh:
            fh.write(src.replace(old, new))
        b = subprocess.run(["make", "-s", "-j2", "build/test_roots_real"], cwd=work, capture_output=True, text=True)
        if b.returncode != 0:
            print(f"-- {k}. {name}: BUILD FAILED\n{b.stderr[-800:]}")
            continue
        r = subprocess.run(["timeout", "120", "./build/test_roots_real"], cwd=work, capture_output=True, text=True,
                           preexec_fn=limit)
        out = r.stdout.splitlines()
        failed = [x for x in out if x.startswith("FAIL ") and "failed checks)" in x]
        tail = [x for x in out if "tests," in x and "failed checks" in x]
        err = (r.stderr.strip().splitlines() or [""])[-1]
        verdict = "CAUGHT" if r.returncode != 0 else "NOT CAUGHT"
        caught += r.returncode != 0
        print(f"-- {k}. {name}: {verdict}, exit {r.returncode}" + (" (the timeout)" if r.returncode == 124 else ""))
        for x in failed:
            print(f"   {x}")
        for x in tail:
            print(f"   {x}")
        if r.returncode != 0 and not tail:
            print(f"   the program ended early; last line of stderr: {err[:300]}")
            print(f"   last line of stdout: {out[-1][:300] if out else ''}")
    shutil.rmtree(work, ignore_errors=True)
    print(f"-- {caught} of {len(which)} changes caught")
    return 0 if caught == len(which) else 1


if __name__ == "__main__":
    sys.exit(main())
