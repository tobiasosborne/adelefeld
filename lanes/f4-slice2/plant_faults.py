#!/usr/bin/env python3
"""Planted faults of lane f4-slice2 (docs/api-4.md section 8, faults of 4.3 that touch slice 4d, and the brief).

Run from the repository root:  timeout 1200 python3 -B lanes/f4-slice2/plant_faults.py [--survivors]
With --survivors the mutation survivors of src/rfun.c (lanes/f4-slice2/mutate.log) that were gaps of the tests are
planted instead, in an INV=1 build (one of them removes an entry check).
Copies Makefile, include, src, tests into lanes/f4-slice2/faults/tree (a scratch copy), applies one fault at a
time to the scratch src/rfun.c or src/text.c, builds test_rfun there (make -j2) and runs it. Every fault must
make test_rfun fail. The scratch tree is removed at the end. Prints one line per fault.
"""
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCR = ROOT / 'lanes/f4-slice2/faults'
TREE = SCR / 'tree'

FAULTS = [
    ('B sign under translation', 'src/rfun.c',
     '        acb_add(r->B, a->B, u, p);', '        acb_sub(r->B, a->B, u, p);'),
    ('C without -pi A q^2', 'src/rfun.c',
     '        acb_div_fmpz(u, u, den2, p);\n        acb_sub(r->C, r->C, u, p);',
     '        acb_div_fmpz(u, u, den2, p);'),
    ('C with +B q', 'src/rfun.c',
     '        acb_sub(r->C, a->C, u, p);', '        acb_add(r->C, a->C, u, p);'),
    ('dilation of A by h, not h^2', 'src/rfun.c',
     '        acb_mul_fmpz(r->A, a->A, num2, p);\n        acb_div_fmpz(r->A, r->A, den2, p);',
     '        acb_mul_fmpz(r->A, a->A, num, p);\n        acb_div_fmpz(r->A, r->A, den, p);'),
    ('dilation loses the sign of h in B', 'src/rfun.c',
     '        acb_mul_fmpz(r->B, a->B, num, p);',
     '        acb_mul_fmpz(r->B, a->B, num, p);\n        if (fmpz_sgn(num) < 0) acb_neg(r->B, r->B);'),
    ('product adds only A', 'src/rfun.c',
     '            acb_add(t[k].B, a->B, b->B, p);\n            acb_add(t[k].C, a->C, b->C, p);',
     '            acb_set(t[k].B, a->B);\n            acb_set(t[k].C, a->C);'),
    ('product multiplies A', 'src/rfun.c',
     '            acb_add(t[k].A, a->A, b->A, p);', '            acb_mul(t[k].A, a->A, b->A, p);'),
    ('product order j before i', 'src/rfun.c',
     '            const adf_rterm_struct * a = x->term + i, * b = y->term + j;',
     '            const adf_rterm_struct * a = x->term + (k % x->len), * b = y->term + (k / x->len);'),
    ('sum order y before x', 'src/rfun.c',
     '        rf_term_set(t + i, x->term + i);\n    for (i = 0; i < y->len; i++)\n'
     '        rf_term_set(t + x->len + i, y->term + i);',
     '        rf_term_set(t + y->len + i, x->term + i);\n    for (i = 0; i < y->len; i++)\n'
     '        rf_term_set(t + i, y->term + i);'),
    ('reflection forgets B', 'src/rfun.c',
     '        acb_neg(t[i].B, t[i].B);\n', ''),
    ('conjugation forgets A', 'src/rfun.c',
     '        acb_conj(t[i].A, t[i].A);\n', ''),
    ('Re(A) > 0 not re-checked after rounding', 'src/rfun.c',
     'static int\nrf_result_ok(const adf_rterm_struct * t)\n{\n    return rf_term_ok(t);',
     'static int\nrf_result_ok(const adf_rterm_struct * t)\n{\n    return acb_is_finite(t->A);'),
    ('y written before the last check (product)', 'src/rfun.c',
     '                rf_free(t, n);\n                return ADF_NOT_DETERMINED;',
     '                rf_commit(z, t, n);\n                return ADF_NOT_DETERMINED;'),
    ('evaluation with +pi A x^2', 'src/rfun.c',
     '        acb_sub(e, a->C, e, p);', '        acb_add(e, a->C, e, p);'),
    ('evaluation at the midpoint of x only', 'src/rfun.c',
     '    acb_set_arb(xc, x);', '    acb_set_arb(xc, x);\n    mag_zero(arb_radref(acb_realref(xc)));'),
    ('translation shift with +q', 'src/rfun.c',
     '    fmpz_neg(mnum, num);', '    fmpz_set(mnum, num);'),
    ('dilation by 0 accepted', 'src/rfun.c',
     '    if (fmpz_is_zero(num))\n        return ADF_DOMAIN;', ''),
    ('idele dilation by two signed copies (h h)', 'src/rfun.c',
     '    rf_interval_sqr(h2, a->inf, p);', '    arb_mul(h2, a->inf, a->inf, p);'),
    ('an inexact trailing coefficient removed (reader)', 'src/text.c',
     '        _acb_poly_normalise(t[k].P);        /* exact trailing zeros only',
     '        while (t[k].P->length > 0 && acb_contains_zero(t[k].P->coeffs + t[k].P->length - 1))\n'
     '            t[k].P->length--;\n        _acb_poly_normalise(t[k].P);        /* exact trailing zeros only'),
    ('reader: Re(A) <= 0 not DOMAIN', 'src/text.c',
     '        if (fmpq_sgn(lo) <= 0)\n        {\n            st = ADF_DOMAIN;',
     '        if (fmpq_sgn(hi) <= 0)\n        {\n            st = ADF_DOMAIN;'),
    ('printer: Re(A) unconstrained', 'src/text.c',
     '        if (tx_put_rf_complex(&b, t->A, digits, 1))', '        if (tx_put_rf_complex(&b, t->A, digits, 0))'),
    ('LIMIT cap of terms off by one', 'src/rfun.c',
     '    if (n > ADF_RFUN_TERMS_MAX)\n        return ADF_LIMIT;',
     '    if (n > ADF_RFUN_TERMS_MAX + 1)\n        return ADF_LIMIT;'),
]


SURVIVORS = [
    ('238:14 set_terms validates from term 1', 'src/rfun.c',
     '    for (i = 0; i < n; i++)\n        if (!rf_term_ok(terms + i))',
    '    for (i = 1; i < n; i++)\n        if (!rf_term_ok(terms + i))'),
    ('289:16 mul input cap LIMIT -> OK', 'src/rfun.c',
     '    if (!rf_input_ok(x) || !rf_input_ok(y))\n        return ADF_LIMIT;\n    if (x->len > 0',
     '    if (!rf_input_ok(x) || !rf_input_ok(y))\n        return ADF_OK;\n    if (x->len > 0'),
    ('290:30 mul terms cap > -> >=', 'src/rfun.c',
     'y->len > ADF_RFUN_TERMS_MAX / x->len', 'y->len >= ADF_RFUN_TERMS_MAX / x->len'),
    ('294:14 nx counted from term 1', 'src/rfun.c',
     '    for (i = 0; i < x->len; i++)\n        nx +=', '    for (i = 1; i < x->len; i++)\n        nx +='),
    ('298:25 coefficients nx * sy -> nx + sy', 'src/rfun.c',
     'coeffs = (ulong) nx * (ulong) sy', 'coeffs = (ulong) nx + (ulong) sy'),
    ('299:16 coefficients cap > -> >=', 'src/rfun.c',
     'if (coeffs > (ulong) ADF_RFUN_COEFFS_MAX', 'if (coeffs >= (ulong) ADF_RFUN_COEFFS_MAX'),
    ('299:73 work cap > -> >=', 'src/rfun.c',
     '(ulong) sx * (ulong) sy > (ulong) ADF_RFUN_WORK_MAX', '(ulong) sx * (ulong) sy >= (ulong) ADF_RFUN_WORK_MAX'),
    ('330:18 translation work starts at 1', 'src/rfun.c',
     '    ulong work = 0;', '    ulong work = 1;'),
    ('344:43 a zero polynomial costs 1', 'src/rfun.c',
     'l * (l - 1) / 2 : 0;', 'l * (l - 1) / 2 : 1;'),
    ('105:13 predicate n > 0 -> n > 1', 'src/rfun.c',
     '    if (n > 0 && acb_is_zero(t->P->coeffs + n - 1))', '    if (n > 1 && acb_is_zero(t->P->coeffs + n - 1))'),
    ('259:5 add without the entry check of x', 'src/rfun.c',
     '        return ADF_LIMIT;\n    ADF_INV_RFUN(x);\n    ADF_INV_RFUN(y);\n'
     '    if (!rf_input_ok(x) || !rf_input_ok(y) || x->len',
     '        return ADF_LIMIT;\n    ADF_INV_RFUN(y);\n    if (!rf_input_ok(x) || !rf_input_ok(y) || x->len'),
]


def run(cmd, timeout):
    return subprocess.run(cmd, cwd=TREE, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          timeout=timeout).returncode


def main():
    if TREE.exists():
        shutil.rmtree(TREE)
    TREE.mkdir(parents=True)
    for d in ('include', 'src', 'tests'):
        shutil.copytree(ROOT / d, TREE / d)
    shutil.copy(ROOT / 'Makefile', TREE / 'Makefile')
    orig = {f: (TREE / f).read_text() for f in ('src/rfun.c', 'src/text.c')}
    faults, inv = (SURVIVORS, ' INV=1') if '--survivors' in sys.argv else (FAULTS, '')
    mk = 'make -s -j2%s BUILD=b b/test_rfun' % inv
    assert run(mk, 900) == 0
    assert run('timeout 300 b/test_rfun', 300) == 0, 'unmutated test fails'
    caught = 0
    for name, f, a, b in faults:
        src = orig[f]
        assert src.count(a) == 1, name
        (TREE / f).write_text(src.replace(a, b))
        rc = run(mk, 900)
        if rc != 0:
            res = 'not compiled'
        else:
            rc = run('timeout 300 b/test_rfun', 300)
            res = 'caught (exit %d)' % rc if rc != 0 else 'SURVIVED'
        caught += res.startswith('caught')
        print('%-48s %s' % (name, res), flush=True)
        (TREE / f).write_text(src)
    print('faults: %d, caught: %d' % (len(faults), caught))
    shutil.rmtree(SCR)
    return 0 if caught == len(faults) else 1


if __name__ == '__main__':
    sys.exit(main())
