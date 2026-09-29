#!/usr/bin/env python3
"""Plant faults only in lane copies, then run the unchanged author's differential harness."""
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
LANE = Path(__file__).resolve().parent
source = (ROOT / 'src/roots.c').read_text()
objects = sorted(str(p) for p in (LANE / 'pic').glob('*.o') if p.name != 'roots.o')
faults = {
    'missing_root': ('nmod_poly_clear(d);\n    return m;',
                     'nmod_poly_clear(d);\n    return m > 0 ? m - 1 : 0;'),
    'small_gcd': ('nmod_poly_gcd(d, h, t);',
                  'nmod_poly_gcd(d, h, t);\n    nmod_poly_one(d);'),
    'reduced_flag': ('T->reduced = reduced;', 'T->reduced = pu > 1031 ? 0 : reduced;'),
    'shared_reduced': ('*reduced = fmpz_poly_degree(h) > 0;',
                       '*reduced = fmpz_poly_degree(h) > 0 && FLINT_ABS(fmpz_poly_max_bits(f)) < 512;'),
}


def run(cmd, output, expected=0):
    with open(LANE / output, 'w') as log:
        result = subprocess.run(['timeout', '90'] + cmd, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
    print('command:', ' '.join(cmd), 'exit:', result.returncode, 'log:', output, flush=True)
    assert result.returncode == expected


for name, (old, new) in faults.items():
    assert source.count(old) == 1
    (LANE / f'{name}.c').write_text(source.replace(old, new))
    run(['cc', '-std=c11', '-O2', '-g', '-fPIC', '-Iinclude', '-c', str(LANE / f'{name}.c'),
         '-o', str(LANE / f'{name}.o')], name + '-compile.log')
    so = str(LANE / f'{name}.so')
    run(['cc', '-shared', '-o', so, str(LANE / f'{name}.o')] + objects + ['-lflint', '-lgmp', '-lm'],
        name + '-link.log')
    run(['python3', 'tests/fuzz/diff_roots_padic.py', '--lib', so, '--seconds', '15', '--seed', '294'],
        name + '-fuzz.log', 0 if name == 'shared_reduced' else 1)

# The original code and its harness are tested separately from these three altered libraries.
run(['cc', '-shared', '-o', str(LANE / 'original.so')] +
    sorted(str(p) for p in (LANE / 'pic').glob('*.o')) + ['-lflint', '-lgmp', '-lm'], 'original-link.log')
run(['python3', 'tests/fuzz/diff_roots_padic.py', '--lib', str(LANE / 'original.so'),
     '--seconds', '15', '--seed', '294'], 'original-fuzz.log')
