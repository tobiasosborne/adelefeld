"""Four planted faults per completed slice, in scratch source under the lane build directory."""
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
build = root / 'lanes/f-slice13/build'
source = (root / ('src/fball.c' if sys.argv[1]=='C' else 'src/catalogue.c')).read_text()
faults = {'A': [
    ('A-radius-lcm', 'fmpz_gcd(R,H,t);', 'fmpz_lcm(R,H,t);'),
    ('A-zero-inexact', 'if (k==0 || fmpz_is_zero(H)) fmpz_zero(R);',
     'if (k==0 || fmpz_is_zero(H)) fmpz_one(R);'),
    ('A-last-sample', 'j<=k;j++', 'j<k;j++'),
    ('A-alias-clobber', 'adf_fball_get_fmpz3(A,H,d,x);',
     'adf_fball_one(y); adf_fball_get_fmpz3(A,H,d,x);'),
], 'B': [
    ('B-coarse-criterion', 'fmpz_gcd(D,D,r);', 'fmpz_set(D,u->N);'),
    ('B-centre-no-CRT', 'else if (!fmpz_is_one(B)) fmpz_CRT(r,r,A,one,B,0);',
     'else if (!fmpz_is_one(B)) fmpz_mod(r,r,A);'),
    ('B-inverse-sign', 'if (fmpz_sgn(e)<0)', 'if (fmpz_sgn(e)>0)'),
    ('B-outside-block', 'fmpz_divexact(B,B,h);', 'fmpz_one(B);'),
], 'C': [
    ('C-point-one', 'fmpq_zero(vol->q);', 'fmpq_one(vol->q);'),
    ('C-reciprocal', 'fmpq_set_fmpz_frac(vol->q, x->d, x->H);',
     'fmpq_set_fmpz_frac(vol->q, x->H, x->d);'),
    ('C-denominator', 'fmpq_set_fmpz_frac(vol->q, x->d, x->H);',
     'fmpq_set_si(vol->q,1,1); fmpz_set(fmpq_denref(vol->q),x->H); fmpq_canonicalise(vol->q);'),
    ('C-centre', 'fmpq_set_fmpz_frac(vol->q, x->d, x->H);',
     'fmpq_set_fmpz_frac(vol->q, x->A, x->H);'),
], 'D': [
    ('D-divisibility-reversed', 'fmpz_divisible(N,target)', 'fmpz_divisible(target,N)'),
    ('D-target-not-canonical', 'canon_modulus(target,n);', 'fmpz_set(target,n);'),
    ('D-u-returns-inverse', 'return cyclo(j,x,n,0);', 'return cyclo(j,x,n,1);'),
    ('D-uinv-returns-u', 'return cyclo(j,x,n,1);', 'return cyclo(j,x,n,0);'),
]}
results = []
for name, old, new in faults[sys.argv[1]]:
    assert source.count(old) == 1, (name, source.count(old))
    path = build / (name + '.c')
    path.write_text(source.replace(old, new))
    obj, exe = path.with_suffix('.o'), path.with_suffix('.exe')
    cmd = ['timeout', '60', 'cc', '-std=c11', '-O1', '-g', '-Iinclude', '-Isrc',
           '-c', str(path), '-o', str(obj)]
    c = subprocess.run(cmd, cwd=root, capture_output=True, text=True)
    assert c.returncode == 0, c.stderr
    cmd = ['timeout', '60', 'cc', '-std=c11', '-O1', '-g', '-Iinclude', '-Itests',
           'tests/test_catalogue.c', str(obj), str(build / 'support/jsonl.o'),
           str(build / 'libadelefeld.a'), '-lflint', '-lgmp', '-lm', '-o', str(exe)]
    c = subprocess.run(cmd, cwd=root, capture_output=True, text=True)
    assert c.returncode == 0, c.stderr
    log = build / (name + '.log')
    with log.open('w') as f:
        c = subprocess.run(['timeout', '60', str(exe)], cwd=root, stdout=f, stderr=subprocess.STDOUT)
    lines = log.read_text().splitlines()
    summary = next((line for line in reversed(lines) if 'failed checks' in line), 'no summary')
    first = next((line for line in lines if line.startswith('FAIL')), 'no assertion line')
    results.append(f'{name}: compiled=1 exit={c.returncode}; {summary}; {first}')
    assert c.returncode not in (0, 124), name
out = '\n'.join(results) + '\n'
(root / 'lanes/f-slice13' / ('faults-' + sys.argv[1] + '.log')).write_text(out)
print(out, end='')
