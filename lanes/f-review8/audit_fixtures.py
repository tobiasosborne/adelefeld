#!/usr/bin/env python3
from pathlib import Path
import json
from math import gcd

def vp(a,p):
    k=0
    while a%p==0: k+=1; a//=p
    return k
pow_rows=[json.loads(s) for s in Path('tests/ref/vectors/f-slice9/powrat_balls.jsonl').read_text().splitlines()]
print('powrat stored rows',len(pow_rows),'nprime1',sum(r['n']//gcd(abs(r['e']),r['n'])==1 for r in pow_rows),
      'independently_enumerated_branch_images',sum(len(r['ok']) for r in pow_rows))
scaled=0
for p in (2,3,5,7):
    c=2 if p==2 else 1
    for n in range(2,13):
        for r in range(1,(5 if p==2 else 3)+1):
            scaled+=2*int(r>=c+vp(n,p))
print('lroot generated scaled rows',scaled,'source lines 72-75; proof scaling, not new enumeration')
for fn in ('powrat_exact','powunit'):
    rows=[json.loads(s) for s in Path(f'tests/ref/vectors/f-slice9/{fn}.jsonl').read_text().splitlines()]
    print(fn,'rows',len(rows))
print('Julia and driver certificates: 6520516^2 mod 5^10=',pow(6520516,2,5**10),
      '379^3 mod 2^10=',pow(379,3,2**10),'516 mod 5^4=',6520516%(5**4))
assert pow(6520516,2,5**10)==6 and pow(379,3,2**10)==3 and 6520516%(5**4)==516
# Audit the literal exact-image phrase in N-D15 by an independent residue-set comparison.
H=4
image={(1+9*t)%(3**H) for t in range(3**(H-2))}
returned={(1+3*t)%(3**H) for t in range(3**(H-1))}
assert image<returned and 4 in returned and 4 not in image
print('N-D15 literal exact-image counterexample: actual_count',len(image),'returned_count',len(returned),
      'precision=3^4 extra_point=4')
