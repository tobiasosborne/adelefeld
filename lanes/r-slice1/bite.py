#!/usr/bin/env python3
"""lanes/r-slice1/bite.py: do the tests bite? Each fault below is put into a scratch copy of src/roots_real.c
under build/bite/<name>/, the library is built there from the other sources of src/ unchanged, and the two test
programs of the real roots are built against it and run (each under a timeout). A fault is caught when a test
program fails or does not end in time. Run from the repository root:

    timeout 900 python3 lanes/r-slice1/bite.py

Not a mutation run: six faults chosen by hand (brief item 6)."""
import os
import shutil
import subprocess
import sys

FAULTS = [
    ("var_accepts_2",
     "the sign-variation test accepts v = 2 as one root (a cell with two roots)",
     "            if (s == 1)\n                items_push(it, c, k - 1, 0);",
     "            if (s == 1 || s == 2)\n                items_push(it, c, k - 1, 0);"),
    ("wrong_shift",
     "the right half is left(X + 2) instead of left(X + 1)",
     "        fmpz_poly_set(right, left);\n        _fmpz_poly_taylor_shift(right->coeffs, one, len);",
     "        fmpz_poly_set(right, left);\n        fmpz_add_ui(one, one, 1);\n"
     "        _fmpz_poly_taylor_shift(right->coeffs, one, len);\n        fmpz_sub_ui(one, one, 1);"),
    ("drop_exact_root",
     "a root met at a midpoint of the isolation is divided out but not recorded",
     "            items_push(it, c, k - 1, 1);\n            fmpz_sub_ui(c, c, 1);",
     "            fmpz_sub_ui(c, c, 1);"),
    ("gallop_sign",
     "step G takes v0 = sign g(p) without the sign of g' when p is a root",
     "    if (v0 == 0)\n        v0 = sigma * sign_2exp(dg, pc, *k);",
     "    if (v0 == 0)\n        v0 = sigma;"),
    ("qir_round",
     "step Q keeps the cell (m', e) when the sign at m' equals s_lo without testing m' + 1",
     "        else if (s2 == -s_lo)\n            r = 1;                              /* the cell (m', e) */",
     "        else\n            r = 1;                              /* the cell (m', e) */"),
]

FAULTS.append(
    ("moving_floor",
     "the floor of an item is the right end of the previous REFINED ball, not of the previous isolated item",
     "                    item_right(floor, &it, i - 1);",
     "                    arb_get_ubound_arf(floor, cand + i - 1, ARF_PREC_EXACT);"))

TESTS = ["test_roots_real_isolate", "test_roots_real"]
CFLAGS = ["-std=c11", "-O2", "-g", "-Iinclude"]


def run(cmd, timeout):
    try:
        p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
        return p.returncode, p.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return "timeout", ""


def main():
    src = open("src/roots_real.c").read()
    caught = 0
    for name, what, old, new in FAULTS:
        if src.count(old) != 1:
            print(f"{name}: the text to change is not found exactly once")
            return 2
        d = os.path.join("build", "bite", name)
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(d)
        with open(os.path.join(d, "roots_real.c"), "w") as fh:
            fh.write(src.replace(old, new))
        objs = []
        for f in sorted(os.listdir("src")):
            if not f.endswith(".c"):
                continue
            path = os.path.join(d, "roots_real.c") if f == "roots_real.c" else os.path.join("src", f)
            o = os.path.join(d, f[:-2] + ".o")
            rc, out = run(["cc"] + CFLAGS + ["-c", path, "-o", o], 300)
            if rc != 0:
                print(f"{name}: {f} did not compile\n{out}")
                return 2
            objs.append(o)
        lib = os.path.join(d, "lib.a")
        run(["ar", "rcs", lib] + objs, 60)
        results = []
        for t in TESTS:
            exe = os.path.join(d, t)
            cmd = ["cc"] + CFLAGS + ["-Itests", os.path.join("tests", t + ".c"), "build/support/golden.o",
                                     "build/support/jsonl.o", lib, "-lflint", "-lgmp", "-lm", "-o", exe]
            rc, out = run(cmd, 300)
            if rc != 0:
                print(f"{name}: {t} did not build\n{out}")
                return 2
            rc, out = run([exe], 150)
            last = [ln for ln in out.splitlines() if " tests, " in ln]
            failed = [ln.split(" (")[0] for ln in out.splitlines()
                      if ln.startswith("FAIL ") and " (" in ln and "check failed" not in ln]
            results.append((t, rc, last[-1] if last else "(no summary)", failed))
        hit = any(rc != 0 for _, rc, _, _ in results)
        caught += hit
        print(f"{name}: {what}: {'CAUGHT' if hit else 'NOT caught'}")
        for t, rc, last, failed in results:
            print(f"    {t}: exit {rc}; {last}; failed tests: {', '.join(x[5:] for x in failed) or '-'}")
    print(f"bite: {caught} of {len(FAULTS)} faults caught")
    return 0 if caught == len(FAULTS) else 1


if __name__ == "__main__":
    sys.exit(main())
