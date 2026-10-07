#!/usr/bin/env python3
"""Bounded, sequential configurations. Each build uses at most two jobs."""
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
LANE = Path('lanes/c-slice2')
os.chdir(ROOT)
NAMES = ['test_char', 'test_char_eval'] + sorted(p.stem for p in Path('tests').glob('test_dump*.c'))
NAMES += ['test_qclass_dump']
failed = []


def run(cmd, log, env=None):
    log.write('$ ' + ' '.join(cmd) + '\n')
    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, env=env)
    log.write(result.stdout)
    log.write('EXIT ' + str(result.returncode) + '\n')
    log.flush()
    print(' '.join(cmd), 'EXIT', result.returncode, flush=True)
    return result.returncode


configs = [('plain', []), ('san', ['SAN=1']), ('inv', ['INV=1']), ('clang', ['CC=clang'])]
if len(sys.argv) > 1:
    configs = [item for item in configs if item[0] in sys.argv[1:]]
for config, flags in configs:
    build = LANE / config
    with (LANE / (config + '.log')).open('w') as log:
        targets = [str(build / n) for n in NAMES]
        if run(['timeout', '180', 'make', '-s', '-j2', 'BUILD='+str(build)] + flags + targets, log):
            sys.exit(1)
        env = dict(os.environ)
        if config == 'san':
            # Try the requested leak detector first, and record its actual result.
            env['ASAN_OPTIONS'] = 'detect_leaks=1'
            run(['timeout', '60', str(build / 'test_char_eval')], log, env)
            env['ASAN_OPTIONS'] = 'detect_leaks=0'
        for n in NAMES:
            if run(['timeout', '60', str(build / n)], log, env):
                failed.append(config + '/' + n)
        if config == 'inv':
            cmd = ['timeout', '30', 'cc', '-Iinclude', '-Itests', '-DADF_CHECK_INVARIANTS',
                   '-DADF_CHAR_EVAL_WRAP', '-std=c11', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                   'tests/test_char_eval.c']
            cmd += list(map(str, (build / 'support').glob('*.o')))
            cmd += [str(build / 'libadelefeld.a'), '-Wl,--wrap=dirichlet_group_init',
                    '-Wl,--wrap=adf_phase_get_acb', '-Wl,--wrap=arb_sqrt_ui',
                    '-lflint', '-lgmp', '-lm', '-pthread',
                    '-o', str(build / 'test_char_eval_wrap')]
            if run(cmd, log) or run(['timeout', '60', str(build / 'test_char_eval_wrap')], log):
                sys.exit(1)
print('FAILED', ', '.join(failed), flush=True)
sys.exit(bool(failed))
