"""Check owned edits, added-line width, and retained artifact newlines."""
from pathlib import Path
import subprocess
import json
import re
owned = {'src/localfactor.c', 'tests/test_tate_local.c', 'tests/test_tate_gamma.c',
         'docs/api-5a.md', 'docs/api-5b.md', 'tests/driver/tate-fe.cmd', 'tests/driver/tate-fe.out'}
changed = subprocess.run(['git', 'diff', '--name-only'], capture_output=True,
                         text=True, check=True).stdout.splitlines()
assert set(changed) <= owned, changed
result = subprocess.run(['git', 'diff', '--check'], capture_output=True, text=True)
assert result.returncode == 0, result.stdout + result.stderr
added = subprocess.run(['git', 'diff', '--unified=0'], capture_output=True,
                       text=True, check=True).stdout.splitlines()
newlines = [line[1:] for line in added if line.startswith('+') and not line.startswith('+++')]
assert all(len(line) <= 116 for line in newlines)
assert all(Path(name).read_bytes().endswith(b'\n') for name in changed)
artifacts = list(Path('lanes/t5-repair2').glob('*.py')) + list(Path('lanes/t5-repair2').glob('*.c'))
for p in artifacts:
    assert p.read_bytes().endswith(b'\n'), p
    assert all(len(line) <= 116 for line in p.read_text().splitlines()), p
print(f'{len(changed)} owned edited files, {len(newlines)} added lines, '
      f'{len(artifacts)} lane source files: 0 ownership, width, newline or diff-check failures')

lane = Path('lanes/t5-repair2')
records = [json.loads(line) for line in (lane / 'checks.jsonl').read_text().splitlines()]
summary = {}
for mode in ('plain', 'san', 'inv', 'clang'):
    tests = {}
    for record in records:
        if record['log'].startswith(str(lane) + '/' + mode + '-') and len(record['command']) == 3:
            tests[Path(record['command'][-1]).name] = record
    assert len(tests) == 6 and all(record['exit'] == 0 for record in tests.values())
    counts = {}
    for name, record in tests.items():
        text = Path(record['log']).read_text()
        counts[name] = int(re.findall(r'(\d+) checks', text)[-1])
        assert 'ERROR: AddressSanitizer' not in text and 'runtime error:' not in text
    summary[mode] = counts
assert summary == json.loads((lane / 'final-counts.json').read_text())
print('24 final test exits are 0; count total =', sum(sum(row.values()) for row in summary.values()))
