#!/usr/bin/env python3
"""Planted faults of lane f-slice10 (run from the repository root): python3 lanes/f-slice10/faults.py A|B.

Each fault is one edit of src/gfunc.c in a scratch copy of the tree under lanes/f-slice10/build/faults/<name>; the
copy's test_gfunc is built (make -j2) and run under timeout; the result line (or the exit status) is printed. A fault
counts as detected when the test reports a failed check or does not end normally."""
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCR = ROOT / "lanes/f-slice10/build/faults"

FAULTS_A = {
    "A1 the sign of the even root taken for odd n": (
        "if ((n % 2 == 1 && s < 0) || (n % 2 == 0 && sign < 0))",
        "if ((n % 2 == 1 && sign < 0) || (n % 2 == 0 && sign < 0))"),
    "A2 the denominator not tested": (
        "if (!int_root(rn, a, n) || !int_root(rd, fmpq_denref(q), n))",
        "if (!int_root(rn, a, n) || (fmpz_set(rd, fmpq_denref(q)), 0))"),
    "A3 n above the bit length passed to fmpz_root": (
        "    if (n >= (ulong) fmpz_bits(a))\n        return 0;\n",
        ""),
    "A4 the real part computed from the finite part": (
        "    st_real = real_branch(t->inf, x->inf, n, sign, prec);\n    if (exact)",
        "    if (exact) { adf_fball_get_center(q, &x->fin); arb_set_fmpq(t->inf, q->q, prec); }\n"
        "    st_real = real_branch(t->inf, exact ? t->inf : x->inf, n, sign, prec);\n    if (exact)"),
    "A5 the inexact idele of degree 2 returned as OK": (
        "    if (adf_ucoset_is_exact(&x->u))",
        "    if (adf_ucoset_is_exact(&x->u) || n == 2)"),
}

FAULTS_B = {
    "B1 the constants of sin and cos exchanged": (
        "    return series(y, where, x, prec, adf_sball_sin_at, 0);",
        "    return series(y, where, x, prec, adf_sball_sin_at, 1);"),
    "B2 the exact non-zero finite part accepted": (
        "        if (fmpz_is_zero(x->fin.A))",
        "        if (1)"),
    "B3 the place of DOMAIN off by one prime": (
        "    for (p = 3; fmpz_fdiv_ui(A, p) == 0; p = n_nextprime(p, 1))\n        ;\n    return p;",
        "    for (p = 3; fmpz_fdiv_ui(A, p) == 0; p = n_nextprime(p, 1))\n        ;\n    return n_nextprime(p, 1);"),
    "B4 the 2-adic domain taken as 2 Z_2 (c = 1 at 2)": (
        "    if (fmpz_fdiv_ui(A, 4) != 0)",
        "    if (fmpz_fdiv_ui(A, 2) != 0)"),
    "B5 a finite ball read as its centre": (
        "    if (adf_fball_is_exact(&x->fin))",
        "    if (x->fin.backend == ADF_GLOBAL)"),
}


def run(name, old, new):
    d = SCR / name.split()[0]
    if d.exists():
        shutil.rmtree(d)
    d.mkdir(parents=True)
    for sub in ("include", "src", "tests"):
        shutil.copytree(ROOT / sub, d / sub, symlinks=True)
    shutil.copy(ROOT / "Makefile", d / "Makefile")
    src = (d / "src/gfunc.c").read_text()
    assert src.count(old) == 1, name
    (d / "src/gfunc.c").write_text(src.replace(old, new))
    b = subprocess.run(["timeout", "600", "make", "-s", "-j2", "build/test_gfunc"], cwd=d, capture_output=True,
                       text=True)
    if b.returncode != 0:
        return f"did not build: {b.stderr.strip().splitlines()[-1:]}"
    r = subprocess.run(["timeout", "300", "build/test_gfunc"], cwd=d, capture_output=True, text=True, errors="replace")
    lines = [x for x in r.stdout.splitlines() if " tests, " in x]
    return f"exit {r.returncode}; " + (lines[-1] if lines else "no summary line (aborted)")


def main():
    faults = FAULTS_A if sys.argv[1] == "A" else FAULTS_B
    for name, (old, new) in faults.items():
        print(f"{name}: {run(name, old, new)}", flush=True)


if __name__ == "__main__":
    main()
