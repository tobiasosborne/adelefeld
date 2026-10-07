#!/usr/bin/env python3
"""Remove this lane's builds and preserve summaries of the oversized acceptance log."""
from pathlib import Path
import shutil

lane = Path('lanes/q-slice3')
log = lane / 'check-all.log'
text = log.read_text()
if len(text.encode()) > 100000:
    original_lines, original_bytes = len(text.splitlines()), len(text.encode())
    selected = []
    next_summary = 0
    for line in text.splitlines():
        if (line.startswith(('== ', 'check passed:', 'check-all passed:', 'test_driver:',
                             'test_exports:', 'selftest:')) or
                (' tests,' in line and ' checks,' in line) or
                'Tate psi' in line or 'passed with LD_PRELOAD=' in line):
            selected.append(line)
        elif line.startswith('Test Summary:'):
            selected.append(line)
            next_summary = 1
        elif next_summary:
            selected.append(line)
            next_summary -= 1
    log.write_text(f'Exit 0; summaries extracted from {original_lines} lines, {original_bytes} bytes.\n' +
                   '\n'.join(selected) + '\n')
    print(f'Acceptance log: {original_bytes} -> {log.stat().st_size} bytes')
removed = 0
for name in ('build', 'san', 'inv', 'clang', 'fault-base', 'final'):
    path = lane / name
    if path.exists():
        shutil.rmtree(path)
        removed += 1
pristine = Path('/tmp/adf-q-slice3-pristine')
if pristine.exists():
    shutil.rmtree(pristine)
    removed += 1
scratch = Path('/tmp/adf-q-slice3-mutate')
if scratch.exists():
    assert not list(scratch.iterdir()), list(scratch.iterdir())
    scratch.rmdir()
    removed += 1
for pattern in ('faults-*', 'recheck-*'):
    assert not [p for p in lane.glob(pattern) if p.is_dir()]
oversized = [p for p in lane.glob('*.log') if p.stat().st_size > 100000]
assert not oversized, oversized
print(f'{removed} remaining build/scratch trees removed; 0 logs over 100000 bytes')
