#!/usr/bin/env python3
"""Named scratch-only mathematical and transaction faults; never edits src/ffun.c."""
import json
import os
from pathlib import Path
import subprocess
import sys
ROOT = Path(__file__).resolve().parents[2]
LANE = ROOT / 'lanes/f4-slice1'
BUILD = LANE / ('combined' if '--san' in sys.argv else 'build')
SCRATCH = LANE / 'faults'
SCRATCH.mkdir(exist_ok=True)
source = (ROOT / 'src/ffun.c').read_text()
faults = [
    ('identity_one_dimension', 'if (x->D!=y->D || x->M!=y->M)',
                               'if (x->D!=y->D && x->M!=y->M)'),
    ('sum_precision_two', 'slong p=FLINT_MAX(prec,2);', 'slong p=FLINT_MAX(2,2);'),
    ('drop_swap_first_INV', 'ADF_INV_FFUN(x); ADF_INV_FFUN(y); t=*x; *x=*y; *y=t;',
                             'ADF_INV_FFUN(y); t=*x; *x=*y; *y=t;'),
    ('fourier_early_commit', 'ffun_allocate(t,x->M,x->D);',
                             'ffun_allocate(t,x->M,x->D);\n'
                             '    if (prec==ADF_REAL_PREC_MAX) adf_ffun_set(y,t);'),
    ('positive_sign', 'h ? L-h : 0,L', 'h,L'),
    ('weight_D', 'acb_div_ui(t->f+k,t->f+k,x->M,p);', 'acb_div_ui(t->f+k,t->f+k,x->D,p);'),
    ('retain_layout', 'ffun_allocate(t,x->M,x->D);', 'ffun_allocate(t,x->D,x->M);'),
    ('phase_den_D', 'h ? L-h : 0,L', '(j*k)%x->D,x->D'),
    ('max_D', 'ffun_lcm(&D,x->D,y->D)!=ADF_OK', '(D=FLINT_MAX(x->D,y->D),0)'),
    ('max_M', 'ffun_lcm(&M,x->M,y->M)!=ADF_OK', '(M=FLINT_MAX(x->M,y->M),0)'),
    ('fill_holes', 'if (k%r==0) acb_set(t->f+k,x->f+(k/r)%L);',
                   'acb_set(t->f+k,x->f+(k/r)%L);'),
    ('lose_repetition', 'if (k%r==0) acb_set(t->f+k,x->f+(k/r)%L);',
                        'if (k%r==0 && k/r<L) acb_set(t->f+k,x->f+(k/r)%L);'),
    ('cap_after_allocation', 'if (L && L>ADF_FFUN_ITEMS_MAX/L) return ADF_LIMIT;', ''),
    ('drop_input_radius', 'acb_mul(term,x->f+j,phase,p);',
                          'acb_get_mid(term,x->f+j); acb_mul(term,term,phase,p);'),
    ('drop_output_radius', 'acb_div_ui(t->f+k,t->f+k,x->M,p);',
                           'acb_div_ui(t->f+k,t->f+k,x->M,p); acb_get_mid(t->f+k,t->f+k);'),
    ('commit_before_validation', 'for (slong j=0;j<n;j++) if (!acb_is_finite(f+j)) return ADF_DOMAIN;',
                                 'ffun_allocate(t,D,M); _acb_vec_set(t->f,f,n);\n'
                                 '    adf_ffun_swap(y,t); ffun_dispose(t);\n'
                                 '    for (slong j=0;j<n;j++) if (!acb_is_finite(f+j)) return ADF_DOMAIN;'),
]
results = []
for name, old, new in faults:
    if name == 'drop_swap_first_INV' and '--san' not in sys.argv:
        continue
    assert old in source, name
    altered = source.replace(old,new,1)
    if name == 'phase_den_D':
        altered = altered.replace('ulong h=(j*k)%L;', 'ulong h=(j*k)%L; (void)h;',1)
    if name == 'cap_after_allocation':
        altered = altered.replace('ffun_allocate(t,x->M,x->D);',
            'ffun_allocate(t,x->M,x->D);\n'
            '    if (L && L>ADF_FFUN_ITEMS_MAX/L) { ffun_dispose(t); return ADF_LIMIT; }',1)
    src = SCRATCH / (name+'.c')
    binary = SCRATCH / name
    src.write_text(altered)
    cmd = ['timeout','30','clang' if '--san' in sys.argv else 'cc',
           '-Iinclude','-Isrc','-Itests','-std=c11','-O1','-g','-Wall','-Wextra','-Werror']
    if '--san' in sys.argv:
        cmd += ['-DADF_CHECK_INVARIANTS','-fsanitize=address,undefined','-fno-omit-frame-pointer']
    cmd += [str(src),'tests/test_ffun.c',str(BUILD/'support/golden.o'),str(BUILD/'support/jsonl.o'),
            str(BUILD/'libadelefeld.a'),'-lflint','-lgmp','-lm','-pthread','-o',str(binary)]
    compiled = subprocess.run(cmd,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    run = None
    output = compiled.stdout
    if compiled.returncode == 0:
        env = dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
        run = subprocess.run(['timeout','30',str(binary)],cwd=ROOT,env=env,
                             stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        output += run.stdout
    (SCRATCH/(name+'.log')).write_bytes(output[:100000])
    record = dict(name=name,compile=compiled.returncode,run=run.returncode if run else None,
                  command=' '.join(cmd),diagnostic=[line for line in output.decode(errors='replace').splitlines()
                              if line and not line.startswith('timeout:')][:2])
    results.append(record)
    print(name,record['compile'],record['run'],record['diagnostic'],flush=True)
(LANE/'fault-results.json').write_text(json.dumps(results,indent=2)+'\n')
assert all(r['compile']==0 and r['run'] not in (None,0,124) for r in results)
