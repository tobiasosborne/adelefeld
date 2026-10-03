#!/usr/bin/env python3
"""f-repair5: lines over 116 characters in the files this lane changed (lanes/COMMON.md rule 6)."""
import sys
for f in sys.argv[1:]:
    for i, line in enumerate(open(f, encoding="utf-8"), 1):
        if len(line.rstrip("\n")) > 116:
            print("%s:%d: %d" % (f, i, len(line.rstrip("\n"))))
