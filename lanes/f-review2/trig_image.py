from fractions import Fraction as Q
from mpmath import mp,iv
import random
import subprocess

mp.prec=iv.prec=6000
proc=subprocess.Popen(['timeout','90','lanes/f-review2/real_bridge-san'],stdin=subprocess.PIPE,
                      stdout=subprocess.PIPE,text=True,bufsize=1)


def rat(t):
    s,m,e,_=t
    return (-1 if s else 1)*Q(m)*Q(2)**e


def exact(q): return iv.mpf(q.numerator)/iv.mpf(q.denominator)


try:
    rng=random.Random(449)
    rows=[(0,0,1,20),(1,0,8,0),(-3,0,2,0)]
    rows += [(int(mp.floor(k*mp.pi/2*2**200)),-200,1,-190) for k in range(-8,9)]
    rows += [(rng.randint(-100,100),rng.randint(-5,1),rng.randint(1,20),rng.randint(-8,2))
             for _ in range(40)]
    cases=0
    for m,e,r,re in rows:
        lo=Q(m)*Q(2)**e-Q(r)*Q(2)**re;hi=Q(m)*Q(2)**e+Q(r)*Q(2)**re
        arg=iv.mpf([exact(lo).a,exact(hi).b])
        for prec in (2,53,4096):
            for name in ('sin','cos'):
                line=f'{name} {m} {e} {r} {re} 0 {prec}'
                proc.stdin.write(line+'\n');proc.stdin.flush()
                reply=proc.stdout.readline().split()
                assert reply[:6]==['OK','1','1','1','1','1'],(line,reply)
                mid,rad=map(Q,reply[6:])
                a,b=map(rat,getattr(iv,name)(arg)._mpi_)
                assert mid-rad<=a and b<=mid+rad,(line,reply,a,b)
                cases+=1
    print(f'whole_interval_trig_cases={cases} mismatches=0',flush=True)
finally:
    proc.stdin.close();assert proc.wait(timeout=5)==0
