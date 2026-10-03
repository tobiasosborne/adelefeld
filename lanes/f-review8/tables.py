#!/usr/bin/env python3
from pathlib import Path
import json
import re
P=Path('lanes/f-review8')
rows=[json.loads(s) for s in (P/'build/faults/results.jsonl').read_text().splitlines()]
last={(r['fault'],r['test']):r for r in rows}
names=list(dict.fromkeys(r['fault'] for r in rows if r['fault']!='baseline'))
lines=['| Fault | lpow failed/checks | lroot failed/checks | rfunc failed/checks |',
       '|---|---:|---:|---:|']
for name in names:
    cells=[]
    for test in ('test_lpow','test_lroot','test_rfunc_prime'):
        r=last[name,test]
        if r['counts']:
            counts=r['counts']; cells.append(f'{counts[2]}/{counts[1]}')
        else:
            log=(P/'build/faults'/name/f'{test}.log').read_text()
            count=sum(': check failed:' in line for line in log.splitlines())
            cells.append(f'SIGSEGV; {count} emitted')
    lines.append('| '+name+' | '+' | '.join(cells)+' |')
(P/'fault-table.md').write_text('\n'.join(lines)+'\n')
print('fault_runs',len(rows),'final_faults',len(names),'test_timeouts',sum(r['exit']==124 for r in rows),
      'baseline_runs',sum(r['fault']=='baseline' for r in rows))
print('survivors',[n for n in names if all(last[n,t]['exit']==0 for t in
                                        ('test_lpow','test_lroot','test_rfunc_prime'))])

def parse_cost():
    data={}
    for log in P.glob('cost-pow*.log'):
        text=log.read_text()
        for line in text.splitlines():
            if not line.startswith('ROW '): continue
            z=line.split(); key=(z[1],int(z[2]),int(z[3]),z[4]); nums=list(map(float,z[5:11]))
            assert int(z[12])==0
            data[key]=(nums[:3],nums[3:],int(re.search(r'trials=(\d+)',text).group(1)),'separate')
    # The supervisor interrupted the next trial of this row; keep its one completed validated pair.
    key=('powrat',65537,10000,'ball')
    text=(P/'cost-powrat-65537-10000-1.log').read_text()
    m=re.search(r'trial=0 total=([\d.]+) components=([\d.]+).*equal=1',text)
    assert m
    data[key]=([float(m[1])]*3,[float(m[2])]*3,1,'partial supervisor; one complete pair')
    key=('powrat',18446744073709551557,10000,'exact')
    total=float(re.search(r'completed_total trial=0 seconds=([\d.]+)',
                         (P/'cost-powrat-18446744073709551557-10000-0.log').read_text())[1])
    text=(P/'cost-phase-powrat-exact-components-isolated.log').read_text()
    assert 'valid=1' in text
    comp=float(re.search(r'PHASE .* components ([\d.]+)',text)[1])
    data[key]=([total]*3,[comp]*3,1,'separate phases')
    for log in P.glob('cost-instr-*.log'):
        text=log.read_text()
        if 'valid=1' not in text: continue
        m=re.search(r'INSTR (\w+) (\d+) (\d+) (\w+) ([\d.]+) ([\d.]+)',text)
        assert m
        key=(m[1],int(m[2]),int(m[3]),m[4])
        data[key]=([float(m[5])]*3,[float(m[6])]*3,1,'inside call')
    return data

data=parse_cost()
print('completed_cost_rows',len(data),'above_1_5',[(k,v[0][1]/v[1][1]) for k,v in data.items()
                                                if v[0][1]/v[1][1]>1.5])
lines=['| Function | p | N | Input | Total min/med/max s | Components min/med/max s | Ratio | Trials |',
       '|---|---:|---:|---|---:|---:|---:|---:|']
for fn in ('powrat','powunit'):
    for p in (3,65537,18446744073709551557):
        for N in (100,1000,10000):
            for kind in ('exact','ball'):
                key=(fn,p,N,kind)
                if key not in data: continue
                a,b,trials,method=data[key]
                label='P64' if p>65537 else str(p)
                vals=lambda xs:'/'.join(f'{x:.4g}' for x in xs)
                lines.append(f'| {fn} | {label} | {N} | {kind} | {vals(a)} | {vals(b)} | '
                             f'{a[1]/b[1]:.3f} | {trials} |')
(P/'cost-table.md').write_text('\n'.join(lines)+'\n')
(P/'cost-data.json').write_text(json.dumps([{'function':k[0],'p':k[1],'N':k[2],'input':k[3],
                                            'total':v[0],'components':v[1],'trials':v[2],'method':v[3]}
                                           for k,v in sorted(data.items())],indent=2)+'\n')
