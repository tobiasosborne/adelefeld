import json
import pathlib
import subprocess

lane=pathlib.Path(__file__).resolve().parent
root=lane.parent.parent
claims=root/'ux/ledger/claims'
records=root/'ux/ledger/seals'
commands=lines=0
for path in sorted(claims.glob('*.json')):
 c=json.loads(path.read_text())
 if not c['script']:
  continue
 r=subprocess.run(['timeout','20',str(root/'build/adf')],input='\n'.join(c['script'])+'\n',
                  capture_output=True,text=True)
 s=json.loads((records/path.name).read_text())
 assert r.stdout.splitlines()==s['actual'],c['id']+' output mismatch'
 assert r.returncode==s['exit'],c['id']+' exit mismatch'
 commands+=1
 lines+=len(s['actual'])
print('Independent driver invocations:',commands)
print('Output lines reproduced:',lines)
print('Output and exit comparisons:',2*commands)
print('Mismatch count: 0')
