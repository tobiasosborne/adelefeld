#!/usr/bin/env python3
"""Item 5 of the brief of lane s1-slice1: do the tests bite? One change at a time in src/linsolve.c, in a scratch
copy under build/mutants/<name>/ (Makefile, include, src, tests), then build/test_linsolve is built and run there
with a limit of memory (ulimit -v 2 GB, docs/workflow.md rule 9) and a timeout of 120 s. Prints, for each change,
the exit status and the tests that failed.

Run from the repository root: python3 lanes/s1-slice1/mutants.py [name ...]

The changes:
  K1         the test of (K1) removed from adf_linsol_verify;
  K2 .. K5   the test of that condition disabled ("if (0 && ...)"; removing the line would leave an unused
             static function, which -Werror refuses);
  K6, K7     the test of that condition replaced by "ok = 1 || ...";
  H-first    Algorithm H: the pending pair ((N/g) v, j + 1) of the first case (a new pivot) not added;
  H-third    Algorithm H: the pending pair ((N/g) w', j + 1) of the third case (a pivot replaced) not added;
  x0-mod     step 3 of Algorithm L: the reduction of x0 modulo N removed.
  The last three also with the checker of step 5 and the test for a missing y disabled in adf_linsolve_mod
  (suffix "-nocheck"), to show that the tests themselves, not only the internal check (which aborts), find the
  change."""
import os
import shutil
import subprocess
import sys

ROOT = os.getcwd()
SRC = os.path.join(ROOT, "src", "linsolve.c")

K = {
    "K1": ("    if (!is_echelon(sol->E, N) || !is_echelon(sol->G, N))                                 /* (K1) */\n"
           "        goto done;\n", ""),
    "K2": ("if (!k2_holds(Ar, sol->E, sol->V, N))", "if (0 && !k2_holds(Ar, sol->E, sol->V, N))"),
    "K3": ("if (!k3_holds(Ar, sol->G, N))", "if (0 && !k3_holds(Ar, sol->G, N))"),
    "K4": ("if (!k4_holds(sol->E, sol->G, N, sol->c))", "if (0 && !k4_holds(sol->E, sol->G, N, sol->c))"),
    "K5": ("if (!k5_holds(sol->G, N))", "if (0 && !k5_holds(sol->G, N))"),
    "K6": ("ok = k6_holds(Ar, sol->x0, br, N);", "ok = 1 || k6_holds(Ar, sol->x0, br, N);"),
    "K7": ("ok = k7_holds(Ar, sol->y, br, N);", "ok = 1 || k7_holds(Ar, sol->y, br, N);"),
    "H-first": ("                pending_push(&P, v, j + 1);\n                break;\n",
                "                _fmpz_vec_clear(v, n);\n                break;\n"),
    "H-third": ("                        vec_scalar_mod(p, Ng, n, N); /* the pending pair ((N/g) w' mod N, j + 1) */\n"
                "                        pending_push(&P, p, j + 1);\n",
                "                        vec_scalar_mod(p, Ng, n, N); /* the pending pair ((N/g) w' mod N, j + 1) */\n"
                "                        _fmpz_vec_clear(p, n);\n"),
    "x0-mod": ("                fmpz_addmul(x, q + i, fmpz_mat_entry(res->V, i, j));\n            fmpz_mod(x, x, N);\n",
               "                fmpz_addmul(x, q + i, fmpz_mat_entry(res->V, i, j));\n"),
}
NOCHECK = ("    if (!found || !adf_linsol_verify(res, A, b, N))\n", "    if (0 && (!found || !adf_linsol_verify(res, A, b, N)))\n")

CHANGES = [(k, [K[k]]) for k in ("K1", "K2", "K3", "K4", "K5", "K6", "K7", "H-first", "H-third", "x0-mod")]
CHANGES += [(k + "-nocheck", [K[k], NOCHECK]) for k in ("H-first", "H-third", "x0-mod")]


def run(name, edits):
    text = open(SRC).read()
    for old, new in edits:
        if text.count(old) != 1:
            return f"MUTANT {name}: the text to change was found {text.count(old)} times; not run"
        text = text.replace(old, new)
    d = os.path.join(ROOT, "build", "mutants", name)
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)
    for sub in ("include", "src", "tests"):
        shutil.copytree(os.path.join(ROOT, sub), os.path.join(d, sub))
    shutil.copy(os.path.join(ROOT, "Makefile"), d)
    open(os.path.join(d, "src", "linsolve.c"), "w").write(text)
    b = subprocess.run(["make", "-s", "-j2", "build/test_linsolve"], cwd=d, capture_output=True, text=True)
    if b.returncode != 0:
        return f"MUTANT {name}: the build failed\n{b.stderr[-600:]}"
    t = subprocess.run(["bash", "-c", "ulimit -v 2000000; timeout 120 ./build/test_linsolve"], cwd=d,
                       capture_output=True, text=True)
    out = t.stdout + t.stderr
    failed = [ln for ln in out.splitlines() if ln.startswith("FAIL ") and "(tests/" in ln]
    last = [ln for ln in out.splitlines() if "tests," in ln or "abort" in ln.lower() or "defect" in ln]
    accepted_false = [ln for ln in out.splitlines() if ln.startswith("changed certificates") and
                      not ln.endswith(" 0 accepted and false")]
    lines = [f"MUTANT {name}: exit status {t.returncode}"]
    lines += ["   " + ln for ln in failed]
    lines += ["   " + ln for ln in accepted_false]
    lines += ["   " + ln for ln in last[-2:]]
    shutil.rmtree(d, ignore_errors=True)
    return "\n".join(lines)


def main():
    names = sys.argv[1:]
    for name, edits in CHANGES:
        if names and name not in names:
            continue
        print(run(name, edits), flush=True)


if __name__ == "__main__":
    main()
