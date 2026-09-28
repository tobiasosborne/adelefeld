#!/usr/bin/env python3
"""listkeys.py: every mutant of every file of src/ as `file:line kind key`, one per line."""
import os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tools", "mutate"))
import mutate
for n in sorted(os.listdir(os.path.join(ROOT, "src"))):
    if n.endswith(".c"):
        p = "src/" + n
        with open(os.path.join(ROOT, p)) as fh:
            t = fh.read()
        for m in sorted(mutate.mutants_of(p, t, ROOT), key=lambda m: m.start):
            print("%s:%d %s ## %s" % (p, m.line, m.kind, m.key))
