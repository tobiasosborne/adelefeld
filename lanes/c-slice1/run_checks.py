#!/usr/bin/env python3
"""Bounded final checks, at most two compiler jobs, with per-command exit and output."""
from pathlib import Path
import json
import os
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
LANE = ROOT / 'lanes/c-slice1'
RESULTS = []


def run(name, args, seconds=60, env=None):
    command = ['timeout', str(seconds), *args]
    process = subprocess.run(command, cwd=ROOT, env=env, text=True,
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output = process.stdout
    (LANE / (name + '.log')).write_text(output[-90000:])
    row = {'name': name, 'command': command, 'exit': process.returncode, 'output': output[-3000:]}
    RESULTS.append(row)
    print(name, process.returncode, output[-250:].strip(), flush=True)
    (LANE / 'check-results.json').write_text(json.dumps(RESULTS, indent=2) + '\n')
    if process.returncode != 0:
        raise SystemExit(process.returncode)


if sys.argv[1:] == ['text']:
    names = [p.stem for p in sorted((ROOT / 'tests').glob('test_text*.c'))]
    targets = ['lanes/c-slice1/build/' + name for name in names]
    run('text-build', ['make', '-s', '-j2', 'BUILD=lanes/c-slice1/build', *targets], 180)
    for name, target in zip(names, targets):
        run(name, [target])
else:
    for config, options in [('build', []), ('san', ['SAN=1']), ('inv', ['INV=1']),
                            ('clang', ['CC=clang'])]:
        target = f'lanes/c-slice1/{config}/test_char'
        run(config+'-final-build', ['make', '-s', '-j2', f'BUILD=lanes/c-slice1/{config}',
                                    *options, target], 180)
        env = dict(os.environ)
        if config == 'san':
            env['ASAN_OPTIONS'] = 'detect_leaks=0'
        run(config+'-final-run', [target], env=env)
    command = ['cc', '-Iinclude', '-Itests', '-std=c11', '-O2', '-g', '-Wall', '-Wextra',
               '-Wpedantic', '-Werror', '-DADF_CHAR_WRAP_SETUP', '-DADF_CHECK_INVARIANTS',
               'tests/test_char.c', 'lanes/c-slice1/inv/support/jsonl.o',
               'lanes/c-slice1/inv/support/golden.o', 'lanes/c-slice1/inv/libadelefeld.a',
               '-Wl,--wrap=dirichlet_group_init', '-Wl,--wrap=adf_phase_get_acb', '-Wl,--wrap=arb_sqrt_ui',
               '-lflint', '-lgmp', '-lm', '-pthread', '-o', 'lanes/c-slice1/inv/test_char_wrap']
    run('wrap-inv-build', command, 30)
    run('wrap-inv-run', ['lanes/c-slice1/inv/test_char_wrap'])
