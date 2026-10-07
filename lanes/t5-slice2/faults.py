#!/usr/bin/env python3
"""Planted faults of lane t5-slice2 (step F): timeout 900 python3 -B lanes/t5-slice2/faults.py

Each fault is a scratch copy of src/tate.c (lanes/t5-slice2/scratch/), linked before the lane's library with
tests/test_tate.c; the test must fail. Fault 4 is planted in the reference instead (the oracle's Hurwitz
expression without C^-s), as a scratch vector file read by a scratch copy of the test."""
import os, re, subprocess, sys
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
LANE = os.path.join(ROOT, 'lanes', 't5-slice2')
SCR = os.path.join(LANE, 'scratch')
BUILD = os.path.join(LANE, 'build')
os.makedirs(SCR, exist_ok=True)
SRC = open(os.path.join(ROOT, 'src', 'tate.c')).read()

def rep(s, old, new, count=1):
    assert s.count(old) >= 1, old
    return s.replace(old, new, count)

F = []
F.append(('52.1 C^z omitted (Lambda returned as I)',
          lambda s: rep(s, '    if (C > 1)\n        acb_mul(lam, lam, cz, w);\n', '')))
F.append(('52.2 wrong real parity (e = 0 in the integral)',
          lambda s: rep(s, '    int e = chi->parity, st;\n    adf_rfun_t phi, hat;',
                        '    int e = 0, st;\n    adf_rfun_t phi, hat;')))
F.append(('52.3 chi conjugated twice (conj on the forward coefficients)',
          lambda s: rep(s, '        acb_mul_ui(an + n, f->f + n % C, e ? n : 1, w);\n',
                        '        acb_mul_ui(an + n, f->f + n % C, e ? n : 1, w);\n'
                        '        acb_conj(an + n, an + n);\n')))
F.append(('52.5 sign-half factor 2 (Lambda doubled)',
          lambda s: rep(s, '    /* I = C^-z Lambda (P11) */\n', '    acb_mul_2exp_si(lam, lam, 1);\n')))
F.append(('52.6 xi treated as Lambda (times s(s-1)/2)',
          lambda s: rep(s, '    /* I = C^-z Lambda (P11) */\n',
                        '    acb_sub_ui(t, s, 1, w); acb_mul(t, t, s, w); acb_mul_2exp_si(t, t, -1);'
                        ' acb_mul(lam, lam, t, w);\n')))
F.append(('completion with z = s/2 for odd e',
          lambda s: rep(rep(s, '    acb_add_si(zz, s, e, w);', '    acb_add_si(zz, s, 0, w);'),
                        '    acb_add_si(zd, zd, 1 + e, w);', '    acb_add_si(zd, zd, 1, w);')))
F.append(('completion factor pi^-z Gamma(z) dropped (L returned)',
          lambda s: rep(s, '    acb_swap(out, lam);\n    written = 1;',
                        '    { acb_t g; acb_init(g); acb_gamma(g, zz, w); acb_const_pi(t, w); acb_neg(y, zz);'
                        ' acb_pow(t, t, y, w); acb_mul(g, g, t, w); acb_div(lam, lam, g, w); acb_clear(g); }\n'
                        '    acb_swap(out, lam);\n    written = 1;')))
F.append(('dual coefficients without conj (W chi(n) n^e)',
          lambda s: rep(s, '        acb_mul(bn + n, t, g->f + n % C, w);\n    }\n',
                        '        acb_mul(bn + n, t, g->f + n % C, w);\n    }\n'
                        '    for (n = N; n >= 1; n--) acb_mul(bn + n, an + n, bn + 1, w);\n')))
F.append(('W_chi dropped (conj(chi(n)) n^e)',
          lambda s: rep(s, '        acb_mul(bn + n, t, g->f + n % C, w);\n    }\n',
                        '        acb_mul(bn + n, t, g->f + n % C, w);\n    }\n'
                        '    for (n = 1; n <= N; n++) acb_conj(bn + n, an + n);\n')))
F.append(('lower half not replaced through Poisson (second integral dropped)',
          lambda s: rep(s, '    acb_add(lam, lam, t, w);\n    if (C == 1)', '    if (C == 1)')))
F.append(('quadrature remainder dropped',
          lambda s: rep(rep(s, '    arb_add(En, En, el, TT_TP);\n', ''), '    arb_add(En, En, er, TT_TP);\n', '')))
F.append(('width checked before the remainder is added',
          lambda s: rep(rep(rep(s, '    acb_add_error_arf(lam, lo);\n', '    arf_set(tt_planted, lo);\n'),
                            '#include "invariants.h"\n',
                            '#include "invariants.h"\nstatic arf_struct tt_planted[1];\n'),
                        '            acb_swap(z, res);\n',
                        '            acb_add_error_arf(res, tt_planted);\n            acb_swap(z, res);\n')))
F.append(('z written on NOT_DETERMINED',
          lambda s: rep(s, '    acb_clear(s0);\n    acb_clear(res);',
                        '    if (st == ADF_NOT_DETERMINED) acb_set(z, res);\n'
                        '    acb_clear(s0);\n    acb_clear(res);')))

def build_run(name, src_text, test_src=os.path.join(ROOT, 'tests', 'test_tate.c')):
    c = os.path.join(SCR, 'tate_fault.c'); exe = os.path.join(SCR, 'test_fault')
    open(c, 'w').write(src_text)
    cmd = ['cc', '-std=c11', '-O2', '-Iinclude', '-Isrc', '-Itests', test_src, c,
           os.path.join(BUILD, 'support', 'jsonl.o'), os.path.join(BUILD, 'support', 'golden.o'),
           os.path.join(BUILD, 'libadelefeld.a'), '-lflint', '-lgmp', '-lm', '-o', exe]
    b = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    if b.returncode:
        return 'BUILD FAILED: ' + b.stderr[:300]
    try:
        r = subprocess.run([exe], cwd=ROOT, capture_output=True, text=True, timeout=300)
    except subprocess.TimeoutExpired:
        return 'detected (timeout 300 s)'
    if r.returncode == 0:
        return 'SURVIVED'
    line = [l for l in r.stderr.splitlines() if 'test_tate.c' in l]
    return 'detected: ' + (line[0][:150] if line else f'exit {r.returncode}')

rows = []
for name, f in F:
    rows.append((name, build_run(name, f(SRC))))
    print(rows[-1], flush=True)
# 52.4: C^-s omitted from the Hurwitz reference (a fault of the reference, planted in the generator)
sys.path.insert(0, os.path.join(ROOT, 'proto')); sys.path.insert(0, LANE)
import mpmath as mp
import tate_checks as T
import gen_vectors as G
orig = T.l_reference
def wrong(chi, s):
    return orig(chi, s) if chi.C == 1 else orig(chi, s)*chi.C**s
T.l_reference = wrong
G.OUT = os.path.join(SCR, 'tate_fault.jsonl')
G.I_flint_orig = G.I_flint
G.I_flint = lambda chi, t: None
import flint
def mainwrong():
    # the generator asserts that the two sources agree; the planted reference must be caught there first
    try:
        G.main()
        return 'SURVIVED (generator)'
    except (AssertionError, TypeError) as ex:
        return 'detected by the generator cross-check (mpmath vs FLINT): ' + type(ex).__name__
G.I_flint = G.I_flint_orig
rows.append(('52.4 C^-s omitted from the Hurwitz reference (generator)', mainwrong()))
print(rows[-1], flush=True)
# the same wrong reference written with the cross-check disabled (MARGIN huge): the C test must refuse it
T.MARGIN = mp.mpf(10)**10
G.main()
t = open(os.path.join(ROOT, 'tests', 'test_tate.c')).read().replace('"tests/ref/vectors/t-slice2/tate.jsonl"',
                                                                  '"' + G.OUT + '"')
open(os.path.join(SCR, 'test_fault.c'), 'w').write(t)
rows.append(('52.4 C^-s omitted from the Hurwitz reference (C test on the wrong vectors)',
             build_run('52.4', SRC, os.path.join(SCR, 'test_fault.c'))))
print(rows[-1], flush=True)
T.l_reference = orig
print('\n| fault | result |\n|---|---|')
for n, r in rows:
    print(f'| {n} | {r} |')
