"""Selection of IL8 rows and complete H=5 image sets for 20 representative rows."""
import sys, json
from pathlib import Path
sys.path.insert(0, 'proto')
import idlog_checks as o
import mpmath as mp
out = Path('tests/ref/vectors/f-slice11')
rows = []
for p, r, c, M in o.grid():
    k = o.valuation(M, p)
    m = o.unit_part(r, p)[0]
    if M // p**k not in (1, 11) or m not in (-2, 0, 2) or c not in {1, M-1, p}:
        continue
    E = o.image_exponent(p, M)
    for N in sorted({-2, 0, 1, 2, E-1, E, E+1, 6}):
        row = dict(p=p, r=list(r), c=c, M=M, requested=N,
                   expected=o.expected_ball(p, r, c, M, N))
        if r == (1,1) and c == 1 and M == p**k and N == 6:
            image, count = o.enumerated_image(p, r, c, M, 5)
            row.update(H=5, image=sorted(image), enumerated_units=count)
        rows.append(row)
# The eight exact witnesses of the design's fault table are included verbatim.
for i,(p,r,c,M,N) in enumerate(((3,(3,1),1,9,5),(3,(1,3),1,9,5),(3,(4,1),1,9,5),
                              (2,(1,1),1,1,5),(3,(1,1),1,1,5),(2,(1,1),1,8,1),
                              (2,(1,1),1,2,5),(3,(3,1),1,0,5)),1):
    rows.append(dict(p=p,r=list(r),c=c,M=M,requested=N,witness=i,
                     expected=o.expected_ball(p,r,c,M,N)))
for p in (2,3,5,7):
    for r in o.contents(p):
        for c in (-1,1):
            for N in (-2,0,1,2,6):
                rows.append(dict(p=p,r=list(r),c=c,M=0,requested=N,
                                 expected=o.expected_ball(p,r,c,0,N)))
with (out/'local.jsonl').open('w') as f:
    for row in rows:
        f.write(json.dumps(row, sort_keys=True, separators=(',', ':'))+'\n')
mp.mp.prec = 800
real = []
for m,rm,e in ((1,0,0),(2,0,0),(5,3,-2),(-1,0,0),(-2,0,0),(-5,3,-2)):
    lo,hi = sorted((abs(mp.mpf(m-rm)*mp.power(2,e)),abs(mp.mpf(m+rm)*mp.power(2,e))))
    scale = mp.power(2,500)
    real.append(dict(m=m,rm=rm,e=e,lo=str(int(mp.floor(mp.log(lo)*scale))),
                     hi=str(int(mp.ceil(mp.log(hi)*scale))),bound_exp=-500))
with (out/'real.jsonl').open('w') as f:
    for row in real:
        f.write(json.dumps(row, sort_keys=True, separators=(',', ':'))+'\n')
print('selection rows',len(rows),'image rows',sum('image' in r for r in rows),
      'enumerated units',sum(r.get('enumerated_units',0) for r in rows),
      'bytes',sum(p.stat().st_size for p in out.glob('*.jsonl')))
