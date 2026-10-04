"""big-N profpow, factorisation of N known by construction."""
import random, sys
from math import gcd
sys.path.insert(0, "lanes/f-review12")
from orc import run
from orc2 import PR, pw, canonN
def mr(n, rnd):
    if n < 2: return False
    for p in PR[:20]:
        if n % p == 0: return n == p
    d = n-1; s = 0
    while d % 2 == 0: d//=2; s+=1
    for _ in range(12):
        a = rnd.randrange(2, n-1); x = pow(a, d, n)
        if x in (1, n-1): continue
        for _ in range(s-1):
            x = x*x % n
            if x == n-1: break
        else: return False
    return True
def rprime(bits, rnd):
    while True:
        n = rnd.getrandbits(bits) | (1 << (bits-1)) | 1
        if mr(n, rnd): return n
def vp(n, p):
    v = 0
    while n and n % p == 0: n//=p; v+=1
    return v
def main(n, seed):
    rnd = random.Random(seed); cases=[]
    for _ in range(n):
        fac = {}
        for p in rnd.sample([2,3,5,7,11,13,17,19,23], rnd.randint(0,5)):
            fac[p] = rnd.randint(1, 6)
        for _ in range(rnd.randint(0, 8)):
            fac[rprime(rnd.choice([40,80,150,250]), rnd)] = rnd.choice([1,1,1,2])
        N = 1
        for p, v in fac.items(): N *= p**v
        if N % 4 == 2: pass
        c = rnd.getrandbits(max(2, N.bit_length()+10)) - (N if rnd.random()<0.3 else 0)
        if gcd(c, N) != 1: continue
        g = rnd.choice([1,2,3,4,6,8,12,16,24,30,36,48,64,96,120,128,192,240,256, rnd.randint(1,256)])
        e = g*rnd.choice([0,1,-1,3,5,rnd.getrandbits(rnd.choice([10,100,2000]))])
        mm = rnd.choice([0, 1, 2, 3, 6, rnd.getrandbits(rnd.choice([10,100,2000]))])
        M = g*mm
        if e == 0 and M == 0: e = g
        cases.append((c, N, e, M, fac))
    out = {m: run([f"pp{m} {c} {N} {e} {M}" for c, N, e, M, _ in cases]) for m in (0,1,2)}
    nf = 0
    for i,(c,N,e,M,fac) in enumerate(cases):
        cN = canonN(N)
        g = gcd(e, M) if M else abs(e)
        def fl(*a):
            nonlocal nf
            nf += 1
            if nf<20: print("FAIL", *a, flush=True)
        # strict
        s, *rest = out[0][i].split()
        one = (M == 0) or cN == 1 or (pow(c, M, cN) - 1) % cN == 0
        # Strict for M==0 is always OK
        if (int(s) == 0) != one: fl("strict status", i, out[0][i][:60], one)
        elif int(s)==0:
            exp = pw(c, e, cN) if cN>1 else 1
            if exp==0: exp=cN
            if (int(rest[0]), int(rest[1])) != (exp, cN): fl("strict val", i)
        s, *rest = out[1][i].split()
        D = cN if (M==0 or cN==1) else canonN(gcd(cN, pow(c, M, cN)-1))
        ev = pw(c, e, D) if D>1 else 1
        if int(s)!=0 or (int(rest[0]), int(rest[1])) != (ev, D): fl("coarse", i, out[1][i][:80], (ev,D) if D<10**6 else D.bit_length())
        s, *rest = out[2][i].split()
        # fine via table
        if g>256:
            if int(out[2][i].split()[0])!=10: fl('limit',i,out[2][i][:50])
            continue
        F = 1; rs = []
        qs = set(PR) | set(canonN(N) and fac.keys())
        for q in sorted(qs):
            vN = vp(cN, q)
            if vN:
                if M == 0: t = vN + vp(g, q)
                else:
                    t = vN + vp(g, q)
                    m_ = q**(t)  # v_q(c^M-1) capped at t
                    cap = pow(c, M, q**(t+1)) - 1
                    t = min(t, vp(cap, q) if cap % q**(t+1) else t+1)
                    t = min(t, vN+vp(g,q))
            else:
                if q == 2: t = 1 if g % 2 else 2 + vp(g, 2)
                else: t = (1 + vp(g, q)) if g % (q-1) == 0 else 0
                if q == 2 and t == 1: t = 0
            if q == 2 and t == 1: t = 0
            if t:
                m = q**t
                b0 = c % m if vN else 1
                rs.append((pw(b0, e, m), m)); F *= m
        r = 0; mod = 1
        for (x, m) in rs:
            k = ((x-r)*pow(mod, -1, m)) % m; r += mod*k; mod *= m
        if F == 1: r = 1
        if int(s)!=0 or (int(rest[0]), int(rest[1])) != (r, F):
            fl("fine", i, (c,N,e,M) if N.bit_length()<30 or True and False else "", fac if len(fac)<4 else "",  out[2][i][:80], "N bits", N.bit_length(), "g", g, "M", M.bit_length(), "F bits", F.bit_length())
    print("big cases", len(cases), "fails", nf, "max N bits", max(c[1].bit_length() for c in cases))
if __name__ == "__main__": main(int(sys.argv[1]), int(sys.argv[2]))
