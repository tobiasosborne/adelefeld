#!/usr/bin/env python3
"""Recheck the mutation survivors of the restricted sweep (test_psi_class, test_psi_local) against tests/test_psi.c
and the two new tests together. Each mutant is applied as an exact one-line replacement (from mutate.log) in a
scratch copy, lanes/q-slice5/recheck-scratch, removed at the end. Run from the worktree root."""
import shutil, subprocess
from pathlib import Path
ROOT = Path('.').resolve(); S = ROOT/'lanes/q-slice5/recheck-scratch'
PSI = (ROOT/'src/psi.c').read_text()
M = [
 ('186', '    if (!fmpq_is_zero(u) && !fmpz_is_one(B)) {', '    if (!fmpq_is_zero(u) || !fmpz_is_one(B)) {'),
 ('52', '    return b <= ADF_QCLASS_BITS_MAX && FLINT_MAX(e, b) <= ADF_QCLASS_BITS_MAX &&',
        '    return b <= ADF_QCLASS_BITS_MAX && FLINT_MAX(e, b) < ADF_QCLASS_BITS_MAX &&'),
 ('53', '           (e >= b ? 1 : b-e+1) <= ADF_QCLASS_BITS_MAX;', '           (e >= b ? 1 : b-e+1) < ADF_QCLASS_BITS_MAX;'),
 ('124', 'v >>= 1; fmpz_add_ui(MAG_EXPREF(arb_radref(z)), MAG_EXPREF(arb_radref(z)), 1);',
         'v >>= 1; fmpz_add_ui(MAG_EXPREF(arb_radref(z)), MAG_EXPREF(arb_radref(z)), 0);'),
 ('10', '    return fmpz_bits(fmpq_numref(q)) <= ADF_QCLASS_BITS_MAX &&',
        '    return fmpz_bits(fmpq_numref(q)) <= ADF_QCLASS_BITS_MAX ||'),
 ('154', '{ acb_one(z); return ADF_OK; }', '{  return ADF_OK; }'),
 ('290', '    st = ADF_NOT_DETERMINED;\n    if (strict && !fmpz_is_one(B)) goto done;\n    if ((st = psi_numeric(upper, d, p)) != ADF_OK) goto done;\n    psi_fold(ext, upper, 1); psi_commit(z, ext, p);\ndone:\n    PSI_CLEAR(d, upper, ext); fmpz_clear(B);',
         '    if (strict && !fmpz_is_one(B)) goto done;\n    if ((st = psi_numeric(upper, d, p)) != ADF_OK) goto done;\n    psi_fold(ext, upper, 1); psi_commit(z, ext, p);\ndone:\n    PSI_CLEAR(d, upper, ext); fmpz_clear(B);'),
 ('161', '        fmpz_bits(fmpq_numref(theta))+1 > ADF_QCLASS_BITS_MAX) return ADF_LIMIT;',
         '        fmpz_bits(fmpq_numref(theta))-1 > ADF_QCLASS_BITS_MAX) return ADF_LIMIT;'),
]
def run(c): return subprocess.run(c, cwd=S, shell=True, capture_output=True, text=True)
if S.exists(): shutil.rmtree(S)
S.mkdir()
for d in ('include', 'src', 'tests/support', 'tests/golden', 'tests/ref/vectors'):
    shutil.copytree(ROOT/d, S/d)
for f in ('Makefile', 'tests/test_psi.c', 'tests/test_psi_class.c', 'tests/test_psi_local.c'):
    shutil.copy2(ROOT/f, S/f)
shutil.copytree(ROOT/'lanes/q-slice5/build', S/'build', ignore=shutil.ignore_patterns('*.so', 'test_*'))
try:
    for mid, old, new in M:
        assert PSI.count(old) >= 1, mid  # the first occurrence is the mutated line
        (S/'src/psi.c').write_text(PSI.replace(old, new, 1))
        b = run('timeout 300 make -s -j2 BUILD=build build/test_psi')
        r = run('timeout 150 build/test_psi')
        last = (r.stderr.strip().splitlines() or r.stdout.strip().splitlines() or [''])[-1]
        print(f'psi.c:{mid}\t{"KILLED" if b.returncode or r.returncode else "SURVIVED"}\t{last[:120]}')
    (S/'src/psi.c').write_text(PSI)
    run('timeout 300 make -s -j2 BUILD=build build/test_psi')
    print('BASE test_psi exit', run('timeout 150 build/test_psi').returncode)
finally:
    shutil.rmtree(S)
