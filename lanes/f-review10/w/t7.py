import sys, random, re
sys.path.insert(0, "lanes/f-review10/w")
from o1 import *
random.seed(3)
cases=[];meta=[]
for _ in range(60):
    p=random.choice([2,3,5,7,11]); K=random.choice([20,64,65,130,300,1000])
    typ=random.choice(["exact","mod"])
    rn=random.choice([1,2,3,5,7,13,11]); rd=random.choice([1,3,5,7])
    g=gcd(rn,rd); rn//=g; rd//=g
    rs=str(rn) if rd==1 else "%d/%d"%(rn,rd)
    if typ=="exact":
        c=random.choice([1,-1]); u="[%d]"%c; N=K
    else:
        M=p**K*random.choice([1,3,5]) if p not in(3,5) else p**K
        c=random.randrange(1,M)
        while gcd(c,M)!=1: c=random.randrange(1,M)
        u="[%d mod %d]"%(c,M); N=K+random.choice([0,3])
    cases.append("AT %d %d 5 (1 ; %s * %s)"%(p,N,rs,u)); meta.append((p,K,typ,rn,rd,c,N))
out,rc=run(cases); bad=0
for o,(p,K,typ,rn,rd,c,N) in zip(out,meta):
    mm=re.match(r"st=(\d+) c=(\S+) N=(-?\d+) exact=(\d)",o)
    if not mm or mm.group(1)!="0": print("OUT",o,p,K,typ); bad+=1; continue
    if mm.group(4)=="1":
        if not (typ=="exact" and rn%p==0 and rd==1 and False): pass
        m=vp(rn,p)-vp(rd,p)
        if Fraction(rn,rd)/Fraction(p)**m!=1: print("exact but unit != 1",o); bad+=1
        continue
    assert int(mm.group(3))==K,(o,K)
    m=vp(rn,p)-vp(rd,p); ru=Fraction(rn,rd)/Fraction(p)**m
    mod=p**K
    x=(ru.numerator*c*pow(ru.denominator,-1,mod))%mod
    want=logp(x,p,K)
    got=int(mm.group(2))
    if (got-want)%p**K: print("MISMATCH",p,K,typ,o[:60]); bad+=1
print("cases",len(cases),"bad",bad)
