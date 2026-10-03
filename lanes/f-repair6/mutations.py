#!/usr/bin/env python3
"""Own faults, linked before the one archive. Writes only in this lane."""
from pathlib import Path
import subprocess
import sys
import json
import re
import resource
BASE = Path('lanes/f-repair6/build')
OUT = BASE / 'faults'
OUT.mkdir(exist_ok=True)
LIB = BASE / 'libadelefeld.a'
FLAGS = ['-std=gnu11', '-O2', '-g', '-Iinclude', '-Itests', '-Isrc']
TESTS = ['test_lpow', 'test_lroot', 'test_rfunc_prime']
def run(cmd, log):
    with open(log, 'w') as f:
        return subprocess.run(['timeout', '150', *map(str, cmd)], stdout=f,
                              stderr=subprocess.STDOUT,
                              preexec_fn=lambda: resource.setrlimit(resource.RLIMIT_AS,
                                                                    (1024**3, 1024**3))).returncode
POW = Path('src/lpow.c').read_text()
ROOT = Path('src/lroot.c').read_text()
FORM = 'add_inf(ej, clamp(x->N - x->v - s + ve))'
faults = [
 ('E_ej_plus1', 'lpow', FORM, 'add_inf(add_inf(ej, 1), clamp(x->N - x->v - s + ve))'),
 ('E_relative_minus1', 'lpow', FORM, 'add_inf(ej, clamp((x->N - x->v - 1) - s + ve))'),
 ('E_denominator_plus1', 'lpow', FORM, 'add_inf(ej, clamp(x->N - x->v - (s + 1) + ve))'),
 ('E_numerator_plus1', 'lpow', FORM, 'add_inf(ej, clamp(x->N - x->v - s + (ve + 1)))'),
 ('R_drop_A_beta', 'lpow', 'if (A < INF) R = min2(R, add_inf(A, beta));', 'if (A < INF) R = R;'),
 ('R_drop_B_alpha', 'lpow', 'R = min2(R, add_inf(B, fmpq_is_zero(l->u) ? INF : l->v));', 'R = R;'),
 # Unconditional A+B omission is a prior fault; this own fault omits it only at an uncovered prime.
 ('R_drop_A_B_65537', 'lpow', 'if (A < INF && B < INF) R = min2(R, add_inf(A, B));',
  'if (p != 65537 && A < INF && B < INF) R = min2(R, add_inf(A, B));'),
 ('hull_one_negative_sign', 'lpow',
  'st = signed_one(res, p, 1, min2(Nc, 1), 0);           /* the hull 1 + 2 Z_2 (P5) */',
  '{ adf_lball_t odd; adf_lball_init(odd); odd->p=2; fmpq_one(odd->u); '
  'odd->v=0; odd->N=1; odd->exact=0; '
  'st=principal(res,u,odd,A,1,0,-1,-1,Nc); adf_lball_clear(odd); }'),
 ('powunit_K_N', 'lpow', 'K = min2(Nc, R);', 'K = Nc;'),
 ('root_K_N', 'lroot', 'sh->K=N<E ? N : E;', 'sh->K=N;'),
 ('zeta_order_d_over_q', 'lroot', 'zeta=n_powmod2_ui_preinv(g,(p-1)/d,p,pinv);',
  'ulong q=2; while (d%q) q++; zeta=n_powmod2_ui_preinv(g,((p-1)/d)*q,p,pinv);'),
 ('CRT_swap_two_idempotents', 'lroot',
  'Aq=n_powmod2_ui_preinv(A,n_mulmod2_preinv(P/Q,n_invmod((P/Q)%Q,Q),P,Pinv),p,pinv);',
  'ulong Qbad=Q; if (fd.num>1) { ulong rb=P; ulong qb=fd.p[(i+1)%fd.num]; '
  'Qbad=n_pow(qb,(ulong)n_remove(&rb,qb)); } '
  'Aq=n_powmod2_ui_preinv(A,n_mulmod2_preinv(P/Qbad,n_invmod((P/Qbad)%Qbad,Qbad),P,Pinv),p,pinv);'),
 ('early_status_always_OK', 'lroot', 'ulong general=d-(ulong)sh->nrat, one=0, p=x->p;',
  'return ADF_OK; ulong general=d-(ulong)sh->nrat, one=0, p=x->p;'),
 ('seed_mod_p_minus1', 'lroot', 'power_mod(seed,n,x->p)!=index', 'power_mod(seed,n,x->p-1)!=index'),
 ('powrat_wrong_centre_65537', 'lpow', 'if (st == ADF_OK) adf_lball_set(y, res);\n    adf_lball_clear(r);',
  'if (st == ADF_OK) { if (p == 65537 && !res->exact && !fmpq_is_zero(res->u)) '
  'fmpz_add_ui(fmpq_numref(res->u), fmpq_numref(res->u), p); adf_lball_set(y, res); }\n    adf_lball_clear(r);'),
 ('root_wrong_unsampled_lift', 'lroot', '    adf_lball_set(y,res);',
  '    if (x->p==65537 && seed==42 && !res->exact && !fmpq_is_zero(res->u)) '
  'fmpz_add_ui(fmpq_numref(res->u),fmpq_numref(res->u),x->p); adf_lball_set(y,res);'),
 # R9 bypasses branch() for the fused list. Also plant the same shift at its new write site.
 ('root_wrong_fused_lift', 'lroot', '            fmpq_set_fmpz(tmp[k].u,y);',
  '            fmpq_set_fmpz(tmp[k].u,y); if (p==65537 && seed==42) '
  'fmpz_add_ui(fmpq_numref(tmp[k].u),fmpq_numref(tmp[k].u),p);'),
 # A pure cost fault: preserve early_status's initialization of z0, but defer it until after ids.
 ('early_status_after_identifiers', 'lroot',
  '    st=early_status(&sh,x,n,d,index);\n'
  '    if (st!=ADF_OK) { shared_clear(&sh); return st; }\n'
  '    tmpids=flint_malloc((size_t)d*sizeof(ulong));\n'
  '    st=identifiers(tmpids,&t0,&zeta,x->p,n,d,index);\n'
  '    if (st!=ADF_OK) { flint_free(tmpids); shared_clear(&sh); return st; }',
  '    tmpids=flint_malloc((size_t)d*sizeof(ulong));\n'
  '    st=identifiers(tmpids,&t0,&zeta,x->p,n,d,index);\n'
  '    if (st==ADF_OK) st=early_status(&sh,x,n,d,index);\n'
  '    if (st!=ADF_OK) { flint_free(tmpids); shared_clear(&sh); return st; }'),
]
def prepare():
    for name in TESTS:
        assert run(['cc', *FLAGS, '-c', f'tests/{name}.c', '-o', BASE / f'{name}.test.o'],
                   OUT / f'{name}.compile.log') == 0
    for name in ('golden', 'jsonl'):
        assert run(['cc', *FLAGS, '-c', f'tests/support/{name}.c', '-o', BASE / f'{name}.support.o'],
                   OUT / f'{name}.compile.log') == 0

def build(name, unit, source):
    path = OUT / name
    path.mkdir(exist_ok=True)
    (path / f'{unit}.c').write_text(source)
    assert run(['cc', *FLAGS, '-c', path / f'{unit}.c', '-o', path / f'{unit}.o'], path / 'compile.log') == 0
    for test in TESTS:
        cmd = ['cc', BASE / f'{test}.test.o', BASE / 'golden.support.o', BASE / 'jsonl.support.o',
               path / f'{unit}.o', LIB, '-lflint', '-lgmp', '-lm', '-o', path / test]
        assert run(cmd, path / f'{test}.link.log') == 0
        code = run([path / test], path / f'{test}.log')
        txt = (path / f'{test}.log').read_text()
        match = re.search(r'(\d+) tests, (\d+) checks, (\d+) failed checks, (\d+) failed tests', txt)
        row = {'fault': name, 'test': test, 'exit': code,
               'counts': list(map(int, match.groups())) if match else None,
               'emitted_failed_checks': len(re.findall(r'^FAIL .*check failed:', txt, re.M))}
        with open(OUT / 'results.jsonl', 'a') as f:
            f.write(json.dumps(row) + '\n')
        print(json.dumps(row), flush=True)
if sys.argv[1] == 'prepare':
    prepare()
elif sys.argv[1] == 'baseline':
    build('baseline', 'lpow', POW)
else:
    for name, unit, old, new in faults[int(sys.argv[1]):int(sys.argv[2])]:
        text = POW if unit == 'lpow' else ROOT
        assert text.count(old) == 1, (old, text.count(old))
        build(name, unit, text.replace(old, new))
