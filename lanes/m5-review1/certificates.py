from checks import *
def series(c,e,N):
    K=N+1;out=mp.mpf(0)
    while mp.exp(mp.mpf(e)/K-c*(2*K+1))>mp.mpf('.5'):
        out+=K**e*mp.exp(-c*K*K);K+=1
    return out+K**e*mp.exp(-c*K*K)/(1-mp.exp(mp.mpf(e)/K-c*(2*K+1)))
def jb(r,b,R):
    if b*R>=2*max(r,0):return 2*R**r*mp.exp(-b*R)/b
    R0=2*r/b;t=min(max(r/b,R),R0)
    return (R0-R)*t**r*mp.exp(-b*t)+2*R0**r*mp.exp(-b*R0)/b

def certificates():
    ds=[(1,1),(4,3),(5,2),(9,2),(13,2)]
    ss=[mp.mpc(x,y) for x in (-6,-3,.5,3,6) for y in (0,3)]
    cs=[(q,n,s,bit) for q,n in ds for s in ss for bit in (20,80)]
    lines=run([row(i,2,q,n,0,s,bits=bit,prec=192) for i,(q,n,s,bit) in enumerate(cs)])
    maxratio=0
    for (q,n,s,bit),l in zip(cs,lines):
        status,N,R,K,w=map(int,l[1].split());check(status==0,('trace status',q,n,s,bit,status))
        if status:continue
        deg=list(map(int,l[5].rstrip(',').split(',')))
        e=parity(q,n);a0=mp.pi/q;z=(s+e)/2;zd=(1-s+e)/2;r=z.real-1;rd=zd.real-1
        En=mp.exp(a0)*series(a0,e,N)*(jb(r,a0,1)+jb(rd,a0,1))
        Et=mp.exp(a0)*series(a0,e,0)*(jb(r,a0,R)+jb(rd,a0,R))
        eps=mp.power(2,-bit)
        check(En<=eps/32,('E_n cutoff',q,n,s,bit,N,En))
        check(Et<=eps/32,('E_t cutoff',q,n,s,bit,R,Et))
        if N:check(mp.exp(a0)*series(a0,e,0 if N==1 else N//2)*(jb(r,a0,1)+jb(rd,a0,1))>eps/32,('N not first',q,n,s,N))
        if R>1:check(mp.exp(a0)*series(a0,e,0)*(jb(r,a0,R//2)+jb(rd,a0,R//2))>eps/32,('R not first',q,n,s,R))
        E=En+Et
        for side,v in enumerate((z,zd)):
            for k in range(K):
                m=3*mp.power(2,k-1);h=mp.power(2,k-1);d=m/2;rr=v.real-1
                B=sum(nn**e*mp.exp(-a0*nn**2*(m-d)) for nn in range(1,N+1))*max((m-d)**rr,(m+d)**rr)*mp.exp(mp.pi*abs(v.imag)/2)
                err=2*h*B*mp.power(mp.mpf(2)/3,deg[side*K+k]+1)/(1-mp.mpf(2)/3)
                E+=err;check(err<=eps/(64*K),('panel threshold',q,n,s,bit,side,k))
                counts['certificate_panels']+=1
        for side in (3,4):
            b=box(l[side]);check(min(v[1] for v in b)>=E,('added error below certificate',q,n,s,bit,side,str(E),b))
            maxratio=max(maxratio,float(E/min(v[1] for v in b)))
        counts['certificate_cases']+=1
    counts['max_certificate_to_radius']=maxratio

def jbounds():
    cases=[(r,b,R) for r in (-3,-.5,0,.25,1,2,4,12) for b in (.125,.5,2,4) for R in (1,2,8)]
    p=subprocess.run(['timeout','60',str(ROOT/'certprobe')],input=''.join(f'{r} {b} {R}\n' for r,b,R in cases),text=True,capture_output=True)
    check(p.returncode==0,('certprobe exit',p.returncode,p.stderr));check(len(p.stdout.splitlines())==len(cases),'jbound row count')
    for (r,b,R),line in zip(cases,p.stdout.splitlines()):
        m,rad=dump(line);exact=jb(mp.mpf(r),mp.mpf(b),mp.mpf(R));truth=mp.quad(lambda t:t**r*mp.exp(-b*t),[R,mp.inf])
        check(m+rad>=exact,('L14 upper below formula',r,b,R,m,rad,exact))
        check(m+rad>=truth,('L14 bound below quad',r,b,R,m,rad,truth))
        counts['L14_second' if b*R<2*max(r,0) else 'L14_first']+=1
    counts['jbound_cases']=len(cases)

if __name__=='__main__':
    start=time.monotonic()
    if sys.argv[1]=='bounds':certificates()
    else:jbounds()
    print(json.dumps({'mode':sys.argv[1],'elapsed':time.monotonic()-start,'counts':dict(counts),'failures':repr(failures)}))
    raise SystemExit(bool(failures))
