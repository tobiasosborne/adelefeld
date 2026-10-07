#!/usr/bin/env python3
"""One isolated source fault per executable. The working sources are never changed."""
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
os.chdir(ROOT)
LANE = Path('lanes/c-slice2')
SCRATCH = LANE / 'faults'
SCRATCH.mkdir(exist_ok=True)
SOURCES = {name: Path('src', name+'.c').read_text() for name in ('char', 'dump')}
faults = [
    ('lowering', 'char', 'label = dirichlet_char_exp(H, d);', 'label = n % C;'),
    ('parity', 'char', 'e = dirichlet_parity_char(H, d);', 'e = (int) (n % 2);'),
    ('zero', 'char', '*zero = k == DIRICHLET_CHI_NULL;',
     '*zero = 0; if (k == DIRICHLET_CHI_NULL) k = 0;'),
    ('strict_write', 'char', 'if (strict) return ADF_NOT_DETERMINED;',
     'if (strict) { acb_zero(z); return ADF_NOT_DETERMINED; }'),
    ('first_last_hull', 'char', 'if (a % g == residue && n_gcd(a, C) == 1)',
     'if ((a == 1 || a == C-1) && a % g == residue && n_gcd(a, C) == 1)'),
    ('two_extrema', 'char', 'arf_neg(lo, upper+3); arf_set(hi, upper+2);',
     'arf_neg(lo, upper+1); arf_set(hi, upper);'),
    ('q_minus_n', 'char', 'label = _dirichlet_char_exp(c, G);', 'label = x->q-x->n;'),
    ('strict_exact_only', 'char', 'if (strict) return ADF_NOT_DETERMINED;',
     'if (strict && !fmpz_is_zero(u->N)) return ADF_NOT_DETERMINED;'),
    ('incompatible_lift', 'char', 'g = n_gcd(fmpz_fdiv_ui(u->N, C), C); residue = fmpz_fdiv_ui(u->c, g);',
     'return adf_char_chi(z, chi, u->c, prec);\n'
     '    g = n_gcd(fmpz_fdiv_ui(u->N, C), C); residue = fmpz_fdiv_ui(u->c, g);'),
    ('imprimitive_dump', 'dump', 'if (!primitive) return dp_fail(st, ADF_DOMAIN);',
     '(void) primitive;'),
    ('early_dump_write', 'dump', 'int st = dp_prepare(&P, DP_CHAR, s, len, binds, dp_count_of(nbinds), lim);',
     'x->q = 1;\n    int st = dp_prepare(&P, DP_CHAR, s, len, binds, dp_count_of(nbinds), lim);'),
]
# Strict's C|N branch must be removed too to make the exact-only decision genuinely faulty.
results = []
with (LANE / 'faults.log').open('w') as log:
    for name, kind, old, new in faults:
        source = SOURCES[kind]
        assert source.count(old) == 1, (name, source.count(old))
        source = source.replace(old, new)
        if name == 'strict_exact_only':
            source = source.replace('if (fmpz_is_zero(u->N) || fmpz_fdiv_ui(u->N, C) == 0)',
                                    'if (fmpz_is_zero(u->N) || (!strict && fmpz_fdiv_ui(u->N, C) == 0))')
        path = SCRATCH / (name+'.c')
        path.write_text(source)
        obj = SCRATCH / (name+'.o')
        cmd = ['timeout', '30', 'cc', '-Iinclude', '-Isrc', '-std=c11', '-O2', '-Wall', '-Wextra',
               '-Wpedantic', '-Werror', '-c', str(path), '-o', str(obj)]
        compiled = subprocess.run(cmd, capture_output=True, text=True)
        log.write('$ '+' '.join(cmd)+'\n'+compiled.stdout+compiled.stderr)
        codes = [compiled.returncode]
        if not compiled.returncode:
            for test in ('test_char', 'test_char_eval'):
                exe = SCRATCH / (name+'-'+test)
                cmd = ['timeout', '30', 'cc', '-Iinclude', '-Itests', '-std=c11', '-O2',
                       'tests/'+test+'.c', str(obj)]
                cmd += list(map(str, (LANE / 'plain/support').glob('*.o')))
                cmd += [str(LANE / 'plain/libadelefeld.a'), '-lflint', '-lgmp', '-lm', '-o', str(exe)]
                link = subprocess.run(cmd, capture_output=True, text=True)
                log.write('$ '+' '.join(cmd)+'\n'+link.stdout+link.stderr)
                assert link.returncode == 0, (name, link.stderr)
                cmd = ['timeout', '30', str(exe)]
                run = subprocess.run(cmd, capture_output=True, text=True)
                log.write('$ '+' '.join(cmd)+'\n'+run.stdout+run.stderr+'EXIT '+str(run.returncode)+'\n')
                codes.append(run.returncode)
        results.append({'fault': name, 'compile': codes[0], 'tests': codes[1:]})
        print(name, codes, flush=True)
        assert codes[0] == 0 and any(c != 0 and c != 124 for c in codes[1:]), (name, codes)
(LANE / 'fault_results.json').write_text(json.dumps(results, indent=2)+'\n')
shutil.rmtree(SCRATCH)
