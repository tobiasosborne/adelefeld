#!/usr/bin/env python3
"""classify.py OLD NEW: which old entries of equivalent.txt (line-number keys) are kept, changed or
dropped in NEW (text keys), and which entries of NEW are added.  An old entry and a new one are the
same entry when they have the same file and kind and the reason of the old one quotes the text the
new mutant changes (the call of a swap, the statement of a dropped call, the `A as B` of a
comparison); the few that need a hand are in MANUAL (old line key -> new line text and old, new)."""
import os, re, sys
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tools", "mutate"))
import mutate

def norm(t):
    return " ".join(t.split())

old = []
for l in open(sys.argv[1]):
    l = l.strip()
    if not l or l.startswith("#"):
        continue
    head, _, reason = l.partition(" | ")
    p = head.split(":")
    old.append((p[0], int(p[1]), p[2], reason))
new = []
for l in open(sys.argv[2]):
    l = l.strip()
    if not l or l.startswith("#"):
        continue
    head, _, reason = l.rpartition(" | ")
    new.append(mutate.split_entry(head) + (reason,))
used = set()
res = {"kept": [], "changed": [], "dropped": []}
for (f, n, k, reason) in old:
    r = norm(reason)
    best = None
    for i, (nf, nk, lt, o, nw, occ, nr) in enumerate(new):
        if i in used or nf != f or nk != k:
            continue
        score = 0
        if norm(o) and (norm(o) in r or norm(lt).rstrip(";") in r):
            score = 2
        if k in ("cmp", "zero_one") and (norm(lt).replace("if (", "").rstrip(")") in r):
            score = max(score, 1)
        if score and (best is None or score > best[0]):
            best = (score, i)
    if best is None:
        res["dropped"].append((f, n, k, r))
    else:
        used.add(best[1])
        nr = new[best[1]][6]
        res["kept" if norm(nr) == r else "changed"].append((f, n, k, new[best[1]][2]))
for key in ("kept", "changed", "dropped"):
    print("== %s: %d" % (key, len(res[key])))
    for x in res[key]:
        print("  ", x[0], x[1], x[2], "->" if key != "dropped" else "", (x[3] if key != "dropped" else "")[:130])
print("== added: %d" % (len(new) - len(used)))
for i, e in enumerate(new):
    if i not in used:
        print("  ", e[0], e[1], e[2], "|", e[3][:50], "->", e[4][:40], e[5] if e[5] else "")
