#!/usr/bin/env python3
"""lanes/s2-slice2/bite.py: item 5 of the brief of lane s2-slice2: the tests bite. Six changes of src/roots.c, one
at a time, each in a scratch copy of the tree under build/bite/ (Makefile, include, src, tests); for each the copy
is built and tests/test_roots_padic.c is run under timeout. A change is caught when the test program exits with a
status other than 0 (a failed check, or an abort of the library). No `make mutate`.

Run from the repository root: python3 lanes/s2-slice2/bite.py >> lanes/s2-slice2/redgreen.log"""
import os
import shutil
import subprocess
import sys

CHANGES = [
    ("the condition g'(b) != 0 of a simple root removed",
     "            if (nmod_poly_evaluate_nmod(dm, b) != 0)\n",
     "            if (1)\n"),
    ("j > w - 2e changed to >=",
     "    if (!(j > t) && !add_checked(&j, t, 1))\n",
     "    if (!(j >= t) && !add_checked(&j, t, 1))\n"),
    ("k > s changed to >= (K = the least K >= prec_p with K > s)",
     "    Kv = prec_p > sv ? prec_p : s1;\n",
     "    Kv = prec_p >= sv ? prec_p : s1;\n"),
    ("one residue left out of the evaluation (the loop runs to p - 1)",
     "        for (b = 0; b < pu && st == ADF_OK; b++)\n",
     "        for (b = 0; b < pu - 1 && st == ADF_OK; b++)\n"),
    ("a class dropped instead of kept as unresolved at the depth limit",
     "                item_vec_push(&classes, c, e1, 0);\n",
     "                (void) 0;\n"),
    ("complete set to 1 with a class unresolved",
     "        T->complete = T->nu == 0;               /* P3.5(3) */\n",
     "        T->complete = 1;\n"),
]


def main():
    root = os.getcwd()
    src = open(os.path.join(root, "src/roots.c")).read()
    work = os.path.join(root, "build", "bite")
    caught = 0
    for name, old, new in CHANGES:
        assert src.count(old) == 1, name
        shutil.rmtree(work, ignore_errors=True)
        os.makedirs(work)
        for d in ("include", "src", "tests"):
            shutil.copytree(os.path.join(root, d), os.path.join(work, d))
        shutil.copy(os.path.join(root, "Makefile"), work)
        with open(os.path.join(work, "src/roots.c"), "w") as fh:
            fh.write(src.replace(old, new))
        b = subprocess.run(["make", "-s", "-j2", "build/test_roots_padic"], cwd=work, capture_output=True,
                           text=True)
        if b.returncode != 0:
            print(f"-- {name}: BUILD FAILED\n{b.stderr[-800:]}")
            continue
        r = subprocess.run(["timeout", "300", "./build/test_roots_padic"], cwd=work, capture_output=True, text=True)
        out = r.stdout.splitlines()
        failed = [x for x in out if x.startswith("FAIL ") and "(" in x and "failed checks)" in x]
        tail = [x for x in out if "tests," in x and "failed checks" in x]
        err = (r.stderr.strip().splitlines() or [""])[-1]
        verdict = "CAUGHT" if r.returncode != 0 else "NOT CAUGHT"
        caught += r.returncode != 0
        print(f"-- {name}: {verdict}, exit {r.returncode}")
        for x in failed:
            print(f"   {x}")
        for x in tail:
            print(f"   {x}")
        if r.returncode != 0 and not tail:
            print(f"   the program ended early; last line of stderr: {err[:300]}")
            print(f"   last line of stdout: {out[-1][:300] if out else ''}")
    shutil.rmtree(work, ignore_errors=True)
    print(f"-- {caught} of {len(CHANGES)} changes caught")
    return 0 if caught == len(CHANGES) else 1


if __name__ == "__main__":
    sys.exit(main())
