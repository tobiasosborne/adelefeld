"""The tests bite: plants one fault at a time in a scratch copy of src/lball.c or src/lball_decomp.c under
build/bite3/, links it with build/libadelefeld.a (the mutated object comes first and defines every symbol of its file,
so the archive member of the original is not pulled) and runs the test program of that file.  A fault is caught if the
test program ends with a non-zero status (assertion, crash or timeout).  Run from the repository root after
`make build/libadelefeld.a`:  python3 -B lanes/f-slice3/bite.py

This is not a mutation run (the brief excludes it): the faults are chosen by hand.
"""

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / "build" / "bite3"
CC = ["cc", "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror", "-Iinclude", "-Isrc"]
LIBS = ["-lflint", "-lgmp", "-lm"]

FAULTS = [
    # (name, source file, old text, new text, test file)
    ("teichmuller: Newton step with the wrong sign", "src/lball_decomp.c",
     "        fmpz_sub(w, w, fw);\n", "        fmpz_add(w, w, fw);\n", "tests/test_lball_decomp.c"),
    ("split: the sign at p = 2 taken from the wrong residue", "src/lball_decomp.c",
     "int neg = (p == 2) ? (r == 3) : (r == p - 1);", "int neg = (p == 2) ? (r == 1) : (r == p - 1);",
     "tests/test_lball_decomp.c"),
    ("split: p = 2 with relative precision 1 is not refused", "src/lball_decomp.c",
     "if (!exact && p == 2 && x->N - x->v == 1)", "if (0 && !exact && p == 2 && x->N - x->v == 1)",
     "tests/test_lball_decomp.c"),
    ("split: the principal unit of a ball gets precision prec, not the relative precision", "src/lball_decomp.c",
     "slong nu = exact ? n : k; ", "slong nu = n; ", "tests/test_lball_decomp.c"),
    ("teichmuller: the residue p - 1 is not recognised as the exact -1", "src/lball_decomp.c",
     "rational = (p == 2 || r == 1 || r == p - 1);", "rational = (p == 2 || r == 1);", "tests/test_lball_decomp.c"),
    ("frac: a ball with N = -1 is called determined", "src/lball_decomp.c",
     "if (!x->exact && x->N < 0)\n        return ADF_NOT_DETERMINED;", "if (!x->exact && x->N < -1)\n        return ADF_NOT_DETERMINED;",
     "tests/test_lball_decomp.c"),
    ("unit_mod: k = N - v + 1 is accepted", "src/lball_decomp.c",
     "if (!x->exact && (ulong) k > (ulong) x->N - (ulong) x->v)", "if (!x->exact && (ulong) k > (ulong) x->N - (ulong) x->v + 1)",
     "tests/test_lball_decomp.c"),
    ("pow: the factor v_p(k) is dropped", "src/lball.c",
     "vpn = vp_ulong(n, p);", "vpn = vp_ulong(n, p) * 0;", "tests/test_lball_decomp.c"),
    ("pow: the extra digit of the squares at p = 2 is dropped", "src/lball.c",
     "((p == 2 && rel == 1 && n % 2 == 0) ? 1 : 0)", "0", "tests/test_lball_decomp.c"),
    ("pow: the negative power of a ball uses the centre, not its inverse", "src/lball.c",
     "        if (k < 0)\n        {\n            int ok = fmpz_invmod(c, c, P);", "        if (0)\n        {\n            int ok = fmpz_invmod(c, c, P);",
     "tests/test_lball_decomp.c"),
    ("set_fball: the divisibility test always answers yes", "src/lball.c",
     "            r = fmpz_divisible(H, pt);\n", "            r = 1;\n", "tests/test_lball.c"),
    ("set_fball: the small-centre shortcut (ii) is not recognised", "src/lball.c",
     "        if ((ulong) vA + need <= vHmax)\n            goto out;", "        if ((ulong) vA + need <= vHmax && need == 0)\n            goto out;", "tests/test_lball.c"),
]


def run(cmd, timeout):
    try:
        r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, timeout=timeout)
        return r.returncode, r.stdout + r.stderr
    except subprocess.TimeoutExpired:
        return 124, "timeout"


def main():
    WORK.mkdir(parents=True, exist_ok=True)
    caught = 0
    for i, (name, src, old, new, test) in enumerate(FAULTS, 1):
        text = (ROOT / src).read_text()
        if text.count(old) != 1:
            print(f"fault {i}: old text occurs {text.count(old)} times: {name}")
            sys.exit(2)
        mutated = WORK / f"m{i}.c"
        mutated.write_text(text.replace(old, new))
        obj = WORK / f"m{i}.o"
        code, out = run(CC + ["-c", str(mutated), "-o", str(obj)], 120)
        if code != 0:
            print(f"fault {i}: does not compile: {name}\n{out[:400]}")
            sys.exit(2)
        exe = WORK / f"t{i}"
        code, out = run(CC + ["-Itests", str(ROOT / test), str(obj), "tests/support/jsonl.c", "build/libadelefeld.a", "-o", str(exe)]
                        + LIBS, 200)
        if code != 0:
            print(f"fault {i}: does not link: {name}\n{out[:600]}")
            sys.exit(2)
        code, out = run([str(exe)], 200)
        fails = [l for l in out.splitlines() if l.startswith("FAIL ") and "(" in l and "failed checks" in l]
        verdict = "CAUGHT" if code != 0 else "SURVIVED"
        caught += code != 0
        print(f"fault {i:2d} {verdict} (exit {code}; failing tests: {len(fails)}): {name}", flush=True)
        exe.unlink()
        obj.unlink()
        mutated.unlink()
    print(f"caught {caught} of {len(FAULTS)}")


if __name__ == "__main__":
    main()
