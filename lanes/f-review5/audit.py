"""Final audit of the review evidence, without running repository tests."""
from pathlib import Path
import json

root = Path(__file__).resolve().parent
repo = root.parents[1]
old = (root/'old_lfunc.c').read_text()
current = (repo/'src/lfunc.c').read_text()
assert current.startswith(old)
print(f'old_source_bytes={len(old.encode())} old_source_lines={len(old.splitlines())} prefix_equal=1')
checks = []
for mode in ['local', 'enumerate', 'limits', 'large', 'regression']:
    failures = json.loads((root/f'{mode}.failures.json').read_text())
    assert not failures
    inputs = (root/f'{mode}.in').read_text().splitlines()
    outputs = (root/f'{mode}.out').read_text().splitlines()
    assert len(inputs) == len(outputs)
    print(f'mode={mode} input_rows={len(inputs)} output_rows={len(outputs)} failures=0')
    checks.append(len(inputs))
for log in ['local.stderr', 'enumerate.stderr', 'limits.stderr', 'large.stderr', 'regression.stderr',
            'real.log', 'f13.stderr']:
    text = (root/log).read_text()
    assert 'ERROR: AddressSanitizer' not in text and 'runtime error:' not in text
print('sanitizer_logs=7 diagnostics=0')
rows = (root/'f13.out').read_text().splitlines()
assert [int(r.split()[0]) for r in rows] == [7]*4+[1]*4
print('f13_counterexamples=8 domain=4 not_determined=4 limit=0')
files = ['attack.py', 'probe.c', 'real_probe.c', 'driver_attack.py', 'cost_probe.c',
         'audit.py', 'progress.md', 'referee.md']
long_lines = []
for name in files:
    for i, line in enumerate((root/name).read_text().splitlines(), 1):
        if len(line) > 116:
            long_lines.append((name, i, len(line)))
print(f'line_audit_files={len(files)} lines_over_116={len(long_lines)}')
for hit in long_lines:
    print(hit)
assert not long_lines
print(f'total_stream_cases={sum(checks)}')
