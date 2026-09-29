#!/usr/bin/env python3
"""Item 4 of the brief: show that the tests bite. One change at a time in a scratch copy under build/scratch
(made by the commands in redgreen.log), never in src/. Run from the repository root:
python3 lanes/s1-slice2/mutate.py. Prints, for each change, the last line of the test run."""
import shutil
import subprocess

SRC = "src/linsolve.c"
SCR = "build/scratch/src/linsolve.c"

MUTANTS = [
    ("lcm replaced by the product of the H_i",
     "fmpz_lcm(N, N, h + i);", "fmpz_mul(N, N, h + i);"),
    ("the denominator d of a ball ignored",
     "            fmpz_mul(q, q, d + i);\n", ""),
    ("rho_j without the gcd with N",
     "    fmpz_init_set(rho, sol->N);\n",
     "    fmpz_init(rho);\n"),
    ("contains without the reduction by G",
     "    greedy_reduce(v, NULL, sol->G, 0, sol->N);\n    in = _fmpz_vec_is_zero",
     "    in = _fmpz_vec_is_zero"),
    ("kernel_order with g_i in place of N/g_i",
     "fmpz_divexact(q, sol->N, G->rows[i] + j);", "fmpz_set(q, G->rows[i] + j);"),
]


def run():
    r = subprocess.run("make -s -j2 build/test_linsolve_rest 2>&1 | grep -E 'error' ; "
                       "./build/test_linsolve_rest 2>&1 | grep -E '^[0-9]+ tests|^FAIL [a-z_0-9]+ \\(' ",
                       shell=True, cwd="build/scratch", capture_output=True, text=True)
    return r.stdout.strip()


orig = open(SRC).read()
for name, old, new in MUTANTS:
    assert orig.count(old) == 1, name
    src = orig.replace(old, new)
    if name.startswith("rho_j"):
        # after the gcd loop, rho = 0 (no row) stands for N: only the initial value changed
        old2 = "    status = adf_fball_set_fmpz3(x, fmpz_mat_entry(sol->x0, j, 0), rho, one);"
        assert src.count(old2) == 1
        src = src.replace(old2, "    if (fmpz_is_zero(rho))\n        fmpz_set(rho, sol->N);\n" + old2)
    open(SCR, "w").write(src)
    print("MUTANT:", name)
    print(run())
    print()
shutil.copy(SRC, SCR)
print("scratch restored:")
print(run())
