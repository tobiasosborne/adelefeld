#!/usr/bin/env python3
from fractions import Fraction as Q
from math import gcd, lcm, factorial, log2, ceil
import random, cmath

def qgcd(*xs):
    d=lcm(*(x.denominator for x in map(Q,xs)))
    return Q(gcd(*(int(Q(x)*d) for x in xs)),d)
def contains(c,R,a,N):
    if R==0: return N==0 and c==a
    return (N/R).denominator==1 and ((a-c)/R).denominator==1
def tight(a,N,b,M,op):
    return (a+b,qgcd(N,M)) if op=='+' else (a*b,qgcd(a*M,b*N,N*M))
rng=random.Random(20260927)
count=0
for _ in range(10000):
    K=rng.randint(1,50)
    s=Q(rng.randint(1,50),rng.randint(1,30)); t=Q(rng.randint(1,50),rng.randint(1,30))
    u=rng.randrange(K); v=rng.randrange(K)
    a,N,b,M=s*u,s*K,t*v,t*K
    g=qgcd(s,t)
    z=(int(s/g)*u+int(t/g)*v)%K
    ac,ar=tight(a,N,b,M,'+')
    assert contains(g*z,g*K,ac,ar)
    ac,ar=tight(a,N,b,M,'*')
    assert contains(s*t*((u*v)%K),s*t*K,ac,ar)
    count+=2
print('scaled-policy enclosures passed:',count)
print('fixed absolute cap failure: tight (1/2 mod 1), attempted (1/2 mod 2), contains =', contains(Q(1,2),Q(2),Q(1,2),Q(1)))
print('modular reconstruction mismatch: (1/5-5)/6 =', (Q(1,5)-5)/6)
# Complex arithmetic here is only a diagnostic, not a certified Fourier calculation.
D,M=2,3; L=D*M
f=[complex(k%3-1,k%2) for k in range(L)]
def ft(f,cell):
    return [sum(f[j]*cmath.exp(-2j*cmath.pi*j*k/len(f)) for j in range(len(f)))/cell for k in range(len(f))]
g=ft(f,M); h=ft(g,D)
print('DFT D=2 M=3 double-transform max error:', max(abs(h[k]-f[-k%L]) for k in range(L)))
print('DFT weighted Plancherel error:',abs(sum(abs(x)**2 for x in f)/M-sum(abs(x)**2 for x in g)/D))
# For the convention selected in review, the finite transform uses negative exponent.
print('ramified mod-3 unit-character average:', (1+(-1))/2)
print('20! bits:',log2(factorial(20)))
qs=[2**18,3**8,5**4,7**2,11,13,17,19]
print('20! tuple separately packed bits:',sum((q-1).bit_length() for q in qs))
print('heap arithmetic 528/512:',528/512)
print('information bits own radius 1<=N<2^62:',log2(((2**62-1)*2**62)//2))
