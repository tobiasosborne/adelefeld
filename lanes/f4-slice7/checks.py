#!/usr/bin/env python3
"""Only the requested dump regression programs; two compiler jobs; bounded children."""
from pathlib import Path
import os
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
LANE = ROOT / 'lanes/f4-slice7'
mode = sys.argv[1]
own = '--own' in sys.argv[2:]
flags = {'plain': [], 'san': ['SAN=1', 'CC=clang'], 'inv': ['INV=1'], 'clang': ['CC=clang']}[mode]
build = 'lanes/f4-slice7/' + mode
names = ['test_ffun_dump', 'test_rfun_dump']
if not own:
    names += sorted(p.stem for p in (ROOT / 'tests').glob('test_dump*.c'))
    names += ['test_qclass_dump', 'test_char_eval']
env = dict(os.environ, ASAN_OPTIONS=os.environ.get('ASAN_OPTIONS', 'detect_leaks=1:abort_on_error=1'),
           UBSAN_OPTIONS='halt_on_error=1')
with (LANE / ('checks-' + ('own-' if own else '') + mode + '.log')).open('w') as log:
    cmd = ['timeout', '180', 'make', '-s', '-j' + os.environ.get('TASK_JOBS', '2'), 'BUILD=' + build] + flags
    cmd += [build + '/' + n for n in names]
    print(' '.join(cmd), flush=True)
    r = subprocess.run(cmd, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT)
    print('build exit', r.returncode, flush=True)
    if r.returncode:
        sys.exit(r.returncode)
    failed = 0
    for n in names:
        cmd = ['timeout', '180', build + '/' + n]
        r = subprocess.run(cmd, cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        out = r.stdout.decode(errors='replace')
        log.write('COMMAND: ' + ' '.join(cmd) + '\n' + out + 'EXIT: ' + str(r.returncode) + '\n')
        log.flush()
        print(n, 'exit', r.returncode, '|', ' | '.join(out.strip().splitlines()[-3:]), flush=True)
        failed += r.returncode != 0
    print(mode, len(names), 'programs;', failed, 'failed', flush=True)
    sys.exit(bool(failed))
