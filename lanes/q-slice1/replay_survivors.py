#!/usr/bin/env python3
"""Replay the seven surviving mutants with the strengthened complete qclass test."""
from pathlib import Path
import os
import shutil
import subprocess

source = Path('src/qclass.c').read_text().splitlines()
runs = sorted(Path('lanes/q-slice1/mutate').glob('run-*/keep/*'))
survivors = {65: 'i = 1', 76: 'i = 1', 87: 'x->len < 0',
             99: 'fmpz_sgn(A) < 0 &&', 107: 'fmpz_set(prevH, H);',
             128: '', 171: 'ADF_INV_RAT(q);'}
found = set()
for root in runs:
    mutant = (root / 'src/qclass.c').read_text().splitlines()
    changed = [i+1 for i, (a, b) in enumerate(zip(source, mutant)) if a != b]
    if len(changed) != 1 or changed[0] not in survivors:
        continue
    line = changed[0]
    if survivors[line] not in mutant[line-1]:
        continue
    if line == 128 and mutant[line-1].strip():
        continue
    found.add(line)
    shutil.copy2('tests/test_qclass.c', root / 'tests/test_qclass.c')
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
    result = subprocess.run(['timeout', '90', 'make', '-s', '-j2', 'check', 'INV=1', 'SAN=1'],
                            cwd=root, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            timeout=100)
    log = Path(f'lanes/q-slice1/replay-{line}.log')
    log.write_bytes(result.stdout)
    print(f'line {line}, {root.name}: exit {result.returncode}, log {log}', flush=True)
    if line in (65, 99):
        assert result.returncode == 0, result.stdout.decode(errors='replace')
    else:
        assert result.returncode != 0, result.stdout.decode(errors='replace')
assert found == set(survivors), (found, set(survivors))
print('7 survivors replayed: 5 killed, 1 uninitialized-stack survivor, 1 equivalent survivor')
