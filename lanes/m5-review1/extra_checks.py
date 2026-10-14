from checks import *
def vector():
    descriptors=list(dict.fromkeys([(4,3) if x==(8,7) else (5,3) if x==(10,3) else x for x in GOLD+EXTRA]))
    rows=[]
    for q,n in descriptors:
        for a in (1,0):rows.append(row(len(rows),3,q,n,0,mp.mpc(0),a=a,prec=160))
    pairs=run(rows)
    for i,c in enumerate(descriptors):check(pairs[2*i][1:]==pairs[2*i+1][1:],('vector stored exponent ignored',c))
    lines=pairs[::2]
    for (q,n),l in zip(descriptors,lines):
        st,D,M,length,shape=map(int,l[1].split());check((st,D,M,length,shape)==(0,1,q,1,1),('vector structure',q,n,l[1]))
        for j,b in enumerate(l[2:]):check(contains(box(b),chars(q,n)[j]),('vector entry',q,n,j,b));counts['vector_entries']+=1
        counts['vectors']=len(descriptors)

def real():
    grid=[mp.mpc(x,y) for x in (-5.75,-3.25,-.75,.25,.5,1.25,3.25,5.25) for y in (-3,-.5,0,.75,3)]
    cases=[(q,n,s,-12 if j%3==0 else 9999) for q,n in [(1,1),(4,3),(5,2),(5,4)] for j,s in enumerate(grid)]
    lines=run([row(i,0,q,n,0,s,r,prec=160) for i,(q,n,s,r) in enumerate(cases)])
    for c,l in zip(cases,lines):
        q,n,s,r=c;refs=[local(q,n,0,1,t) for t in corners(s,r)]
        for fn,(st,preserve,where,b) in enumerate(results(l)):
            check(st==0,('real grid status',c,fn,st))
            check(where==0,('real success where',c,fn,where))
            for v in refs:check(contains(b,v[fn]),('real grid enclosure',c,fn,b,v[fn]))
            sep=max(max(v[fn].real for v in refs)-min(v[fn].real for v in refs),max(v[fn].imag for v in refs)-min(v[fn].imag for v in refs))
            bd=64*sep+mp.power(2,-152)*max(1,*(abs(v[fn]) for v in refs))
            check(width(b)<=bd,('real grid tightness',c,fn,str(width(b)),str(bd)));counts['real_grid_calls']+=1
    counts['real_grid_inputs']=len(cases)

def near():
    cases=[]
    for q,n in [(1,1),(4,3),(5,2),(13,2)]:
        for s in [mp.mpc(2**-40),mp.mpc(1+2**-40),mp.mpc(1-2**-40),mp.mpc(1,2**-40)]:
            for rad in [9999,-200]:cases.append((q,n,s,rad))
    lines=run([row(i,1,q,n,0,s,r,bits=80,prec=256) for i,(q,n,s,r) in enumerate(cases)])
    for c,l in zip(cases,lines):
        q,n,s,r=c
        for fn,(st,preserve,where,b) in enumerate(results(l)):
            if fn==0 and s.real<=1:check(st in (DOMAIN,ND),('near half-plane',c,st));continue
            check(st==0,('near pole status',c,fn,st))
            if st:continue
            check(width(b)<=mp.power(2,-80),('near width',c,fn,b))
            for t in corners(s,r):
                check(contains(b,global_value(q,n,t)[int(fn==2)]),('near mp enclosure',c,fn,t,b))
                check(contains(b,flint_value(q,n,t)[int(fn==2)]),('near FLINT enclosure',c,fn,t,b))
            counts['near_OK']+=1
    counts['near_inputs']=len(cases)
if __name__=='__main__':
    mode=sys.argv[1]
    {'vector':vector,'real':real,'near':near}[mode]()
    print(json.dumps({'mode':mode,'counts':dict(counts),'failures':repr(failures)}));raise SystemExit(bool(failures))
