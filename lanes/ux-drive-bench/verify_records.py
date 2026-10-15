"""Independent replay and finite arithmetic checks, without importing the slice."""
import collections
from fractions import Fraction
import json
import pathlib
import re
import subprocess
import sys

CLASS = {'OK':'ok', 'DOMAIN':'no', 'NO_SOLUTION':'no', 'NOT_UNIT':'no',
         'NOT_DETERMINED':'open', 'UNIT_NOT_CERTIFIED':'open', 'NEEDS_SPLIT':'open',
         'NOT_UNIQUE':'open', 'LIMIT':'limit', 'PARSE':'bad', 'UNSUPPORTED':'bad'}

def square(n, p):
    if p == 'R':
        return n > 0
    m = 0
    while n % p == 0:
        n //= p
        m += 1
    if m % 2:
        return False
    return n % 8 == 1 if p == 2 else any(x*x % p == n % p for x in range(p))

def poly(c, x):
    v = 0
    for a in reversed(c):
        v = v*x + a
    return v

for arg in sys.argv[1:]:
    path = pathlib.Path(arg)
    rows = [json.loads(s) for s in path.read_text().splitlines()]
    executable = [r for r in rows if r.get('output') is not None]
    stem = pathlib.Path('lanes/ux-drive-bench') / (path.stem + '.replay')
    digits = 10 if path.stem == 'exp_edge.records' else 20
    commands = f'digits {digits}\n' + ''.join(f"prec {r.get('prec',64)}\n{r['line']}\n" for r in executable)
    stem.with_suffix(stem.suffix + '.cmd').write_text(commands)
    proc = subprocess.run(['timeout','60','build/adf'], input=commands, text=True, capture_output=True)
    stem.with_suffix(stem.suffix + '.out').write_text(proc.stdout)
    got = proc.stdout.splitlines()
    assert len(got) == len(executable), (len(got),len(executable),proc.stderr)
    matched = finite = real = oracle = 0
    for r, output in zip(executable, got):
        assert output == r['output'], (r,output)
        matched += 1
        status = output.removeprefix('error: ') if output.startswith('error: ') else 'OK'
        assert status == r['status'], r
        assert CLASS[status] == r['class'], r
        if r['kind'] != 'well':
            continue
        p = r['row']
        if r['line'].startswith('roots_at '):
            
            if not re.fullmatch(r'-?\d+',r['input']):
                continue
            n = int(r['input'])
            want = square(n,p)
            assert status == ('OK' if want else 'DOMAIN'), r
            assert r['count'] == (2 if want else None), r
            oracle += 1
            if want:
                roots = re.findall(r'(\d+) \[(\d+)\]: (\d+) \+ O\((\d+)\^(\d+)\)',output)
                assert len(roots) == 2, r
                for place,seed,a,base,k in roots:
                    place,seed,a,base,k = map(int,[place,seed,a,base,k])
                    assert place == base == p and (a*a-n) % p**k == 0, r
                    if p != 2:
                        assert a % p == seed, r
                    finite += 1
        elif r['line'].startswith(('roots ', 'realroots ')):
            polynomial = r['line'].split(' with ')[0].split()[1:]
            coeff = list(map(int,polynomial))
            if len(coeff) == 3 and coeff[1:] == [0,1]:
                ns = [-coeff[0]]
            elif coeff == [-48841,0,6851,0,-251,0,1]:
                ns = [13,17,221]
            else:
                continue
            want = 2*sum(square(n,p) for n in ns)
            assert status == 'OK' and r['count'] == want, (r,want)
            oracle += 1
            if p == 'R' and want:
                for ball in output.split('; '):
                    center,radius=map(Fraction,ball.split(' +/- '))
                    assert poly(coeff,center-radius)*poly(coeff,center+radius)<0, r
                    real += 1
            elif p != 'R':
                for a,base,k in re.findall(r'(\d+) mod (\d+)\^(\d+)',output):
                    a,base,k=map(int,[a,base,k])
                    assert base == p and poly(coeff,a) % p**k == 0, r
                    finite += 1
    print(json.dumps(dict(file=str(path), records=len(rows), replayed=matched, driver_exit=proc.returncode,
        status_counts=dict(collections.Counter(r['status'] for r in executable)),
        existence_checks=oracle, finite_roots_checked=finite, real_intervals_checked=real)))
    if path.stem in ['brief_a.records','brief_a_1000.records']:
        for n in ['-1','2']:
            primes=[r['row'] for r in rows if r.get('input')==n and r['count']==2 and r['row']!='R']
            print(n, 'square primes:',primes)
