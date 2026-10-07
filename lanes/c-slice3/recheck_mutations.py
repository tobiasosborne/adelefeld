#!/usr/bin/env python3
"""Finish selected batch 3, or test fifteen generated slice c candidates; total <=45 distinct."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
os.chdir(ROOT)
sys.path.insert(0, str(ROOT / 'tools/mutate'))
import mutate

LANE = Path('lanes/c-slice3')
slice_c = len(sys.argv) > 1 and sys.argv[1] == 'slice-c'
survivors_only = len(sys.argv) > 1 and sys.argv[1] == 'survivors'
radius_only = len(sys.argv) > 1 and sys.argv[1] == 'radius'
suffix = '-c' if slice_c else '-s' if survivors_only else '-r' if radius_only else ''
SCRATCH = LANE / ('rechecks'+suffix)
SCRATCH.mkdir(exist_ok=True)
source = Path('src/char.c').read_text()
available = {str(m): m for m in mutate.mutants_of('src/char.c', source, str(ROOT))}
selection = [line for line in (LANE / 'mutants-3.txt').read_text().splitlines()
             if line.startswith('src/char.c:')]
for batch, line in [(1, ':174:'), (2, ':71:')]:
    selection += [s for s in (LANE / ('mutants-'+str(batch)+'.txt')).read_text().splitlines()
                  if s.startswith('src/char.c:') and line in s]
assert len(selection) == 12
if survivors_only:
    selection = selection[-2:]
if radius_only:
    selection = selection[-2:-1]
if slice_c:
    wanted = {361: ('cmp', 'status'), 365: ('drop_call',), 370: ('negate_if',),
              376: ('negate_if',), 379: ('negate_if',), 386: ('zero_one',), 388: ('zero_one',),
              396: ('cmp', 'status'), 400: ('drop_call',), 407: ('zero_one',), 409: ('zero_one',)}
    selection = [key for key, m in available.items() if m.kind in wanted.get(m.line, ())]
    assert len(selection) == 15
base = LANE / 'mut-base'
flags = ['-Iinclude', '-Isrc', '-Itests', '-std=c11', '-O2', '-g', '-Wall', '-Wextra',
         '-Wpedantic', '-Werror', '-DADF_CHECK_INVARIANTS', '-fsanitize=address,undefined',
         '-fno-omit-frame-pointer']
tests = [('test_char_class', 'ADF_CHAR_CLASS_WRAP',
          ['dirichlet_group_init', 'adf_phase_get_acb', 'arb_pow', 'acb_pow', 'acb_mul',
           'adf_idclass_set_idele']),
         ('test_char', 'ADF_CHAR_WRAP_SETUP', ['dirichlet_group_init', 'adf_phase_get_acb', 'arb_sqrt_ui']),
         ('test_char_eval', 'ADF_CHAR_EVAL_WRAP', ['dirichlet_group_init', 'adf_phase_get_acb', 'arb_sqrt_ui'])]
results = []
with (LANE / ('rechecks'+suffix+'.log')).open('w') as log:
    def run(cmd):
        p = subprocess.run(cmd, capture_output=True, text=True,
                           env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
        log.write('$ '+' '.join(cmd)+'\n'+p.stdout+p.stderr+'EXIT '+str(p.returncode)+'\n')
        return p.returncode
    for j, key in enumerate(selection):
        m = available[key]
        path, obj, exe = [SCRATCH / (str(j)+s) for s in ('.c', '.o', '.exe')]
        path.write_text(m.apply(source))
        code = run(['timeout', '30', 'cc'] + flags + ['-c', str(path), '-o', str(obj)])
        row = {'mutant': key, 'compile': code, 'tests': []}
        if code:
            row['status'] = 'not compiled'
        else:
            row['status'] = 'survived'
            for test, define, wraps in tests:
                cmd = ['timeout', '30', 'cc'] + flags + ['-D'+define, 'tests/'+test+'.c', str(obj)]
                cmd += [str(p) for p in (base / 'support').glob('*.o')]
                cmd += [str(base / 'libadelefeld.a')] + ['-Wl,--wrap='+w for w in wraps]
                cmd += ['-lflint', '-lgmp', '-lm', '-pthread', '-o', str(exe)]
                link = run(cmd)
                assert link == 0, (key, test, link)
                tested = run(['timeout', '60', str(exe)])
                row['tests'].append({'test': test, 'link': link, 'exit': tested})
                if tested:
                    row['status'] = 'timed out' if tested == 124 else 'killed'
                    break
        results.append(row)
        print(key, row['status'], flush=True)
        (LANE / ('recheck_results'+suffix+'.json')).write_text(json.dumps(results, indent=2)+'\n')
shutil.rmtree(SCRATCH)
