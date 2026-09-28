#!/usr/bin/env python3
"""lanes/m1-invariants/hand-mutation.py: step 5 of the brief, the second way.

tools/mutate/mutate.py is not run (the lane's instructions). Instead: the entry checks
(lines `ADF_INV_<TYPE>(arg);` of src/) and the borrow-count sites (lines ADF_INV_RETARGET,
ADF_INV_RELEASE, ADF_INV_BORROW) are listed, a seeded random draw takes 10 checks and 8 count
sites, and for each drawn line, one at a time: the line is deleted from the file, the library and
the two test programs are rebuilt with INV=1, the test programs are run, the file is restored from
a copy. A mutant is killed if a test program reports a failure or ends abnormally.

    python3 lanes/m1-invariants/hand-mutation.py [seed]        (from the repository root, after
                                                                 `make clean`; uses make -j2)
"""
import glob
import os
import random
import re
import shutil
import subprocess
import sys

seed = int(sys.argv[1]) if len(sys.argv) > 1 else 20260928
check_re = re.compile(r"^\s+ADF_INV_(RAT|FBALL|SCALED|ADELE|CADELE|FBALL_NAMED)\(")
count_re = re.compile(r"^\s+ADF_INV_(RETARGET|RELEASE|BORROW)\(")

checks, sites = [], []
for path in sorted(glob.glob("src/*.c")):
    for i, line in enumerate(open(path).read().split("\n"), start=1):
        if check_re.match(line):
            checks.append((path, i, line.strip()))
        elif count_re.match(line):
            sites.append((path, i, line.strip()))
print("checks: %d, count sites: %d, seed %d" % (len(checks), len(sites), seed))
rnd = random.Random(seed)
drawn = [("check", *c) for c in rnd.sample(checks, 10)] + [("count", *c) for c in rnd.sample(sites, 8)]


def run(cmd, timeout=300):
    return subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=timeout)


b = run("make -j2 build/test_invariants build/test_invariants_lifetime INV=1")
t1 = run("./build/test_invariants")
t2 = run("./build/test_invariants_lifetime")
print("baseline: build %d, test_invariants exit %d, test_invariants_lifetime exit %d" %
      (b.returncode, t1.returncode, t2.returncode), flush=True)
assert b.returncode == 0 and t1.returncode == 0 and t2.returncode == 0
killed = 0
for kind, path, lineno, text in drawn:
    orig = open(path).read()
    backup = path + ".orig-mutation"
    shutil.copyfile(path, backup)
    lines = orig.split("\n")
    assert lines[lineno - 1].strip() == text
    del lines[lineno - 1]
    open(path, "w").write("\n".join(lines))
    try:
        b = run("make -j2 build/test_invariants build/test_invariants_lifetime INV=1")
        if b.returncode != 0:
            res = "does not build: " + b.stderr.strip().splitlines()[-1][:120]
            dead = None
        else:
            t1 = run("./build/test_invariants")
            t2 = run("./build/test_invariants_lifetime") if kind == "count" else None
            fails = []
            for name, t in (("test_invariants", t1), ("test_invariants_lifetime", t2)):
                if t is None:
                    continue
                last = [l for l in t.stdout.splitlines() if "failed checks" in l]
                if t.returncode != 0:
                    fails.append("%s: exit %d, %s" % (name, t.returncode, last[-1] if last else "no summary"))
            dead = bool(fails)
            res = "KILLED by " + "; ".join(fails) if dead else "SURVIVED"
    finally:
        shutil.move(backup, path)
        os.utime(path, None)       # the restored file must look newer than the mutant object
    if dead:
        killed += 1
    print("%-5s %s:%d  %s  ->  %s" % (kind, path, lineno, text, res), flush=True)
b = run("make -j2 build/test_invariants build/test_invariants_lifetime INV=1")
t = run("./build/test_invariants")
print("after restoring: build %d, test_invariants exit %d" % (b.returncode, t.returncode))
print("mutants: %d, killed: %d, survived or unbuildable: %d" % (len(drawn), killed, len(drawn) - killed))
