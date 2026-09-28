#!/usr/bin/env python3
"""equiv_keys.py: for every entry FILE:LINE:KIND of tools/mutate/equivalent.txt, list every
mutant that mutate.py generates with that key.  mutate.py excuses a survivor when its key
(path, line, kind) is listed (mutate.py:170-171, 849), so an entry excuses every mutant of that
kind on that line, not only the one its reason describes.  Also: entries that match no mutant
(stale line numbers).  Run from the repository root."""
import os, sys
sys.dont_write_bytecode = True
sys.path.insert(0, "tools/mutate")
import mutate

eq = mutate.read_equivalent("tools/mutate/equivalent.txt")
files = sorted(set(k[0] for k in eq))
by_key = {}
for f in files:
    text = open(f).read()
    for m in mutate.mutants_of(f, text, "."):
        by_key.setdefault(m.key, []).append(m)
multi = stale = 0
for key in sorted(eq):
    ms = by_key.get(key, [])
    if not ms:
        stale += 1
        print("STALE (no mutant has this key): %s:%d:%s" % key)
    elif len(ms) == 1:
        print("LIVE %s:%d:%s -> %s\n       reason: %s" % (key + (str(ms[0]).split(" ", 1)[1][:70],
                                                            eq[key][:70])))
    else:
        multi += 1
        print("ONE ENTRY, %d MUTANTS: %s:%d:%s | %s" % ((len(ms),) + key + (eq[key][:70],)))
        for m in ms:
            print("     ", m)
print("entries %d, covering more than one mutant %d, stale %d" % (len(eq), multi, stale))
