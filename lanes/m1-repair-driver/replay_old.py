#!/usr/bin/env python3
"""replay_old.py: the check of docs/reviews/m1/surface/checks/replay_scripts.py over the
scripts that existed before this lane (01 to 08 and 10), with the assertion on the number of
lines relaxed: the new scripts of this lane hold settings and statuses, which write no line.
    python3 lanes/m1-repair-driver/replay_old.py"""
import sys, os, glob
sys.dont_write_bytecode = True
sys.path.insert(0, "docs/reviews/m1/surface/checks")
import oracle

names = ["01_spec_4_1", "02_spec_4_2", "03_spec_4_3", "04_spec_4_4_cap", "05_spec_4_5",
         "06_pairs", "07_status", "08_show_type", "10_line_language"]
tot = agree = differ = unmod = 0
for n in names:
    cmd = "tests/driver/%s.cmd" % n
    out = open(cmd[:-4] + ".out").read().split("\n")[:-1]
    lines = [l for l in open(cmd).read().split("\n")
             if l.strip() and not l.strip().startswith("#")]
    for l in lines:
        tot += 1
        try:
            e = oracle.run(l)
        except Exception:
            e = None
        if e is None:
            unmod += 1
        elif e in out:
            agree += 1
        else:
            differ += 1
            print("%s: DIFFER: %s (not among the %d expected lines)" % (n, l.strip(), len(out)))
print("commands %d, expected lines %d, agree %d, differ %d, not modelled %d"
      % (tot, sum(len(open("tests/driver/%s.out" % n).read().split("\n")) - 1 for n in names),
         agree, differ, unmod))
