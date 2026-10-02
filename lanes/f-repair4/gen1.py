"""Attack 1: broad grid, seeded and all-branch, balls and exact inputs; own oracle (oracle.py)."""
import random, subprocess, sys
sys.path.insert(0, 'lanes/f-repair4')
from oracle14 import *
H = sys.argv[2] if len(sys.argv) > 2 else 'lanes/f-repair4/build/h'
rng = random.Random(int(sys.argv[1]) if len(sys.argv) > 1 else 1)
PR = [2, 3, 5, 7, 13]
cases = []
def degrees(p):
    ds = list(range(1, 13)) + [p, p * p, p - 1, 2 * (p - 1), 3 * (p - 1)]
    return sorted(set(d for d in ds if d >= 1))
def unit(p, k):
    while True:
        u = rng.randrange(1, p**k)
        if u % p: return u
def mk_ball(p, n, rootable):
    s = vp(n, p); c = cfor(p)
    j = rng.choice([-2, -1, 0, 0, 1, 2])
    r = rng.randrange(max(1, c + s - 2), c + s + 5)
    if rootable:
        w = unit(p, r + 2)
        m = n * j
        U = pow(w, n, p**r)
    else:
        m = rng.choice([n * j, n * j + 1, j, -3, 0, 5])
        U = unit(p, r)
    return dict(kind='ball', p=p, m=m, Un=U % p**r, Ud=1, M=m + r)
def mk_exact(p, n, rootable):
    j = rng.choice([-1, 0, 0, 1])
    if rootable:
        while True:
            a = rng.randrange(1, 40) * rng.choice([1, -1]); bb = rng.randrange(1, 12)
            if a % p and bb % p and math.gcd(a, bb) == 1: break
        A, B = a**n, bb**n
        if rng.random() < 0.5:
            A *= rng.choice([q for q in [1, 2, 3, 5, 7, 11] if q % p and math.gcd(q, B) == 1])
    else:
        while True:
            A = rng.randrange(-300, 300); B = rng.randrange(1, 50)
            if A and A % p and B % p and math.gcd(A, B) == 1: break
    return dict(kind='exact', p=p, m=n * j if rootable else rng.choice([n * j, j + 1]), Un=A, Ud=B, M=0)
for p in PR:
    for n in degrees(p):
        if n > 200: continue
        for _ in range(12 if p < 13 else 6):
            for x in (mk_ball(p, n, True), mk_ball(p, n, False), mk_exact(p, n, True), mk_exact(p, n, False)):
                if (x['kind'] == 'ball' and x['Un'] == 0): continue
                seeds = (list(range(0, p + 2)) if p < 13 else list(range(0, 14))) if p != 2 else [0, 1, 2, 3, 5]
                for sd in seeds:
                    Nreq = rng.choice([-3, 0, 1, 2, 3, 5, 9])
                    cases.append(('S', x, n, sd, Nreq, 0))
                cases.append(('A', x, n, 0, rng.choice([1, 4, 9]), 64))
                cases.append(('C', x, n, 0, 0, 0))
# zeros, n=0
for p in PR:
    for n in [0, 1, 2, 3, p]:
        for sd in [0, 1]:
            cases.append(('S', dict(kind='zexact', p=p, m=0, Un=0, Ud=1, M=0), n, sd, 5, 0))
            cases.append(('S', dict(kind='zball', p=p, m=0, Un=0, Ud=1, M=rng.randrange(-3, 6)), n, sd, 5, 0))
inp = "\n".join(line(c[0], c[1], c[2], c[3], c[4], c[5]) for c in cases) + "\n"
import json
open(sys.argv[2], 'w').write(inp)
json.dump([[c[0], c[1], c[2], c[3], c[4], c[5]] for c in cases], open(sys.argv[2] + '.json', 'w'))
