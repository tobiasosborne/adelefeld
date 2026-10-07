#!/usr/bin/env python3
"""Retest only three already selected mutants after adding their distinguishing inputs."""
from pathlib import Path
import json
import os
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SCRATCH = Path('/tmp/adf-f4-slice7-mutate')
env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0:abort_on_error=1', UBSAN_OPTIONS='halt_on_error=1')
needles = {
    'imaginary midpoint omitted': 'dp_zero(a[0].m) && dp_zero(a[1].rm)) return ADF_DOMAIN;',
    'extra token skipped': 'j <= 8 * (L + 3)',
    'division instead of skip product': 'j < 8 / (L + 3)',
}
results = []
for name, needle in needles.items():
    paths = [p for p in SCRATCH.rglob('dump.c') if p.parent.name == 'src' and needle in p.read_text()]
    assert len(paths) == 1, (name, paths)
    work = paths[0].parent.parent
    shutil.copy2(ROOT / 'lanes/f4-slice7/dump_test.h', work / 'lanes/f4-slice7/dump_test.h')
    for kind in ('ffun', 'rfun'):
        path = Path('tests/ref/vectors/f4-slice7') / (kind + '.jsonl')
        shutil.copy2(ROOT / path, work / path)
    cmd = ['timeout', '120', 'make', '-s', '-j2', 'check', 'SAN=1', 'INV=1']
    run = subprocess.run(cmd, cwd=work, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output = run.stdout.decode(errors='replace')
    print(name, 'exit', run.returncode, '|', ' | '.join(output.splitlines()[-4:]), flush=True)
    results.append({'mutant': name, 'scratch': str(work), 'exit': run.returncode,
                    'witness': output.splitlines()[-2:]})
    assert run.returncode == 2 and 'dump_test.h:' in output, output[-1000:]
(ROOT / 'lanes/f4-slice7/retest-results.json').write_text(json.dumps(results, indent=2) + '\n')
