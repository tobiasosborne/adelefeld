import random, sys
from math import gcd
sys.path.insert(0, "lanes/f-review12")
from orc import run

def primes_upto(n):
    s = [True]*(n+1); r = []
    for i in range(2, n+1):
        if s[i]:
            r.append(i)
            for j in range(i*i, n+1, i): s[j] = False
    return r
PR = primes_upto(300)
def factor(n):
    f = {}
    for p in PR:
        while n % p == 0: f[p] = f.get(p, 0)+1; n //= p
    if n > 1:
        d = 301
        while d*d <= n:
            while n % d == 0: f[d] = f.get(d,0)+1; n //= d
            d += 2
        if n > 1: f[n] = f.get(n,0)+1
    return f
def pw(b, e, m):
    if m == 1: return 0
    if e < 0: b = pow(b, -1, m); e = -e
    return pow(b, e, m)
def onec(q, t, c, N, e, M):
    """is the true set one class mod q^t ?  (exhaustive over base residues b = c mod gcd(N,q^t))"""
    m = q**t
    gN = gcd(N, m)
    b0 = None
    for b in range(1, m+1):
        if gcd(b, m) != 1 or (b - c) % gN: continue
        if b0 is None:
            b0 = b; v0 = pw(b, e, m)
        elif pw(b, e, m) != v0: return False
        if pw(b, M, m) != 1: return False   # M=0 gives 1
    return True
def canonN(N): return N//2 if N % 4 == 2 else N
def true_fine(c, N, e, M):
    N = canonN(N)
    qs = sorted(set(PR) | set(factor(N)))
    F = 1; rs = []
    for q in qs:
        t = 0
        while t < 12 and q**(t+1) < 10**6 and onec(q, t+1, c, N, e, M):
            t += 1
        if t >= 12: raise Exception("cap", q, c, N, e, M)
        if q == 2 and t == 1: t = 0
        if t:
            m = q**t
            b0 = c % m if N % q == 0 else 1
            rs.append((pw(b0, e, m), m)); F *= m
    r = 0; mod = 1
    for (x, m) in rs:
        k = ((x - r) * pow(mod, -1, m)) % m
        r += mod*k; mod *= m
    return (r % F if F > 1 else 1), F
def st(line):
    t = line.split(); return int(t[0]), t[1:]

def main(n, seed):
    rnd = random.Random(seed)
    cases = []
    while len(cases) < n:
        N = rnd.choice([1,2,3,4,5,6,7,8,9,10,12,15,16,20,24,32,48,60,64,100,128,210, 18, 27, 81, 125, 36, 14, 28, rnd.randint(1,400)])
        c = rnd.randint(-30, 400)
        if gcd(c, N) != 1: continue
        e = rnd.choice([0,1,-1,2,-2,3,4,5,6,8,12,24,-6,-12, rnd.randint(-40, 40), rnd.randint(-300,300), 7])
        M = rnd.choice([0,0,1,2,3,4,6,8,12,16,24,rnd.randint(0,64), rnd.randint(0, 600)])
        cases.append((c, N, e, M))
    out = {m: run([f"pp{m} {c} {N} {e} {M}" for c, N, e, M in cases]) for m in (0,1,2)}
    nf = 0; counts = {0:[0,0], 2:[0,0]}
    def fail(*a):
        nonlocal nf
        nf += 1
        if nf < 40: print("FAIL", *a)
    for i, (c, N, e, M) in enumerate(cases):
        cN = canonN(N)
        one2 = all(onec(q, v, c, cN, e, M) for q, v in factor(cN).items())
        s, rest = st(out[0][i])
        if (s == 0) != one2: fail("strict-status", c, N, e, M, out[0][i], one2)
        if s == 0:
            counts[0][0] += 1
            rr, NN = int(rest[0]), int(rest[1])
            if e == 0 and M == 0:
                if (rr, NN) != (1, 0): fail("strict e0", c, N, e, M, out[0][i])
            else:
                exp = pw(c, e, cN) if cN > 1 else 1
                if exp == 0: exp = cN
                if NN != cN or rr != exp: fail("strict-value", c, N, e, M, out[0][i], exp)
        else: counts[0][1] += 1
        s, rest = st(out[1][i])
        if s != 0: fail("coarse-status", c, N, e, M, out[1][i]); continue
        rr, NN = int(rest[0]), int(rest[1])
        if e == 0 and M == 0:
            if (rr, NN) != (1, 0): fail("coarse e0", c, N, e, M, out[1][i])
        else:
            D = gcd(cN, pow(c, M, cN) - 1) if (M and cN > 1) else cN
            D = canonN(D)
            ev = pw(c, e, D) if D > 1 else 1
            if NN != D or rr != ev: fail("coarse-value", c, N, e, M, out[1][i], (ev, D))
        g = gcd(e, M) if M else abs(e)
        s, rest = st(out[2][i])
        if e == 0 and M == 0:
            if s != 0 or (int(rest[0]), int(rest[1])) != (1, 0): fail("fine e0", out[2][i])
            continue
        if g > 256:
            if s != 10: fail("fine-limit", c, N, e, M, out[2][i])
            continue
        if s != 0: fail("fine-status", c, N, e, M, out[2][i]); continue
        counts[2][0] += 1
        try: r, F = true_fine(c, N, e, M)
        except Exception: continue
        rr, NN = int(rest[0]), int(rest[1])
        if (rr, NN) != (r, F):
            fail("fine-value", c, N, e, M, out[2][i], (r, F))
    print("profpow cases", len(cases), "fails", nf, "strict ok/refused", counts[0], "fine checked", counts[2][0])
if __name__ == "__main__": main(int(sys.argv[1]), int(sys.argv[2]))
