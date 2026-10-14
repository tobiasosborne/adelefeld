from pathlib import Path
import json,collections
R=Path(__file__).resolve().parent
sets={'local':[f'local-{i}.log' for i in range(6)],'global':[f'global-{i}.log' for i in range(33)],'memory':[f'memory-{i}-no-leak.log' for i in range(4)],'random':[f'random-{i}.log' for i in range(4)],'FE':[f'fe-{i}.log' for i in range(3)]}
for name,files in sets.items():
 totals=collections.Counter(); maxima={};missing=[]
 for f in files:
  p=R/f
  if not p.exists() or not p.read_text().strip():missing.append(f);continue
  try:obj=json.loads(p.read_text().splitlines()[-1])
  except Exception:missing.append(f);continue
  for k,v in obj['counts'].items():
   if k.startswith('max_'):maxima[k]=max(maxima.get(k,0),v)
   else:totals[k]+=v
  if obj.get('failures')!='[]':print('FAILURES',f,obj['failures'])
 print(name,json.dumps(dict(totals)),json.dumps(maxima),'missing',missing)
for i in range(14):
 p=R/f'fault-{i}.json'
 if p.exists():
  o=json.loads(p.read_text());print('fault',i,o['name'],[(k,v['exit']) for k,v in o['results'].items() if 'exit' in v])
 else:print('MISSING fault',i)
