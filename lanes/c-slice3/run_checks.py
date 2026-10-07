#!/usr/bin/env python3
"""Bounded, sequential configurations. Each build has at most two compiler jobs."""
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
os.chdir(ROOT)
LANE = Path('lanes/c-slice3')
RESULTS = LANE / 'checks.json'
results = json.loads(RESULTS.read_text()) if RESULTS.exists() else []
config = sys.argv[1]
options = {'plain': [], 'san': ['SAN=1'], 'inv': ['INV=1'], 'clang': ['CC=clang']}[config]
build = LANE / config
env = os.environ.copy()
env['ASAN_OPTIONS'] = 'detect_leaks=0'
env['OMP_NUM_THREADS'] = '1'
tests = ['test_char_class'] if len(sys.argv) > 2 and sys.argv[2] == 'class-only' else [
    'test_char', 'test_char_eval', 'test_char_class']


def run(cmd, label):
    with (LANE / (config+'.log')).open('a') as log:
        log.write('$ '+' '.join(cmd)+'\n')
        p = subprocess.run(cmd, capture_output=True, text=True, env=env)
        log.write(p.stdout+p.stderr+'EXIT '+str(p.returncode)+'\n')
    results.append({'config': config, 'step': label, 'command': cmd, 'exit': p.returncode,
                    'output': p.stdout+p.stderr})
    RESULTS.write_text(json.dumps(results, indent=2)+'\n')
    print(config, label, p.returncode, p.stdout.strip(), flush=True)
    if p.returncode:
        print(p.stderr[-2500:])
        sys.exit(1)


run(['timeout', '120', 'make', '-s', '-j2', 'BUILD='+str(build)] + options +
    [str(build / t) for t in tests], 'build')
for test in tests:
    run(['timeout', '60', str(build / test)], test)
cc = 'clang' if config == 'clang' else 'cc'
flags = ['-DADF_CHECK_INVARIANTS'] if config == 'inv' else []
if config == 'san':
    flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
wraps = ['dirichlet_group_init', 'adf_phase_get_acb', 'arb_pow', 'acb_pow', 'acb_mul',
         'adf_idclass_set_idele']
exe = build / 'test_char_class_wrap'
run(['timeout', '30', cc, '-Iinclude', '-Itests', '-std=c11', '-O2', '-g', '-Wall', '-Wextra',
     '-Wpedantic', '-Werror', '-DADF_CHAR_CLASS_WRAP'] + flags + ['tests/test_char_class.c'] +
    [str(p) for p in (build / 'support').glob('*.o')] + [str(build / 'libadelefeld.a')] +
    ['-Wl,--wrap='+w for w in wraps] + ['-lflint', '-lgmp', '-lm', '-pthread', '-o', str(exe)], 'wrap-build')
run(['timeout', '60', str(exe)], 'test_char_class_wrap')
