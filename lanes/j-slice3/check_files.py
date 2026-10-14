#!/usr/bin/env python3
"""Check authored changes' line lengths and final newlines, without reformatting earlier steps."""
from pathlib import Path

files = [Path("julia/Adelefeld/src") / n for n in ("adele.jl", "carrier.jl")]
files += [Path("lanes/j-slice3") / n for n in
          ("gate.sh", "session.sh", "help.sh", "faults.py", "powers_test.jl",
           "check_files.py", "wrap_logs.py", "summarize_faults.py", "cleanup.py",
           "redgreen.md", "report.md")]
violations = []
checked = 0
for p in files:
    if not p.exists():
        continue
    b = p.read_bytes()
    checked += 1
    if not b.endswith(b"\n"):
        violations.append((str(p), "missing final newline"))
    for i, s in enumerate(b.decode().splitlines(), 1):
        if len(s) > 116:
            violations.append((str(p), i, len(s)))
for path, start in (("docs/api-ja.md", "# Step J-a.3:"),
                    ("julia/Adelefeld/test/runtests.jl", "# ---- Step J-a.3:")):
    p = Path(path)
    s = p.read_text()
    for i, line in enumerate(s[s.index(start):].splitlines(), 1):
        if len(line) > 116:
            violations.append((path, f"appended line {i}", len(line)))
    if not s.endswith("\n"):
        violations.append((path, "missing final newline"))
    checked += 1
print(f"files/regions checked: {checked}; violations: {len(violations)}")
for v in violations:
    print(v)
raise SystemExit(bool(violations))
