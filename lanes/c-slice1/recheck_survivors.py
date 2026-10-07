#!/usr/bin/env python3
"""Recheck the three test gaps, with SAN+INV and the complete test_char only."""
from pathlib import Path
import os
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
LANE = ROOT / 'lanes/c-slice1'
base = (ROOT / 'src/char.c').read_text()
changes = [
    ('swap_entry', '    ADF_INV_CHAR(x); ADF_INV_CHAR(y);\n    t = x->q;',
     '    ADF_INV_CHAR(y);\n    t = x->q;'),
    ('both_radii', 'ok = ok && arf_cmp(r, bound) <= 0;', 'ok = ok || arf_cmp(r, bound) <= 0;'),
    ('phase_cap', 'if (chi->q > ADF_CHAR_MOD_MAX) return ADF_LIMIT;',
     'if (chi->q >= ADF_CHAR_MOD_MAX) return ADF_LIMIT;'),
]
scratch = LANE / 'survivors'
scratch.mkdir(exist_ok=True)
rows = []
started = time.monotonic()
for name, old, new in changes:
    assert base.count(old) == 1
    path = scratch / (name + '.c')
    path.write_text(base.replace(old, new, 1))
    binary = scratch / name
    command = ['timeout', '30', 'cc', '-std=c11', '-O2', '-g', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
               '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-DADF_CHECK_INVARIANTS',
               '-DADF_CHAR_WRAP_SETUP', '-Iinclude', '-Isrc', '-Itests', 'tests/test_char.c', str(path),
               'lanes/c-slice1/san-inv/support/jsonl.o', 'lanes/c-slice1/san-inv/support/golden.o',
               'lanes/c-slice1/san-inv/libadelefeld.a', '-Wl,--wrap=dirichlet_group_init',
               '-Wl,--wrap=adf_phase_get_acb', '-Wl,--wrap=arb_sqrt_ui',
               '-lflint', '-lgmp', '-lm', '-pthread', '-o', str(binary)]
    build = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    assert build.returncode == 0, build.stdout
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
    test = subprocess.run(['timeout', '60', str(binary)], cwd=ROOT, env=env,
                          text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    assert test.returncode not in (0, 124), (name, test.stdout)
    row = f'{name}: build={build.returncode}, test={test.returncode}; {test.stdout.strip()[-250:]}'
    rows.append(row)
    print(row, flush=True)
rows.append('seconds=' + str(round(time.monotonic()-started, 3)))
(LANE / 'survivor-results.txt').write_text('\n'.join(rows) + '\n')
