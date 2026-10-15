from pathlib import Path
import json
import subprocess

lane=Path(__file__).resolve().parent
root=lane.parents[1]
source=root/'ux/notebook'
copy=lane/'work/ux/notebook'
changed=[]; long=[]; count=0
for p in source.rglob('*'):
    if not p.is_file(): continue
    rel=p.relative_to(source); count+=1
    if p.read_bytes()!=(copy/rel).read_bytes(): changed.append(str(rel))
    for i,s in enumerate(p.read_text().splitlines(),1):
        if len(s)>116: long.append((str(rel),i,len(s)))
print('slice files compared',count,'different',len(changed),changed)
print('lines above 116',len(long),'in',len({p for p,i,n in long}),'files')
print('long-line files',sorted({p for p,i,n in long}))
print('longest line',max(long,key=lambda x:x[2]) if long else None)
print('README lines',len((source/'README.md').read_text().splitlines()))
for name in ['a_roots','b_which_rational']:
    rs=[json.loads(s) for s in (copy/'out'/f'{name}.records.jsonl').read_text().splitlines()]
    html=(copy/'out'/f'{name}.html').read_text()
    print(name,'records',len(rs),'dump missing',sum(r['dump'] is None for r in rs),
          'version fields',sum('version' in r for r in rs),'dump controls',html.count('<details class="dump">'),
          'strip tooltips',html.count('role="table" title='))
    for r in rs:
        if r['status']!='OK':
            print('status',r['cell'],r['status'],'place',r['place'])
print('No screenshots:', not any((lane/n).exists() for n in ['a_roots.png','b_which_rational.png','blank.png']))
print('END_TIME',subprocess.check_output(['timeout','5','date','-Iseconds'],text=True).strip())
