#!/usr/bin/env python3
"""Only rerun already selected mutations. No new sweep candidates."""
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
os.chdir(ROOT)
LANE = Path('lanes/c-slice2')
SCRATCH = LANE / 'survivors'
SCRATCH.mkdir(exist_ok=True)
base = (LANE / 'mut-base')
source = Path('src/char.c').read_text()
mutations = [
    ('nonfinite', 'if (!acb_is_finite(z)) return 0;', 'if (!acb_is_finite(z)) return 1;', 30, -6),
    ('principal_setup', 'if (x->q != 1) {', 'if (x->q != 0) {', 30, -6),
    ('cosine_width', 'acb_realref(point)), -p-1)', 'acb_realref(point)), -p-0)', 30, -6),
    ('distance_sign', 'fmpq_sub(distance, distance, theta);', 'fmpq_add(distance, distance, theta);', 30, -6),
    ('infinite_macro', 'while (0)', 'while (!(0))', 2, 124),
]
results = []
with (LANE / 'survivors.log').open('w') as log:
    for name, old, new, seconds, want in mutations:
        # The macro spelling is different from ordinary while statements.
        if name == 'infinite_macro':
            if old not in source:
                old, new = old.replace(' ', ''), new.replace(' ', '')
        assert source.count(old) == 1, (name, source.count(old))
        path = SCRATCH / (name+'.c'); obj = SCRATCH / (name+'.o'); exe = SCRATCH / name
        path.write_text(source.replace(old, new))
        flags = ['-Iinclude', '-Isrc', '-Itests', '-std=c11', '-O2', '-g', '-Wall', '-Wextra',
                 '-Wpedantic', '-Werror', '-DADF_CHECK_INVARIANTS', '-fsanitize=address,undefined',
                 '-fno-omit-frame-pointer']
        cmd = ['timeout', '30', 'cc'] + flags + ['-c', str(path), '-o', str(obj)]
        comp = subprocess.run(cmd, capture_output=True, text=True)
        log.write('$ '+' '.join(cmd)+'\n'+comp.stdout+comp.stderr); assert comp.returncode == 0
        cmd = ['timeout', '30', 'cc'] + flags + ['-DADF_CHAR_EVAL_WRAP', 'tests/test_char_eval.c', str(obj)]
        cmd += list(map(str, (base / 'support').glob('*.o')))
        cmd += [str(base / 'libadelefeld.a'), '-Wl,--wrap=dirichlet_group_init',
                '-Wl,--wrap=adf_phase_get_acb', '-Wl,--wrap=arb_sqrt_ui', '-lflint', '-lgmp', '-lm',
                '-pthread', '-o', str(exe)]
        link = subprocess.run(cmd, capture_output=True, text=True)
        log.write('$ '+' '.join(cmd)+'\n'+link.stdout+link.stderr); assert link.returncode == 0
        cmd = ['timeout', str(seconds), str(exe)]
        result = subprocess.run(cmd, capture_output=True, text=True,
                                env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
        log.write('$ '+' '.join(cmd)+'\n'+result.stdout+result.stderr+'EXIT '+str(result.returncode)+'\n')
        results.append({'survivor':name,'compile':0,'link':0,'exit':result.returncode})
        print(results[-1], flush=True); assert result.returncode == want, (name, result.returncode, want)
(LANE / 'survivor_results.json').write_text(json.dumps(results,indent=2)+'\n')
shutil.rmtree(SCRATCH)
