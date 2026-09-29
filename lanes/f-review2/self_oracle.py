import oracle as o
from fractions import Fraction as Q

cases=0
try:
    for p in (2,3,5,7,2**64-59):
        for v in range(-4,5):
            for k in (1,2,4):
                for c in (Q(0),Q(p)**v,Q(3,11)*Q(p)**v):
                    for x in (o.exact(p,c),o.ball(p,c,v+k)):
                        for op in ('sub','div'):
                            o.check_result(op,x,x,(0,1,2,p-1))
                            reference=o.call(op,x,x)
                            for alias in (3,4):
                                got=o.call(op,x,x,alias)
                                if reference[0] != 'OK' and alias==4:
                                    assert got[0]==reference[0] and got[1]==x,(op,x,got)
                                else:
                                    assert got==reference,(op,x,got,reference)
                                cases+=1
    print('same_object_calls',cases,'enumeration',dict(o.COUNTS),flush=True)
finally:
    o.proc.stdin.close();assert o.proc.wait(timeout=5)==0
