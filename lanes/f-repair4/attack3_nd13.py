"""Attack 3: thousands of bits, word-size degrees, large N, p | n with large s."""
import random, subprocess, sys
sys.path.insert(0, 'lanes/f-review6')
from oracle import *
H = sys.argv[2] if len(sys.argv) > 2 else 'lanes/f-repair4/build/h'
rng = random.Random(11)
cases = []
def unit(p, k):
    while True:
        u = rng.randrange(1, p**k)
        if u % p: return u
# (a) big balls: r up to 3000 digits
for p in [2, 3, 7, 13]:
    for n in [2, 3, 5, p, p * p, 12]:
        for _ in range(3):
            r = rng.choice([200, 1000, 3000 if p < 10 else 1500]); j = rng.choice([-5, 0, 7])
            w = unit(p, r + 3); U = pow(w, n, p**r)
            x = dict(kind='ball', p=p, m=n * j, Un=U, Ud=1, M=n * j + r)
            t = w % (4 if p == 2 else p)
            if p == 2 and n % 2 == 0: t = w % 4
            cases.append(('S', x, n, t, 0, 0))
# (b) exact with big numerators/denominators: rational roots and irrational ones at large N
for p in [2, 3, 5, 7, 13, 65537]:
    for n in [2, 3, 4, 6, 7]:
        for _ in range(3):
            while True:
                a = rng.getrandbits(rng.choice([300, 1500])) | 1; b = rng.getrandbits(200) | 1
                if a % p and b % p and math.gcd(a, b) == 1: break
            sign = rng.choice([1, -1])
            A, B = (sign * a)**n, b**n
            if rng.random() < 0.3: A = A + p   # irrational (perturbed), keep p-free
            if A % p == 0: A += 1
            g = math.gcd(A, B); A //= g; B //= g
            x = dict(kind='exact', p=p, m=n * rng.choice([-3, 0, 2]), Un=A, Ud=B, M=0)
            for t in ((1, 3) if p == 2 else sorted(set([1, p - 1, rng.randrange(1, p), (a * pow(b, -1, p)) % p,
                                                        (-a * pow(b, -1, p)) % p]))):
                cases.append(('S', x, n, t, rng.choice([3, 50, 400]), 0))
# (c) word-size degrees with unit 1 and +-1, exact and ball
W = [2**63, 2**64 - 1, 2**63 - 1, 2**62, 3**40, 5**27]
for p in [2, 3, 5, 7]:
    for n in W:
        s = vp(n, p); c = cfor(p)
        for U in [1, p**(s + c) + 1, 2 * p**(s + c + 1) + 1]:
            for (kind, M) in [('ball', s + c), ('ball', s + c + 3), ('exact', 0)]:
                Um = U % p**M if kind == 'ball' else U
                if kind == 'ball' and Um == 0: continue
                x = dict(kind=kind, p=p, m=0, Un=Um, Ud=1, M=M)
                for t in ([1, 3] if p == 2 else range(1, p)):
                    cases.append(('S', x, n, t, 12, 0))
inp = "\n".join(line(*c) for c in cases) + "\n"
out = subprocess.run(['timeout', '110', H], input=inp, capture_output=True, text=True).stdout.splitlines()
print("lines", len(out), "cases", len(cases))
bad = 0; stats = {}
for (mode, x, n, sd, Nreq, cap), o in zip(cases, out):
    tok = o.split(); st = tok[0]
    ex = expect(x, n, sd); err = None
    if st != ex: err = "status %s expected %s" % (st, ex)
    elif st == 'OK': err = check_value(x, n, sd, Nreq, parse_ball(tok[1:5]))
    stats[st] = stats.get(st, 0) + 1
    if err:
        bad += 1
        print("FAIL", err, "|", line(mode, x, n, sd, Nreq, cap), "->", o[:200])
print("fail", bad, stats)
