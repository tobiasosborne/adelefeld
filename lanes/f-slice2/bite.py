"""Shows that tests/test_sball.c and tests/test_rfunc.c bite: plants faults, one at a time, in a scratch copy of
src/sball.c or src/rfunc.c under build/bite2/ and runs the test of that file against each. Run from the repository
root after `make -j2 build/test_sball build/test_rfunc`:
    python3 -B lanes/f-slice2/bite.py
Each fault is (file, name, old text, new text); the old text must occur exactly once. The real sources are not
changed. This is not a mutation run (the brief excludes it): the faults are chosen by hand."""
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = {"sball": (ROOT / "src" / "sball.c").read_text(), "rfunc": (ROOT / "src" / "rfunc.c").read_text()}
TEST = {"sball": "tests/test_sball.c", "rfunc": "tests/test_rfunc.c"}
OUT = ROOT / "build" / "bite2"
OUT.mkdir(parents=True, exist_ok=True)

FAULTS = [
    ("sball", "project does not sort the places",
     "    qsort(ps, (size_t) n, sizeof(adf_place_t), place_cmp_ptr);\n", "    (void) place_cmp_ptr;\n"),
    ("sball", "project drops the real place",
     "    has_inf = n > 0 && adf_place_is_archimedean(ps[0]);", "    has_inf = 0; first_prime = 0;"),
    ("sball", "binary pairs component i of x with component i + 1 of y",
     "st = adf_lball_add(&t.loc[i], &x->loc[i], &y->loc[i]);",
     "st = adf_lball_add(&t.loc[i], &x->loc[i], &y->loc[(i + 1) % x->len]);"),
    ("sball", "places_differ reports the larger prime",
     "x->loc[i].p < y->loc[j].p ? x->loc[i].p : y->loc[j].p", "x->loc[i].p > y->loc[j].p ? x->loc[i].p : y->loc[j].p"),
    ("sball", "contains compares the real balls the wrong way",
     "!arb_contains(acb_realref(y->inf), acb_realref(x->inf)))\n        return 0;\n    if (x->arch == ADF_ARCH_COMPLEX && !acb_contains(y->inf, x->inf))",
     "!arb_contains(acb_realref(x->inf), acb_realref(y->inf)))\n        return 0;\n    if (x->arch == ADF_ARCH_COMPLEX && !acb_contains(y->inf, x->inf))"),
    ("sball", "neg does not negate the real part",
     "arb_neg(acb_realref(t.inf), acb_realref(x->inf));", "arb_set(acb_realref(t.inf), acb_realref(x->inf));"),
    ("sball", "is_canonical accepts primes in any order",
     "if (i > 0 && !(x->loc[i - 1].p < x->loc[i].p))", "if (i > 0 && x->loc[i - 1].p == x->loc[i].p + 1)"),
    ("sball", "mul adds the real parts",
     "arb_mul(acb_realref(t.inf), acb_realref(x->inf), acb_realref(y->inf), prec);",
     "arb_add(acb_realref(t.inf), acb_realref(x->inf), acb_realref(y->inf), prec);"),
    ("sball", "binary writes its output before it reports a status",
     "            adf_sball_clear(&t);\n            return st;\n        }\n    }\n    adf_sball_swap(z, &t);",
     "            adf_sball_clear(&t);\n            adf_sball_set(z, x);\n            return st;\n        }\n    }\n    adf_sball_swap(z, &t);"),
    ("sball", "set_arb_lballs accepts two components at one prime",
     "        if (k[i - 1].p == k[i].p)\n", "        if (0)\n"),
    ("sball", "binary checks the tag before the places (UNSUPPORTED before DOMAIN)",
     "    if (places_differ(where, x, y))\n        return ADF_DOMAIN;\n    if (x->arch == ADF_ARCH_COMPLEX)\n    {\n        report(where, adf_place_inf());\n        return ADF_UNSUPPORTED;\n    }\n    if (prec < 2)",
     "    if (x->arch == ADF_ARCH_COMPLEX)\n    {\n        report(where, adf_place_inf());\n        return ADF_UNSUPPORTED;\n    }\n    if (places_differ(where, x, y))\n        return ADF_DOMAIN;\n    if (prec < 2)"),
    ("sball", "overlaps of real balls uses contains",
     "!arb_overlaps(acb_realref(x->inf), acb_realref(y->inf)))", "!arb_contains(acb_realref(x->inf), acb_realref(y->inf)))"),
    ("rfunc", "the odd root of a negative ball calls arb_root_ui on it",
     "    if (!even && arb_is_negative(x))", "    if (0)"),
    ("rfunc", "log of the exact 0 is NOT_DETERMINED instead of DOMAIN",
     "if (f == F_LOG && arb_is_nonpositive(x))", "if (f == F_LOG && arb_is_negative(x))"),
    ("rfunc", "sqrt of the exact 0 is DOMAIN",
     "if (f == F_ROOT && n % 2 == 0 && arb_is_negative(x))", "if (f == F_ROOT && n % 2 == 0 && arb_is_nonpositive(x))"),
    ("rfunc", "prec below 2 is not raised to 2",
     "    if (prec < 2)\n        prec = 2;\n    if (!arb_is_finite(x))", "    if (!arb_is_finite(x))"),
    ("rfunc", "at a place that is not in the ball: where is not reported",
     "    if (!adf_sball_has_place(x, v))\n    {\n        if (where != NULL)\n            *where = v;\n        return ADF_DOMAIN;",
     "    if (!adf_sball_has_place(x, v))\n    {\n        return ADF_DOMAIN;"),
    ("rfunc", "sin computed as cos",
     "            arb_sin(t, x, prec);", "            arb_cos(t, x, prec);"),
    ("rfunc", "odd root of a ball across 0: the lower end is not negated",
     "            arf_neg(a, a);\n            rootpos_arf(lo, a, n, prec);", "            rootpos_arf(lo, a, n, prec);"),
    ("rfunc", "the exact 0 of an odd root is left to arb_root_ui (NaN)",
     "    if (arb_is_zero(x))\n    {\n        arb_zero(t);", "    if (0)\n    {\n        arb_zero(t);"),
    ("rfunc", "log_abs of a ball containing 0 is computed",
     "    if (f == F_LOG_ABS && arb_contains_zero(x))\n        return ADF_NOT_DETERMINED;\n", ""),
    ("rfunc", "a non-finite result is stored with OK",
     "    if (!arb_is_finite(t))   /* conventions 4.4, CV-08: never stored */", "    if (0)"),
]

results = []
for i, (which, name, old, new) in enumerate(FAULTS):
    src_text = SRC[which]
    if src_text.count(old) != 1:
        print(f"fault {i}: anchor occurs {src_text.count(old)} times: {name}")
        sys.exit(2)
    src = OUT / f"{which}_f{i}.c"
    src.write_text(src_text.replace(old, new))
    exe = OUT / f"test_{which}_f{i}"
    cmd = ["cc", "-Iinclude", "-Itests", "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Wno-unused-function",
           "-Wno-unused-variable", "-Wno-unused-parameter", "-Wno-unused-but-set-variable", str(src), TEST[which],
           "build/support/golden.o", "build/support/jsonl.o", "build/libadelefeld.a", "-lflint", "-lgmp", "-lm", "-o",
           str(exe)]
    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    if r.returncode != 0:
        print(f"fault {i}: does not compile: {r.stderr[:400]}")
        sys.exit(2)
    try:
        t = subprocess.run([str(exe)], cwd=ROOT, capture_output=True, text=True, timeout=200)
        lines = t.stdout.strip().splitlines()
        tail = lines[-1] if lines else "(no output, exit %d)" % t.returncode
        failed = [l[5:].split(" (")[0] for l in lines if l.startswith("FAIL ") and "failed checks" in l]
        if not failed and t.returncode != 0:
            failed = ["(exit %d: crash or abort)" % t.returncode]
        results.append((i, which, name, tail, failed))
    except subprocess.TimeoutExpired:
        results.append((i, which, name, "timeout", ["timeout"]))
for i, which, name, tail, failed in results:
    print(f"fault {i} [{which}]: {name}\n    -> {tail}\n    tests that failed: {', '.join(failed) if failed else 'NONE'}")
survivors = [i for i, _, _, _, f in results if not f]
print(f"{len(results)} faults, {len(results) - len(survivors)} caught, survivors: {survivors}")
