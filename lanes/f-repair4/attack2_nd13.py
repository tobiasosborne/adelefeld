"""Attack 2: p = 65537 and 2^64-59; degrees sharing large factors with p-1, n = p, word-size n."""
import random, subprocess, sys
sys.path.insert(0, 'lanes/f-review6')
from oracle import *
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
import time; t0 = time.time()
out = subprocess.run(['timeout', '115', H], input=inp, capture_output=True, text=True).stdout.splitlines()
print("time", round(time.time() - t0, 1), "lines", len(out), "cases", len(cases))
bad = 0; stats = {}
for (mode, x, n, sd, Nreq, cap), o in zip(cases, out):
    p = x['p']; tok = o.split(); st = tok[0]; err = None
    s = vp(n, p); g = math.gcd(n, p - 1)
    base = expect(x, n, None)
    if base is None:
        # existence: some branch; for s=0 Euler's criterion; else check the residue roots found by the code
        Ub = unit_mod(x['Un'], x['Ud'], p, 1)
        exists = pow(Ub, (p - 1) // g, p) == 1
        if exists and s > 0:
            # need v(t^n - U) >= s+1 for the root residues: t = U^(e) style unknown; use ids from code if any
            exists = None
    if mode == 'S':
        ex = expect(x, n, sd)
        if st != ex: err = "status %s expected %s" % (st, ex)
        elif st == 'OK': err = check_value(x, n, sd, Nreq, parse_ball(tok[1:5]))
    elif mode == 'C':
        if base is None:
            if exists is not None and st != ('OK' if exists else 'DOMAIN'): err = "count status"
            if st == 'OK' and int(tok[1]) != g: err = "count %s gcd %d" % (tok[1], g)
        elif st != base: err = "count status"
    else:
        if 'TOUCHED' in o: err = 'touched'
        elif st == 'OK':
            parts = o.split('|')[1:]
            got = [int(q.split()[0]) for q in parts]
            if len(got) != g or got != sorted(set(got)): err = "ids not %d distinct sorted" % g
            for q, t in zip(parts, got):
                if not branch_ok(p, n, x['Un'], x['Ud'], t): err = "id %d not a branch" % t; break
                e2 = check_value(x, n, t, Nreq, parse_ball(q.split()[1:5]))
                if e2: err = "branch %d: %s" % (t, e2); break
        elif base is None and exists: err = "roots status %s but root exists" % st
    stats[(mode, st)] = stats.get((mode, st), 0) + 1
    if err:
        bad += 1
        print("FAIL", err, "|", line(mode, x, n, sd, Nreq, cap), "->", o[:300])
print("fail", bad, sorted(stats.items()))
