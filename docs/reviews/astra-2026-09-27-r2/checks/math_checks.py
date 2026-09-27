"""Exact-rational series checks, using tail bounds proved in the review.
Run from repository root: python3 docs/reviews/astra-2026-09-27-r2/checks/math_checks.py
No external modules or bytecode files are used.
"""
from fractions import Fraction as F
from math import factorial, ceil
import runpy

def vp(x, p):
    x = F(x)
    if not x:
        return float('inf')
    a, b, v = abs(x.numerator), x.denominator, 0
    while a % p == 0:
        a //= p
        v += 1
    while b % p == 0:
        b //= p
        v -= 1
    return v

def exp_family(fn, x, p, prec):
    x = F(x)
    if not x:
        return F(fn in ('exp', 'cos', 'cosh'))
    d = vp(x, p)
    assert (p - 1) * d > 1
    stop = max(2, ceil(((p - 1) * prec - 1) / ((p - 1) * d - 1)))
    ans = F(0)
    for k in range(stop):
        if fn in ('sin', 'sinh') and k % 2 == 0:
            continue
        if fn in ('cos', 'cosh') and k % 2:
            continue
        sign = (-1) ** (k // 2) if fn in ('sin', 'cos') else 1
        ans += sign * x ** k / factorial(k)
    return ans

def log_series(x, p, prec):
    z = F(x) - 1
    assert not z or vp(z, p) >= 1
    # For k >= 4, floor(log_p(k)) <= k/2, so k-v_p(k) >= k/2.
    return sum(((-1) ** (k - 1) * z ** k / k for k in range(1, 2 * prec + 4)), F(0))

def residue(x, p, prec):
    x = F(x)
    modulus = p ** prec
    assert x.denominator % p
    return (x.numerator * pow(x.denominator, -1, modulus)) % modulus

prec = 16
for p, d in ((2, 2), (3, 1), (5, 1)):
    a, h = p ** d, p ** (d + 1)
    for fn in ('exp', 'sin', 'sinh'):
        difference = exp_family(fn, a+h, p, prec) - exp_family(fn, a, p, prec)
        assert vp(difference, p) == d + 1
        print(f'p={p} {fn}: v(f(a+h)-f(a))={vp(difference,p)} = v(h)')
    for fn in ('cos', 'cosh'):
        difference = exp_family(fn, p**d, p, prec) - 1
        expected = 2*d - (p == 2)
        assert vp(difference, p) == expected
        print(f'p={p} {fn}: v(f(p^{d})-1)={vp(difference,p)}')

for p,x in ((2,3),(2,-1),(2,5),(3,4)):
    result=residue(log_series(x,p,prec),p,prec)
    print(f'log series: p={p} x={x} mod p^{prec} = {result}, valuation={vp(result,p)}')
assert residue(log_series(-1,2,prec),2,prec)==0
assert residue(log_series(3,2,prec)-log_series(-3,2,prec),2,prec)==0
print('Iwasawa radius counterexamples:')
print('p=3: x=3,y=12, v(y-x)=2, v(log(y)-log(x))=',vp(log_series(4,3,prec),3))
print('p=2: x=2,y=10, v(y-x)=3, v(log(y)-log(x))=',vp(log_series(5,2,prec),2))

for k in (2,3,4,5):
    n=2**k
    e=vp(F(2**n,factorial(n)),2)
    s=vp(F(2**(n+1),factorial(n+1)),2)
    assert e==1 and s==2
    print(f'2-adic boundary: degree {n}: exp/cos term v={e}; degree {n+1}: sin term v={s}')

for p,n,m in ((2,2,3),(3,3,2)):
    modulus=p**m
    units=range(1,modulus,2) if p==2 else range(1,modulus,p)
    images=sorted({pow(x,n,modulus) for x in units})
    print(f'principal/unit n-th power residues: p={p}, n={n}, modulus={modulus}: {images}')

# Proposition 4 with negative exponents and nonintegral centres.
centres={2:F(1,2),3:F(2,9),5:F(7,5)}
exponents={2:2,3:-1,5:1}
d=90
mods={p:p**(vp(d,p)+exponents[p]) for p in centres}
rs={p:residue(d*centres[p],p,vp(d,p)+exponents[p]) for p in centres}
A=next(a for a in range(600) if all(a % mods[p]==rs[p] for p in centres))
assert A==101
assert all(vp(F(A,d)-centres[p],p)>=exponents[p] for p in centres)
print(f'CRT neighbourhood: moduli={mods}, residues={rs}, A={A}, a={F(A,d)}, N=20/3')

prototype=runpy.run_path('proto/precision_rules.py')
for name,args in [('overlaps',(F(1),F(0),F(1),F(0))),('contains',(F(1),F(0),F(1),F(0)))]:
    try:
        value=prototype[name](*args)
        print('prototype exact-point edge:',name,value)
    except ZeroDivisionError:
        print('prototype exact-point edge:',name,'ZeroDivisionError (not exercised by its main checks)')
print('all exact series, boundary, power-residue and CRT assertions passed')
