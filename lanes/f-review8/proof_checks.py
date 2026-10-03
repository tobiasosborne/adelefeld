#!/usr/bin/env python3
"""Exact finite comparisons for the proof review. Does not reuse a lane oracle."""
from fractions import Fraction
from math import gcd
from pathlib import Path
import json
import subprocess

OUT = Path('lanes/f-review8')
def vp(q,p):
    q=Fraction(q)
    if not q: return 10**6
    def f(a):
        k=0
        while a%p==0: a//=p; k+=1
        return k
    return f(abs(q.numerator))-f(q.denominator)
def mod(q,p,h):
    q=Fraction(q); m=p**h
    return q.numerator*pow(q.denominator,-1,m)%m

# P1: exact arithmetic, and injectivity on all roots of 1 in small finite torsion groups.
assert 3**2!=4 and (-2)**2==2**2==4
print('P1 exact: 3^2=9 !=4; (-2)^2=4; residues at 5: 2,3')
cases=0
for p in (2,3,5,7,11,13,17,19):
    order=2 if p==2 else p-1
    for n in range(2,20):
        d=gcd(n,order)
        for e in range(-20,21):
            if not e: continue
            # The subgroup of n-th roots of unity has exponents k*(order/d).
            vals={(e*k)%d for k in range(d)}
            assert (len(vals)==d)==(gcd(e,d)==1)
            cases+=1
print('P1 injectivity cases=%d failures=0 precision=exact_cyclic_group'%cases)

# P2: actual rational input points from nearby rational roots, not logarithm approximations.
# Compare exact Fraction difference valuations, including j<0 and e'<0.
cases=witnesses=0
rows=[]
for p in (2,3,5,7):
    c=2 if p==2 else 1
    for n in range(2,13):
        s=vp(n,p)
        for e in range(-13,14):
            if not e or gcd(abs(e),n)!=1: continue
            ve=vp(e,p)
            assert not(s>0 and ve>0)
            for j in (-3,-1,0,2):
                for rel in (c,c+1,c+3):
                    r=rel+s; m=n*j; M=m+r; Ep=e*j+rel+ve
                    b=Fraction(p)**j
                    # A perturbation by one relative unit supplies a tightness witness.
                    b1=b*(1+p**rel)
                    x=b**n; x1=b1**n; y=b**e; y1=b1**e
                    assert vp(x1-x,p)==M
                    assert vp(y1-y,p)==Ep
                    for t in (0,1,p,-1,1+p):
                        bt=b*(1+p**rel*t)
                        assert vp(bt**n-x,p)>=M
                        assert vp(bt**e-y,p)>=Ep
                        cases+=1
                    witnesses+=1
                    if p==3 and n==3 and e==-2 and j==-1 and rel==1:
                        rows.append({'p':p,'n':n,'e':e,'j':j,'M':M,'Eprime':Ep,
                                     'x':str(x),'x1':str(x1),'y':str(y),'y1':str(y1)})
print('P2 point comparisons=%d tightness_pairs=%d failures=0 precision=exact_Fraction'%(cases,witnesses))
print('P2 witness',json.dumps(rows))

# P3: prove the integer precision inequalities over admitted finite parameters. Independently
# enumerate the image of each intermediate ball at p^h, including the 2-adic rel=1 case.
cases=special=0
for p in (2,3,5,7):
    c=2 if p==2 else 1
    for n in range(2,12):
        s=vp(n,p)
        for e in range(-20,21):
            if not e or gcd(abs(e),n)!=1: continue
            ve=vp(e,p)
            for j in (-100,-3,-1,0,2):
                for r in range(c+s,c+s+5):
                    Ep=e*j+r-s+ve
                    for K in (e*j+1,Ep-2,Ep-1,Ep):
                        if K<=e*j or K>Ep: continue
                        rr=max(1,K-e*j-ve)
                        gain=int(p==2 and rr==1 and e%2==0)
                        assert rr<=r-s and rr+ve+gain>=K-e*j
                        special+=gain; cases+=1
print('P3 precision inequalities=%d exceptional_2adic=%d failures=0 precision=exact_integers'%(cases,special))

# P4/P5: enumerate all exponent and base residue classes modulo 2^H. The exponent period
# 2^(H-2) for odd units includes parity when H>=3; use 2^H to avoid relying on the claimed H'.
# Exact bases are rational residues; exact exponents include negative and rational integers in Z_2.
cases=missed=full=cosets=0
for H in (3,4,5,6):
    modulus=2**H
    bases=[(A,u) for A in range(1,5) for u in range(1,2**A,2)]
    bases += [(None,u) for u in (1,-1,3,5,7,9,Fraction(1,3))]
    exponents=[(B,s) for B in range(0,4) for s in range(2**B)]
    exponents += [(None,s) for s in (0,1,2,3,4,8,-1,-2,Fraction(1,3))]
    for A,u in bases:
        us={mod(u,2,H)} if A is None else {(u+2**A*t)%modulus for t in range(2**max(H-A,0))}
        for B,s in exponents:
            ss={mod(s,2,H)} if B is None else {(s+2**B*t)%modulus for t in range(2**max(H-B,0))}
            image={pow(a,b,modulus) for a in us for b in ss}
            beta=vp(s,2); par_fixed=B is None or B>=1
            par=bool(s and beta==0)
            w=1 if mod(u,2,2)==1 else -1
            if (B is None and s==0) or (A is None and u==1):
                want={1}
            elif A is None and u==-1 and par_fixed:
                want={(-1 if par else 1)%modulus}
            elif A==1 and par_fixed and not par:
                R=2+min(beta,B if B is not None else 10**6)
                want={(1+2**R*t)%modulus for t in range(2**max(H-R,0))}; cosets+=1
            elif A==1 or (w==-1 and B==0):
                assert {a%4 for a in image}=={1,3}
                if A==1:
                    want=set(range(1,modulus,2)); full+=1
                else:
                    assert not any(a%8==5 for a in image); missed+=1
                    assert image<set(range(1,modulus,2))
                    cases+=1; continue
            else:
                alpha=vp(w*u-1,2)
                terms=[]
                if A is not None: terms.append(A+beta)
                if B is not None: terms.append(B+alpha)
                if A is not None and B is not None: terms.append(A+B)
                if not terms: want=image # exact/exact singletons are checked below.
                else:
                    R=min(terms); centre=pow(mod(u,2,H),mod(s,2,H),modulus)
                    want={(centre+2**R*t)%modulus for t in range(2**max(H-R,0))}; cosets+=1
            assert image==want,(H,A,u,B,s,image,want)
            if A is None and B is None: assert len(image)==1
            cases+=1
print('P5 unions=%d fixed_sign_missed_classes=%d full_hulls=%d cosets=%d failures=0 H=3,4,5,6'%
      (cases,missed,full,cosets))

# P7: compare the prescribed reduced exponent period with a deliberately excessive period.
cases=0
for p in (2,3,5,7):
    for H in range(1,5):
        k=max(H-1,1); P=p**H
        for B in range(0,4):
            for s0 in range(min(p**B,5)):
                short=[s0+p**B*t for t in range(p**max(0,k-B))]
                long=[s0+p**B*t for t in range(p**max(0,H+1-B))]
                for u in range(1,P):
                    if u%p!=1: continue
                    assert {pow(u,s,P) for s in short}=={pow(u,s,P) for s in long}
                    cases+=1
print('P7 exponent_period comparisons=%d failures=0 H=1..4 extra_period=H+1'%cases)

# Header precision counterexample and LONG_MIN endpoint. Exact small centres, finite comparison 5^9.
commands=[
 'P 5 1 1 -3 5 0 2 3 1 0', # j=-1, E'=6, K=0, rel_r=2, Nr=1>K
 'S 5 1 1 -3 5 0 3 1 1',
 'P 5 1 1 0 3 0 -9223372036854775808 3 1 3',
]
(OUT/'proof-inputs.in').write_text('\n'.join(commands)+'\n')
res=subprocess.run(['timeout','30',str(OUT/'build/h')],input='\n'.join(commands)+'\n',
                   text=True,capture_output=True,check=True)
(OUT/'proof-library.log').write_text(res.stdout)
print('P3/P8 library endpoint outputs:'); print(res.stdout,end='')
