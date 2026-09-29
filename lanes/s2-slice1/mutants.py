#!/usr/bin/env python3
"""Do the tests bite? (item 5 of the brief of lane s2-slice1). One change at a time in a scratch copy under
build/mutants/<name>/ (Makefile, include, src, tests and the objects of build/, copied with their times, so that
only what the change touches is rebuilt); then build/test_roots_seed is built and run there, with a timeout of
150 s. A mutant is KILLED if the test program exits non-zero (failed checks, or an abort, or a sanitizer
report) or times out; it SURVIVES if the program passes. For a killed mutant the names of the failed tests and
the summary line are printed.

Run from the repository root after `make -j2 build/test_roots_seed`: python3 lanes/s2-slice1/mutants.py [name ...]
"""
import os
import re
import shutil
import subprocess
import sys

MUTANTS = [
    # (name, what, old, new): old must occur exactly once in src/roots.c
    ("strong_ge", "the strong form > changed to >= (v(g(a)) = 2 s accepted)",
     "        if (vg <= s2)\n", "        if (vg < s2)\n"),
    ("strong_ge_noguard", "the same, with the guard of newton_step (k2 <= k aborts) removed",
     None, None),
    ("strong_ge_nolift", "the same, and no Newton step from k <= s (so the mutant neither aborts nor loops)",
     None, None),
    ("s_from_f_core", "the core run on f instead of g: condition, s and lifting from f (the review's finding)",
     "    st = adf_roots_seed_core(ap, &K, &s, g, fp, a, prec_p);\n",
     "    st = adf_roots_seed_core(ap, &K, &s, f, fp, a, prec_p);\n"),
    ("s_from_f_only", "only s taken from f: s = v(f'(a)) after the core has computed the centre on g",
     "    if (st == ADF_OK)\n    {\n        /* L written only now",
     "    if (st == ADF_OK)\n    {\n        {\n            fmpz_poly_t df;\n            fmpz_t v;\n"
     "            fmpz_poly_init(df);\n            fmpz_init(v);\n            fmpz_poly_derivative(df, f);\n"
     "            fmpz_poly_evaluate_fmpz(v, df, a);\n            if (!fmpz_is_zero(v))\n"
     "                s = val_p(v, fp);\n            fmpz_poly_clear(df);\n            fmpz_clear(v);\n"
     "        }\n        /* L written only now"),
    ("newton_one_less", "one Newton step dropped (the loop stops when the next step would reach K)",
     "        while (k < Kv)\n", "        while (2 * k - sv < Kv)\n"),
    ("no_final_reduction", "the final reduction of the centre modulo p^K dropped",
     "        /* reduced modulo p^K (P3.2(3)) */\n        fmpz_pow_ui(q, p, (ulong) Kv);\n        fmpz_mod(a1, a1, q);\n",
     "        /* reduced modulo p^K (P3.2(3)) */\n"),
    ("verify_no_R1", "(R1) removed from the verifier: K > s and 0 <= a < p^K not tested (s >= 0 kept as a guard)",
     "    if (!(s >= 0 && K > s) || !prec_within_limit(K, fmpz_bits(p)) || !add_checked(&Ks, K, s))\n        return 0;\n"
     "    if (!below_power(a, p, K))\n        return 0;\n",
     "    if (!(s >= 0) || !prec_within_limit(K, fmpz_bits(p)) || !add_checked(&Ks, K, s))\n        return 0;\n"),
    ("verify_no_R2", "(R2) removed from the verifier",
     "    r = !fmpz_is_zero(v);\n    if (r)\n    {\n        fmpz_pow_ui(q, p, (ulong) s);\n        r = fmpz_divisible(v, q);\n    }\n",
     "    r = 1;\n"),
    ("verify_no_R3", "(R3) removed from the verifier",
     "        eval_mod(v, g, a, q);\n        r = fmpz_is_zero(v);\n",
     "        r = 1;\n"),
    ("plain_s_plus_1", "the checked addition s + 1 replaced by a plain one",
     "    if (!add_checked(&s1, sv, 1))\n    {\n        st = ADF_LIMIT;\n        goto done;\n    }\n",
     "    s1 = sv + 1;\n"),
    ("plain_2k", "the checked addition k + k of newton_step replaced by a plain one",
     "    if (!add_checked(&k2k, *k, *k) ||", "    k2k = *k + *k;\n    if ("),
    ("limit_after_normalise", "the first limit test of adf_root_padic_from_seed removed: LIMIT then comes from "
     "the core, after g has been formed",
     "    if (!prec_within_limit(prec_p, FLINT_BIT_COUNT(pw)))\n        return ADF_LIMIT;\n", ""),
    ("plain_limit_product", "the limit 2 K bits(p) <= ADF_ROOTS_BITS_MAX computed as a plain product (unsigned)",
     "    return K >= 0 && (ulong) K <= (ulong) ADF_ROOTS_BITS_MAX / (2 * bits);\n",
     "    return K >= 0 && 2 * (ulong) K * bits <= (ulong) ADF_ROOTS_BITS_MAX;\n"),
]


def apply(src, old, new, name):
    n = src.count(old)
    if n != 1:
        raise SystemExit(f"{name}: the pattern occurs {n} times")
    return src.replace(old, new)


def main():
    names = sys.argv[1:]
    base = open("src/roots.c").read()
    for name, what, old, new in MUTANTS:
        if names and name not in names:
            continue
        src = base
        if name == "strong_ge_noguard":
            src = apply(src, "        if (vg <= s2)\n", "        if (vg < s2)\n", name)
            src = apply(src, "    if (k2 <= *k || kms < 1)\n", "    if (0)\n", name)
        elif name == "strong_ge_nolift":
            src = apply(src, "        if (vg <= s2)\n", "        if (vg < s2)\n", name)
            src = apply(src, "        while (k < Kv)\n", "        while (k < Kv && k > sv)\n", name)
        else:
            src = apply(src, old, new, name)
        d = os.path.join("build", "mutants", name)
        if os.path.exists(d):
            shutil.rmtree(d)
        os.makedirs(d)
        for x in ("Makefile", "include", "src", "tests"):
            if os.path.isdir(x):
                shutil.copytree(x, os.path.join(d, x), symlinks=True)
            else:
                shutil.copy2(x, os.path.join(d, x))
        os.makedirs(os.path.join(d, "build"))
        for x in os.listdir("build"):
            p = os.path.join("build", x)
            if x.endswith((".o", ".d", ".a")):
                shutil.copy2(p, os.path.join(d, "build", x))
        shutil.copytree(os.path.join("build", "support"), os.path.join(d, "build", "support"))
        with open(os.path.join(d, "src", "roots.c"), "w") as fh:
            fh.write(src)
        b = subprocess.run(["make", "-s", "-j2", "build/test_roots_seed"], cwd=d, capture_output=True, text=True)
        if b.returncode != 0:
            print(f"== {name}: {what}\n   BUILD FAILED\n{b.stderr[-2000:]}")
            continue
        try:
            r = subprocess.run(["./build/test_roots_seed"], cwd=d, capture_output=True, text=True, timeout=150)
            out, rc = r.stdout, r.returncode
        except subprocess.TimeoutExpired as e:
            out, rc = (e.stdout or b"").decode() if isinstance(e.stdout, bytes) else (e.stdout or ""), "timeout"
        failed = re.findall(r"^FAIL ([a-zA-Z_0-9]+) \(.*?, (\d+) failed checks\)", out, re.M)
        summary = [l for l in out.splitlines() if l.endswith("failed tests")]
        changed = [l for l in out.splitlines() if l.startswith("changed lists") or l.startswith("enumeration:")]
        verdict = "SURVIVED" if rc == 0 else "KILLED"
        print(f"== {name}: {what}\n   {verdict} (exit {rc})")
        for t, c in failed:
            print(f"   failed test {t}: {c} failed checks")
        if rc != 0 and not failed:
            tail = [l for l in (out + r.stderr if not isinstance(rc, str) else out).splitlines()][-3:]
            print("   no test summary (abort or timeout); last lines:", *tail, sep="\n     ")
        for l in summary:
            print("  ", l)
        if name.startswith("verify"):
            for l in changed:
                if l.startswith("changed"):
                    print("  ", l)
        sys.stdout.flush()


if __name__ == "__main__":
    main()
