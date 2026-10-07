#!/usr/bin/env python3
"""Artifact checks; mathematical checks remain in proto/char_checks.py."""
import ast
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
FILES = ['docs/api-3c.md', 'proto/char_checks.py', 'lanes/d-char/progress.md',
         'lanes/d-char/mutate_oracle.py', 'lanes/d-char/flint_probe.c',
         'lanes/d-char/check_design.py']
if (ROOT/'lanes/d-char/report.md').exists():
    FILES.append('lanes/d-char/report.md')
long_lines = []
python_files = 0
for name in FILES:
    source = (ROOT/name).read_text()
    long_lines.extend((name, i+1, len(s)) for i, s in enumerate(source.splitlines()) if len(s) > 116)
    if name.endswith('.py'):
        ast.parse(source, filename=name)
        python_files += 1
assert not long_lines, long_lines
lines = len((ROOT/'docs/api-3c.md').read_text().splitlines())
assert lines <= 450, lines
print(f'QA: {len(FILES)} text files, {python_files} Python ASTs, 0 long lines; design {lines}/450 lines')
