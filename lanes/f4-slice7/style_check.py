#!/usr/bin/env python3
"""Check owned prose/code, final newlines, vector budget and retained-log budget."""
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[2]
LANE = ROOT / 'lanes/f4-slice7'
paths = list(LANE.glob('*.py')) + list(LANE.glob('*.h')) + list(LANE.glob('*.c')) + list(LANE.glob('*.md'))
paths += [ROOT / name for name in ('tests/test_ffun_dump.c', 'tests/test_rfun_dump.c',
                                  'tests/julia/ffun_dump.jl', 'tests/driver/ffun-dump.cmd',
                                  'tests/driver/ffun-dump.out', 'tests/driver/rfun-dump.cmd',
                                  'tests/driver/rfun-dump.out')]
failures = []
for p in paths:
    text = p.read_text()
    if not text.endswith('\n'):
        failures.append((str(p), 'missing final newline'))
    for i, line in enumerate(text.splitlines(), 1):
        if len(line) > 116:
            failures.append((str(p), i, len(line)))
for name, marker in [('src/dump.c', '/* Slice 4c dump forms:'),
                     ('include/adelefeld/dump.h', '/* Slice 4c,'),
                     ('docs/api-4a.md', '## Slice 4c, the dump forms'),
                     ('docs/api-4b.md', '## Slice 4c, the dump forms')]:
    text = (ROOT / name).read_text().split(marker, 1)[1]
    assert text.endswith('\n')
    failures += [(name, i, len(line)) for i, line in enumerate(text.splitlines(), 1) if len(line) > 116]
vector_bytes = 0
long_records = 0
for p in (ROOT / 'tests/ref/vectors/f4-slice7').glob('*.jsonl'):
    vector_bytes += p.stat().st_size
    data = p.read_text()
    assert data.endswith('\n')
    for line in data.splitlines():
        json.loads(line)
        long_records += len(line) > 116
large_logs = [str(p) for p in LANE.glob('*.log') if p.stat().st_size > 100000]
print('Prose/code files:', len(paths), '; line/newline failures:', len(failures))
print('Vector bytes:', vector_bytes, '; JSONL records over 116 columns:', long_records)
print('Retained logs above 100000 bytes:', len(large_logs))
assert not failures, failures
assert vector_bytes <= 400000
assert not large_logs, large_logs
