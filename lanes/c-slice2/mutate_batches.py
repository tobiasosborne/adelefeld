#!/usr/bin/env python3
"""Six bounded 10-run samples; at most 60 attempted mutants in all."""
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
os.chdir(ROOT)
LANE = Path('lanes/c-slice2')
results = []
for j, seed in enumerate(range(310309, 310315), 1):
    cmd = ['timeout', '180', 'python3', '-u', 'tools/mutate/mutate.py', '--root', '.',
           '--scratch', '/tmp/adf-c-slice2-mutate-'+str(j), '--files', 'src/char.c',
           '--limit', '10', '--seed', str(seed), '--jobs', '1', '--timeout', '45', '--san',
           '--make', 'timeout 150 sh lanes/c-slice2/mutate_check.sh',
           '--copy', 'Makefile', 'include', 'src', 'tests', 'lanes']
    listed = subprocess.run(cmd + ['--list'], capture_output=True, text=True)
    (LANE / ('mutants-'+str(j)+'.txt')).write_text(listed.stdout)
    assert listed.returncode == 0
    with (LANE / ('mutate-'+str(j)+'.log')).open('w') as log:
        log.write('$ '+' '.join(cmd)+'\n'); log.flush()
        env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
        run = subprocess.run(cmd, stdout=log, stderr=subprocess.STDOUT, env=env)
        log.write('EXIT '+str(run.returncode)+'\n')
    results.append({'batch': j, 'seed': seed, 'exit': run.returncode})
    print(results[-1], flush=True)
    (LANE / 'mutation_batches.json').write_text(json.dumps(results, indent=2)+'\n')
    if run.returncode in (2, 124):
        break
