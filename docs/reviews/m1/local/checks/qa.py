#!/usr/bin/env python3
"""Verify review artifacts without writing outside the owned directories."""
import ast
import hashlib
from pathlib import Path
import subprocess

here = Path(__file__).resolve().parent
root = here.parents[4]
mds = [here.parent / 'review.md', root / 'lanes/m1-review-local/report.md']
errors = []
for p in mds:
    for n, line in enumerate(p.read_text().splitlines(), 1):
        if len(line) > 116:
            errors.append(f'{p.relative_to(root)}:{n}: length={len(line)}')
print('Markdown files=', len(mds), 'overlong lines=', len(errors))
for error in errors:
    print(error)
pyfiles = sorted(here.glob('*.py'))
for p in pyfiles:
    ast.parse(p.read_text(), filename=str(p))
print('Python files parsed=', len(pyfiles))
for name in ('run.sh', 'memory.sh'):
    subprocess.run(['sh', '-n', str(here / name)], check=True)
print('Shell scripts checked=2, syntax failures=0')
nhashes = 0
for line in (here / 'reviewed.sha256').read_text().splitlines():
    digest, name = line.split(None, 1)
    assert hashlib.sha256((root / name).read_bytes()).hexdigest() == digest, name
    nhashes += 1
print('Reviewed source hashes checked=', nhashes, 'changes=0')
assert not errors
