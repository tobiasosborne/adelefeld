#!/usr/bin/env python3
"""Planted faults of lane f4-slice5 (brief step F): each fault is one textual change of src/tensor.c in a scratch
copy of the tree; test_tensor must fail (exit status not 0, or killed). Run from the repository root:
    timeout 3000 python3 -B lanes/f4-slice5/plant_faults.py SCRATCH_DIR
    (optionally followed by fault numbers). SCRATCH_DIR must be outside the repository; it is removed at the end."""
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FAULTS = [
    ('faults_44 a: a jump sampled at one point (the first coset met only)',
     '            tn_box_join(&b, f->f + j, p);\n    }\n    tn_box_get(h, &b, p);\n    tn_box_clear(&b);'
     '\n    fmpz_clear(A);',
     '            { tn_box_join(&b, f->f + j, p); break; }\n    }\n    tn_box_get(h, &b, p);\n    tn_box_clear(&b);'
     '\n    fmpz_clear(A);'),
    ('faults_44 b: the outside test without d | D H ((1/4) Zhat)',
     'if (!fmpz_divisible(AD, d) || !fmpz_divisible(t, d))', 'if (!fmpz_divisible(AD, d))'),
    ('faults_44 c: the empty set (exact point 1/5) is not exactly 0',
     '        if (b->one == NULL)\n            acb_zero(h);',
     '        if (b->one == NULL)\n            acb_one(h);'),
    ('faults_44 d: the partial evaluator without the zero of an absent prime',
     '    tn_box_init(&b);\n    tn_box_join(&b, NULL, p);\n    fmpq_init(q);',
     '    tn_box_init(&b);\n    fmpq_init(q);'),
    ('faults_44 e: the integral weight 1/D for 1/M', 'acb_div_ui(s, s, f->M, p);', 'acb_div_ui(s, s, f->D, p);'),
    ('faults_44 f: the norm with |f| for |f|^2', 'arb_sqr(t, acb_realref(f->f + j), p);',
     'arb_abs(t, acb_realref(f->f + j));'),
    ('gcd criterion with lcm', 'fmpz_gcd(g, g, t);', 'fmpz_lcm(g, g, t);'),
    ('support test dropped (zero never included)',
     '        tn_box_join(&b, NULL, p);                         /* outside',
     '        (void) p;                 /* outside'),
    ('a jump sampled at the midpoint only (the coset of the centre A/d)',
     '        if (fmpz_divisible(t, g))\n', '        if (fmpz_divisible(t, g) && fmpz_cmp_ui(t, 0) == 0)\n'),
    ('step 4 with max instead of min', 'thr = (l->exact || l->N > m) ? m : l->N;',
     'thr = (l->exact || l->N < m) ? m : l->N;'),
    ('step 4 exact point with min(0, v_p(M))', 'thr = (l->exact || l->N > m) ? m : l->N;',
     'thr = (l->N > m) ? m : l->N;'),
    ('step 5 without the factor exp(beta^2/(2 alpha))', '        arb_div(e, e, alpha, p);', '        arb_zero(e);'),
    ('the norm not clipped', '        flint_abort();\n    arb_nonnegative_part(s, s);', '        flint_abort();'),
    ('z written before the last check (tensor_eval)', '    st = adf_rfun_eval(r, phi, x->inf, prec);',
     '    acb_set(z, h);\n    st = adf_rfun_eval(r, phi, x->inf, prec);'),
    ('bits cap one bit short', 'return (slong) fmpz_bits(a) <= TN_BITS_MAX;',
     'return (slong) fmpz_bits(a) < TN_BITS_MAX;'),
    ('work cap off by one', 'L > (ulong) ADF_TENSOR_WORK_MAX / (ulong) x->len',
     'L >= (ulong) ADF_TENSOR_WORK_MAX / (ulong) x->len'),
    ('COMPLEX arch accepted as NONE', '    if (x->arch == ADF_ARCH_COMPLEX)\n        return ADF_DOMAIN;', ''),
    ('the hull radius always rounded up (arb_set_interval_arf)', '    if (inexact || !arf_equal(u, t))',
     '    if (1)'),
    # the mutation survivors that were test gaps (mutate.log), after the tests of guard_cases and debug
    ('survivor: integral loop reads entry L', '    for (j = 0; j < L; j++)\n        acb_add(s, s, f->f + j, p);',
     '    for (j = 0; j <= L; j++)\n        acb_add(s, s, f->f + j, p);'),
    ('survivor: the box clear of hi[k] as hi[-k] (SAN)', '        arf_clear(b->hi + k);',
     '        arf_clear(b->hi - k);'),
    ('survivor: tensor_eval without the rfun entry check (INV)',
     '    TN_INV_RFUN(phi);\n    TN_INV_FFUN(f);\n    ADF_INV_ADELE(x);',
     '    TN_INV_FFUN(f);\n    ADF_INV_ADELE(x);'),
    ('survivor: tensor_eval without the ffun entry check (INV)',
     '    TN_INV_RFUN(phi);\n    TN_INV_FFUN(f);\n    ADF_INV_ADELE(x);',
     '    TN_INV_RFUN(phi);\n    ADF_INV_ADELE(x);'),
]
import os
MODE = os.environ.get('FAULT_MODE', '')                   # '', 'SAN=1' or 'INV=1': the build of the copies
PREBUILT = {'': 'build', 'SAN=1': 'b-san', 'INV=1': 'b-inv'}[MODE]


def main():
    scratch = Path(sys.argv[1]).resolve()
    assert ROOT not in scratch.parents
    rows = []
    only = [int(a) for a in sys.argv[2:]]                 # optional: the 1-based numbers of the faults to run
    for k, (name, old, new) in enumerate(FAULTS):
        if only and k + 1 not in only:
            continue
        d = scratch / f'f{k}'
        if d.exists():
            shutil.rmtree(d)
        d.mkdir(parents=True)
        for part in ['Makefile', 'include', 'src', 'tests']:
            src = ROOT / part
            (shutil.copytree if src.is_dir() else shutil.copy)(src, d / part)
        shutil.copytree(ROOT / 'lanes' / 'f4-slice5' / PREBUILT, d / 'b')   # objects of the clean tree
        text = (d / 'src' / 'tensor.c').read_text()
        assert text.count(old) >= 1, name
        (d / 'src' / 'tensor.c').write_text(text.replace(old, new, 1))
        b = subprocess.run(['make', '-s', '-j2', 'BUILD=b'] + ([MODE] if MODE else []) + ['b/test_tensor'], cwd=d,
                           capture_output=True,
                           timeout=900)
        if b.returncode:
            rows.append((name, 'BUILD FAILED'))
        else:
            r = subprocess.run(['timeout', '300', './b/test_tensor'], cwd=d, capture_output=True, text=True,
                               env=dict(os.environ, ASAN_OPTIONS='detect_leaks=1'))
            line = (r.stderr.strip().splitlines() or ['(no message)'])[-1][:110]
            rows.append((name, 'caught: ' + line if r.returncode else 'NOT CAUGHT'))
        print(f'{k + 1:2d}. {rows[-1][0]}: {rows[-1][1]}', flush=True)
        shutil.rmtree(d)
    shutil.rmtree(scratch, ignore_errors=True)
    print('caught', sum(r[1].startswith('caught') for r in rows), 'of', len(rows))


if __name__ == '__main__':
    main()
