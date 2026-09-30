#!/usr/bin/env python3
"""lanes/f-slice6/faults.py: shows that the tests bite. Run from the repository root after `make check`
(needs build/libadelefeld.a, build/support/*.o, and build/drv-plain/libadelefeld.a from tools/adf).

For each fault a copy of src/rfunc.c (or tools/adf/adf.c) with one edit is built under build/scratch-f6/, linked in
front of the archive (the object defines every symbol of rfunc.o, so the archive member is not pulled), and the
tests are run. The fault is caught when the test program exits non-zero (or the driver output differs). Nothing is
written outside build/. No mutation tool is used.
"""
import os
import subprocess
import sys

ROOT = os.getcwd()
SC = os.path.join(ROOT, "build", "scratch-f6")
os.makedirs(SC, exist_ok=True)
CFLAGS = ["-std=c11", "-O2", "-g", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Iinclude", "-Itests"]
LIBS = ["-lflint", "-lgmp", "-lm"]
SUPPORT = ["build/support/golden.o", "build/support/jsonl.o"]

LIB_FAULTS = [
    ("A: the status of the function at a prime is reported with the archimedean place",
     "    if (st != ADF_OK && where != NULL)\n        *where = v;\n    adf_lball_clear(c);",
     "    if (st != ADF_OK && where != NULL)\n        *where = adf_place_inf();\n    adf_lball_clear(c);"),
    ("B: prec is clamped to 2 at a prime (as at the real place)",
     "return at_prime(y, where, x, v, f, prec);",
     "return at_prime(y, where, x, v, f, prec < 2 ? 2 : prec);"),
    ("C: Log_at at a prime computes log instead of Log",
     "        else\n            st = adf_lball_Log(r, c, N);",
     "        else\n            st = adf_lball_log(r, c, N);"),
    ("D: ADF_REAL_PREC_MAX applies at a prime too",
     "if (adf_place_is_archimedean(v) && prec > ADF_REAL_PREC_MAX)",
     "if (prec > ADF_REAL_PREC_MAX)"),
    ("E: Log_at at the real place is log |t|",
     "        f = F_LOG;   /* the real place: Log = log on t > 0",
     "        f = F_LOG_ABS;   /* the real place: Log = log on t > 0"),
    ("F: sin_at at a prime is computed (it falls into the Log branch)",
     "    if (f != F_EXP && f != F_LOG && f != F_LOG_IW)",
     "    if (f != F_EXP && f != F_LOG && f != F_LOG_IW && f != F_SIN)"),
]
DRV_FAULTS = [
    ("G: the driver accepts a leading zero in a place",
     "    if (tok[i] == '0' && digits > 1)\n        return ADF_PARSE;",
     "    if (0)\n        return ADF_PARSE;"),
    ("H: the driver prints the components in the order of the places given",
     "        status = adf_sball_project(s, NULL, a, places, n);",
     "        status = adf_sball_project(s, NULL, a, places, n);\n    if (status == ADF_OK && n > 1)\n        { adf_lball_swap(&s->loc[0], &s->loc[s->len - 1]); }"),
    ("I: exp_at calls log_at",
     "        status = adf_sball_exp_at(y, NULL, s, places[0], st->prec);",
     "        status = adf_sball_log_at(y, NULL, s, places[0], st->prec);"),
]


def run(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, timeout=600, **kw)


def edit(src, old, new, dst):
    text = open(src).read()
    if old not in text:
        raise SystemExit("edit target not found: " + old[:60])
    open(dst, "w").write(text.replace(old, new, 1))


def main():
    caught = 0
    total = 0
    for name, old, new in LIB_FAULTS:
        total += 1
        dst = os.path.join(SC, "rfunc_fault.c")
        edit("src/rfunc.c", old, new, dst)
        obj = os.path.join(SC, "rfunc_fault.o")
        r = run(["cc"] + CFLAGS + ["-c", dst, "-o", obj])
        if r.returncode != 0:
            print("%s: did not compile: %s" % (name, r.stderr[:300]))
            continue
        fails = []
        for t in ("test_rfunc_prime", "test_rfunc"):
            exe = os.path.join(SC, t)
            r = run(["cc"] + CFLAGS + ["tests/%s.c" % t] + SUPPORT + [obj, "build/libadelefeld.a", "-o", exe] + LIBS)
            if r.returncode != 0:
                print("%s: test did not link: %s" % (name, r.stderr[:300]))
                continue
            r = run([exe])
            last = [l for l in r.stdout.splitlines() if "failed checks" in l]
            if r.returncode != 0:
                fails.append("%s: %s" % (t, last[-1] if last else "exit %d" % r.returncode))
        if fails:
            caught += 1
        print("%s\n    %s" % (name, "CAUGHT by " + "; ".join(fails) if fails else "NOT CAUGHT"))
    for name, old, new in DRV_FAULTS:
        total += 1
        dst = os.path.join(SC, "adf_fault.c")
        edit("tools/adf/adf.c", old, new, dst)
        exe = os.path.join(SC, "adf_fault")
        r = run(["cc"] + CFLAGS + ["-Iinclude", dst, "build/drv-plain/libadelefeld.a", "-o", exe] + LIBS)
        if r.returncode != 0:
            print("%s: did not compile: %s" % (name, r.stderr[:300]))
            continue
        diffs = []
        for c in ("f-at-prime", "f-project", "f-places-hostile", "f-places-1000"):
            with open("tests/driver/%s.cmd" % c, "rb") as fin:
                r = subprocess.run([exe], stdin=fin, capture_output=True, timeout=120)
            want = open("tests/driver/%s.out" % c, "rb").read()
            if r.stdout != want:
                diffs.append(c)
        if diffs:
            caught += 1
        print("%s\n    %s" % (name, "CAUGHT by cases " + ", ".join(diffs) if diffs else "NOT CAUGHT"))
    print("caught %d of %d faults" % (caught, total))


if __name__ == "__main__":
    main()
