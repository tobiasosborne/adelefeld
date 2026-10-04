"""Static audit of this lane's delivered inputs; no generated files outside the lane."""
import ast
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
NAMES = ['docs/api-3.md', 'proto/quotient3_checks.py', 'lanes/d-quotient/progress.md',
         'lanes/d-quotient/check_contract.py', 'lanes/d-quotient/audit.py']
lines = 0
for name in NAMES:
    text = (ROOT/name).read_text()
    for n, line in enumerate(text.splitlines(), 1):
        assert len(line) <= 116, (name, n, len(line))
        lines += 1
    if name.endswith('.py'):
        ast.parse(text, filename=name)
doc = (ROOT/'docs/api-3.md').read_text()
assert doc.count('```') % 2 == 0
names = re.findall(r'^(?:void|int|slong|size_t|char \*)\s*([a-z_0-9]+)\(', doc, re.M)
assert len(names) == len(set(names))
for phrase in ['Statements to add to quotient.md', 'Decisions for TJO', 'Thin slices',
               'Findings against the specification', 'D3-1', 'D3-2', 'D3-3']:
    assert phrase in doc
for path in re.findall(r'`(refs/[^`:]+):\d+', doc):
    assert (ROOT/path).is_file(), path
print(f'PASS static audit: {len(NAMES)} files, {lines} lines <= 116, 3 Python ASTs')
print(f'PASS design shape: {len(names)} unique declarations, closed fences, 7 required labels')
