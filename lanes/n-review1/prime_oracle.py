from fractions import Fraction as Q
from math import factorial
from collections import Counter
import subprocess
import sys

proc = subprocess.Popen(['timeout','170',sys.argv[1] if len(sys.argv)>1 else 'lanes/n-review1/prime_bridge'],
                        stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
counts = Counter()
fails = []

def chk(ok, detail):
    counts['checks'] += 1
    if not ok:
        fails.append(detail)
        if len(fails)<=15:
            print('FAIL',detail)

def call(op,x,N,mode):
    p,exact,v,n,u=x
    proc.stdin.write(f'{op} {p} {exact} {v} {n} {u} {N} {mode}\n'); proc.stdin.flush()
    t=proc.stdout.readline().split()
    if not t:
        raise RuntimeError((op,x,N,mode))
    st,where,unchanged,transport,canonical=map(int,t[:5])
    counts['calls']+=1
    chk(unchanged==transport==canonical==1,('transport',op,x,N,mode,t))
    chk(where==(17 if st==0 else 13 if mode&4 else p),('where',op,x,N,mode,t))
    B=tuple(map(int,t[5:9]))+(Q(t[9]),) if st==0 else None
    return st,B

def val(q,p):
    if q==0:return 10**100
    a,b=abs(q.numerator),q.denominator
    n=0
    while a%p==0:a//=p;n+=1
    while b%p==0:b//=p;n-=1
    return n

def mod(q,P):return q.numerator*pow(q.denominator,-1,P)%P

def point(op,q,p,N):
    if op=='E':
        return sum((q**i/Q(factorial(i)) for i in range(2*N+4)),Q(0))
    if op=='G':
        q/=Q(p)**val(q,p)
        if p==2:
            if mod(q,4)==3:q=-q
        else:
            P=p**(N+3);om=mod(q,p)
            for _ in range(N+3):om=pow(om,p,P)
            q=Q(mod(q,P)*pow(om,-1,P)%P)
    z=q-1
    return sum(((-1)**(i+1)*z**i/Q(i) for i in range(1,2*N+5)),Q(0))

def inside(q,B):
    p,exact,v,N,u=B
    delta=q-Q(p)**v*u
    return delta==0 if exact else val(delta,p)>=N

for p in (2,3,5,7,2**64-59):
    c=2 if p==2 else 1
    for op in 'ELG':
        exacts = [Q(0),Q(p)**c,Q(2)*p**c,Q(-p)**c,Q(1)] if op=='E' else [Q(0),Q(1),Q(-1),Q(1)+p,Q(1)+p**2,Q(2),Q(p),Q(1,p)]
        for q in exacts:
            v=0 if q==0 else val(q,p)
            x=(p,1,v,0,q/Q(p)**v)
            want=0 if (val(q,p)>=c if op=='E' else q!=0 if op=='G' else val(q-1,p)>=1) else 7
            for N in (1,2,4,7,12):
                for mode in (0,1,2,3,4,5):
                    st,B=call(op,x,N,mode)
                    chk(st==(7 if mode&4 else want),('status',op,x,N,mode,st,want))
                    if st==0:
                        exact_result = 1 if op=='E' and q==0 else 0 if op!='E' and q in (1,-1) else None
                        if exact_result is not None and B[1]:
                            chk(Q(p)**B[2]*B[4]==exact_result,('special',op,x,N,B))
                        else:
                            true=point(op,q,p,N)
                            chk(inside(true,B),('series_enclosure',op,x,N,B,true))
                        counts['series_points']+=1
    for x in ((p,0,0,c,Q(0)),(p,0,0,0,Q(0)),(p,0,0,1,Q(1)),(p,0,1,2,Q(1))):
        for op in 'ELGS':
            for N in (-3,0,1,4,2**21+1,2**63-1,-2**63):
                for mode in (0,1,2,3,4,5):
                    # Skip nontrivial huge working moduli: limits are tested with exact zeros below.
                    if N>100 and x[4]!=0 and op in 'ELG':continue
                    call(op,x,N,mode)
    for op,u,want in (('E',Q(0),0),('L',Q(1),0),('G',Q(1),0)):
        x=(p,1,0,0,u)
        for N in (2**21+1,2**60,2**60+1,2**63-1,-2**63):
            for mode in (0,1,2,3,4,5):
                st,B=call(op,x,N,mode)
                # Exact exp(0), log(1), Log(1) are exempt from requested local precision limits.
                chk(st==(7 if mode&4 else want),('precision_special',op,x,N,mode,st))

proc.stdin.close()
chk(proc.wait(timeout=5)==0,'bridge_exit')
print('counts',dict(counts));print('failures',len(fails))
raise SystemExit(bool(fails))
