"""Exact IL8 refinements, shuffled place lists, and complete membership over two CRT periods."""
import sys,json
from pathlib import Path
sys.path.insert(0,'proto')
import idlog_checks as o
rows=[]
for r,c,M in (((4,1),1,9),((3,4),5,36),((7,3),1,1),((1,1),-1,0),((4,1),1,0)):
    for S in ((),(2,),(3,),(2,3),(3,5),(2,3,5),(5,3,2),(7,5,3)):
        for N in (-2,0,1,2,3,4,6):
            conditions={}
            local=[]
            for p in S:
                b,L=o.expected_refinement(p,r,c,M,N)
                local.append(dict(p=p,center=b,exponent=L))
                if L: conditions[p]=(b,p**L)
            conditions.setdefault(2,(0,4))
            a,R=o.crt([conditions[p] for p in sorted(conditions)])
            # Big moduli are field/projected checks; no memory-sized residue array is stored.
            rows.append(dict(r=list(r),c=c,M=M,primes=list(S),N=N,A=str(a),R=str(R),local=local))
path=Path('tests/ref/vectors/f-slice11/crt.jsonl')
with path.open('w') as f:
    for row in rows:f.write(json.dumps(row,separators=(',',':'),sort_keys=True)+'\n')
print('CRT rows',len(rows),'bytes',path.stat().st_size,'all vector bytes',
      sum(p.stat().st_size for p in path.parent.glob('*.jsonl')))
