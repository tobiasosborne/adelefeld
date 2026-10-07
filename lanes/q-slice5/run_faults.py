#!/usr/bin/env python3
"""Plant the faults of brief step F in scratch copies of src/psi.c and run test_psi_class and test_psi_local.

Run from the worktree root after `make BUILD=lanes/q-slice5/build lanes/q-slice5/build/test_psi_class
lanes/q-slice5/build/test_psi_local`. The scratch tree lanes/q-slice5/fault-scratch is a copy with the built
objects (timestamps kept), so only psi.o, the archive and the two tests are rebuilt per fault. Each test runs
under timeout 120 and a 4 GB address-space limit. The scratch tree is removed at the end.
"""
import shutil, subprocess, sys
from pathlib import Path

ROOT = Path('.').resolve()
S = ROOT/'lanes/q-slice5/fault-scratch'
PSI = (ROOT/'src/psi.c').read_text()

FAULTS = [
    ('F1l', 'local finite sign reversed (b = -fp_p)',
     '    fmpq_set_fmpz_frac(b, t, M);\n',
     '    fmpz_sub(t, M, t); fmpz_mod(t, t, M); fmpq_set_fmpz_frac(b, t, M);\n'),
    ('F3', 'ordinary fractional part at p',
     '    if (!fmpz_invmod(t, dd, M)) flint_abort();     /* p does not divide dd: unreachable */\n'
     '    fmpz_mul(t, t, n); fmpz_mod(t, t, M);\n    fmpq_set_fmpz_frac(b, t, M);\n',
     '    fmpz_mul(t, M, dd); fmpq_set_fmpz_frac(b, n, t); psi_mod1(b);\n'),
    ('F4', 'A roots instead of B (numerator of N in the class preflight)',
     '    if (psi_read(b, r, N, x) && psi_dists(d, b, r, fmpq_denref(N))) {\n        fmpz_set(B, fmpq_denref(N));',
     '    if (psi_read(b, r, N, x) && psi_dists(d, b, r, fmpq_numref(N))) {\n        fmpz_set(B, fmpq_numref(N));'),
    ('F4l', 'A roots instead of B at a prime (numerator of N)',
     '            (void) fmpz_remove(dd, fmpq_denref(N), pp);\n            fmpz_divexact(B, fmpq_denref(N), dd);',
     '            (void) fmpz_remove(dd, fmpq_numref(N), pp);\n            fmpz_divexact(B, fmpq_numref(N), dd);'),
    ('F5', 'end points only (distance to the nearest arc end even inside an arc)',
     '    if (fmpq_sgn(d) < 0) fmpq_zero(d);', '    if (fmpq_sgn(d) < 0) fmpq_neg(d, d);'),
    ('F6c', 'whole square for an ambiguous class',
     '        psi_fold(ext, upper, i == 0);\n    }\n    psi_commit(z, ext, p);',
     '        psi_fold(ext, upper, i == 0);\n    }\n    if (frac) { arf_set_si(ext, -1); arf_one(ext + 1);'
     ' arf_set_si(ext + 2, -1); arf_one(ext + 3); }\n    psi_commit(z, ext, p);'),
    ('F6l', 'whole square for an ambiguous local ball',
     '    psi_fold(ext, upper, 1); psi_commit(z, ext, p);\ndone:\n    PSI_CLEAR(d, upper, ext);\n    return st;',
     '    psi_fold(ext, upper, 1);\n    if (!fmpz_is_one(B)) { arf_set_si(ext, -1); arf_one(ext + 1);'
     ' arf_set_si(ext + 2, -1); arf_one(ext + 3); }\n    psi_commit(z, ext, p);\ndone:\n'
     '    PSI_CLEAR(d, upper, ext);\n    return st;'),
    ('X1', 'only the first stored entry evaluated',
     '    for (i = 0; i < x->len; i++) {\n        (void) psi_exact(d, B, x->piece + i);',
     '    for (i = 0; i < 1; i++) {\n        (void) psi_exact(d, B, x->piece + i);'),
    ('X2a', 'strict on a class decided from the first entry only',
     '        if (s == ADF_OK && !fmpz_is_one(B)) frac = 1;',
     '        if (i == 0 && s == ADF_OK && !fmpz_is_one(B)) frac = 1;'),
    ('X2b', 'strict on a class as a singleton image (rejected alternative of D3-3)',
     '        if (s == ADF_OK && !fmpz_is_one(B)) frac = 1;',
     '        if (s == ADF_OK && (!fmpz_is_one(B) || !mag_is_zero(arb_radref(x->piece[i].inf)))) frac = 1;'),
    ('X3', 'branch at e = 0 wrong (ball with e <= 0 taken as a root family of order p^(1-e))',
     '    if (!x->exact && x->N < 0) { if (!psi_ppow(B, x->p, (ulong) -x->N)) goto done; }',
     '    if (!x->exact && x->N < 1) { if (!psi_ppow(B, x->p, (ulong) (1 - x->N))) goto done; }'),
    ('X4', 'real place with the finite sign', '        fmpq_neg(b, b); psi_mod1(b);', '        psi_mod1(b);'),
    ('X5', 'where written on OK', '    return st == ADF_OK ? ADF_OK : psi_fail(st, where, v);',
     '    return psi_fail(st, where, v);'),
    ('X6', 'power formed before the bit check',
     '    if (k >= (ulong) ADF_QCLASS_BITS_MAX || (bp-1)*k+1 > (ulong) ADF_QCLASS_BITS_MAX) return 0;\n',
     '    (void) bp;\n'),
    ('X8', 'finite denominator cofactor not inverted',
     '    if (!fmpz_invmod(t, dd, M)) flint_abort();     /* p does not divide dd: unreachable */\n',
     '    fmpz_one(t); (void) dd;\n'),
]


def run(cmd, **kw):
    return subprocess.run(cmd, cwd=S, shell=True, capture_output=True, text=True, **kw)


def main():
    if S.exists():
        shutil.rmtree(S)
    S.mkdir()
    for d in ('include', 'src', 'tests/support', 'tests/golden', 'tests/ref/vectors/q-slice5'):
        shutil.copytree(ROOT/d, S/d)
    for f in ('Makefile', 'tests/test_psi_class.c', 'tests/test_psi_local.c'):
        shutil.copy2(ROOT/f, S/f)
    shutil.copytree(ROOT/'lanes/q-slice5/build', S/'build', ignore=shutil.ignore_patterns('*.so', 'test_*'))
    rows = []
    try:
        for fid, what, old, new in FAULTS:
            assert PSI.count(old) == 1, fid
            (S/'src/psi.c').write_text(PSI.replace(old, new))
            b = run('timeout 300 make -s -j2 BUILD=build build/test_psi_class build/test_psi_local')
            if b.returncode:
                rows.append((fid, what, 'NOT COMPILED', b.stderr.strip().splitlines()[-1:])); continue
            res = []
            for t in ('test_psi_class', 'test_psi_local'):
                r = run(f'ulimit -v 4000000; timeout 120 build/{t}')
                line = (r.stderr.strip().splitlines() or r.stdout.strip().splitlines() or [''])[-1]
                res.append(f'{t}: exit {r.returncode}: {line[:150]}')
            killed = any(not x.split(': exit ')[1].startswith('0:') for x in res)
            rows.append((fid, what, 'REJECTED' if killed else 'SURVIVED', res))
        (S/'src/psi.c').write_text(PSI)
        b = run('timeout 300 make -s -j2 BUILD=build build/test_psi_class build/test_psi_local')
        base = [run(f'timeout 120 build/{t}').returncode for t in ('test_psi_class', 'test_psi_local')]
        rows.append(('BASE', 'pristine src/psi.c', 'PASS' if base == [0, 0] and not b.returncode else 'FAIL',
                     [f'exits {base}']))
    finally:
        shutil.rmtree(S)
    for fid, what, verdict, res in rows:
        print(f'{fid}\t{verdict}\t{what}')
        for x in res:
            print(f'\t{x}')


main()
