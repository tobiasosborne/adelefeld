from pathlib import Path
import json
import re
import subprocess

lane=Path(__file__).resolve().parent
rs=[json.loads(s) for s in (lane/'builder_followups.records.jsonl').read_text().splitlines()]
checks=0
def check(ok,label):
    global checks
    assert ok,label
    checks+=1
    print('PASS',label)

for r in rs:
    if not r['driver']: continue
    got=subprocess.run(['timeout','10',str(lane/'work/build/adf')],
        input='\n'.join(r['driver'])+'\n',text=True,capture_output=True).stdout.strip()
    if r['cell']==10:
        check(got=='1' and r['value']=='error: NO_SOLUTION', 'reproduced fresh-record status mismatch')
    else:
        check(got==r['value'],f"cell {r['cell']} direct driver agrees")

check(8*1009*1013==8176936,'ball modulus 8176936 and cofactor 1022117')
check(rs[0]['places']==[{'p':2,'v':0,'k':3,'digits':'~001'}], '1 mod 8 has valuation 0 and digits 001')
x=3156215819652023
check((x*x+1)%101**8==0 and x%101==10,'101-adic square root has seed 10 at precision 8')
check(rs[6]['dump']==rs[7]['dump'],'adding exact zero preserves every stored bit')
html=(lane/'builder_followups.html').read_text()
body=re.search(r'<section class="cell" id="c4"[\s\S]*?</section>',html)[0]
check('click the fog' in body and 'data-rewrite=' not in body,
      'reproduced instruction to click absent fog control above p=97')
commands='''project (0.99 +/- 0.0099 ; 0 mod 1) with real
reconstruct (0.99 +/- 0.0099 ; 0 mod 1)
reconstruct (0.99 +/- 0.01 ; 0 mod 1)
'''
result=subprocess.run(['timeout','10',str(lane/'work/build/adf')],input=commands,text=True,capture_output=True)
print('Driver inputs:\n'+commands+'Driver outputs:\n'+result.stdout)
check(result.stdout.splitlines()==['real: 0.99 +/- 0.01','error: NO_SOLUTION','1'],
      'literal interval rejects 1, reparsed printed interval admits 1')
print('PASS',checks,'checks')
