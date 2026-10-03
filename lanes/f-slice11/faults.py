"""Scratch source copies; rebuild only the changed translation unit against the lane archive."""
import subprocess, sys
from pathlib import Path
root=Path.cwd()
build=root/'lanes/f-slice11/build'
source=(root/'src/gfunc_log.c').read_text()
faults={
 'F1 absolute m+k image exponent':('if (d->k > E) E = d->k;', 'if (d->k > E) E = d->k; E += d->m;'),
 'F2 missing input valuation then subtract m':('if (d->k > E) E = d->k;', 'if (d->k > E) E = d->k; E -= d->m;'),
 'F3 content translation dropped':('fmpz_mod(residue, fmpq_numref(d.unit), mod);','fmpz_one(residue);'),
 'F4 unrestricted 2 gives 2 Z_2':('E = p == 2 ? 2 : 1;', 'E = 1;'),
 'F5 unrestricted odd gives Z_p':('if (d->k > E) E = d->k;', 'if (d->k > E) E = d->k; if (p != 2 && d->k == 0) E = 0;'),
 'F6 requested cap ignored':('d->K = d->exact || N < E ? N : E;', 'd->K = d->exact ? N : E;'),
 'F7 stored k=1 treated as ordinary':('if (d->k > E) E = d->k;', 'if (d->k > E) E = d->k; if (p == 2 && d->k == 1) E = 1;'),
 'F8 exact unit treated as unrestricted':('d->exact = fmpz_is_zero(x->u.N);','d->exact = 0;'),
 'O1 Log silently takes absolute value':('return conservative(y, where, x, prec, adf_sball_Log_at);',
                                         'return conservative(y, where, x, prec, adf_sball_log_abs_at);'),
 'O2 real coordinate from content':('st = at(t, NULL, s, adf_place_inf(), prec);',
                                    '{ arb_set_fmpq(acb_realref(s->inf), x->r, prec); st = at(t, NULL, s, adf_place_inf(), prec); }'),
 'O3 baseline factor four replaced by two':('fmpz_set_ui(t->fin.H, 4);\n        adf_adele_swap(y, t);',
                                         'fmpz_set_ui(t->fin.H, 2);\n        adf_adele_swap(y, t);'),
 'O4 OK writes where':('if (st == ADF_OK) adf_sball_swap(y, t);',
                       'if (st == ADF_OK) { adf_sball_swap(y, t); if (where != NULL) *where = v; }'),
}
if len(sys.argv)>1 and sys.argv[1]=='B':
    faults={**{k:v for k,v in faults.items() if k.startswith('F')},
      'B1 CRT baseline dropped':('fmpz_set_ui(t->fin.H, 4); /* baseline */','fmpz_set_ui(t->fin.H, 2); /* baseline */'),
      'B2 exact zero not rounded':('K = l->exact ? N : l->N;', 'K = l->exact ? 0 : l->N;'),
      'B3 stop before larger prime status':('if (st_real > st_fin) st = st_real;', 'if (st_real != ADF_OK) st = st_real;'),
      'B4 CRT centre always zero':('fmpz_CRT(A, t->fin.A, t->fin.H, b, q, 0);','fmpz_zero(A);'),
    }
scratch=build/'faults'
scratch.mkdir(exist_ok=True)
for name,(old,new) in faults.items():
    if source.count(old)!=1: raise RuntimeError((name,source.count(old)))
    d=scratch/name.split()[0]; d.mkdir(exist_ok=True)
    (d/'gfunc_log.c').write_text(source.replace(old,new))
    cmd=['timeout','120','cc','-Iinclude','-Isrc','-Itests','-DADF_CHECK_INVARIANTS','-std=c11',
         '-O1','-g',str(d/'gfunc_log.c'),'tests/test_gfunc_log.c',str(build/'support/jsonl.o'),
         str(build/'support/golden.o'),str(build/'libadelefeld.a'),'-lflint','-lgmp','-lm','-pthread','-o',str(d/'test')]
    cp=subprocess.run(cmd,capture_output=True,text=True)
    if cp.returncode:
        print(name,'NOT COMPILED',cp.returncode,cp.stderr[-1000:],flush=True);continue
    with (d/'run.log').open('w') as f:
        cp=subprocess.run(['timeout','90',str(d/'test')],stdout=f,stderr=subprocess.STDOUT,
                          preexec_fn=lambda: __import__('resource').setrlimit(__import__('resource').RLIMIT_AS,
                                                                             (4*1024**3,4*1024**3)))
    lines=(d/'run.log').read_text().splitlines()
    failed=next((x for x in lines if x.startswith('FAIL ')),lines[-1] if lines else '(empty)')
    print(name,'exit',cp.returncode,failed,flush=True)
    if cp.returncode==0: raise RuntimeError('fault survived: '+name)
print('faults: compiled and rejected',len(faults),flush=True)
