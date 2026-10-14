from checks import *
cases=[]
for p in (2,3,5,7,11,13):
    for k in (-3,-2,-1,1,2,3):
        y=2*mp.pi*k/mp.log(p)
        cases.append((p,mp.mpc(0,y),1,(ND,OK,OK)))
        cases.append((p,mp.mpc(1,y),1,(OK,ND,OK)))
        y=(mp.pi/2+2*mp.pi*k)/mp.log(p)
        cases.append((p,mp.mpc(0,y),1j,(ND,OK,OK)))
        cases.append((p,mp.mpc(1,y),1j,(OK,ND,OK)))
lines=run([row(i,0,1,1,p,s,-45,a=a,prec=160) for i,(p,s,a,ex) in enumerate(cases)])
for c,l in zip(cases,lines):
    for fn,(st,preserve,where,b) in enumerate(results(l)):
        check(st==c[-1][fn],('nonzero pole lattice',c,fn,st))
        if st:check(preserve and where,('lattice failure atomicity',c,fn,st))
        counts[f'lattice_status_{st}']+=1
counts['lattice_inputs']=len(cases)
# Nonfinite raw arguments, including alpha ignored at infinity.
for p,s,a,prec,ex in [(2,mp.mpc(float('nan')),1,128,DOMAIN),(2,mp.mpc('.5'),float('nan'),128,DOMAIN),(0,mp.mpc('.5'),float('nan'),128,OK),(0,mp.mpc(float('nan')),1,128,DOMAIN),(2,mp.mpc(float('nan')),float('nan'),2097153,LIMIT)]:
    for fn,(st,preserve,where,b) in enumerate(results(run([row(0,0,1,1,p,s,a=a,prec=prec)])[0])):
        check(st==ex,('nonfinite status',p,s,a,prec,fn,st,ex))
        if st:check(preserve and where,('nonfinite atomicity',p,fn,st))
        counts['raw_status_calls']+=1
print(json.dumps({'mode':'more statuses','counts':dict(counts),'failures':repr(failures)}));raise SystemExit(bool(failures))
