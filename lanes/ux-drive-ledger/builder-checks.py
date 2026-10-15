import hashlib
import importlib.util
import json
import pathlib
import re
import subprocess
import sys
import tempfile
import unittest

lane = pathlib.Path(__file__).resolve().parent
root = lane.parent.parent
original = root / 'ux/ledger'
copy = lane / 'ledger'
checks = 0

def check(value, label):
    global checks
    checks += 1
    assert value, label
    print('PASS:', label)

def run(name, args, expected=0):
    print('COMMAND:', ' '.join(map(str,args)))
    r = subprocess.run(['timeout','60',*map(str,args)], text=True, capture_output=True)
    (lane/(name+'.stdout')).write_text(r.stdout+r.stderr)
    print('exit=',r.returncode,'tail=',(r.stdout+r.stderr)[-800:])
    check(r.returncode==expected, name+' exit')
    return r

def snapshot(directory):
    paths = list((directory/'claims').glob('*.json')) + list((directory/'seals').glob('*.json'))
    paths += [directory/'ledger.html']
    return {str(p.relative_to(directory)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}

before=snapshot(copy)
run('repeat-import',['python3','-B',original/'import_fixtures.py','--out',copy/'claims'])
run('repeat-seal',['python3','-B',original/'seal.py','--ledger',copy],1)
run('repeat-render',['python3','-B',original/'render.py','--ledger',copy])
check(snapshot(copy)==before,'200 claim/seal/page files byte-identical after repeat')
check(snapshot(original)==before,'200 generated files equal committed original')
page=(copy/'ledger.html').read_text()
js=re.search(r'<script>([\s\S]*?)</script>',page)[1]
(lane/'page-script.js').write_text(js)
run('node-syntax',['node','--check',lane/'page-script.js'])
longs=[(p.name,i) for p in list(original.glob('*.md'))+list(original.glob('*.py'))
       for i,line in enumerate(p.read_text().splitlines(),1) if len(line)>116]
check(not longs,'0 source/README lines over 116')
check(len((original/'README.md').read_text().splitlines())==73,'README has 73 lines')
for x,n,result in [(3,8,958),(12,8,5125),(3,2,4),(12,2,4),(3,3,13),(12,3,22)]:
    r=run('oracle-'+str(x)+'-'+str(n),['python3','-B',original/'oracle.py',str(x),'3',str(n)])
    check(r.stdout.strip()==f'3: {result} + O(3^{n})','oracle centre matches independent driver run')

reopened=pathlib.Path(tempfile.mkdtemp(prefix='reopened-',dir=lane))
(reopened/'claims').mkdir(parents=True,exist_ok=True)
(reopened/'seals').mkdir(exist_ok=True)
name='a2-v2-log-3.json'
(reopened/'claims'/name).write_bytes((original/'claims'/name).read_bytes())
s=json.loads((original/'seals'/name).read_text())
s['build']['adf_sha256']='f'*64
s['actual'][0]='2: 0 + O(2^3)'
(reopened/'seals'/name).write_text(json.dumps(s,indent=2)+'\n')
run('reopen-seal',['python3','-B',original/'seal.py','--ledger',reopened],1)
run('reopen-render',['python3','-B',original/'render.py','--ledger',reopened])
new=json.loads((reopened/'seals'/name).read_text())
check(new['verdict']=='open' and new['run_verdict']=='sealed','reopened verdict open, current run sealed')
check(len(list((reopened/'seals/history').glob('*.json')))==1,'one historical seal')
html=(reopened/'ledger.html').read_text()
check('REOPENED.' in html and 'f'*12 in html and '2: 0 + O(2^3)' in html,'page names earlier result and build')
run('reopen-repeat',['python3','-B',original/'seal.py','--ledger',reopened],1)
check(json.loads((reopened/'seals'/name).read_text())['verdict']=='open','reopening persists')

# One bounded mutation, checked by the builder's actual comparison test.
source=(original/'ledger.py').read_text()
check(source.count("'LIMIT': 'budget'")==1,'one exact mutation site')
mutant=lane/'ledger-mutant.py'
mutant.write_text(source.replace("'LIMIT': 'budget'","'LIMIT': 'no'"))
spec=importlib.util.spec_from_file_location('ledger',mutant)
module=importlib.util.module_from_spec(spec)
sys.modules['ledger']=module
spec.loader.exec_module(module)
sys.path.insert(0,str(original))
import test_ledger
suite=unittest.TestSuite([test_ledger.TestCompare('test_compare_rules')])
result=unittest.TextTestRunner(verbosity=2).run(suite)
check(len(result.failures)==1 and not result.errors,'LIMIT-to-proved-no mutant killed by 1 assertion failure')
print('Builder comparison assertions:',checks)
