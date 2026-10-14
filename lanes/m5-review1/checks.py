import sys,subprocess,time,random,collections,json,math,os
from pathlib import Path
from models import *
ROOT=Path(__file__).resolve().parent
OK,ND,DOMAIN,LIMIT=0,1,7,10
counts=collections.Counter();failures=[]
def check(ok,what):
    counts['checks']+=1
    if not ok:
        counts['failures']+=1
        if len(failures)<20:failures.append(what);print('FAIL',what,flush=True)
def dump(s):
    m,e,r,re=(int(t,16) for t in s.split());return mp.mpf(m)*mp.power(2,e),mp.mpf(r)*mp.power(2,re)
def box(s):return tuple(dump(t) for t in s.split(';'))
def contains(b,v,tol=mp.mpf('1e-50')):return all(abs(m-t)<=r+tol*max(1,abs(v)) for (m,r),t in zip(b,(v.real,v.imag)))
def width(b):return 2*max(x[1] for x in b)
def row(i,kind,q,n,p,s,sr=9999,a=1,ar=9999,bits=53,prec=160):
    return f'{i} {kind} {q} {n} {p} {float(s.real):.17g} {float(s.imag):.17g} {sr} {float(mp.re(a)):.17g} {float(mp.im(a)):.17g} {ar} {bits} {prec}\n'
def run(rows,binary='runner',limit=160):
    if binary=='runner':binary=os.environ.get('ADF_REVIEW_RUNNER',binary)
    args=['timeout',str(limit),str(ROOT/binary)]
    if binary=='runner-valgrind':
        args=['timeout',str(limit),'/home/tobias/.local/bin/valgrind','--error-exitcode=9','--track-origins=yes','--leak-check=full','--show-leak-kinds=all','--errors-for-leak-kinds=definite,indirect','--log-file='+str(ROOT/'valgrind.log'),str(ROOT/'runner')]
    st=time.monotonic();p=subprocess.run(args,input=''.join(rows),text=True,capture_output=True)
    counts['runner_calls']+=1;counts['runner_seconds']+=time.monotonic()-st
    check(p.returncode==0,('runner exit',p.returncode,p.stderr[-1000:]));out=p.stdout.splitlines();check(len(out)==len(rows),('row count',len(out),len(rows)))
    return [l.split(' | ') for l in out]
def results(l):
    ans=[]
    for part in l[1:]:
        st,preserve,where,b=part.split(' ',3);ans.append((int(st),int(preserve),int(where),box(b)))
    return ans
def corners(s,r):
    if r==9999:return [s]
    d=mp.power(2,r);return [s]+[s+x*d+1j*y*d for x in (-1,1) for y in (-1,1)]
GOLD=[(1,1),(3,2),(4,3),(5,2),(5,3),(5,4),(7,3),(7,6),(8,3),(8,5),(8,7),(12,11),(13,2),(15,2),(16,3),(9,2),(10,3)]
EXTRA=[(11,2),(11,3),(17,3),(19,2),(20,3),(21,2),(23,5),(25,2),(27,2),(28,3),(29,2),(32,5),(37,2),(31,3),(33,2),(40,3)]
def phasecheck():
    for q in list(range(1,41))+[49,121,169]:
        for n in range(1,q):
            if math.gcd(n,q)!=1:continue
            ch=flint.dirichlet_char(q,n)
            for u,val in enumerate(chars(q,n)):
                v=ch(u);m=mp.mpc(v.real.str(65,radius=False),v.imag.str(65,radius=False))
                check(abs(val-m)<mp.mpf('1e-55'),('phase',q,n,u,val,m))
    counts['phase_descriptors']=sum(sum(math.gcd(n,q)==1 for n in range(1,q)) for q in list(range(1,41))+[49,121,169])

def localcheck(part):
    primes=[2,3,5,7,11,13];p=primes[part]
    qs=[1,4,8,16] if p==2 else [1,p,p*p]
    desc=[]
    for q in qs:
        labels=[1] if q==1 else [n for n in range(2,q) if math.gcd(n,q)==1 and flint.dirichlet_char(q,n).is_primitive()]
        labels=labels[:3]+labels[-1:] if len(labels)>4 else labels
        desc +=[(q,n) for n in labels]
    alphas=[1,-1,1j,-1j,2,mp.mpc('.5','1'),mp.mpc('.25','-.75'),mp.mpc('-.5','.25'),mp.mpf('.125'),4,mp.mpc('1.5','.5'),mp.mpc('-2','-1')]
    grid=[mp.mpc(x,y) for x in (-2.25,-.75,.25,.5,1.25,2.25,3.25,5.25) for y in (-2,-.5,0,.75,3)]
    cases=[]
    for q,n in desc:
        for ai,a in enumerate(alphas):
            for j,s in enumerate(grid):cases.append((q,n,p,s,-30 if j%13==0 else 9999,a,-30 if ai==11 else 9999))
    lines=run([row(i,0,*c[:3],c[3],c[4],c[5],c[6]) for i,c in enumerate(cases)])
    maxratio=0
    for c,l in zip(cases,lines):
        q,n,p,s,sr,a,ar=c
        vals=[local(q,n,p,aa,ss) for aa in corners(a,ar) for ss in corners(s,sr)]
        for fn,(status,preserve,where,b) in enumerate(results(l)):
            counts[f'local_status_{status}']+=1
            if status!=OK:check(preserve==1 and where==1,('local failure atomicity',c,fn));continue
            check(where==0,('local success where changed',c,fn))
            counts['local_OK']+=1
            for v in vals:check(contains(b,v[fn]),('local enclosure',c,fn,b,v[fn]))
            sep=max(max(v[fn].real for v in vals)-min(v[fn].real for v in vals),max(v[fn].imag for v in vals)-min(v[fn].imag for v in vals))
            # Stated finite test bound, not an API requirement: factor 64 plus arithmetic slack.
            bound=64*sep+mp.power(2,-152)*max(1,*(abs(v[fn]) for v in vals))
            check(width(b)<=bound,('local tightness',c,fn,str(width(b)),str(bound)))
            if bound:maxratio=max(maxratio,float(width(b)/bound))
    # Direct transform value versus an independently reduced exact cyclotomic polynomial.
    for q,n in desc:
        if q>1:check(abs(q*transformed_average(q,n,p)-cyclo(q,n,True,True))<mp.mpf('1e-55'),('Gminus',q,n))
    counts['local_inputs']=len(cases);counts['local_descriptors']=len(desc);counts['max_width_bound_ratio']=maxratio

def globalcheck(index):
    q,n=(GOLD+EXTRA)[index];ch=flint.dirichlet_char(q,n)
    if not ch.is_primitive():
        # Two raw goldens lower. Test the public constructor, then use its resulting primitive descriptor.
        q,n={(8,7):(4,3),(10,3):(5,3)}[(q,n)]
    e=parity(q,n)
    grid=[mp.mpc(x,y) for x in (-6,-3,-1,0,.5,1,2,4,6) for y in (-20,-7,0,.75,7,20)]
    grid += [mp.mpc(-e-2),mp.mpc(-e-4),mp.mpc(2**-20),mp.mpc(1+2**-20),mp.mpc(1-2**-20),mp.mpc('.375','.75')]
    cases=[]
    for s in grid:
        for bits in (20,53,80):cases.append((s,9999,bits,128 if bits!=20 else 53))
    for s in [mp.mpc(2,3),mp.mpc('.375','.75'),mp.mpc(-1,2)]:
        for rad in (-30,-60):
            for bits in (20,53,80):cases.append((s,rad,bits,128))
    lines=run([row(i,1,q,n,0,s,r,bits=bits,prec=prec) for i,(s,r,bits,prec) in enumerate(cases)])
    references={};flrefs={};maxratio=0
    for (s,r,bits,prec),l in zip(cases,lines):
        pole=q==1 and s.imag==0 and s.real in (0,1)
        if not pole:
            for t in corners(s,r):
                if t not in references:
                    references[t]=global_value(q,n,t)
                    flrefs[t]=flint_value(q,n,t)
                    check(abs(references[t][0]-flrefs[t][0])<mp.mpf('1e-48')*max(1,abs(references[t][0])),('independent references',q,n,t))
        for fn,(status,preserve,where,b) in enumerate(results(l)):
            counts[f'global_status_{status}']+=1
            expected=DOMAIN if fn==0 and s.real+ (0 if r==9999 else mp.power(2,r))<=1 else DOMAIN if pole else None
            if expected is not None:check(status==expected,('global status',q,n,s,r,bits,fn,status,expected))
            if status!=OK:check(preserve==1,('global atomicity',q,n,s,r,bits,fn));continue
            counts['global_OK']+=1
            check(width(b)<=mp.power(2,-bits),('global width',q,n,s,r,bits,fn,str(width(b))))
            maxratio=max(maxratio,float(width(b)/mp.power(2,-bits)))
            for t in corners(s,r):
                j=int(fn==2)
                check(contains(b,references[t][j]),('global mp enclosure',q,n,t,bits,fn,b,references[t][j]))
                check(contains(b,flrefs[t][j]),('global flint enclosure',q,n,t,bits,fn,b,flrefs[t][j]))
    counts['global_inputs']=len(cases);counts['reference_points']=len(references);counts['max_width_ratio']=maxratio

def realquad():
    for e in (0,1):
        q,n=(1,1) if e==0 else (4,3)
        for s in [mp.mpc(x,y) for x in (.5,1.5,3.5,6) for y in (0,.75,3)]:
            # Integral itself over R, after x=exp(u). No Gamma call in this reference.
            f=lambda u:2*mp.exp((s+e)*u-mp.pi*mp.exp(2*u))
            ref=mp.quad(f,[-300,-30,-10,-3,0,1,3])
            line=run([row(0,0,q,n,0,s,prec=128)])[0]
            check(contains(results(line)[0][3],ref),('real integral quad',e,s,ref))
            counts['real_quad_integrals']+=1
        for s in (mp.mpc('.375','.25'),mp.mpc('.625','-.5')):
            def zint(t):return mp.quad(lambda u:2*mp.exp((t+e)*u-mp.pi*mp.exp(2*u)),[-500,-50,-10,-3,0,1,3])
            g=1j**e*zint(1-s)/zint(s)
            line=run([row(0,0,q,n,0,s,prec=128)])[0]
            check(contains(results(line)[1][3],g),('real gamma quadrature FE',e,s,g))
            counts['real_quad_FE']+=1

def main():
    mode=sys.argv[1];arg=int(sys.argv[2]) if len(sys.argv)>2 else 0
    start=time.monotonic()
    if mode=='phase':phasecheck()
    elif mode=='local':localcheck(arg)
    elif mode=='global':globalcheck(arg)
    elif mode=='real':realquad()
    print(json.dumps({'mode':mode,'arg':arg,'elapsed':time.monotonic()-start,'counts':dict(counts),'failures':repr(failures)}),flush=True)
    return int(bool(failures))
if __name__=='__main__':raise SystemExit(main())
