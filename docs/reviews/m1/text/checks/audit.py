"""Final artifact checks, without touching any reviewed file."""
import ast
import json
import re
import subprocess
from pathlib import Path

here = Path(__file__).resolve().parent
root = here.parents[4]
documents = [here.parent / 'review.md', root / 'lanes/m1-review-text/report.md']
for p in documents:
    assert p.is_file()
    assert all(len(line) <= 116 for line in p.read_text().splitlines()), p
review = documents[0].read_text()
findings = re.findall(r'^### R\d+: (BLOCKER|MAJOR|MINOR)\.', review, re.M)
assert [findings.count(k) for k in ['BLOCKER', 'MAJOR', 'MINOR']] == [1, 6, 3]
scripts = list(here.glob('*.py'))
for p in scripts:
    ast.parse(p.read_text(), str(p))
subprocess.run(['sh', '-n', str(here / 'build.sh')], check=True)
result = subprocess.run(['sha256sum', '--check', str(here / 'reviewed.sha256')],
                        capture_output=True, text=True, check=True)
(here / 'fingerprints.log').write_text(result.stdout)
print(json.dumps(dict(documents=2, overlong_lines=0, blockers=1, majors=6, minors=3,
                      python_scripts=len(scripts), shell_scripts=1,
                      unchanged_reviewed_files=result.stdout.count(': OK'), failures=0)))
