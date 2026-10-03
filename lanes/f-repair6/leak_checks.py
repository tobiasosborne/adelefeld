#!/usr/bin/env python3
"""Valgrind with documented FLINT cache cleanup. No assertion is disabled."""
import subprocess
from pathlib import Path
import sys

lane = Path('lanes/f-repair6')
build = lane/'build'
test = sys.argv[1]
assert test in ('test_lpow', 'test_lroot', 'test_rfunc_prime')
obj = build/(test+'.cleanup.o')
exe = build/(test+'-cleanup')
flags = ['-std=c11', '-O2', '-g', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-Iinclude', '-Itests']
subprocess.run(['timeout', '60', 'cc', *flags, '-Dmain=repair6_tests_main', '-c',
                'tests/'+test+'.c', '-o', str(obj)], check=True)
subprocess.run(['timeout', '60', 'cc', *flags, str(lane/'cleanup_main.c'), str(obj),
                str(build/'support/golden.o'), str(build/'support/jsonl.o'),
                str(build/'libadelefeld.a'), '-lflint', '-lgmp', '-lm', '-o', str(exe)], check=True)
log = lane/('valgrind-clean-'+test+'.log')
with log.open('w') as f:
    result = subprocess.run(['timeout', '170', 'valgrind', '--leak-check=full',
                             '--show-leak-kinds=all', '--errors-for-leak-kinds=definite,indirect,possible',
                             '--error-exitcode=99', str(exe)], stdout=f, stderr=subprocess.STDOUT)
print(f'{test}: exit={result.returncode}; {log}', flush=True)
(lane/('valgrind-clean-'+test+'.status')).write_text(str(result.returncode)+'\n')
