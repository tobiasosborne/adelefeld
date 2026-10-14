"""Independent models. No proto imports and no reference-vector reads."""
import math, functools, fractions, sympy as sp
import mpmath as mp
import flint
mp.mp.dps=60
flint.ctx.prec=256
F=fractions.Fraction
@functools.lru_cache(None)
def phases(q,label):
    if q==1:return (F(0),)
    tables=[]
    for p,a in sp.factorint(q).items():
        p,a=int(p),int(a);mod=p**a
        if p==2:
            if a==1:tables.append((mod,{1:(0,)},(1,)))
            elif a==2:tables.append((mod,{1:(0,),3:(1,)},(2,)))
            else:
                tab={}
                for j in range(2**(a-2)):
                    for sign in (0,1):tab[((-1)**sign*pow(5,j,mod))%mod]=(sign,j)
                tables.append((mod,tab,(2,2**(a-2))))
        else:
            # Choose and lift a primitive root independently. Agreement for the finite
            # conductors tested is checked separately; no universal label identification is asserted.
            g=int(sp.primitive_root(p))
            if a>1 and pow(g,p-1,p*p)==1:g+=p
            phi=(p-1)*p**(a-1)
            tab={pow(g,j,mod):(j,) for j in range(phi)}
            tables.append((mod,tab,(phi,)))
    out=[]
    for u in range(q):
        if math.gcd(u,q)!=1:out.append(None);continue
        t=F(0)
        for mod,tab,orders in tables:
            for x,y,d in zip(tab[label%mod],tab[u%mod],orders):t+=F(x*y,d)
        out.append(t%1)
    return tuple(out)
@functools.lru_cache(None)
def chars(q,n):
    return tuple(0 if t is None else mp.exp(2j*mp.pi*mp.mpf(t.numerator)/t.denominator) for t in phases(q,n))
@functools.lru_cache(None)
def cyclo(q,n,negative=False,inverse=False):
    ph=phases(q,n);L=math.lcm(q,*(t.denominator for t in ph if t is not None));x=sp.Symbol('X')
    powers=[]
    for u,t in enumerate(ph):
        if t is not None:
            a=(-t if inverse else t)+F(-u if negative else u,q)
            powers.append(int((a%1)*L))
    poly=sp.Poly(sum(x**a for a in powers),x).rem(sp.Poly(sp.cyclotomic_poly(L,x),x))
    root=mp.exp(2j*mp.pi/L)
    return sum(mp.mpf(int(c))*root**k[0] for k,c in poly.terms())
def parity(q,n):return int(phases(q,n)[(-1)%q]==F(1,2))
def rootnumber(q,n):return cyclo(q,n)/(1j**parity(q,n)*mp.sqrt(q))
@functools.lru_cache(None)
def transformed_average(q,n,p):
    if q==1:return mp.mpf(1)
    units=[u for u in range(q) if u%p];ch=chars(q,n)
    roots=[mp.exp(-2j*mp.pi*k/q) for k in range(q)]
    # Additive integration over residue cosets, followed by a separate multiplicative shell average.
    transformed={v:sum(mp.conj(ch[u])*roots[(u*v)%q] for u in units)/q for v in units}
    val=sum(transformed[v]*mp.conj(ch[v]) for v in units)/len(units)
    return val

def shell_sum(r):
    out=mp.mpc(0);term=mp.mpc(1);count=0
    while abs(term)>mp.mpf('1e-63'):
        out+=term;term*=r;count+=1
        if count>10000:raise ValueError('slow shell sum')
    return out

def local(q,n,p,alpha,s):
    e=parity(q,n)
    if not p:
        z=(s+e)/2;dual=(1-s+e)/2
        g=0 if z.imag==0 and z.real<=0 and z.real==int(z.real) else 1j**e*mp.pi**(s-mp.mpf('.5'))*mp.gamma(dual)*mp.rgamma(z)
        return mp.pi**(-z)*mp.gamma(z),g,1j**e
    a=0;t=q
    while t>1:t//=p;a+=1
    if a:
        return mp.mpc(1),alpha**a*p**(a*(1-s))*transformed_average(q,n,p),alpha**a*p**(a*(1-s))*transformed_average(q,n,p)
    r=alpha*p**(-s);rd=1/alpha*p**(s-1)
    first=shell_sum(r) if abs(r)<mp.mpf('.92') else 1/(1-r)
    gamma=(shell_sum(rd)/first if abs(rd)<mp.mpf('.92') and abs(r)<mp.mpf('.92') else (1-r)/(1-rd))
    return first,gamma,mp.mpc(1)

@functools.lru_cache(None)
def hurwitz_values(q,s):return tuple(mp.zeta(s,mp.mpf(a)/q) for a in range(1,q+1))
def L(q,n,s):
    if q==1:return mp.zeta(s)
    ch=chars(q,n)
    if s==1:return -sum(ch[a%q]*mp.digamma(mp.mpf(a)/q) for a in range(1,q+1))/q
    return q**(-s)*sum(ch[a%q]*v for a,v in enumerate(hurwitz_values(q,s),1))
def global_value(q,n,s):
    e=parity(q,n);z=(s+e)/2
    if z.imag==0 and z.real<=0 and z.real==int(z.real):
        k=int(-z.real)
        # Differentiate the residue-class identity termwise. Finite differencing at s=0
        # can lose digits inside Hurwitz's parameter-dependent evaluation.
        if q==1:
            deriv=mp.zeta(s,1,1)
        else:
            ch=chars(q,n)
            deriv=q**(-s)*sum(ch[a%q]*(mp.zeta(s,mp.mpf(a)/q,1)-mp.log(q)*mp.zeta(s,mp.mpf(a)/q)) for a in range(1,q+1))
        val=2*(-1)**k*mp.pi**k*deriv/mp.factorial(k)
    else:val=mp.pi**(-z)*mp.gamma(z)*L(q,n,s)
    return val,q**z*val

def flint_value(q,n,s):
    u=flint.acb(str(s.real),str(s.imag));ch=flint.dirichlet_char(q,n);e=ch.parity()
    z=(u+e)/2
    if z.imag.is_zero() and z.real.is_integer() and z.real<=0:
        # Independent second source at removable Gamma poles through FLINT's FE and Gauss/root code.
        dual=(1-u+e)/2;val=flint.acb(q/flint.arb.pi())**dual*dual.gamma()*flint.dirichlet_char(q,pow(n,-1,q) if q>1 else 1).l_function(1-u)
        tau=sum(ch(a)*flint.acb(0,2)*flint.arb.pi()*0 for a in []) if False else sum(ch(a)*flint.acb(0,2*flint.arb.pi()*flint.arb(a)/q).exp() for a in range(q))
        completed=tau/(flint.acb(0,1)**e*flint.arb(q).sqrt())*val
        raw=completed/flint.acb(q)**z
    else:
        raw=flint.arb.pi()**(-z)*z.gamma()*ch.l_function(u);completed=flint.acb(q)**z*raw
    return tuple(mp.mpc(v.real.str(70,radius=False),v.imag.str(70,radius=False)) for v in (raw,completed))
