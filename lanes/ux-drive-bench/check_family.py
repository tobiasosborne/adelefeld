"""Check the brief B family, from records and independently enumerated square classes."""
import collections
from fractions import Fraction
import json
import math
import pathlib
import re
import subprocess

lane=pathlib.Path('lanes/ux-drive-bench')
rows=[json.loads(s) for s in (lane/'out/brief_b_family.records.jsonl').read_text().splitlines()]
primes=[p for p in range(2,2000) if all(p%d for d in range(2,math.isqrt(p)+1))]
residues={p:{x*x%p for x in range(p)} for p in primes}

def square(n,p):
    if p=='R':
        return True
    v=0
    while n%p==0:
        n//=p
        v+=1
    return v%2==0 and (n%8==1 if p==2 else n%p in residues[p])

commands=''.join(f"prec {r['prec']}\n{r['line']}\n" for r in rows)
(lane/'family.replay.cmd').write_text(commands)
proc=subprocess.run(['timeout','90','build/adf'],input=commands,text=True,capture_output=True)
(lane/'family.replay.out').write_text(proc.stdout)
assert proc.returncode==0 and len(proc.stdout.splitlines())==len(rows)
failures={};finite=real=0;columns=collections.Counter()
for r,output in zip(rows,proc.stdout.splitlines()):
    assert r['output']==output and r['status']=='OK' and r['class']=='ok',r
    a,b=map(int,re.findall(r'\d+',r['input'])); ns=[a,b,a*b];p=r['row']
    want=2*sum(square(n,p) for n in ns)
    assert r['count']==want,(r,want)
    columns[(a,b)]+=1
    if not want:
        failures.setdefault((a,b),p)
    if p=='R':
        for ball in output.split('; '):
            c,e=map(Fraction,ball.split(' +/- '))
            def value(x):
                return (x*x-a)*(x*x-b)*(x*x-a*b)
            assert value(c-e)*value(c+e)<0,r
            real+=1
    elif want:
        roots=re.findall(r'(\d+) mod (\d+)\^(\d+)',output)
        assert len(roots)==want,r
        for x,base,k in roots:
            x,base,k=map(int,[x,base,k]);assert p==base
            assert (x*x-a)*(x*x-b)*(x*x-a*b)%p**k==0,r
            finite+=1
survivors=sorted(set(columns)-set(failures))
assert survivors==[(2,17),(5,29),(10,26),(13,17),(13,29),(17,19),(17,26)]
assert len(columns)==153 and set(columns.values())=={304}
print(json.dumps(dict(records=len(rows),replayed=len(rows),count_checks=len(rows),prime_count=len(primes),
                     last_prime=primes[-1],pairs=len(columns),survivors=survivors,
                     failing_pairs=len(failures),finite_roots_checked=finite,real_intervals_checked=real)))
(lane/'family_summary.json').write_text(json.dumps({'survivors':survivors,
    'first_failures':{str(k):v for k,v in failures.items()}},indent=2)+'\n')
