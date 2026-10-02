"""Attack 2: p = 65537 and 2^64-59; degrees sharing large factors with p-1, n = p, word-size n."""
import random, subprocess, sys
sys.path.insert(0, 'lanes/f-repair4')
from oracle14 import *
H = sys.argv[2] if len(sys.argv) > 2 else 'lanes/f-repair4/build/h'
rng = random.Random(7)
P64 = 2**64 - 59
cases = []
def unit(p, k):
    while True:
        u = rng.randrange(1, p**k)
        if u % p: return u
for p, degs in [(65537, [2, 3, 4, 12, 16, 256, 4096, 65536, 131072, 65537, 3 * 65536, 2**63, 2**64 - 1]),
                (P64, [2, 3, 4, 5, 12, P64, P64 - 1, (P64 - 1) // 4, 2**63 - 1, 2**64 - 1])]:
    for n in degs:
        g = math.gcd(n, p - 1)
        for _ in range(6):
            s = vp(n, p)
            j = rng.choice([-1, 0, 1]) if n < 1000 else 0
            r = rng.randrange(1 + s, 4 + s)
            w = unit(p, r + 1)
            kind = rng.choice(['ball', 'ball', 'exact'])
            if kind == 'ball':
                U = pow(w, n, p**r) if rng.random() < 0.7 else unit(p, r)
                x = dict(kind='ball', p=p, m=n * j, Un=U, Ud=1, M=n * j + r)
            else:
                ww = rng.randrange(2, 60)
                U = pow(ww, n, p**8) if rng.random() < 0.6 else rng.randrange(2, 10**6)
                if U % p == 0: U += 1
                x = dict(kind='exact', p=p, m=n * j, Un=U, Ud=1, M=0)
            cap = g if g <= 70000 else 0
            Nreq = rng.choice([1, 5, 30])
            if g <= 70000: cases.append(('A', x, n, 0, Nreq, cap))
            cases.append(('C', x, n, 0, 0, 0))
            for sd in [1, p - 1, rng.randrange(1, p), 0, p, w % p]:
                cases.append(('S', x, n, sd, Nreq, 0))
inp = "\n".join(line(*c) for c in cases) + "\n"
import json
open(sys.argv[2], 'w').write(inp)
json.dump([[c[0], c[1], c[2], c[3], c[4], c[5]] for c in cases], open(sys.argv[2] + '.json', 'w'))
