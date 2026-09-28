#!/usr/bin/env python3
"""replay_scripts.py: recompute every expected line of tests/driver/NN_*.cmd with oracle.py.

Run from the repository root:  python3 docs/reviews/m1/surface/checks/replay_scripts.py
For each command line, the oracle's text is compared with the line of the .out file.
Lines the oracle does not model (reconstruct, settings, unsupported kinds, parse errors)
are counted and listed as 'not modelled'."""
import sys, os, glob
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(__file__))
import oracle

tot = agree = differ = unmod = 0
for cmd in sorted(glob.glob("tests/driver/*.cmd")):
    name = os.path.basename(cmd)
    if name.startswith("09"):
        continue            # settings change the printer; handled separately
    out = open(cmd[:-4] + ".out").read().split("\n")[:-1]
    lines = [l for l in open(cmd).read().split("\n")
             if l.strip() and not l.strip().startswith("#")]
    assert len(lines) == len(out), (name, len(lines), len(out))
    for l, o in zip(lines, out):
        tot += 1
        try:
            e = oracle.run(l)
        except Exception as ex:
            e = None
        if e is None:
            unmod += 1
            print("%s: not modelled: %s  -> %s" % (name, l.strip(), o))
        elif e == o:
            agree += 1
        else:
            differ += 1
            print("%s: DIFFER: %s\n   expected line: %s\n   oracle:        %s" % (name, l, o, e))
print("lines %d, agree %d, differ %d, not modelled %d" % (tot, agree, differ, unmod))
