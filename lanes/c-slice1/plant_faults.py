#!/usr/bin/env python3
"""Compile independent scratch faults; run only the complete test_char under timeout."""
from pathlib import Path
import os
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
LANE = ROOT / 'lanes/c-slice1'
BASE = (ROOT / 'src/char.c').read_text()
FAULTS = [
    ('33_lower_label', 'label = dirichlet_char_exp(H, d);', 'label = n % C;'),
    ('33_parity', 'e = dirichlet_parity_char(H, d);', 'e = n % 2;'),
    ('33_zero_flag', '*zero = k == DIRICHLET_CHI_NULL;', '*zero = 0;\n    if (k == DIRICHLET_CHI_NULL) k = 0;'),
    ('34_negative_sign', 'fmpq_set_ui(additive, a, chi->q);',
     'fmpq_set_ui(additive, (chi->q-a) % chi->q, chi->q);'),
    ('34_conjugate_chi', 'dirichlet_char_log(c, G, chi->n);\n    w = char_work',
     'dirichlet_char_log(c, G, n_invmod(chi->n, chi->q));\n    w = char_work'),
    ('34_omit_principal', 'if (chi->q == 1) { acb_one(tau); return ADF_OK; }',
     'if (chi->q == 1) { acb_zero(tau); return ADF_OK; }'),
    ('34_omit_rotation', 'if (chi->parity) acb_div_onei(out, out);', '(void)chi->parity;'),
    ('34_input_modulus', 'fmpq_set_ui(additive, a, chi->q);',
     'fmpq_set_ui(additive, a, chi->q == 4 ? 8 : chi->q);'),
    ('34_character_order', 'fmpq_set_ui(theta, k, G->expo);',
     'fmpq_set_ui(theta, k, dirichlet_order_char(G, c));\n'
     '    fmpz_fdiv_r(fmpq_numref(theta), fmpq_numref(theta), fmpq_denref(theta));\n'
     '    fmpq_canonicalise(theta);'),
    ('extra_early_write', 'if (q == 0 || !acb_is_finite(s)',
     'x->n = n;\n    if (q == 0 || !acb_is_finite(s)'),
]
# The cap comparison moves behind setup without leaking the scratch group.
late = BASE.replace('if (q > ADF_CHAR_MOD_MAX) return ADF_LIMIT;',
                    'if (q > ADF_CHAR_MOD_MAX) {\n'
                    '        dirichlet_group_t probe;\n'
                    '        if (dirichlet_group_init(probe, q)) dirichlet_group_clear(probe);\n'
                    '        return ADF_LIMIT;\n    }')
FAULTS.append(('extra_late_cap', None, late))


def run(args):
    return subprocess.run(['timeout', '30', *args], cwd=ROOT, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


scratch = LANE / 'faults'
scratch.mkdir(exist_ok=True)
rows = []
for name, old, new in FAULTS:
    if old is not None:
        assert old in BASE, (name, old)
        source = BASE.replace(old, new, 1)
    else:
        source = new
    path = scratch / (name + '.c')
    path.write_text(source)
    binary = scratch / name
    command = ['cc', '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
               '-DADF_CHAR_WRAP_SETUP', '-Iinclude', '-Isrc', '-Itests', 'tests/test_char.c', str(path),
               'lanes/c-slice1/build/support/jsonl.o', 'lanes/c-slice1/build/support/golden.o',
               'lanes/c-slice1/build/libadelefeld.a', '-Wl,--wrap=dirichlet_group_init',
               '-Wl,--wrap=adf_phase_get_acb', '-Wl,--wrap=arb_sqrt_ui',
               '-lflint', '-lgmp', '-lm', '-o', str(binary)]
    build = run(command)
    assert build.returncode == 0, (name, build.stdout)
    result = run([str(binary)])
    lines = [line for line in result.stdout.splitlines() if line.startswith('test_char:')]
    row = {'fault': name, 'build': build.returncode, 'test': result.returncode,
           'failure': lines[-1] if lines else result.stdout[-200:]}
    rows.append(row)
    print(row, flush=True)
    assert result.returncode != 0 and result.returncode != 124, (name, result.stdout)
(LANE / 'fault-results.txt').write_text('\n'.join(str(row) for row in rows) + '\n')
print('FAULTS', len(rows), 'ALL_REJECTED', flush=True)
