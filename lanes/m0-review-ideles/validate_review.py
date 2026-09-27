#!/usr/bin/env python3
"""Check owned deliverables without generating bytecode or touching proof files."""
import ast
from collections import Counter
from pathlib import Path


review = Path('docs/reviews/m0-proofs/ideles-review.md')
checks = Path('docs/reviews/m0-proofs/ideles_review_checks.py')
lane = Path('lanes/m0-review-ideles')
files = [review, checks, lane/'report.md', lane/'author_mutation_probe.py', lane/'validate_review.py']
long_lines = [(str(p), i, len(line)) for p in files
              for i, line in enumerate(p.read_text().splitlines(), 1) if len(line) > 116]
print(f'line_width: files={len(files)}; over_116={len(long_lines)}')
assert not long_lines, long_lines
python_files = [p for p in files if p.suffix == '.py']
for p in python_files:
    ast.parse(p.read_text(), filename=str(p))
print(f'python_parse: files={len(python_files)}; errors=0')
verdicts = Counter()
per_file = Counter()
for line in review.read_text().splitlines():
    cells = [c.strip() for c in line.split('|')]
    if len(cells) >= 6 and cells[1] in ('policies.md', 'ideles.md', 'quotient.md'):
        verdicts[cells[3]] += 1
        per_file[cells[1]] += 1
assert verdicts == {'VALID': 46, 'MINOR': 3, 'INVALID': 2}, verdicts
assert per_file == {'policies.md': 22, 'ideles.md': 16, 'quotient.md': 13}, per_file
print('verdicts: total=51; VALID=46; MINOR=3; INVALID=2; files=22/16/13')
log = (lane/'final_checks.txt').read_text()
assert 'TOTAL assertions=113317; refuted_claims=2' in log and 'exit=1;' in log
print('review_log: assertions=113317; refuted_claims=2; exit=1')
assert len(list(ast.walk(ast.parse(checks.read_text())))) > 0
imports = [n for n in ast.walk(ast.parse(checks.read_text())) if isinstance(n, (ast.Import, ast.ImportFrom))]
modules = {n.module if isinstance(n, ast.ImportFrom) else a.name
           for n in imports for a in n.names}
assert modules <= {'collections', 'fractions', 'itertools', 'math', 'random', 'sys'}, modules
print(f'independent_imports: modules={len(modules)}; author_modules=0')
