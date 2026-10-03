"""Attack A4: powrat(x, e, n, seed, N) for n' >= 2 against pow_si(root_seed(x, n', seed, N), e') for ball x with
N >= E' (both the exact image) and N = big; statuses compared too (root_seed status first)."""
import sys, random
from oracle import *
import attack_powrat as AP

def main(sd, count):
    rng = random.Random(sd); random.seed(sd)
    H = Harness()
    cases = []
    while len(cases) < count:
        p = rng.choice((2, 3, 5, 7, 13, 65537))
        xf = AP.gen(rng, p)
        n = rng.choice((2, 3, 4, 6, 8, 9, 12, 25, 27))
        e = rng.choice((1, -1, 2, 3, -3, 5, -5, 7, 9, -9, 25, 27)) * rng.choice((1, 1, 1, 2, 3))
        g = math.gcd(abs(e), n); n1, e1 = n // g, e // g
        if n1 < 2: continue
        if xf[0] == 'ball' and xf[1] != 0:
            m, U = unit_part(F(xf[1]), p); m2 = (m // n1) * n1
            xf = ('ball', U * F(p) ** m2, xf[2] - m + m2)
            ids = [t for t in ((1, 3) if p == 2 else range(1, min(p, 200))) if branch_exists(p, n1, U, t)]
            seed = rng.choice(ids) if ids else 1
        else:
            seed = rng.randint(0, 4)
        N = rng.choice((10 ** 6, 300)) if xf[0] == 'ball' else 60
        cases.append((p, xf, e, n, seed, N, n1, e1))
    l1 = ['P %d %s %d %d %d %d' % (p, AP.xfields(p, xf), e, n, seed, N) for (p, xf, e, n, seed, N, n1, e1) in cases]
    l2 = ['S %d %s %d %d %d' % (p, AP.xfields(p, xf), n1, seed, N) for (p, xf, e, n, seed, N, n1, e1) in cases]
    import os
    if os.environ.get('DUMP'): open(os.environ['DUMP'],'w').write('\n'.join(l1)+'\n'); return
    o1, o2 = H.run(l1), H.run(l2)
    l3, k3 = [], []
    for k, o in enumerate(o2):
        f = o.split()
        if f[0] == 'OK':
            ex, v, N, u = int(f[1]), int(f[2]), int(f[3]), F(f[4])
            l3.append('I %d %d %d %d %d %d %d' % (cases[k][0], u.numerator, u.denominator, v, N, ex, cases[k][7]))
            k3.append(k)
    o3 = dict(zip(k3, H.run(l3) if l3 else []))
    bad = 0; same = 0; stats = {}
    for k in range(len(cases)):
        a = o1[k].replace(' ALIASDIFF', '!')
        b = o3.get(k, o2[k])
        stats[a.split()[0]] = stats.get(a.split()[0], 0) + 1
        if cases[k][1][0] == 'exact' and a != b and a.startswith('OK 0') and b.startswith('OK 0'):
            stats['exact_irrational_diff'] = stats.get('exact_irrational_diff', 0) + 1; continue
        if cases[k][1][0] == 'ball': stats['ball_' + a.split()[0]] = stats.get('ball_' + a.split()[0], 0) + 1
        if a != b:
            # a powrat ball at K = min(N, E') may be coarser than the chained ball only when N < E'
            bad += 1
            if bad < 10: print('DIFF', cases[k][:6], '\n  powrat', a[:150], '\n  chain ', b[:150])
        else: same += 1
    print('cases', len(cases), 'identical', same, 'different', bad, stats)

if __name__ == '__main__':
    main(int(sys.argv[1]), int(sys.argv[2]))
