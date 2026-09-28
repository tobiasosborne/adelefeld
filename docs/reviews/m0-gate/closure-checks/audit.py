#!/usr/bin/env python3
"""Check review coverage, line lengths and record exactly which inputs were read."""
from collections import Counter
import hashlib
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[4]
HERE = Path(__file__).resolve().parent
closure = ROOT / "docs/reviews/m0-gate/closure.md"
report = ROOT / "lanes/m0-gate-closure/report.md"
text = closure.read_text()
findings = re.findall(r"^\| (G\d+) \| (CLOSED WITH EDIT|CLOSED|OPEN) \|", text, re.M)
decisions = re.findall(r"^\| (CV-\d+) \| (CLOSED WITH EDIT|CLOSED|OPEN) \|", text, re.M)
assert {x for x, _ in findings} == {"G%d" % i for i in range(1, 17)}
assert {x for x, _ in decisions} == {"CV-%02d" % i for i in (11, 12, 29, 30, 35, 37, 39, 41)}
assert len(findings) == 16 and len(decisions) == 8
assert re.findall(r"^C(\d+)\.", text, re.M) == [str(i) for i in range(1, 6)]
assert re.findall(r"^E(\d+)\.", text, re.M) == [str(i) for i in range(1, 5)]
assert "GATE PASSED AFTER THE LISTED EDITS" in report.read_text()
print("findings=%d decisions=%d new_findings=5 edits=4" % (len(findings), len(decisions)))
print("finding_verdicts=" + str(dict(Counter(s for _, s in findings))))
print("decision_verdicts=" + str(dict(Counter(s for _, s in decisions))))
written_text = [closure, report, *sorted(HERE.glob("*.py"))]
overlong = [(str(p.relative_to(ROOT)), i, len(line)) for p in written_text
            for i, line in enumerate(p.read_text().split("\n"), 1) if len(line) > 116]
print("review_source_files=%d overlong_lines=%d" % (len(written_text), len(overlong)))
for issue in overlong:
    print("overlong=%s:%s length=%s" % issue)
assert not overlong
inputs = [ROOT / p for p in (
    "CLAUDE.md", "lanes/COMMON.md", "docs/conventions.md", "docs/SPEC.md", "docs/PLAN.md",
    "docs/reviews/m0-gate/review.md", "docs/proofs/analysis.md", "docs/proofs/policies.md",
    "proto/text_grammar.py", "proto/test_text_grammar.py", "proto/policies_checks.py",
    "lanes/m0-gate-apply-conv/report.md", "lanes/m0-gate-apply-docs/report.md",
    "docs/reviews/m0-gate/checks/flint_probe.c", "refs/src/flint-3.0.1/arb.rst")]
inputs += sorted((ROOT / "tests/ref").rglob("*.py"))
inputs += sorted((ROOT / "tests/golden").glob("*.tsv"))
inputs += [ROOT / "tests/ref/README.md", ROOT / "tests/golden/README.md"]
manifest = "".join(hashlib.sha256(p.read_bytes()).hexdigest() + "  " + str(p.relative_to(ROOT)) + "\n"
                   for p in inputs)
(HERE / "reviewed-inputs.sha256").write_text(manifest)
print("input_hashes=%d report_exists=1" % len(inputs))
