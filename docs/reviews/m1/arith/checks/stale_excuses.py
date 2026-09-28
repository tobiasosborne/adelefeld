#!/usr/bin/env python3
"""For every entry of tools/mutate/equivalent.txt on src/rat.c, src/fball.c, src/adele.c,
src/recon.c: print the line the entry names as the file stands now, and whether that line contains
the call the reason names (the first identifier followed by "(" in the reason). mutate.py matches an
excuse by (file, line, kind) alone (tools/mutate/mutate.py:638-657, :849), so an entry whose line
has moved excuses whatever mutant of that kind now sits on that line.
Run from the root of the worktree."""
import re

FILES = ("src/rat.c", "src/fball.c", "src/adele.c", "src/recon.c")
src = {f: open(f).read().split("\n") for f in FILES}
stale = 0
total = 0
for raw in open("tools/mutate/equivalent.txt"):
    line = raw.strip()
    if not line or line.startswith("#"):
        continue
    head, _, reason = line.partition("|")
    parts = [p.strip() for p in head.split(":")]
    if len(parts) != 3 or parts[0] not in FILES:
        continue
    f, n, kind = parts[0], int(parts[1]), parts[2]
    total += 1
    cur = src[f][n - 1] if n - 1 < len(src[f]) else "<past end of file>"
    m = re.search(r"([A-Za-z_][A-Za-z_0-9]*)\(", reason)
    want = m.group(1) if m else None
    tok = {"cmp": ">=", "zero_one": None}.get(kind)
    ok = (want is None or want in cur)
    if not ok:
        stale += 1
    print("%-5s %s:%d:%s\n      reason names: %s\n      line now:     %s" %
          ("OK" if ok else "STALE", f, n, kind, want, cur.strip()))
print("entries: %d, stale (the named call is not on the named line): %d" % (total, stale))
