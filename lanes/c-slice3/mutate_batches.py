#!/usr/bin/env python3
"""At most 60 mutants in six individually bounded batches; under 20 minutes total."""
import json
import os
from pathlib import Path
import subprocess
import time
import shutil

ROOT = Path(__file__).resolve().parents[2]
os.chdir(ROOT)
LANE = Path('lanes/c-slice3')
results = []
started = time.monotonic()
STAGED = Path('/tmp/adf-c-slice3-mutation-root')
if STAGED.exists():
    shutil.rmtree(STAGED)
STAGED.mkdir()
shutil.copy2('Makefile', STAGED / 'Makefile')
for entry in ('include', 'src', 'tests'):
    shutil.copytree(entry, STAGED / entry, ignore=shutil.ignore_patterns('__pycache__'))
target = STAGED / LANE
target.mkdir(parents=True)
shutil.copy2(LANE / 'mutate_check.sh', target / 'mutate_check.sh')
shutil.copytree(LANE / 'mut-base', target / 'mut-base')
for j, seed in enumerate(range(310409, 310415), 1):
    if time.monotonic()-started > 1000:
        break
    cmd = ['timeout', '180', 'python3', '-u', 'tools/mutate/mutate.py', '--root', str(STAGED),
           '--scratch', '/tmp/adf-c-slice3-mutate-'+str(j), '--files', 'src/char.c',
           '--limit', '10', '--seed', str(seed), '--jobs', '1', '--timeout', '120', '--san',
           '--make', 'timeout 110 sh lanes/c-slice3/mutate_check.sh',
           '--copy', 'Makefile', 'include', 'src', 'tests', 'lanes']
    listed = subprocess.run(cmd + ['--list'], capture_output=True, text=True)
    (LANE / ('mutants-'+str(j)+'.txt')).write_text(listed.stdout)
    assert listed.returncode == 0
    t = time.monotonic()
    with (LANE / ('mutate-'+str(j)+'.log')).open('w') as log:
        log.write('$ '+' '.join(cmd)+'\n'); log.flush()
        run = subprocess.run(cmd, stdout=log, stderr=subprocess.STDOUT,
                             env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
        log.write('EXIT '+str(run.returncode)+'\n')
    row = {'batch': j, 'seed': seed, 'exit': run.returncode, 'seconds': round(time.monotonic()-t, 1)}
    results.append(row)
    print(row, flush=True)
    (LANE / 'mutation_batches.json').write_text(json.dumps(results, indent=2)+'\n')
    if run.returncode in (2, 124):
        break
