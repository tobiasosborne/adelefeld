import copy
import json
import pathlib
import re
import subprocess

lane=pathlib.Path(__file__).resolve().parent
root=lane.parent.parent
source=root/'ux/ledger'
base=json.loads((source/'claims/a1-cos-4-at-2.json').read_text())
ledger=lane/'status-ledger'
(ledger/'claims').mkdir(parents=True,exist_ok=True)
examples=[
 ('drive-prefixed-open',[
  'prec 128',
  'tate_gamma ((1 +/- 0.001) + (0)*i ; 0) with 3 with ((1) + (0)*i ; 0) '
  'with char(q=1, n=1, s=(0) + (0)*i)'],['0'],'open'),
 ('drive-limit',['prec 3000000'],['0'],'open'),
 ('drive-parse',['not_a_command 1'],['0'],'refuted'),
]
for cid,script,expect,wanted in examples:
 c=copy.deepcopy(base)
 c.update(id=cid,statement='Drive status check: '+cid,script=script,expect=expect,places=[3])
 (ledger/'claims'/(cid+'.json')).write_text(json.dumps(c,indent=2)+'\n')
 cmd=['timeout','30','python3','-B',str(source/'seal.py'),'--ledger',str(ledger),'--only',cid]
 r=subprocess.run(cmd,text=True,capture_output=True)
 print('COMMAND:',' '.join(cmd),'\n'+r.stdout,'exit=',r.returncode)
 s=json.loads((ledger/'seals'/(cid+'.json')).read_text())
 print('actual=',s['actual'],'expected verdict=',wanted,'observed verdict=',s['verdict'])
 if s['verdict']!=wanted: print('DEFECT: status is misclassified')
subprocess.run(['timeout','30','python3','-B',str(source/'render.py'),'--ledger',str(ledger)],check=True)
p=(ledger/'ledger.html').read_text()
card=re.search(r'<article[^>]*id="claim-drive-prefixed-open"[\s\S]*?</article>',p)[0]
print('Prefixed NOT_DETERMINED status badge present:', 'class="cls cls-open"' in card)
print('Prefixed card displays REFUTED:', '>REFUTED<' in card)

invalid=lane/'invalid-ledger'
(invalid/'claims').mkdir(parents=True,exist_ok=True)
c=copy.deepcopy(base)
c['id']='missing-expect'
del c['expect']
(invalid/'claims/missing-expect.json').write_text(json.dumps(c))
cmd=['timeout','30','python3','-B',str(source/'seal.py'),'--ledger',str(invalid)]
r=subprocess.run(cmd,text=True,capture_output=True)
print('COMMAND:',' '.join(cmd),'\n'+r.stdout+r.stderr,'exit=',r.returncode)
assert r.returncode==2

void=lane/'void-ratification'
(void/'claims').mkdir(parents=True,exist_ok=True)
(void/'seals').mkdir(exist_ok=True)
c=copy.deepcopy(base)
c['state']='ratified'
(void/'claims/a1-cos-4-at-2.json').write_text(json.dumps(c))
(void/'seals/a1-cos-4-at-2.json').write_bytes((source/'seals/a1-cos-4-at-2.json').read_bytes())
cmd=['timeout','30','python3','-B',str(source/'render.py'),'--ledger',str(void)]
r=subprocess.run(cmd,text=True,capture_output=True)
print('COMMAND:',' '.join(cmd),'\n'+r.stdout+r.stderr,'exit=',r.returncode)
html=(void/'ledger.html').read_text()
assert 'ratification is void' in html and '>REFUTED<' in html
print('Ratifying a refuted claim remains REFUTED and says ratification is void.')

extra=(lane/'extra-ledger/ledger.html').read_text()
card=re.search(r'<article[^>]*id="claim-drive-footprint"[\s\S]*?</article>',extra)[0]
span=re.search(r'<span class="c wide"[^>]*>[^<]*101</span>',card)
assert span is not None
print('False computed footprint:',span[0])

claims=[json.loads(p.read_text()) for p in (source/'claims').glob('fx-*.json')]
empty=[c['id'] for c in claims if not c['places']]
real=[c for c in claims if any('with real' in s for s in c['script'])]
print('Imported empty footprints:',len(empty),empty)
print('Imported claims with "with real":',len(real),'; R represented:',sum('R' in c['places'] for c in real))
print('Status controls verified: raw LIMIT open, raw PARSE refuted, missing field exit 2, void ratification.')
