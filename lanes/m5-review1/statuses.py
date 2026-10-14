from checks import *
def status_cases():
    cases=[]
    # Integer finite poles and closed contact; gamma has its own shifted pole set.
    for p in (2,3,5,7,11,13):
        for k in (-3,-1,0,1,3):
            if k<0 and p!=2:continue
            a=mp.mpf(p)**k;s=mp.mpc(k)
            cases.append((0,1,1,p,s,9999,a,9999,53,128,(DOMAIN,OK,OK)))
            cases.append((0,1,1,p,s,-20,a,9999,53,128,(ND,OK,OK)))
            a=mp.mpf(p)**(k-1)
            if k>=1 or p==2:
                cases.append((0,1,1,p,s,9999,a,9999,53,128,(OK,DOMAIN,OK)))
                cases.append((0,1,1,p,s,-20,a,9999,53,128,(OK,ND,OK)))
        for q,n in ((1,1),(4,3)):
            cases.append((0,q,n,p,mp.mpc('.5'),9999,0,9999,53,128,(DOMAIN,DOMAIN,DOMAIN) if q==1 or p==2 else (DOMAIN,DOMAIN,DOMAIN)))
            cases.append((0,1,1,p,mp.mpc('.5'),9999,0,-30,53,128,(ND,ND,ND)))
        cases.append((0,4,3,p,mp.mpc('.5'),9999,1,9999,53,128,(OK,OK,OK) if p==2 else (DOMAIN,DOMAIN,DOMAIN)))
    for e in (0,1):
        q,n=(1,1) if e==0 else (4,3)
        for k in range(10):
            s=mp.mpc(-e-2*k)
            cases.append((0,q,n,0,s,9999,0,9999,53,128,(DOMAIN,OK,OK)))
            cases.append((0,q,n,0,s,-20,0,9999,53,128,(ND,OK,OK)))
            s=mp.mpc(1+e+2*k)
            cases.append((0,q,n,0,s,9999,0,9999,53,128,(OK,DOMAIN,OK)))
            cases.append((0,q,n,0,s,-20,0,9999,53,128,(OK,ND,OK)))
            # A pole beside the rectangle; it may refuse numerically but must not say DOMAIN.
            cases.append((0,q,n,0,mp.mpc(1+e+2*k,2**-10),-20,0,9999,53,128,(OK,OK,OK)))
    for q,n in ((1,1),(4,3),(5,2)):
        for s,sr in ((mp.mpc(0),9999),(mp.mpc(1),9999),(mp.mpc(1),-30),(mp.mpc('.5'),-1)):
            if q==1:ex=(DOMAIN,DOMAIN,DOMAIN) if sr==9999 else ((ND,ND,ND) if s.real==1 else (DOMAIN,ND,ND))
            else:ex=(DOMAIN,OK,OK) if sr==9999 else ((ND,None,None) if s.real==1 else (DOMAIN,None,None))
            cases.append((1,q,n,0,s,sr,1,9999,4,128,ex))
        for bits in (-1,2097153):cases.append((1,q,n,0,mp.mpc(2),9999,1,9999,bits,128,(DOMAIN,)*3))
        cases.append((1,q,n,0,mp.mpc(0),9999,1,9999,-1,2097153,(LIMIT,)*3))
    cases.append((1,1031,2,0,mp.mpc(0),9999,1,9999,-1,128,(LIMIT,)*3))
    lines=run([row(i,*c[:10]) for i,c in enumerate(cases)])
    for c,l in zip(cases,lines):
        for fn,(st,preserve,where,b) in enumerate(results(l)):
            if c[-1][fn] is not None:check(st==c[-1][fn],('status',c,fn,st,c[-1][fn]))
            if st:check(preserve==1 and (c[0]!=0 or where==1),('status atomicity/where',c,fn,st))
            counts[f'status_{st}']+=1
    counts['status_inputs']=len(cases)

def random_width(part,binary='runner'):
    rng=random.Random(51407+part);cases=[]
    for i in range(500):
        q,n=rng.choice([(1,1),(4,3),(5,2),(7,3),(9,2)])
        s=mp.mpc(rng.randrange(-48,49)/8,rng.randrange(-160,161)/8)
        if q==1 and s in (0,1):s+=mp.mpf('.125')
        rad=rng.choice([9999,-30,-60,-10]);bits=rng.choice([0,20,53,80]);prec=rng.choice([2,53,128])
        cases.append((q,n,s,rad,bits,prec))
    lines=run([row(i,4,q,n,0,s,rad,bits=bits,prec=prec) for i,(q,n,s,rad,bits,prec) in enumerate(cases)],binary)
    for c,l in zip(cases,lines):
        st,preserve,where,b=results(l)[0];counts[f'width_status_{st}']+=1
        if st==0:check(width(b)<=mp.power(2,-c[4]),('random width',c,b));counts['width_OK']+=1
        else:check(preserve==1,('random atomicity',c,st))
    counts['random_inputs']=len(cases)

def nd_witness():
    # Exact endpoint separation proves a width floor. Cases deliberately remain away from both zeta poles.
    for q,n in ((1,1),(4,3),(5,2),(7,3)):
        for s in (mp.mpc(2),mp.mpc('.375','.75'),mp.mpc(-1,2)):
            for r in (-30,-60):
                vals=[global_value(q,n,t)[1] for t in corners(s,r)]
                sep=max(max(v.real for v in vals)-min(v.real for v in vals),max(v.imag for v in vals)-min(v.imag for v in vals))
                bit=-r+20
                check(sep>mp.power(2,-bit),('witness separation',q,n,s,r,sep))
                st,preserve,where,b=results(run([row(0,4,q,n,0,s,r,bits=bit,prec=128)])[0])[0]
                check(st==ND and preserve==1,('width floor refusal',q,n,s,r,st));counts['width_witnesses']+=1
if __name__=='__main__':
    start=time.monotonic();mode=sys.argv[1]
    if mode=='status':status_cases()
    elif mode=='random':random_width(int(sys.argv[2]),sys.argv[3] if len(sys.argv)>3 else 'runner')
    else:nd_witness()
    print(json.dumps({'mode':mode,'elapsed':time.monotonic()-start,'counts':dict(counts),'failures':repr(failures)}));raise SystemExit(bool(failures))
