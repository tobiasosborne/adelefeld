#!/usr/bin/env python3
"""Wrap retained logs at 116 characters without deleting any text."""
from pathlib import Path
import textwrap

changed = 0
# The runner continuously appends its own transcript. Do not rewrite those live files.
logs = [p for p in Path("lanes/j-slice3").glob("*.log") if p.name not in ("stdout.log", "stderr.log")]
for p in logs:
    lines = p.read_text().splitlines()
    result = []
    for line in lines:
        result.extend(textwrap.wrap(line, width=116, replace_whitespace=False, drop_whitespace=False)
                      if len(line) > 116 else [line])
    data = "\n".join(result) + "\n"
    if data != p.read_text():
        changed += 1
        p.write_text(data)
print(f"logs checked: {len(logs)}; logs wrapped: {changed}")
