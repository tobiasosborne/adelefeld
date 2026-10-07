#!/usr/bin/env python3
"""Re-run the mutation survivors judged test gaps against the extended tests/test_tate.c (lane t5-slice2).
timeout 900 python3 -B lanes/t5-slice2/resurvivors.py"""
import os, subprocess
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SCR = os.path.join(ROOT, 'lanes', 't5-slice2', 'scratch'); BUILD = os.path.join(ROOT, 'lanes', 't5-slice2', 'build')
SRC = open(os.path.join(ROOT, 'src', 'tate.c')).read()
M = [('122 tt_charge never refuses', '        return 0;\n    *w += n;', '        return 1;\n    *w += n;'),
     ('387 (J+1)*nz -> (J+1)+nz', 'tt_charge(work, (J + 1) * nz)', 'tt_charge(work, (J + 1) + nz)'),
     ('383 work guard - -> +', '((ulong) ADF_TATE_WORK_MAX - *work) / nz',
      '((ulong) ADF_TATE_WORK_MAX + *work) / nz'),
     ('231 L14 branch 2 mul -> add',
      '        arb_exp(u, u, TT_TP);\n        arb_mul(t, t, u, TT_TP);\n        arb_sub(u, R0',
      '        arb_exp(u, u, TT_TP);\n        arb_add(t, t, u, TT_TP);\n        arb_sub(u, R0'),
     ('232 L14 R0 - R dropped', '        arb_sub(u, R0, RR, TT_TP);\n', ''),
     ('218 L14 factor 2 dropped', '        arb_mul(J, t, u, TT_TP);\n        arb_mul_2exp_si(J, J, 1);\n',
      '        arb_mul(J, t, u, TT_TP);\n'),
     ('560 N search from 1', '    for (N = 0;; N = N ? 2 * N : 1)', '    for (N = 1;; N = N ? 2 * N : 1)'),
     ('374 J = 0 error mul -> add', '        arb_mul(err, err, two3, TT_TP);                   /* J = 0 */',
      '        arb_add(err, err, two3, TT_TP);                   /* J = 0 */'),
     ('551 r2 upper bound dropped', '    arb_get_ubound_arf(r2, acb_realref(zd), TT_TP);\n', ''),
     ('356 m + d scaling dropped', '        arb_mul_2exp_si(y, y, (slong) k - 2);\n', ''),
     ('336 r = Re(z) - 1 dropped', '    arb_set(r, acb_realref(zm1));\n', '')]
for name, old, new in M:
    assert SRC.count(old) == 1, name
    c = os.path.join(SCR, 'tate_mut.c'); exe = os.path.join(SCR, 'test_mut')
    open(c, 'w').write(SRC.replace(old, new))
    b = subprocess.run(['cc', '-std=c11', '-O2', '-Iinclude', '-Isrc', '-Itests', 'tests/test_tate.c', c,
                        os.path.join(BUILD, 'support', 'jsonl.o'), os.path.join(BUILD, 'support', 'golden.o'),
                        os.path.join(BUILD, 'libadelefeld.a'), '-lflint', '-lgmp', '-lm', '-o', exe],
                       cwd=ROOT, capture_output=True, text=True)
    if b.returncode:
        print(f'| {name} | build failed |'); continue
    try:
        r = subprocess.run([exe], cwd=ROOT, capture_output=True, text=True, timeout=120)
        line = [l for l in r.stderr.splitlines() if 'test_tate.c' in l]
        res = ('SURVIVED' if r.returncode == 0
               else 'killed: ' + (line[0].split('tests/')[-1][:90] if line else 'exit'))
    except subprocess.TimeoutExpired:
        res = 'killed (timeout 120 s)'
    print(f'| {name} | {res} |', flush=True)
