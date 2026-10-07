#!/usr/bin/env python3
"""Check authored prose/code, excluding machine JSONL and verbatim command/output logs."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
whole = ["src/qclass.c", "include/adelefeld/qclass.h", "tests/test_qclass_text.c",
         "tests/test_qclass_dump.c", "tests/julia/qclass_text.jl"]
whole += [str(p.relative_to(ROOT)) for p in (ROOT / "lanes/q-slice4").glob("*.py")]
whole += [str(p.relative_to(ROOT)) for p in (ROOT / "lanes/q-slice4").glob("*.c")]
whole += [str(p.relative_to(ROOT)) for p in (ROOT / "tests/driver").glob("qclass-text.*")]
whole += [str(p.relative_to(ROOT)) for p in (ROOT / "tests/driver").glob("qclass-dump.*")]
for name in ("redgreen.md", "report.md"):
    if (ROOT / "lanes/q-slice4" / name).exists():
        whole.append("lanes/q-slice4/" + name)
blocks = {
    "src/text.c": ("/* Slice 3.1-d. Reuse", "\n#include <stdlib.h>\n\n/* conventions 9.4"),
    "src/dump.c": ("/* ---- Slice 3.1-d:", None),
    "include/adelefeld/dump.h": ("/* ---- adf_qclass:", "/* Layout queries"),
    "docs/api-3a.md": ("## Slice 3.1-d", None),
    "tests/test_julia.sh": ("# Slice 3.1-d union", 'rm -f "$qclass_text_output"\n\n'),
}
bad = checked = 0
for name in whole + list(blocks):
    data = (ROOT / name).read_text()
    assert data.endswith("\n"), name
    offset = 0
    if name in blocks:
        start, stop = blocks[name]
        a = data.index(start)
        b = data.index(stop, a) + len(stop) if stop else len(data)
        offset = data[:a].count("\n")
        data = data[a:b]
    checked += 1
    for n, line in enumerate(data.splitlines(), offset+1):
        if len(line) > 116:
            print(name, n, len(line))
            bad += 1
for p in (ROOT / "lanes/q-slice4").glob("*.log"):
    assert p.stat().st_size <= 100000, str(p)
print(f"style: {checked} files/blocks, {bad} overlong lines, all final newlines present")
raise SystemExit(1 if bad else 0)
