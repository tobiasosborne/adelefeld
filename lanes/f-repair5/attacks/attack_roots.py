"""Attack B1: adf_lball_roots against a search over all residues (identifiers, order, count) and certified roots."""
import sys, random
from oracle import *
from sympy import primerange

def parse_roots(o):
    t = o.split(' | ')
    head = t[0].split()
    st, ln = head[0], int(head[1])
    br = []
    for b in t[1:]:
        f = b.split()
        br.append((int(f[0]), (int(f[1]), int(f[2]), int(f[3]), F(f[4]))))
    return st, ln, br, ('TOUCHED' in o)

def main(sd, mode):
    rng = random.Random(sd)
    cases, lines = [], []
    for p in primerange(3, 300):
        for n in range(2, 41):
            for rep in range(2):
                s = vp(n, p)
                t = rng.randrange(1, p ** 3)
                while t % p == 0: t = rng.randrange(1, p ** 3)
                if rep == 0 or rng.random() < 0.5: U = F(t) ** n * F(1 + p ** (s + 1) * rng.randrange(1, 50))
                else: U = F(rng.randrange(1, 10 ** 6) * p + rng.randrange(1, p), 1 + p * rng.randrange(0, 30))
                m = n * rng.randint(-2, 2)
                N = rng.choice((m // n + 1, m // n + 3, m // n + 8))
                if mode == 'ball':
                    M = m + s + 1 + rng.randint(0, 4)
                    xf = fields_ball(p, U * F(p) ** m, M)
                else:
                    xf = fields_exact(p, U * F(p) ** m)
                cases.append((p, n, U, m, N, mode))
                lines.append('A %d %s %d %d %d' % (p, xf, n, N, 100000))
    out = Harness().run(lines)
    bad = 0; nok = 0; nbr = 0
    for (p, n, U, m, N, md), o in zip(cases, out):
        s = vp(n, p)
        st, ln, br, touched = parse_roots(o)
        if md == 'ball':
            U = F(mod_unit(U, p, 50))
        ids = [t for t in range(1, p) if branch_exists(p, n, U, t)]
        if touched: bad += 1; print('TOUCHED', p, n, o[:200]); continue
        if not ids:
            if st != 'DOMAIN': bad += 1; print('FAIL expected DOMAIN', p, n, U, o[:200])
            continue
        if len(ids) != math.gcd(n, p - 1):
            print('ORACLE count', p, n, len(ids)); bad += 1; continue
        if st != 'OK' or ln != len(ids) or [b[0] for b in br] != ids:
            bad += 1
            if bad < 20: print('FAIL ids', p, n, U, 'expected', ids, 'got', o[:300])
            continue
        nok += 1
        j = m // n
        for t, res in br:
            nbr += 1
            if res[0] == 1:
                val = F(p) ** res[1] * res[3]
                if val ** n != U * F(p) ** m or mod_unit(val / F(p) ** j, p, 1) != t:
                    bad += 1; print('FAIL exact root', p, n, t, res)
                continue
            K = res[2]
            if K <= j:
                if res[3] != 0: bad += 1; print('FAIL zero ball', p, n, res)
                continue
            beta = nroot(p, n, U, t, K - j + 1)
            if not in_ball(p, (j, beta, K - j + 1), None, res):
                bad += 1
                if bad < 20: print('FAIL enclosure', p, n, U, t, res)
    print('mode', mode, 'cases', len(cases), 'listed OK', nok, 'branches checked', nbr, 'failures', bad)

if __name__ == '__main__':
    main(int(sys.argv[1]), sys.argv[2])
