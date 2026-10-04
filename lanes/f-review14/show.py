import sys,json
for l in sys.stdin:
    l=l.strip()
    if not l.startswith('{'):
        print(l); continue
    r=json.loads(l)
    print(r['id'],'exit',r['exit'],'|',r['summary'][:64],'|drv',[k for k,v in r['driver'].items() if v=='FAIL'],'|probe',r['probe_diff'][:9])
