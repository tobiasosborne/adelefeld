"""Compile planted source faults without altering product source or archive."""
from pathlib import Path
import subprocess
import sys
import os
os.environ['ASAN_OPTIONS']='detect_leaks=0'

root = Path(__file__).resolve().parents[2]
build = root / 'lanes/f-slice12/build'
source = (root/'src/symbol.c').read_text()
slice_name = sys.argv[1]
faults = {'A': [
 ('entry-check-removed', 'int adf_fball_jacobi(int *z, adf_place_t *where, const adf_fball_t a, const fmpz_t b)\n{\n    ADF_INV_FBALL(a);', 'int adf_fball_jacobi(int *z, adf_place_t *where, const adf_fball_t a, const fmpz_t b)\n{'),
 ('gcd-removed','fmpz_gcd(g,H,d);',';'),
 ('odd-kron-overprecision','kind == 2 && fmpz_is_even(b)','kind == 2 || fmpz_is_even(b)'),
]}
faults['B'] = [('endpoint-three-digits', 'a->v <= WORD_MAX-3', 'a->v < WORD_MAX-3')]

results = []
for name, old, new in faults[slice_name]:
    assert source.count(old)==1, (name,source.count(old))
    path = build / ('fault-'+name+'.c')
    path.write_text(source.replace(old,new))
    obj = path.with_suffix('.o')
    exe = path.with_suffix('.exe')
    cmd = ['timeout','60','cc','-std=c11','-O1','-g','-Wall','-Wextra','-Werror',
           '-DADF_CHECK_INVARIANTS','-Iinclude','-Isrc','-c',str(path),'-o',str(obj)]
    c = subprocess.run(cmd,cwd=root,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    assert c.returncode==0, c.stdout
    cmd = ['timeout','60','cc','-std=c11','-O1','-g','-DADF_CHECK_INVARIANTS','-Iinclude','-Itests',
           'tests/test_symbol.c',str(obj),str(build/'support/jsonl.o'),
           str(root/'lanes/f-slice12/build-mutbase/libadelefeld.a'),'-fsanitize=address,undefined','-lflint','-lgmp','-lm','-pthread','-o',str(exe)]
    c = subprocess.run(cmd,cwd=root,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    assert c.returncode==0, c.stdout
    with (build/('fault-'+name+'.log')).open('w') as log:
        c = subprocess.run(['timeout','60',str(exe)],cwd=root,stdout=log,stderr=subprocess.STDOUT)
    lines = (build/('fault-'+name+'.log')).read_text().splitlines()
    fail = next((l for l in lines if l.startswith('FAIL')), 'no assertion line')
    results.append(f'{name}: compile=0 run={c.returncode}; {fail}')
    assert c.returncode!=0, name
(root/'lanes/f-slice12'/('survivors-'+slice_name+'.log')).write_text('\n'.join(results)+'\n')
print('\n'.join(results))
