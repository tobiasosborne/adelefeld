#!/usr/bin/env python3
"""Isolate the reported i <= k mutant; never alter the production file."""
from pathlib import Path
import os
import subprocess

here = Path(__file__).resolve().parent
root = here.parents[4]
source = (root / 'src/fball.c').read_text()
old = 'for (i = 0; i < k && h_is_one; i++)'
new = 'for (i = 0; i <= k && h_is_one; i++)'
assert source.count(old) == 1
mutated = here / 'fball-mutant.c'
mutated.write_text(source.replace(old, new))
flags = ['-std=c11', '-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-Iinclude']
subprocess.run(['cc', *flags, '-c', str(mutated), '-o', str(here / 'fball-mutant.o')], check=True)
subprocess.run(['cc', *flags, str(here / 'probe.c'), str(here / 'fball-mutant.o'),
                str(here / 'san/libadelefeld.a'), '-lflint', '-lgmp', '-lm',
                '-o', str(here / 'probe-mutant')], check=True)
env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
case = '* 1 1 0 1 1 4 1 1 4 1 1 1 1 1\n'
(here / 'mutant.input').write_text(case)
for binary in ('probe-san', 'probe-mutant'):
    p = subprocess.run([str(here / binary)], input=case, text=True, capture_output=True, env=env, timeout=30)
    (here / (binary + '.log')).write_text(p.stdout + p.stderr)
    print(binary, 'exit=', p.returncode, 'asan_overflow=', 'heap-buffer-overflow' in p.stderr)
    if binary == 'probe-san':
        assert p.returncode == 0 and p.stdout.strip() == '0 -1 1 1 1 4 1 1 1'
    else:
        assert p.returncode != 0 and 'heap-buffer-overflow' in p.stderr
