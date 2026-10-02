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
out = subprocess.run(['timeout', '110', H], input=inp, capture_output=True, text=True).stdout.splitlines()
assert len(out) == len(cases), (len(out), len(cases))
bad = 0; stats = {}
for (mode, x, n, sd, Nreq, cap), o in zip(cases, out):
    tok = o.split(); st = tok[0]; err = None
    if mode == 'S':
        ex = expect(x, n, sd)
        if st != ex: err = "status %s expected %s" % (st, ex)
        elif st == 'OK': err = check_value(x, n, sd, Nreq, parse_ball(tok[1:5]))
        elif tok[1] != 'UNTOUCHED': err = "output touched"
    elif mode == 'C':
        ex = expect(x, n, None)
        if ex is None:
            ids = idents(x['p'], n, x['Un'], x['Ud'])
            ex = 'OK' if ids else 'DOMAIN'
            if st == 'OK' and int(tok[1]) != len(ids): err = "count %s, oracle %d" % (tok[1], len(ids))
        elif ex == 'OK' and st == 'OK':
            if int(tok[1]) != 1: err = "count"
        if st != ex: err = "count status %s expected %s" % (st, ex)
    else:
        ex = expect(x, n, None)
        if ex is None:
            ids = idents(x['p'], n, x['Un'], x['Ud']); ex = 'OK' if ids else 'DOMAIN'
        else: ids = [0]
        if st != ex: err = "roots status %s expected %s" % (st, ex)
        elif 'TOUCHED' in o: err = "roots touched outputs"
        elif st == 'OK':
            parts = o.split('|')[1:]
            got = [int(q.split()[0]) for q in parts]
            if got != ids: err = "ids %r oracle %r" % (got, ids)
            else:
                for q, t in zip(parts, ids):
                    e2 = check_value(x, n, t, Nreq, parse_ball(q.split()[1:5]))
                    if e2: err = "branch %d: %s" % (t, e2); break
    stats[(mode, st)] = stats.get((mode, st), 0) + 1
    if err:
        bad += 1
        if bad <= 25: print("FAIL", err, "|", line(mode, x, n, sd, Nreq, cap), "->", o)
print("cases", len(cases), "fail", bad, sorted(stats.items()))
print("oracle14 counts", COUNT)
