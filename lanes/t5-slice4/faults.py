#!/usr/bin/env python3
"""Faults of slice 5d in scratch copies of src/tate.c: timeout 1500 python3 -B lanes/t5-slice4/faults.py

Needs lanes/t5-slice4/build built first
(make -j2 BUILD=lanes/t5-slice4/build lanes/t5-slice4/build/test_tate_continue).
Each fault is one textual replacement in a copy of src/tate.c under lanes/t5-slice4/scratch/; the copy is compiled
and linked before the library archive (its symbols win), tests/test_tate_continue.c is linked against it and run.
A fault is detected when the program exits non-zero; the first failing line is printed. The scratch directory is
removed at the end. The faults: api-5.md section 7 for 5.3 (six) and the six of the lane brief."""
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
B = os.path.join(HERE, 'build')
S = os.path.join(HERE, 'scratch')
CFLAGS = ['-std=c11', '-O2', '-g', '-Wall', '-Wextra', '-Wpedantic', '-Werror']

FAULTS = [
    ('53.1 wrong -a/s sign', '''        tt_recip(t, s, 0, w);
        acb_sub(lam, lam, t, w);''', '''        tt_recip(t, s, 0, w);
        acb_add(lam, lam, t, w);'''),
    ('53.2 zero mode retained (b/(s-1) counted twice)', '''        tt_recip(t, s, 1, w);
        acb_add(lam, lam, t, w);''', '''        tt_recip(t, s, 1, w);
        acb_add(lam, lam, t, w);
        acb_add(lam, lam, t, w);'''),
    ('53.3 dual s instead of 1 - s', '''    acb_neg(zd, s);
    acb_add_si(zd, zd, 1 + e, w);''', '''    acb_set(zd, s);
    acb_add_si(zd, zd, e, w);'''),
    ('53.4 L14 factor omitted (2 R^r)', '''        arb_mul(J, t, u, TT_TP);
        arb_mul_2exp_si(J, J, 1);
        arb_div(J, J, bb, TT_TP);''', '''        arb_set(J, u);
        arb_div(J, J, bb, TT_TP);'''),
    ('53.5 discarded n tail (E_n not added)', '''    arb_add(En, En, Et, TT_TP);
    arb_add(En, En, el, TT_TP);''', '''    arb_set(En, Et);
    arb_add(En, En, el, TT_TP);'''),
    ('53.6 input radius dropped', '''    acb_set(s0, s);                                       /* z may be s */''',
     '''    acb_get_mid(s0, s);'''),
    ('Gamma(z) L(s) formed (I = pi^-z Gamma(z) L)', '''    /* I = C^-z Lambda (P11) */
    if (C > 1 && !lmode)
        acb_mul(lam, lam, cz, w);''', '''    /* I = C^-z Lambda (P11) */
    if (C > 1 && !lmode)
        acb_mul(lam, lam, cz, w);
    {   /* FAULT: L = Lambda (C/pi)^-z / Gamma(z), then (C/pi)^z Gamma(z) L */
        acb_t g, r; acb_init(g); acb_init(r);
        acb_rgamma(r, zz, w); acb_gamma(g, zz, w);
        acb_mul(lam, lam, r, w); acb_mul(lam, lam, g, w);
        acb_clear(g); acb_clear(r);
    }'''),
    ('pole test on the open rectangle', '''    if (arb_contains_zero(acb_imagref(s))
        && (arb_contains_si(acb_realref(s), 0) || arb_contains_si(acb_realref(s), 1)))
        return ADF_NOT_DETERMINED;''',
     '''    {   /* FAULT: x0 in the open rectangle (an exact coordinate must equal it) */
        arb_t x;
        arb_srcptr re = acb_realref(s), ims = acb_imagref(s);
        int im, in0, in1;
        arb_init(x);
        im = arb_is_exact(ims) ? arb_is_zero(ims) : arb_contains_interior(ims, x);
        in0 = arb_is_exact(re) ? arb_equal(re, x) : arb_contains_interior(re, x);
        arb_one(x);
        in1 = arb_is_exact(re) ? arb_equal(re, x) : arb_contains_interior(re, x);
        arb_clear(x);
        if (im && (in0 || in1))
            return ADF_NOT_DETERMINED;
    }'''),
    ('C^z applied to a width-limited I without recheck', '''    return tt_run(z, chi, s, bits, prec, TT_COMP);''',
     '''    acb_t y, c;
    int st;
    acb_init(y); acb_init(c);
    st = tt_run(y, chi, s, bits, prec, TT_CONT);
    if (st == ADF_OK)
    {
        acb_add_si(c, s, chi->parity, FLINT_MAX(prec, 2) + 64);
        acb_mul_2exp_si(c, c, -1);
        acb_set_ui(z, chi->q);
        acb_pow(c, z, c, FLINT_MAX(prec, 2) + 64);
        acb_mul(z, y, c, FLINT_MAX(prec, 2) + 64);
    }
    acb_clear(y); acb_clear(c);
    return st;'''),
    ('the dual exponent z instead of z\'', 'st = tt_side(t, er, zd, bn, N, e, a0, R, tg, w, work,',
     'st = tt_side(t, er, zz, bn, N, e, a0, R, tg, w, work,'),
    ('z written on NOT_DETERMINED', '''    acb_clear(s0);
    acb_clear(res);
    mag_clear(cur);''', '''    if (st == ADF_NOT_DETERMINED && mode != TT_HALF)
        acb_swap(z, res);
    acb_clear(s0);
    acb_clear(res);
    mag_clear(cur);'''),
    ('the forbidden acb_dirichlet_zeta called', '''    if (C == 1)
    {   /* delta [1/(s-1) - 1/s] (P13 step 3), each reciprocal by T4 */''', '''    if (C == 1 && tr == NULL)
        acb_dirichlet_zeta(y, s, w);
    if (C == 1)
    {   /* delta [1/(s-1) - 1/s] (P13 step 3), each reciprocal by T4 */'''),
]


def run(cmd, **kw):
    return subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, **kw)


def main():
    src = open(os.path.join(ROOT, 'src', 'tate.c')).read()
    os.makedirs(S, exist_ok=True)
    support = sorted(os.path.join(B, 'support', f) for f in os.listdir(os.path.join(B, 'support'))
                     if f.endswith('.o'))
    rows = []
    for k, (name, old, new) in enumerate(FAULTS, 1):
        assert src.count(old) == 1, name
        text = src.replace(old, new)
        if 'acb_dirichlet_zeta' in new:
            text = text.replace('#include <string.h>', '#include <string.h>\n#include <flint/acb_dirichlet.h>', 1)
        c = os.path.join(S, f'fault{k}.c')
        o = os.path.join(S, f'fault{k}.o')
        exe = os.path.join(S, f'fault{k}.exe')
        open(c, 'w').write(text)
        r = run(['cc', '-Iinclude', '-Isrc'] + CFLAGS + ['-c', c, '-o', o], timeout=300)
        if r.returncode:
            rows.append((name, 'BUILD FAILED: ' + r.stderr.strip().splitlines()[0]))
            continue
        r = run(['cc', '-Iinclude', '-Itests'] + CFLAGS + ['tests/test_tate_continue.c'] + support +
                [o, os.path.join(B, 'libadelefeld.a'), '-lflint', '-lgmp', '-lm', '-o', exe], timeout=300)
        if r.returncode:
            rows.append((name, 'LINK FAILED: ' + r.stderr.strip().splitlines()[0]))
            continue
        try:
            r = run(['timeout', '300', exe], timeout=320)
            fail = [x for x in r.stderr.splitlines() if 'test_tate_continue.c:' in x]
            tail = [x for x in r.stderr.splitlines() if x.strip()]
            first = fail[0] if fail else (tail[-1] if tail else '')
            if r.returncode == 0:
                rows.append((name, 'NOT DETECTED'))
            else:
                rows.append((name, f'detected (exit {r.returncode}) {first.strip()}'))
        except subprocess.TimeoutExpired:
            rows.append((name, 'TIMEOUT'))
        print(f'{k:2d} {rows[-1][0]}: {rows[-1][1]}', flush=True)
    shutil.rmtree(S)
    print(f'{sum(1 for _, r in rows if r.startswith("detected"))} of {len(rows)} faults detected')


if __name__ == '__main__':
    sys.exit(main())
