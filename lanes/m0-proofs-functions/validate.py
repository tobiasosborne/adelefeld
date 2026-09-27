"""Structural and line-length audit for this lane's deliverables; no third-party packages."""

import ast
import re
from pathlib import Path


proof = Path("docs/proofs/functions.md").read_text()
source = Path("proto/functions_checks.py").read_text()
names = {node.name for node in ast.parse(source).body if isinstance(node, ast.FunctionDef)}
used = set(re.findall(r"`(check_[a-z_]+)`", proof))
assert used <= names, used-names
statements = re.findall(r"^## (?:Definition|Lemma|Proposition) (\d+)", proof, re.M)
assert statements == [str(i) for i in range(1, 23)]
assert proof.count("Check:") == 22
assert proof.count("Used by:") == 22
index = re.findall(r"^\| (\d+) \|", proof, re.M)
assert index == statements
files = ("docs/proofs/functions.md", "proto/functions_checks.py",
         "lanes/m0-proofs-functions/report.md", "lanes/m0-proofs-functions/validate.py")
long_count = 0
for name in files:
    lines = Path(name).read_text().splitlines()
    long = [(i, len(s)) for i, s in enumerate(lines, 1) if len(s) > 116]
    long_count += len(long)
    print(f"{name}: lines={len(lines)} over_116={len(long)} positions={long}")
print(f"statements={len(statements)} check_labels={proof.count('Check:')} "
      f"used_by_labels={proof.count('Used by:')} linked_functions={len(used)} index_rows={len(index)}")
assert long_count == 0
