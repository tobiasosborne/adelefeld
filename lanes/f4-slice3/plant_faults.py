#!/usr/bin/env python3
"""Planted faults of lane f4-slice3 (docs/api-4.md section 8, faults of 4.3 touching slice 4e, and the brief's
list). Each fault is a text replacement in a scratch copy of src/rfun.c; the copy is built and
test_rfun_fourier is run; a fault is caught if the test fails (exit status other than 0).

Run from the repository root:  timeout 1500 python3 -B lanes/f4-slice3/plant_faults.py SCRATCH
SCRATCH must be an empty or absent directory; it is removed at the end.
"""
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

FAULTS = [
    ('negative real kernel (B\' = -iB/A and z = B - 2 pi i y)', [
        ('acb_mul_onei(r->B, r->B);', 'acb_div_onei(r->B, r->B);'),
        ('acb_div_onei(c1, c1);', 'acb_mul_onei(c1, c1);'),
        ('acb_mul_onei(iinv, inv);', 'acb_div_onei(iinv, inv);')]),
    ('A for 1/A', [('acb_set(r->A, inv);', 'acb_set(r->A, a->A);')]),
    ('B\' without i', [('    acb_mul_onei(r->B, r->B);', '')]),
    ('C\' without B^2/(4 pi A)', [('acb_add(r->C, a->C, wB, p);', 'acb_set(r->C, a->C);')]),
    ('the other root branch', [('    acb_inv(r, r, p);\n    return acb_is_finite(r);',
                                '    acb_inv(r, r, p);\n    acb_neg(r, r);\n    return acb_is_finite(r);')]),
    ('A^(+1/2) for A^(-1/2)', [('    acb_inv(r, r, p);\n    return acb_is_finite(r);',
                                '    return acb_is_finite(r);')]),
    ('amplitude omitted (A = 4)', [('acb_poly_scalar_mul(r->P, S, root, p);', 'acb_poly_set(r->P, S);')]),
    ('reflection dropped from F^2 (polynomial with the other kernel)', [
        ('acb_div_onei(c1, c1);', 'acb_mul_onei(c1, c1);'),
        ('acb_mul_onei(iinv, inv);', 'acb_div_onei(iinv, inv);')]),
    ('Re(1/A) > 0 of the result not checked', [
        ('if (!rf_fourier_term(t + i, x->term + i, p) || !rf_result_ok(t + i))',
         'if (!rf_fourier_term(t + i, x->term + i, p))')]),
    ('y written before the certificate', [
        ('            rf_free(t, x->len);\n            return ADF_NOT_DETERMINED;\n        }\n'
         '    rf_commit(y, t, x->len);',
         '            rf_commit(y, t, x->len);\n            return ADF_NOT_DETERMINED;\n        }\n'
         '    rf_commit(y, t, x->len);')]),
    ('transform work cap one unit low', [('2 * L * L - L + 1;', '2 * L * L - L;')]),
    ('derivative without -2 pi A x P', [('        acb_poly_sub(r->P, r->P, T, p);\n', '')]),
    ('derivative without B P', [('        acb_poly_add(r->P, r->P, T, p);\n'
                                 '        acb_poly_shift_left(T, a->P, 1);',
                                 '        acb_poly_shift_left(T, a->P, 1);')]),
    ('derivative coefficient cap: L for L + 1', [('coeffs += L > 0 ? L + 1 : 0;', 'coeffs += L;')]),
    ('integral: j + 1 for j in the recurrence', [('acb_mul_si(h2, h0, j, p);', 'acb_mul_si(h2, h0, j + 1, p);')]),
    ('integral without C', [('    acb_add(e, e, a->C, p);\n    acb_exp(e, e, p);', '    acb_exp(e, e, p);')]),
    ('integral without B^2/(4 pi A)', [('    acb_mul(e, e, w, p);\n    acb_mul_2exp_si(e, e, -1);\n'
                                       '    acb_add(e, e, a->C, p);',
                                       '    acb_zero(e);\n    acb_add(e, e, a->C, p);')]),
    ('a cross term of the norm dropped (sum of the norms of the terms)', [
        ('        st = adf_rfun_mul(prod, phi, c, prec);',
         '        { slong k; adf_rfun_t u, vv, pp; adf_rfun_init(u); adf_rfun_init(vv); adf_rfun_init(pp);\n'
         '          for (k = 0; k < phi->len && st == ADF_OK; k++) {\n'
         '              adf_rfun_set_terms(u, phi->term + k, 1); adf_rfun_set_terms(vv, c->term + k, 1);\n'
         '              st = adf_rfun_mul(pp, u, vv, prec);\n'
         '              if (st == ADF_OK) st = adf_rfun_add(prod, prod, pp, prec); }\n'
         '          adf_rfun_clear(u); adf_rfun_clear(vv); adf_rfun_clear(pp); }')]),
    ('norm of phi times phi (conjugate dropped)', [('st = adf_rfun_mul(prod, phi, c, prec);',
                                                    'st = adf_rfun_mul(prod, phi, phi, prec);')]),
    ('norm written on failure', [('    if (st == ADF_OK)\n    {\n        if (arb_is_negative',
                                  '    arb_zero(z);\n    if (st == ADF_OK)\n    {\n        if (arb_is_negative')]),
]


def run(cmd, cwd, timeout):
    try:
        r = subprocess.run(cmd, cwd=cwd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           timeout=timeout)
        return r.returncode, r.stdout.decode(errors='replace')
    except subprocess.TimeoutExpired:
        return 'timeout', ''


def main():
    scratch = Path(sys.argv[1]).resolve()
    if scratch.exists():
        shutil.rmtree(scratch)
    scratch.mkdir(parents=True)
    for d in ('include', 'src', 'tests'):
        shutil.copytree(ROOT / d, scratch / d)
    shutil.copy2(ROOT / 'Makefile', scratch / 'Makefile')
    shutil.copytree(ROOT / 'lanes/f4-slice3/build', scratch / 'build', symlinks=True)
    orig = (ROOT / 'src/rfun.c').read_text()
    caught = 0
    for name, reps in FAULTS:
        text = orig
        for old, new in reps:
            assert text.count(old) == 1, (name, old)
            text = text.replace(old, new)
        (scratch / 'src/rfun.c').write_text(text)
        code, out = run('make -s -j2 BUILD=build build/test_rfun_fourier', scratch, 600)
        if code != 0:
            res = 'did not build'
        else:
            code, out = run('./build/test_rfun_fourier', scratch, 300)
            line = [ln for ln in out.splitlines() if 'test_rfun_fourier.c:' in ln or 'checks' in ln]
            res = 'caught (%s)' % (line[0] if line else 'exit %s' % code) if code != 0 else 'SURVIVED'
            caught += code != 0
        print('%-62s %s' % (name, res), flush=True)
    (scratch / 'src/rfun.c').write_text(orig)
    print('caught %d of %d' % (caught, len(FAULTS)))
    shutil.rmtree(scratch)


if __name__ == '__main__':
    main()
