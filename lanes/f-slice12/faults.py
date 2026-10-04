"""Compile planted source faults without altering product source or archive."""
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
build = root / 'lanes/f-slice12/build'
source = (root/'src/symbol.c').read_text()
slice_name = sys.argv[1]
faults = {
 'A': [
  ('two-residues', 'r == 1 || r == 7', 'r == 1 || r == 5'),
  ('even-nonzero', 'if (fmpz_is_even(a)) return 0;', 'if (fmpz_is_even(a)) return 1;'),
  ('coarse-accepted', 'st = fmpz_divisible(N,K) ? ADF_OK : ADF_NOT_DETERMINED;',
   'st = ADF_OK;'),
  ('odd-jacobi', 'fmpz_is_even(b)) ? ADF_DOMAIN', 'fmpz_is_odd(b)) ? ADF_DOMAIN'),
  ('value-clobber', 'if (st == ADF_OK) *z = symbol(kind,a,b);',
   'if (st == ADF_OK) *z = symbol(kind,a,b); else *z = 0;'),
  ('where-missing', 'if (st != ADF_OK && where != NULL) *where = p;',
   'if (st == ADF_OK && where != NULL) *where = p;'),
 ]
}
faults['B'] = [
 ('epsilon-omitted', 'int exponent=eps_u*eps_w + alpha*om_w + beta*om_u;',
  'int exponent=alpha*om_w + beta*om_u; (void)eps_u; (void)eps_w;'),
 ('valuation-parity-ignored', 'c->parity = an%2 != bn%2;',
  'c->parity = 0; (void)an; (void)bn;'),
 ('real-negative-positive', '*z=arb_is_negative(a) && arb_is_negative(b) ? -1 : 1;',
  '*z=1;'),
 ('cofactor-omitted', 'ulong residue=(ulong)scale.u[0]*fmpz_fdiv_ui(a->u.c,8)%8;',
  'ulong residue=fmpz_fdiv_ui(a->u.c,8);'),
 ('coarse-local-accepted', 'two_classes(c,unit_residue(a->u,8),k);',
  'two_classes(c,unit_residue(a->u,8),3); (void)k;'),
 ('odd-reciprocity-sign-omitted', 'alpha && beta && p%4==3',
  'alpha && beta && p%4==1'),
]

results = []
for name, old, new in faults[slice_name]:
    assert source.count(old)==1, (name,source.count(old))
    path = build / ('fault-'+name+'.c')
    path.write_text(source.replace(old,new))
    obj = path.with_suffix('.o')
    exe = path.with_suffix('.exe')
    cmd = ['timeout','60','cc','-std=c11','-O1','-g','-Wall','-Wextra','-Werror',
           '-Iinclude','-Isrc','-c',str(path),'-o',str(obj)]
    c = subprocess.run(cmd,cwd=root,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    assert c.returncode==0, c.stdout
    cmd = ['timeout','60','cc','-std=c11','-O1','-g','-Iinclude','-Itests',
           'tests/test_symbol.c',str(obj),str(build/'support/jsonl.o'),
           str(build/'libadelefeld.a'),'-lflint','-lgmp','-lm','-o',str(exe)]
    c = subprocess.run(cmd,cwd=root,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    assert c.returncode==0, c.stdout
    with (build/('fault-'+name+'.log')).open('w') as log:
        c = subprocess.run(['timeout','60',str(exe)],cwd=root,stdout=log,stderr=subprocess.STDOUT)
    lines = (build/('fault-'+name+'.log')).read_text().splitlines()
    fail = next((l for l in lines if l.startswith('FAIL')), 'no assertion line')
    results.append(f'{name}: compile=0 run={c.returncode}; {fail}')
    assert c.returncode!=0, name
(root/'lanes/f-slice12'/('faults-'+slice_name+'.log')).write_text('\n'.join(results)+'\n')
print('\n'.join(results))
