#!/usr/bin/env python3
from pathlib import Path
import subprocess
import sys
OUT=Path('lanes/f-review8')
counter=-1
for fn in ('powrat','powunit'):
    for p in (3,65537,18446744073709551557):
        for N in (100,1000,10000):
            for ball in (0,1):
                counter+=1
                if len(sys.argv)>1 and not(int(sys.argv[1])<=counter<int(sys.argv[2])): continue
                log=OUT/f'cost-{fn}-{p}-{N}-{ball}.log'
                assert not log.exists(), log
                with log.open('w') as f:
                    r=subprocess.run(['timeout','150',str(OUT/'build/cost'),fn,str(p),str(N),str(ball),*(['single'] if p==18446744073709551557 and N==10000 else [])],
                                     stdout=f,stderr=subprocess.STDOUT)
                text=log.read_text()
                print(log.name,'exit',r.returncode, next((s for s in text.splitlines() if s.startswith('ROW ')),''),
                      flush=True)
                if r.returncode: sys.exit(r.returncode)
