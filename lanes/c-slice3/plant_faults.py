#!/usr/bin/env python3
"""Six required faults in scratch copies. The working source is never mutated."""
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
os.chdir(ROOT)
LANE = Path('lanes/c-slice3')
SCRATCH = LANE / 'faults'
SCRATCH.mkdir(exist_ok=True)
source = Path('src/char.c').read_text()
faults = [
    ('drop_real_sign', 'if (st == ADF_OK) st = char_class(z, chi, c, prec, strict);',
     'if (st == ADF_OK) { adf_ucoset_set(&c->u, &x->u); st = char_class(z, chi, c, prec, strict); }'),
    ('conjugate_s', 'acb_set_arb(base, x->t); acb_pow(power, base, chi->s, p);',
     'acb_set_arb(base, x->t); acb_conj(out, chi->s); acb_pow(power, base, out, p);'),
    ('omit_content', 'if (st == ADF_OK) st = char_class(z, chi, c, prec, strict);',
     'if (st == ADF_OK) { arb_abs(c->t, x->inf); st = char_class(z, chi, c, prec, strict); }'),
    ('strict_t_only', 'st = char_unit(unit, chi, &x->u, prec, strict);',
     '(void) strict; st = char_unit(unit, chi, &x->u, prec, 0);'),
    ('one_corner', 'acb_mul(out, power, unit, p);',
     'arb_get_ubound_arf(arb_midref(acb_realref(unit)), acb_realref(unit), p);\n'
     '            arb_get_ubound_arf(arb_midref(acb_imagref(unit)), acb_imagref(unit), p);\n'
     '            mag_zero(arb_radref(acb_realref(unit))); mag_zero(arb_radref(acb_imagref(unit)));\n'
     '            acb_mul(out, power, unit, p);'),
    ('early_write', 'if (!acb_is_finite(power)) st = ADF_NOT_DETERMINED;',
     'acb_set(z, power); if (!acb_is_finite(power)) st = ADF_NOT_DETERMINED;'),
]
wraps = ['dirichlet_group_init', 'adf_phase_get_acb', 'arb_pow', 'acb_pow', 'acb_mul',
         'adf_idclass_set_idele']
results = []
with (LANE / 'faults.log').open('w') as log:
    def run(cmd):
        p = subprocess.run(cmd, capture_output=True, text=True)
        log.write('$ '+' '.join(cmd)+'\n'+p.stdout+p.stderr+'EXIT '+str(p.returncode)+'\n')
        return p.returncode
    for name, old, new in faults:
        assert source.count(old) == 1, name
        path, obj, exe = [SCRATCH / (name+suffix) for suffix in ('.c', '.o', '.exe')]
        path.write_text(source.replace(old, new))
        compiled = run(['timeout', '30', 'cc', '-Iinclude', '-Isrc', '-std=c11', '-O2', '-Wall',
                        '-Wextra', '-Wpedantic', '-Werror', '-c', str(path), '-o', str(obj)])
        assert compiled == 0, name
        linked = run(['timeout', '30', 'cc', '-Iinclude', '-Itests', '-std=c11', '-O2',
                      '-DADF_CHAR_CLASS_WRAP', 'tests/test_char_class.c', str(obj)] +
                     [str(p) for p in (LANE / 'plain/support').glob('*.o')] +
                     [str(LANE / 'plain/libadelefeld.a')] + ['-Wl,--wrap='+w for w in wraps] +
                     ['-lflint', '-lgmp', '-lm', '-o', str(exe)])
        assert linked == 0, name
        tested = run(['timeout', '30', str(exe)])
        results.append({'fault': name, 'compile': compiled, 'link': linked, 'test': tested})
        print(name, compiled, linked, tested, flush=True)
        assert tested not in (0, 124), name
(LANE / 'fault_results.json').write_text(json.dumps(results, indent=2)+'\n')
shutil.rmtree(SCRATCH)
