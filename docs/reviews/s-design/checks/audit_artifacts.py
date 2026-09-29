#!/usr/bin/env python3
"""Final review-artifact checks, without writing bytecode or changing reviewed files."""
import ast
from collections import Counter
import hashlib
from pathlib import Path
import re

root = Path(__file__).resolve().parents[4]
base = root / 'docs/reviews/s-design'
review = (base / 'review.md').read_text()
report = root / 'lanes/s-review/report.md'
assert report.is_file()
assert review.splitlines()[1] == 'VALID 28 / MINOR 3 / INVALID 2. NOT READY'
counts = Counter(re.findall(r'^\| [^|]+ \| (VALID|MINOR|INVALID) \|', review, re.M))
assert counts == {'VALID': 28, 'MINOR': 3, 'INVALID': 2}
files = [base / 'review.md', report, base / 'review_checks.py']
files.extend(sorted((base / 'checks').glob('*.py')))
files.extend(sorted((base / 'checks').glob('*.c')))
long = [(str(p.relative_to(root)), i, len(line)) for p in files
        for i, line in enumerate(p.read_text().splitlines(), 1) if len(line) > 116]
assert not long, long
pyfiles = [p for p in files if p.suffix == '.py']
for p in pyfiles:
    ast.parse(p.read_text(), filename=str(p))
author = root / 'proto/solvers_checks.py'
assert hashlib.sha256(author.read_bytes()).hexdigest() == (
    '09588ed7a576dc85971458fe5a53069c09b917347f3ba9bd6696507b75fd6b36')
print(f'artifacts: files={len(files)}, python_syntax={len(pyfiles)}, overlong_lines={len(long)}, reports=2')
print('verdicts: VALID=28, MINOR=3, INVALID=2, total=33')
print('author_check_hash: unchanged=1')
print('total: failures=0')
