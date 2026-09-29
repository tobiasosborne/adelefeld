"""lanes/f-slice4/faults.py: show that tests/test_lfunc.c bites (brief, item 4).

    python3 -B lanes/f-slice4/faults.py        (from the repository root, after `make -j2 all`)

For each fault: a copy of src/lfunc.c under build/faults/<k>/ with one textual change, compiled into a copy of
build/libadelefeld.a (the object lfunc.o replaced), tests/test_lfunc.c linked against it and run under timeout.
A fault is caught when the test program exits non-zero; the number of failed checks and the tests that failed are
printed. Nothing outside build/ is written."""

import os
import re
import shutil
import subprocess
import sys

FAULTS = [
    ("radius of Log taken from the input precision, without the loss or gain of m digits",
     "        r = x->N - x->v;\n", "        r = x->N;\n"),
    ("exp: one term too few in the truncation (Proposition 7)",
     "    slong L = count_exp(p, K, w) - 1, D, W, k;", "    slong L = count_exp(p, K, w) - 2, D, W, k;"),
    ("log: one term too few in the truncation (Proposition 7b), both branches",
     "    return lo - 1;\n}", "    return lo - 2 < 0 ? 0 : lo - 2;\n}"),
    ("Log at odd p: the sum of log(a^(p-1)) not divided by p - 1",
     "        fmpz_mul(S, S, t);\n", "        (void) t;\n"),
    ("exp: the domain p Z_p at p = 2 as well (c = 1 everywhere)",
     "        D->N = x->p == 2 ? 2 : 1;", "        D->N = 1;"),
    ("Log at p = 2, r = 1: the exponent r = 1 instead of the image 4 Z_2",
     "        E = r >= c ? r : 2;", "        E = r;"),
    ("log: the domain 1 + p^0 Z_p = Z_p instead of 1 + p Z_p",
     "        D->N = 1;                              /* 1 + p Z_p: centre 1, exponent 1 */",
     "        D->N = 0;"),
]

CC = os.environ.get("CC", "cc")
CFLAGS = ["-std=c11", "-O2", "-g", "-Wall", "-Wextra", "-Wpedantic", "-Iinclude"]


def run(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


def main():
    src = open("src/lfunc.c").read()
    if not os.path.exists("build/libadelefeld.a") or not os.path.exists("build/support/jsonl.o"):
        print("run `make -j2 all build/test_lfunc` first")
        return 2
    caught = 0
    for k, (what, old, new) in enumerate(FAULTS, 1):
        d = "build/faults/%d" % k
        os.makedirs(d, exist_ok=True)
        assert src.count(old) == 1, (k, old)
        open(d + "/lfunc.c", "w").write(src.replace(old, new))
        r = run([CC] + CFLAGS + ["-Isrc", "-c", d + "/lfunc.c", "-o", d + "/lfunc.o"])
        if r.returncode:
            print(k, "compile error", r.stderr[:400])
            return 1
        shutil.copy("build/libadelefeld.a", d + "/lib.a")
        r = run(["ar", "rcs", d + "/lib.a", d + "/lfunc.o"])
        assert r.returncode == 0, r.stderr
        r = run([CC] + CFLAGS + ["-Itests", "tests/test_lfunc.c", "build/support/golden.o", "build/support/jsonl.o",
                                 d + "/lib.a", "-lflint", "-lgmp", "-lm", "-o", d + "/test_lfunc"])
        assert r.returncode == 0, r.stderr[:400]
        r = run(["timeout", "300", d + "/test_lfunc"])
        failed_tests = sorted(set(re.findall(r"^FAIL (\w+) \(", r.stdout, re.M)))
        summary = [ln for ln in r.stdout.splitlines() if " tests, " in ln and "checks" in ln]
        ok = r.returncode != 0
        caught += ok
        print("fault %d: %s\n  exit %d: %s\n  failed tests: %s" % (k, what, r.returncode,
              summary[-1] if summary else "(no summary)", ", ".join(failed_tests)))
    print("caught %d of %d faults" % (caught, len(FAULTS)))
    return 0 if caught == len(FAULTS) else 1


if __name__ == "__main__":
    sys.exit(main())
