#!/usr/bin/env python3
"""Compile scratch localfactor copies over the unmodified library.
Every program has timeout <=120; one core. No live source file is mutated.
"""
import os
from pathlib import Path
import subprocess as sp
import shutil
ROOT = Path(__file__).resolve().parents[2]
LANE = ROOT/'lanes/t-slice1'
SCRATCH = LANE/'fault-scratch'
SCRATCH.mkdir(exist_ok=True)
source = (ROOT/'src/localfactor.c').read_text()
start = source.index('/* Slice 5a:')
prefix, body = source[:start], source[start:]
# These change the integral's value or its public contract, not a gamma-only helper.
faults = [
    ('inverse_omitted', '            acb_one(t);', '            acb_zero(t);'),
    ('alpha_omitted', 'acb_mul(t, alpha, t, w);', '/* alpha omitted */'),
    ('parity_omitted', 'if (eta0->parity && tate_exact_odd_pole(s))',
                       'if (0 && eta0->parity && tate_exact_odd_pole(s))'),
    ('unit_volume_doubled', '            acb_one(t);', '            acb_set_ui(t, 2);'),
    ('alpha_inverse', 'acb_mul(t, alpha, t, w);', 'acb_div(t, t, alpha, w);'),
    ('odd_uses_trivial_poles', 'acb_add_ui(shifted, s, 1, w);', 'acb_set(shifted, s);'),
    ('where_omitted', 'return st == ADF_OK ? ADF_OK : fail(st, where, v);', 'return st;'),
    ('early_output_write', 'if (!acb_is_finite(s)) return fail(ADF_DOMAIN, where, v);',
                          'acb_zero(z);\n    if (!acb_is_finite(s)) return fail(ADF_DOMAIN, where, v);'),
    ('guard_cap_raise',
     'slong w = prec > ADF_REAL_PREC_MAX-32 ? ADF_REAL_PREC_MAX : prec+32;\n'
     '    int st = ADF_NOT_DETERMINED;',
     'slong w = prec > ADF_REAL_PREC_MAX+32 ? ADF_REAL_PREC_MAX : prec+32;\n'
     '    int st = ADF_NOT_DETERMINED;'),
    ('odd_zero_false_pole', 'arf_sgn(arb_midref(acb_realref(s))) >= 0) return 0;',
                          'arf_sgn(arb_midref(acb_realref(s))) > 0) return 0;'),
    ('exact_power_boundary', 'arf_cmp_si(arb_midref(acb_realref(s)), bound) > 0) return 0;',
                             'arf_cmp_si(arb_midref(acb_realref(s)), bound) >= 0) return 0;'),
    ('out_of_bound_false_pole', 'arf_cmp_si(arb_midref(acb_realref(s)), bound) > 0) return 0;',
                               'arf_cmp_si(arb_midref(acb_realref(s)), bound) > 0) return 1;'),
    ('exponent_sign', '    acb_neg(t, t);\n    acb_exp(t, t, w);', '    acb_exp(t, t, w);'),
    ('mixed_pole_domain', 'if (!acb_is_finite(denominator) || acb_contains_zero(denominator)) goto done;',
                         'if (!acb_is_finite(denominator) || acb_contains_zero(denominator))\n'
                         '    { st = ADF_DOMAIN; goto done; }'),
]
records = []
env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
for name, old, new in faults:
    assert body.count(old) == 1, (name, body.count(old))
    changed = body.replace(old, new, 1)
    if name == 'parity_omitted':
        changed = changed.replace('else if (!eta0->parity)', 'else if (1)', 1)
    c = SCRATCH/(name+'.c'); obj = SCRATCH/(name+'.o'); exe = SCRATCH/name
    c.write_text(prefix+changed)
    commands = [
        ['timeout','120','cc','-Iinclude','-Isrc','-std=c11','-O2','-g','-Wall','-Wextra',
         '-c',str(c),'-o',str(obj)],
        ['timeout','120','cc','-Iinclude','-Itests','-std=c11','-O2','-g','tests/test_tate_local.c',
         str(obj),'lanes/t-slice1/build-plain/libadelefeld.a',
         'lanes/t-slice1/build-plain/support/jsonl.o','lanes/t-slice1/build-plain/support/golden.o',
         '-lflint','-lgmp','-lm','-o',str(exe)],
        ['timeout','120',str(exe)],
    ]
    for i, cmd in enumerate(commands):
        proc = sp.run(cmd,cwd=ROOT,env=env,stdout=sp.PIPE,stderr=sp.STDOUT,text=True)
        if i < 2 and proc.returncode:
            print(proc.stdout[-3000:]); raise RuntimeError((name,'compile',proc.returncode))
    lines = proc.stdout.splitlines()
    summary = next((line for line in reversed(lines) if 'failed checks' in line), 'no test summary')
    killed = proc.returncode != 0
    records.append((name,proc.returncode,summary))
    print(name,proc.returncode,summary,flush=True)
    assert killed, ('survived',name)
(LANE/'faults.tsv').write_text(''.join('\t'.join(map(str,r))+'\n' for r in records))
shutil.rmtree(SCRATCH)
print(f'{len(records)} faults killed')
