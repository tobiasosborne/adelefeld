#!/usr/bin/env python3
"""Check statement/check links, Python syntax, and the lane's line-width rule."""
import ast
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
proof = (root/'docs/proofs/analysis.md').read_text()
script = (root/'proto/analysis_checks.py').read_text()
functions = {n.name for n in ast.parse(script).body if isinstance(n, ast.FunctionDef)}
checks = set(re.findall(r'`(check_[a-z_]+)`', proof))
missing = sorted(checks-functions)
statements = len(re.findall(r'^## (?:Definition|Lemma|Proposition) [0-9]+\.', proof, re.M))
check_labels = len(re.findall(r'^Check:', proof, re.M))
used_by = len(re.findall(r'^Used by:', proof, re.M))
print(f'statements={statements} check_labels={check_labels} used_by_labels={used_by}')
print(f'check_names={len(checks)} missing={len(missing)}')
assert statements == check_labels == used_by == 15
assert not missing, missing
for name in ['docs/proofs/analysis.md', 'proto/analysis_checks.py',
             'lanes/m0-proofs-analysis/mutation_checks.py',
             'lanes/m0-proofs-analysis/structure_checks.py',
             'lanes/m0-proofs-analysis/report.md']:
    text = (root/name).read_text()
    lines = text.splitlines()
    bad = [(i,len(line)) for i,line in enumerate(lines,1) if len(line)>116]
    if name.endswith('.py'):
        ast.parse(text)
    print(f'{name}: lines={len(lines)} overlength={len(bad)}')
    assert not bad, bad
