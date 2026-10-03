"""Attack B3: each branch listed by adf_lball_roots equals adf_lball_root_seed for that identifier (same fields),
over exact and ball inputs, N from j-2 to E+2, odd p < 60 and p = 2, n up to 24; statuses equal for failures."""
import sys, random
from oracle import *
from attack_roots import parse_roots
from sympy import primerange

def main(sd, count):
    rng = random.Random(sd)
    H = Harness()
    lines, meta = [], []
    primes = [2] + list(primerange(3, 60))
    for _ in range(count):
        p = rng.choice(primes); n = rng.randint(2, 24); s = vp(n, p); c = cfor(p)
        t = rng.randrange(1, p ** 4)
        while t % p == 0: t += 1
        U = F(t) ** n * F(1 + p ** (s + c) * rng.randrange(0, 40))
        m = n * rng.randint(-2, 2); j = m // n
        if rng.random() < 0.5:
            xf = fields_exact(p, U * F(p) ** m); E = None
        else:
            M = m + s + c + rng.randint(0, 5); xf = fields_ball(p, U * F(p) ** m, M); E = j + (M - m) - s
        N = rng.randint(j - 2, (E if E is not None else j + 8) + 2)
        lines.append('A %d %s %d %d %d' % (p, xf, n, N, 1000))
        meta.append((p, xf, n, N))
    out = H.run(lines)
    lines2, idx = [], []
    for k, o in enumerate(out):
        st, ln, br, touched = parse_roots(o)
        p, xf, n, N = meta[k]
        if st == 'OK':
            for (t, res) in br:
                lines2.append('S %d %s %d %d %d' % (p, xf, n, t, N)); idx.append((k, t, res))
    out2 = H.run(lines2)
    bad = 0
    for (k, t, res), o2 in zip(idx, out2):
        f = o2.split()
        if f[0] != 'OK' or (int(f[1]), int(f[2]), int(f[3]), F(f[4])) != res:
            bad += 1
            if bad < 10: print('DIFF', meta[k], t, res, o2)
    print('lists', len(lines), 'branches compared', len(lines2), 'differences', bad)

if __name__ == '__main__':
    main(int(sys.argv[1]), int(sys.argv[2]))
