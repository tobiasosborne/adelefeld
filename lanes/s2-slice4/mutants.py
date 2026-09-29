#!/usr/bin/env python3
"""lanes/s2-slice4/mutants.py: item 5 of the brief. Four hand-made mutants of src/roots.c, each built in a scratch
copy under build/mut-s2-slice4/ and run against tests/test_roots_bigp.c and tests/test_roots_padic.c under timeout
and a limit of 4 GB of virtual memory (the mutant without the reduction modulo h asks for X^p with p of 32 bits
and more). A mutant is killed when a test program fails (a failed check, an abort, a signal or the timeout).

    python3 lanes/s2-slice4/mutants.py        (from the repository root)
"""
import os
import shutil
import subprocess
import sys

ROOT = os.getcwd()
MUT = os.path.join(ROOT, "build", "mut-s2-slice4")

MUTANTS = [
    ("the test of each candidate by evaluation removed",
     "        if (cand[i] >= h->mod.n || nmod_poly_evaluate_nmod(h, cand[i]) != 0)\n",
     "        if (cand[i] >= h->mod.n)\n"),
    ("the comparison with deg d removed",
     "    if (m != degd)\n        return 0;\n",
     "    (void) degd;\n"),
    ("X^p replaced by X^(p-1)",
     "    nmod_poly_powmod_ui_binexp(t, x, h->mod.n, h);\n",
     "    nmod_poly_powmod_ui_binexp(t, x, h->mod.n - 1, h);\n"),
    ("the reduction modulo h dropped from the powering",
     "    nmod_poly_powmod_ui_binexp(t, x, h->mod.n, h);\n",
     "    nmod_poly_pow(t, x, h->mod.n);\n"),
]


def run(cmd, cwd, timeout):
    try:
        p = subprocess.run(cmd, cwd=cwd, shell=True, capture_output=True, text=True, timeout=timeout)
        return p.returncode, p.stdout + p.stderr
    except subprocess.TimeoutExpired:
        return "timeout", ""


def main():
    if os.path.exists(MUT):
        shutil.rmtree(MUT)
    os.makedirs(MUT)
    for d in ("include", "src", "tests"):
        shutil.copytree(os.path.join(ROOT, d), os.path.join(MUT, d))
    shutil.copy(os.path.join(ROOT, "Makefile"), MUT)
    orig = open(os.path.join(ROOT, "src", "roots.c")).read()
    killed = 0
    for name, a, b in MUTANTS:
        assert orig.count(a) == 1, name
        open(os.path.join(MUT, "src", "roots.c"), "w").write(orig.replace(a, b))
        rc, out = run("make -s -j2 build/test_roots_bigp build/test_roots_padic", MUT, 600)
        if rc != 0:
            print(f"{name}: build failed\n{out[-2000:]}")
            return 1
        res = []
        for t in ("test_roots_bigp", "test_roots_padic"):
            rc, out = run(f"ulimit -v 4000000; timeout 120 ./build/{t}", MUT, 150)
            last = [l for l in out.splitlines() if "tests," in l]
            fails = [l for l in out.splitlines() if l.startswith("FAIL ") and "(" in l and ":" not in l.split()[1]]
            tail = " | ".join(l for l in out.splitlines()[-3:])
            res.append((t, rc, last[-1] if last else "(no summary line; last lines: " + tail[:600] + ")", fails))
        dead = any(r[1] != 0 for r in res)
        killed += dead
        print(f"mutant: {name}: {'KILLED' if dead else 'SURVIVED'}")
        for t, rc, last, fails in res:
            print(f"    {t}: exit {rc}; {last}")
            for f in fails:
                print(f"        {f}")
    print(f"{killed} of {len(MUTANTS)} mutants killed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
