#!/usr/bin/env python3
"""excuse_lines.py: do the excuses of src/scaled.c in tools/mutate/equivalent.txt name the lines
they describe? equivalent.txt, header: "LINE is the line of the file as it stands now"; mutate.py
matches an excuse by the key (path, line, kind) only (tools/mutate/mutate.py:170, :638-656).
For each entry the code quoted at the start of the reason (e.g. `fmpz_mul(t, d, K)`, or
`fmpq_sgn(q) < 0`) is looked up on the named line of src/scaled.c as it stands, and everywhere
in the file. Run from the worktree root."""
import re

src = open("src/scaled.c").read().splitlines()
n = ok = 0
for raw in open("tools/mutate/equivalent.txt"):
    line = raw.strip()
    if not line.startswith("src/scaled.c:"):
        continue
    head, _, reason = line.partition("|")
    _, ln, kind = [p.strip() for p in head.split(":")]
    ln = int(ln)
    reason = reason.strip()
    m = re.match(r"(.+?)( against | as | after | in the |:)", reason)
    code = m.group(1).strip() if m else reason
    here = code in src[ln - 1] if ln <= len(src) else False
    where = [i + 1 for i, s in enumerate(src) if code in s]
    n += 1
    ok += here
    print("%-5s %-10s line %3d %-40s now on line %-3s: %s" % ("OK" if here else "STALE", kind, ln, code[:40],
                                                              ln, src[ln - 1].strip()[:50]),
          "| code found on lines", where)
print("scaled.c excuses: %d, naming the right line: %d, stale: %d" % (n, ok, n - ok))
