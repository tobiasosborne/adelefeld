from fractions import Fraction as Q
from collections import Counter
import itertools
import random
import subprocess
import sys

counts = Counter()
rng = random.Random(42991)
bin = sys.argv[1] if len(sys.argv) > 1 else 'lanes/f-review2/sb_bridge'
p = subprocess.Popen(['timeout', '160', bin], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                     text=True, bufsize=1)
INF = 10**100


def vp(q, prime):
    if not q: return INF
    a,b,v = abs(q.numerator),q.denominator,0
    while a % prime == 0: a //= prime; v += 1
    while b % prime == 0: b //= prime; v -= 1
    return v


def lb(prime, c, N=None):
    c = Q(c); v = vp(c, prime)
    if N is None: return (prime,1,c/Q(prime)**(v if c else 0),v if c else 0,0)
    if v >= N: return (prime,0,Q(0),0,N)
    t = c/Q(prime)**v; mod = prime**(N-v)
    return (prime,0,Q(t.numerator*pow(t.denominator,-1,mod)%mod),v,N)


def center(b): return b[2]*Q(b[0])**b[3]
def interval(x):
    a,m,e,r,re,_ = x
    return Q(m)*Q(2)**e-Q(r)*Q(2)**re, Q(m)*Q(2)**e+Q(r)*Q(2)**re
def encode(x): return ' '.join(map(str,x[:5]))+' '+str(len(x[5]))+' '+' '.join(str(t) for b in x[5] for t in b)
SENT = (1,13,0,0,0,[])


def call(line):
    p.stdin.write(line+'\n'); p.stdin.flush()
    t = p.stdout.readline().split()
    assert len(t) >= 8,(line,t)
    st,w,bit,canon,arch,m,r,n = t[:8]
    n = int(n); loc=[]
    assert len(t) == 8+5*n,(line,t)
    for i in range(n):
        prime,ex,u,v,N = t[8+5*i:13+5*i]
        loc.append((int(prime),int(ex),Q(u),int(v),int(N)))
    return st,int(w),int(bit),int(canon),(int(arch),Q(m),Q(r),loc)


def parsed(x):
    lo,hi = interval(x)
    return x[0],(lo+hi)/2,(hi-lo)/2,x[5]


def points(b):
    return [center(b)] if b[1] else [center(b)+Q(b[0])**b[4]*i for i in (0,1,2,b[0]-1)]


def assert_hull(b, values):
    c = center(b)
    assert all(t == c if b[1] else vp(t-c,b[0]) >= b[4] for t in values),(b,values)
    h = min(vp(t-values[0],b[0]) for t in values)
    assert (b[1] and h == INF) or (not b[1] and h == b[4]),(b,h)
    counts['local_point_results'] += len(values); counts['tight_local_hulls'] += 1


def arithmetic(op,x,y):
    ref = call(f'{op} 0 '+encode(x)+' '+encode(y))
    assert ref[:2] == ('OK',11) and ref[3] == 1,ref
    z = ref[4]; assert z[0] == x[0] and len(z[3]) == len(x[5])
    xx,yy = interval(x),interval(y)
    fn = {'add':lambda a,b:a+b,'sub':lambda a,b:a-b,'mul':lambda a,b:a*b,
          'selfsub':lambda a,b:a-b,'neg':lambda a,b:-a}[op]
    if op == 'selfsub': yy=xx; y=x
    if x[0]:
        values = [fn(a,b) for a in xx for b in yy]
        assert z[1]-z[2] <= min(values) and max(values) <= z[1]+z[2],(op,x,y,z)
        counts['real_point_results'] += len(values)
    for a,b,c in zip(x[5],y[5],z[3]):
        assert a[0] == b[0] == c[0]
        assert_hull(c,[fn(s,t) for s in points(a) for t in points(b)])
    for alias in (1,2):
        assert call(f'{op} {alias} '+encode(x)+' '+encode(y)) == ref
        counts['alias_calls'] += 1
    counts['arithmetic_cases'] += 1


def predicate(op,x,y):
    same = x[0] == y[0] and [b[0] for b in x[5]] == [b[0] for b in y[5]]
    want=same
    if x[0]:
        a,b=interval(x); c,d=interval(y)
        want &= {'eq':a==c and b==d,'over':max(a,c)<=min(b,d),'inside':c<=a and b<=d}[op]
    for a,b in zip(x[5],y[5]):
        diff = vp(center(a)-center(b),a[0])
        eq=a==b
        over = center(a)==center(b) if a[1] and b[1] else diff >= min(t[4] for t in (a,b) if not t[1])
        inside=(a[1] and center(a)==center(b)) if b[1] else (a[1] or a[4]>=b[4]) and diff>=b[4]
        want &= {'eq':eq,'over':over,'inside':inside}[op]
    got=call(f'{op} 0 '+encode(x)+' '+encode(y))[2]
    assert got == want,(op,x,y,got,want)
    counts['predicate_calls'] += 1


def project(A,H,d,places,local):
    line=f'project 0 {A} {H} {d} 3 -1 1 -2 {local} {len(places)} '+' '.join(map(str,places))
    st,w,_,canon,z=call(line)
    dup=sorted(v for v in set(places) if places.count(v)>1)
    if dup:
        assert (st,w,z) == ('DOMAIN',dup[0],parsed(SENT)),(line,st,w,z)
        counts['projection_status_cases'] += 1
        return
    assert (st,w,canon) == ('OK',11,1),(line,st,w,canon)
    assert z[:3] == ((1,Q(3,2),Q(1,4)) if 0 in places else (0,Q(0),Q(0)))
    assert [b[0] for b in z[3]] == sorted(v for v in places if v)
    for b in z[3]:
        prime=b[0]
        want=lb(prime,Q(A,d),None if not H else vp(Q(H,d),prime))
        assert b==want,(line,b,want)
        assert_hull(b,[Q(A,d)+Q(H,d)*i for i in (0,1,2,prime-1)])
        counts['projection_components'] += 1
    counts['projections'] += 1


try:
    primes=(2,3,5,7,2**64-59)
    def rnd():
        loc=[lb(prime,Q(rng.randint(-20,20),rng.randint(1,19))*Q(prime)**rng.randint(-3,3),
                None if rng.randrange(3)==0 else rng.randint(-4,5)) for prime in primes]
        arch=rng.randrange(2)
        return (arch,rng.randint(-12,12) if arch else 0,rng.randint(-3,1),
                rng.randint(0,8) if arch else 0,rng.randint(-5,1),loc)
    for _ in range(200):
        x,y=rnd(),rnd(); y=(x[0],*y[1:])
        for op in ('add','sub','mul','neg','selfsub'): arithmetic(op,x,y)
        for op in ('eq','over','inside'):
            for a,b in ((x,y),(y,x),(x,x)): predicate(op,a,b)
    empty=(0,0,0,0,0,[])
    for op in ('add','sub','mul','neg','selfsub'): arithmetic(op,empty,empty)
    for _ in range(150):
        A,H,d=rng.randint(-100,100),rng.randint(0,180),rng.randint(1,100)
        places=list(primes)+( [0] if rng.randrange(2) else []); rng.shuffle(places)
        project(A,H,d,places,0)
        project(A,180,d,places,1)
    for order in itertools.permutations((0,2,2**64-59)): project(2,180,3,list(order),0)
    project(2,0,3,list(primes)+[0],0)
    project(-5,0,2,[0]+list(reversed(primes)),0)
    for places in ([],[0,0],[2,3,2],[3,3,2,2],[0,2,2,0]): project(1,180,6,places,0)
    first=[]
    candidate=2
    while len(first)<1000:
        if all(candidate%q for q in range(2,int(candidate**.5)+1)): first.append(candidate)
        candidate+=1
    project(1,180,6,list(reversed(first))+[0],0)
    E=2**60
    def s(arch,loc): return (arch,0,0,0,0,loc)
    rows=[]
    for op in ('add','sub','mul'):
        rows += [(op,s(1,[]),s(0,[]),'DOMAIN',0),
                 (op,s(0,[lb(2,1)]),s(0,[lb(3,1)]),'DOMAIN',2),
                 (op,s(2,[]),s(2,[]),'UNSUPPORTED',0)]
    huge=(2,1,Q(1),E,0)
    for op in ('add','sub'): rows.append((op,s(0,[huge]),s(0,[lb(2,1)]),'LIMIT',2))
    rows += [('mul',s(0,[huge]),s(0,[huge]),'LIMIT',2),
             ('neg',s(0,[(2,0,Q(1),0,E)]),s(0,[]),'LIMIT',2)]
    for op,x,y,want,where in rows:
        for alias in (0,1,2):
            got=call(f'{op} {alias} '+encode(x)+' '+encode(y))
            assert got[:2]==(want,where) and got[3]==1,(op,alias,got)
            assert got[4]==parsed(x if alias==1 else y if alias==2 else SENT),(op,alias,got)
            counts['operation_status_alias_cases'] += 1
    print(dict(counts),flush=True)
finally:
    p.stdin.close(); assert p.wait(timeout=5)==0
