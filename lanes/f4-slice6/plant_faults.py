#!/usr/bin/env python3
"""Planted faults of lane f4-slice6 (brief step F): each fault is a textual change of src/poisson.c (one or more
replacements) in a scratch copy of the tree; test_poisson must fail (exit status not 0, or killed). Run from the
repository root:
    timeout 3000 python3 -B lanes/f4-slice6/plant_faults.py SCRATCH_DIR [fault numbers]
SCRATCH_DIR must be outside the repository; it is removed at the end."""
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FAULTS = [
    ('faults_45 a / F: the tail bound without the factor 2 of Lemma 6',
     [('        arb_mul_2exp_si(B, B, 1);                         /* the two sides', '        /* the two sides')]),
    ('faults_45 b: reverse only the real transform sign (phihat(-n/M))',
     [('        fmpq_set_si(q, n, f->M);', '        fmpq_set_si(q, -n, f->M);')]),
    ('faults_45 c / F: q = 0 (n = 0) omitted on both sides',
     [('            st = pn_charge(w, ul) ? ADF_OK : ADF_LIMIT;',
       '            if (j == 0 && n == 0) continue;\n            st = pn_charge(w, ul) ? ADF_OK : ADF_LIMIT;'),
      ('        st = pn_charge(w, ur) ? ADF_OK : ADF_LIMIT;',
       '        if (n == 0) continue;\n        st = pn_charge(w, ur) ? ADF_OK : ADF_LIMIT;')]),
    ('faults_45 d: right spacing 1/D', [('        fmpq_set_si(q, n, f->M);', '        fmpq_set_si(q, n, f->D);')]),
    ('faults_45 e: omitted n > N bounded by S(N + 1)', [('    ulong K = N + 1;', '    ulong K = N + 2;')]),
    ('faults_45 f: beta dropped from the shifted tail bound',
     [('    arb_set_arf(beta, b);', '    arb_zero(beta);')]),
    ('F: the ratio rho not certified (geometric bound at K = N + 1 always)',
     [('        if (arf_is_finite(b) && arf_cmp_2exp_si(b, -1) <= 0)', '        if (1)')]),
    ('F: the tail added to the real coordinate only',
     [('        acb_add_error_mag(lsum, m);', '        arb_add_error_mag(acb_realref(lsum), m);'),
      ('        acb_add_error_mag(rsum, m);', '        arb_add_error_mag(acb_realref(rsum), m);')]),
    ('F: right derived from left',
     [('    if (st == ADF_OK)\n    {\n        arb_get_mag(m, EL);',
       '    acb_set(rsum, lsum);\n    if (st == ADF_OK)\n    {\n        arb_get_mag(m, EL);')]),
    ('F: the width checked before the tail is added',
     [('#define PN_STALL_PREC 64\n', '#define PN_STALL_PREC 64\nstatic mag_struct pn_fl[1], pn_fr[1];\n'),
      ('        arb_get_mag(m, EL);\n        acb_add_error_mag(lsum, m);                       /* P1 step 4: both '
       'coordinates */\n        arb_get_mag(m, ER);\n        acb_add_error_mag(rsum, m);',
       '        arb_get_mag(pn_fl, EL);\n        arb_get_mag(pn_fr, ER);'),
      ('            if (mag_cmp_2exp_si(d, -bits - 1) <= 0)\n                break;',
       '            if (mag_cmp_2exp_si(d, -bits - 1) <= 0)\n'
       '            { acb_add_error_mag(l, pn_fl); acb_add_error_mag(r, pn_fr); break; }')]),
    ('F: NL reported as the number of terms', [('        *NL = nl;', '        *NL = 2 * nl + 1;')]),
    ('F: the retry not raising the precision (beyond 64)',
     [('        p = p > ADF_REAL_PREC_MAX / 2 ? ADF_REAL_PREC_MAX : 2 * p;',
      '        p = FLINT_MAX(p, PN_STALL_PREC);')]),
    ('F: outputs written on NOT_DETERMINED',
     [('    if (st == ADF_OK)\n    {\n        acb_swap(left, l);',
       '    if (st == ADF_OK || st == ADF_NOT_DETERMINED)\n    {\n        acb_swap(left, l);')]),
    ('the left lattice j/D + D n instead of j/D + M n',
     [('fmpq_set_si(q, (slong) j + (slong) L * n, f->D);', 'fmpq_set_si(q, (slong) j + (slong) f->D * n, f->D);')]),
    ('bits cap one short', [('    if (bits < 0 || bits > PN_BITS_MAX)',
     '    if (bits < 0 || bits >= PN_BITS_MAX)')]),
    ('the right tail with |g[0]| for max_k |g[k]|', [('        mag_max(gmax, gmax, m);',
     '        if (j == 0) mag_set(gmax, m);')]),
    ('the left tail with the term j = 0 only',
     [('        acb_get_mag(m, f->f + j);\n        if (mag_is_zero(m))\n            continue;',
       '        acb_get_mag(m, f->f + j);\n        if (mag_is_zero(m) || j > 0)\n            continue;')]),
    ('the Taylor shift dropped (q_j = p_j h^j)',
     [('    acb_poly_taylor_shift(Q, t->P, ac, pp);', '    acb_poly_set(Q, t->P);')]),
    ('the search from N = 1', [('    ulong N = 0;\n    arf_t b;', '    ulong N = 1;\n    arf_t b;')]),
    ('the goal epsilon/4 instead of epsilon/8', [('arf_cmp_2exp_si(b, -bits - 3) <= 0',
     'arf_cmp_2exp_si(b, -bits - 2) <= 0')]),
    ('mutation survivor 95: the terms cap of D1 not checked in the preflight',
     [('    if (phi->len > ADF_RFUN_TERMS_MAX)\n        return 0;',
      '    if (phi->len > ADF_RFUN_TERMS_MAX)\n        return 1;')]),
    ('the stall rule without the halving (any non-decrease)',
     [('            mag_mul_2exp_si(prev, prev, -1);\n', '')]),
]
MODE = os.environ.get('FAULT_MODE', '')                   # '', 'SAN=1' or 'INV=1': the build of the copies


def main():
    scratch = Path(sys.argv[1]).resolve()
    assert ROOT not in scratch.parents
    rows = []
    only = [int(a) for a in sys.argv[2:]]
    for k, (name, reps) in enumerate(FAULTS):
        if only and k + 1 not in only:
            continue
        d = scratch / f'f{k}'
        if d.exists():
            shutil.rmtree(d)
        d.mkdir(parents=True)
        for part in ['Makefile', 'include', 'src', 'tests']:
            src = ROOT / part
            (shutil.copytree if src.is_dir() else shutil.copy)(src, d / part)
        shutil.copytree(ROOT / 'lanes' / 'f4-slice6' / 'build', d / 'b',
                        ignore=shutil.ignore_patterns('test_*'))       # objects of the clean tree
        text = (d / 'src' / 'poisson.c').read_text()
        for old, new in reps:
            assert text.count(old) >= 1, (name, old)
            text = text.replace(old, new, 1)
        (d / 'src' / 'poisson.c').write_text(text)
        b = subprocess.run(['make', '-s', '-j2', 'BUILD=b'] + ([MODE] if MODE else []) + ['b/test_poisson'], cwd=d,
                           capture_output=True, timeout=900)
        if b.returncode:
            rows.append((name, 'BUILD FAILED ' + b.stderr.decode()[-200:]))
        else:
            r = subprocess.run(['timeout', '120', './b/test_poisson'], cwd=d, capture_output=True, text=True,
                               env=dict(os.environ, ASAN_OPTIONS='detect_leaks=1'))
            line = (r.stderr.strip().splitlines() or ['(no message, status %d)' % r.returncode])[-1][:100]
            rows.append((name, 'caught: ' + line if r.returncode else 'NOT CAUGHT'))
        print(f'{k + 1:2d}. {rows[-1][0]}: {rows[-1][1]}', flush=True)
        shutil.rmtree(d)
    shutil.rmtree(scratch, ignore_errors=True)
    print('caught', sum(r[1].startswith('caught') for r in rows), 'of', len(rows))


if __name__ == '__main__':
    main()
