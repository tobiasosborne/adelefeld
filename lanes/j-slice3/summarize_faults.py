#!/usr/bin/env python3
"""Normalize fault summaries from the retained Pkg.test output."""
from pathlib import Path
import re

p = Path("lanes/j-slice3/faults.log")
result = []
for line in p.read_text().splitlines():
    ident = line.split()[0]
    log = Path(f"lanes/j-slice3/fault-{ident}.log").read_text()
    m = re.search(r"Adelefeld step J-a\.3 *\| *(.+)", log)
    assert m, ident
    header = log[:m.start()].splitlines()[-1].split("|")[1].split()
    data = dict(zip(header, m[1].split()))
    counts = "/".join(data.get(k, "0") for k in ("Pass", "Fail", "Error", "Total"))
    result.append(line.split("; J-a.3")[0] + "; J-a.3 " + counts)
p.write_text("\n".join(result) + "\n")
print(f"fault summaries normalized: {len(result)}; counts: pass/fail/error/total")
