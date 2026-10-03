"""Attack A3: large primes, thousands of bits, high precision. powrat by certified roots (D2) at sampled points;
powunit by integer powers modulo p^K (D3) at sampled points; tightness by a pair of sampled points."""
import sys, random
from oracle import *
import attack_powrat as AP

STATS = {}
def bump(k): STATS[k] = STATS.get(k, 0) + 1

def rand_unit(rng, p, bits):
    while True:
        a = rng.getrandbits(bits) | 1
        if a % p: return a

def powunit_point(p, u, s, K):
    """u^s modulo p^K for a unit u (Fraction or int) in the domain and s in Z_p (Fraction/int), by D3"""
    c = cfor(p); k = max(K - c, 1)
    return pow(mod_unit(u, p, K), mod_unit(s, p, k), p ** K)

def check_powunit(p, uf, sf, N, out, rng):
    st, res, _ = parse(out)
    if any(t in out for t in ('ALIASDIFF',)): return 'alias ' + out
    if st != 'OK': return 'status ' + out
    if res[0] == 1: bump('exact'); return None
    ex, v, K, u = res
    if K <= 0: return None if u == 0 else 'centre at K<=0'
    cen = mod_unit(F(p) ** v * u, p, K)
    pts = []
    for i in range(6):
        if uf[0] == 'exact': ut = F(uf[1])
        else: ut = F(uf[1]) + F(p) ** uf[2] * (0 if i == 0 else rng.randrange(p ** 3))
        if sf[0] == 'exact': st_ = F(sf[1])
        else: st_ = F(sf[1]) + F(p) ** sf[2] * (0 if i == 0 else rng.randrange(p ** 3))
        pts.append(powunit_point(p, ut, st_, K + 1))
    for t in pts:
        if t % p ** K != cen: return 'enclosure: %d vs centre %d mod p^%d; %s' % (t % p ** K, cen, K, out[:200])
    if K < N and (uf[0] == 'ball' or sf[0] == 'ball'):
        bump('tight')
        if len({t for t in pts}) == 1: bump('tight_inconclusive')
    return None

def main(count, sd):
    rng = random.Random(sd); random.seed(sd)
    primes = (2, 3, 5, 65537, 2 ** 64 - 59)
    cases, lines, kinds = [], [], []
    for _ in range(count):
        p = rng.choice(primes)
        bits = rng.choice((64, 500, 3000))
        if rng.random() < 0.5:
            # powrat: exact or ball x with a large unit
            n = rng.choice((2, 3, 4, 5, 6, 8, 12, 2 * p if p < 100 else 2, p if p < 100 else 3))
            e = rng.choice((1, -1, 2, -3, 5, 7, -7, p if p < 100 else 11, -p * 2 if p < 100 else -2))
            g = math.gcd(abs(e), n); n1 = n // g
            t = rand_unit(rng, p, bits)
            m = n1 * rng.randint(-3, 3)
            U = F(t) ** n1 if rng.random() < 0.3 else F(rand_unit(rng, p, bits), rand_unit(rng, p, 64))
            if p == 2 and n1 % 2 == 0: U = F(pow(t, n1, 2 ** 4000))
            ids = [x for x in ((1, 3) if p == 2 else ([1, p - 1] + [rng.randrange(1, p) for _ in range(4)]))
                   if branch_exists(p, n1, U, x)]
            if not ids:
                U = F(t) ** n1 if bits <= 500 else F(pow(t, n1, p ** 60)); ids = [x for x in ((1, 3) if p == 2 else
                    [mod_unit(F(t), p, 1), p - mod_unit(F(t), p, 1)]) if branch_exists(p, n1, U, x)]
            seed = rng.choice(ids)
            N = rng.choice((5, 40, 200, 700))
            if rng.random() < 0.5:
                xf = ('exact', U * F(p) ** m)
            else:
                M = m + vp(n1, p) + cfor(p) + rng.choice((0, 1, 5, 50, 300))
                xf = ('ball', F(mod_unit(U, p, M - m)) * F(p) ** m, M)
            cases.append(('P', p, xf, e, n, seed, N))
            lines.append('P %d %s %d %d %d %d' % (p, AP.xfields(p, xf), e, n, seed, N))
        else:
            c = cfor(p)
            u0 = 1 + p ** rng.choice((1, 2, 3)) * rand_unit(rng, p, bits)
            if p == 2 and rng.random() < 0.5: u0 = -u0
            if rng.random() < 0.5: uf = ('exact', F(u0, 1 + p * rand_unit(rng, p, 64)))
            else: uf = ('ball', F(u0 % p ** 900), rng.choice((c, c + 1, 10, 60, 400)))
            s0 = F(rng.getrandbits(bits) * rng.choice((1, -1)), rand_unit(rng, p, 32)) * F(p) ** rng.choice((0, 0, 1, 3))
            if rng.random() < 0.5: sf = ('exact', s0)
            else: sf = ('ball', F(mod_unit(s0, p, 900)), rng.choice((1, 2, 10, 60, 400)))
            N = rng.choice((5, 40, 200, 700) if p < 100 else (5, 40, 120))
            cases.append(('W', p, uf, sf, N))
            fu = fields_exact(p, uf[1]) if uf[0] == 'exact' else fields_ball(p, uf[1], uf[2])
            fs = fields_exact(p, sf[1]) if sf[0] == 'exact' else fields_ball(p, sf[1], sf[2])
            lines.append('W %d %s %s %d' % (p, fu, fs, N))
    if os.environ.get('DUMP'):
        open(os.environ['DUMP'], 'w').write('\n'.join(lines) + '\n'); return
    out = Harness().run(lines, timeout=110)
    bad = 0
    for cs, o in zip(cases, out):
        if cs[0] == 'P':
            if AP.expected_powrat(*cs[1:])[0] == 'POWSI': bump('skip_n1'); continue
            r = AP.check(*cs[1:], o, None)
            bump('P_' + o.split()[0])
        else:
            r = check_powunit(*cs[1:], o, rng)
            bump('W_' + o.split()[0])
        if r:
            bad += 1
            if bad < 20: print('FAIL', str(cs)[:300], '->', r[:300])
    print('cases', len(cases), 'failures', bad, STATS, AP.STATS)

if __name__ == '__main__':
    main(int(sys.argv[1]), int(sys.argv[2]))
