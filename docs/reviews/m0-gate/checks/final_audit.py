#!/usr/bin/env python3
"""Validate review structure and record SHA-256 of the read-only review inputs."""
import hashlib
import re
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
review = ROOT / "docs/reviews/m0-gate/review.md"
text = review.read_text()
findings = re.findall(r"^### (G\d+)\. (BLOCKER|MAJOR|MINOR):", text, re.M)
decisions = re.findall(r"^\| (CV-\d+) \| (accept|reject) \|", text, re.M)
assert [n for n, _ in findings] == [f"G{i}" for i in range(1, 17)]
assert [n for n, _ in decisions] == [f"CV-{i:02}" for i in range(1, 61)]
assert Counter(s for _, s in findings) == {"BLOCKER": 2, "MAJOR": 8, "MINOR": 6}
assert Counter(s for _, s in decisions) == {"accept": 52, "reject": 8}
prose_and_code = [review, ROOT / "lanes/m0-gate/report.md", *HERE.glob("*.py"), *HERE.glob("*.c"),
                  *HERE.glob("*.md")]
bad = [(str(p.relative_to(ROOT)), i, len(line)) for p in prose_and_code
       for i, line in enumerate(p.read_text().split("\n"), 1) if len(line) > 116]
print("findings", dict(Counter(s for _, s in findings)))
print("decisions", dict(Counter(s for _, s in decisions)))
print("line_length_files=%d over_116=%d" % (len(prose_and_code), len(bad)))
if bad:
    print(bad)
    raise SystemExit(1)
inputs = [ROOT / "CLAUDE.md", *[ROOT / "docs" / f for f in
          ["SPEC.md", "PLAN.md", "PERF.md", "conventions.md", "seams.md"]],
          *sorted((ROOT / "docs/proofs").glob("*.md")), ROOT / "proto/text_grammar.py",
          ROOT / "proto/analysis_checks.py", *sorted((ROOT / "tests/ref/adfref").glob("*.py")),
          *sorted((ROOT / "tests/golden").glob("*.tsv"))]
manifest = "".join(hashlib.sha256(p.read_bytes()).hexdigest() + "  " + str(p.relative_to(ROOT)) + "\n"
                   for p in inputs)
(HERE / "reviewed_inputs.sha256").write_text(manifest)
print("hashed_inputs=%d" % len(inputs))
print("golden_vectors=%d" % sum(sum(bool(line) and not line.startswith("#")
                                     for line in p.read_text().split("\n"))
                                   for p in (ROOT / "tests/golden").glob("*.tsv")))
print("audit_failures=0")
