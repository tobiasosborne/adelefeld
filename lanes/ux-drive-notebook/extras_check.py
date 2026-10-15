from fractions import Fraction as F
from pathlib import Path
import json
import re
import subprocess

lane=Path(__file__).resolve().parent
records=[json.loads(s) for s in (lane/'extras.records.jsonl').read_text().splitlines()]
checks=0
def check(ok,label):
    global checks
    assert ok,label
    checks+=1
    print('PASS',label)

for r in records:
    if r['driver'] is None: continue
    got=subprocess.run(['timeout','10',str(lane/'work/build/adf')],
        input='\n'.join(r['driver'])+'\n',text=True,capture_output=True)
    check(got.stdout.strip()==r['value'], f"cell {r['cell']} driver value {got.stdout.strip()}")

for pl in records[0]['places']:
    p=pl['p']; per,rest=re.fullmatch(r'\(([^)]+)\)(.*)',pl['digits']).groups()
    rdigits=rest.replace('.',''); nfrac=len(rest.split('.')[1]) if '.' in rest else 0
    value=(F(int(rdigits or '0',p))+F(int(per,p)*p**len(rdigits),1-p**len(per)))/p**nfrac
    check(value==F(6,35), f"6/35 base {p} exact periodic fraction = {value}")
    check(pl['v']=={2:1,3:1,5:-1,7:-1}[p], f"6/35 base {p} valuation {pl['v']}")
check(F(6,35)*F(1,2)*F(1,3)*5*7==1,'6/35 product of absolute values = 1')
check(records[1]['rest']=='0 at every prime','zero is not called a unit')
check(all(1009%d for d in range(2,1000)), '1009 has no divisor 2..999; cofactor 1009 has 4 digits')
check(1009*1013==1022117 and all(1022117%d for d in range(2,1000)),
      '1022117 = 1009*1013 has no divisor 2..999; cofactor has 7 digits')
for branch in records[5]['branches']:
    x=int(branch['value'].split()[0])
    check((x*x+1)%5**20==0 and x%5==int(branch['seed']),f"20-digit branch {branch['seed']} is a root")
    check(branch['digits']=='~'+''.join(str(x//5**i%5) for i in reversed(range(20))),
          f"20-digit branch {branch['seed']} digits agree")
    check('prec = 20' in branch['rewrite'],f"branch {branch['seed']} keeps precision")
for branch in records[6]['branches']:
    check(branch['rewrite'].startswith('r = root('),f"branch {branch['seed']} keeps name r")
check(records[7]['status']=='DOMAIN','composite place is rejected with DOMAIN')
print('PASS',checks,'checks')
