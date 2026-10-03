from math import gcd
BOUND=1<<60
ES=[-(1<<63),(1<<63)-1,1<<50,-(1<<50),3**30,-3**30,5**20,-5**20,1,-1]
NS=[2,3,4,9,17,(1<<63)+1,(1<<64)-1]
cases=0
for p in (2,3,5,7,65537,18446744073709551557):
    c=2 if p==2 else 1
    def v(k):
        z=0
        while k%p==0: k//=p; z+=1
        return z
    for e in ES:
        for n in NS:
            g=gcd(abs(e),n); e1=e//g; n1=n//g
            if n1==1: continue
            s=v(n1); ve=v(abs(e1))
            assert not(s and ve)
            for j in (-BOUND//4,-100,-1,0,1,BOUND//4):
                # Only nonzero valuations divisible by the admitted word degree fit the input cap.
                if abs(n1*j)>BOUND: continue
                for relative in (c+s,c+s+1,c+s+100):
                    Ep=e1*j+relative-s+ve
                    for K in (e1*j+1,Ep-1,Ep):
                        if K<=e1*j or K>Ep: continue
                        rr=max(1,K-e1*j-ve)
                        gain=int(p==2 and rr==1 and e1%2==0)
                        assert rr<=relative-s and e1*j+rr+ve+gain>=K
                        cases+=1
print('P3 word-endpoint precision inequalities=%d failures=0 precision=exact_integers'%cases)
print('P8 unsigned abs LONG_MIN=%d; signed magnitude bound is <=2^63, not <2^63'%(1<<63))
