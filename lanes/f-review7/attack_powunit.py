"""Attack A2: adf_lball_powunit against an enumeration of the image modulo p^H (fact D3 of oracle.py)."""
import sys, random
from oracle import *

STATS = {}
def bump(k): STATS[k] = STATS.get(k, 0) + 1

def meets_inside(p, xf, cen, D):
    """the set xf against the ball cen + p^D Z_p: 1 inside, 0 meets both, -1 disjoint"""
    if xf[0] == 'exact':
        return 1 if vq(F(xf[1]) - cen, p) >= D else -1
    c, A = F(xf[1]), xf[2]
    d = vq(c - cen, p)
    if d < min(A, D): return -1
    return 1 if A >= D else 0

def residues_u(p, uf, H):
    P = p ** H
    if uf[0] == 'exact': return [mod_unit(uf[1], p, H)]
    c, A = F(uf[1]), uf[2]
    c0 = mod_unit(c, p, H)
    if A >= H: return [c0]
    return [(c0 + p ** A * i) % P for i in range(p ** (H - A))]

def residues_s(p, sf, k):
    P = p ** k
    if sf[0] == 'exact': return [mod_unit(sf[1], p, k)]
    c, B = F(sf[1]), sf[2]
    c0 = mod_unit(c, p, k) if c != 0 else 0
    if B >= k: return [c0]
    return [(c0 + p ** B * i) % P for i in range(p ** (k - B))]

def image(p, uf, sf, H):
    c = cfor(p); k = max(H - c, 1); P = p ** H
    nu = 1 if uf[0] == 'exact' or uf[2] >= H else p ** (H - uf[2])
    ns = 1 if sf[0] == 'exact' or sf[2] >= k else p ** (k - sf[2])
    if nu * ns > 300000: return None
    us, ss = residues_u(p, uf, H), residues_s(p, sf, k)
    return {pow(a, b, P) for a in us for b in ss}

def expected_status(p, uf, sf):
    du = meets_inside(p, uf, F(1), 1)
    ds = meets_inside(p, sf, F(0), 0)
    if du < 0 or ds < 0: return 'DOMAIN'
    if du == 0 or ds == 0: return 'NOT_DETERMINED'
    return 'OK'

def check(p, uf, sf, N, out):
    st, res, line = parse(out)
    for tag in ('ALIASDIFF_U', 'ALIASDIFF_S', 'ALIASDIFF_US'):
        if tag in out: return 'aliasing: ' + out
    es = expected_status(p, uf, sf)
    if es != 'OK':
        return None if (st == es and 'UNTOUCHED' in out) else 'expected %s got %s' % (es, out)
    if st != 'OK': return 'expected OK got ' + out
    if res[0] == 1:
        bump('exact')
        HH = 24
        img = image(p, uf, sf, HH)
        if img is None: HH = 8; img = image(p, uf, sf, HH)
        if img is None: bump('exact_unchecked'); return None
        val = F(p) ** res[1] * res[3]
        if len(img) != 1 or mod_unit(val, p, HH) != next(iter(img)):
            return 'exact result %s but image mod p^24 has %d points' % (val, len(img))
        return None
    ex, v, K, u = res
    if K != N and K > N: return 'exponent above N: ' + out
    if K <= 0:
        bump('K<=0')
        return None if u == 0 else 'K<=0 with a centre: ' + out
    img = image(p, uf, sf, K + 1)
    if img is None: bump('unchecked'); return None
    cen = mod_unit(F(p) ** v * u, p, K) if u != 0 else 0
    for t in img:
        if t % p ** K != cen: return 'enclosure: image point %d mod p^%d not in %s' % (t, K + 1, out)
    if K < N:
        bump('tightcheck')
        if len({t % p ** (K + 1) for t in img}) == 1:
            return 'not tight: the image is one class modulo p^%d, result %s' % (K + 1, out)
        if len({t % p ** (K + 1) for t in img}) != p:
            bump('notfull')
            if STATS['notfull'] <= 6: print('NOTFULL', p, uf, sf, N, out, sorted({t % p ** (K + 1) for t in img}))
    else:
        bump('K=N')
    return None

def gen_u(rng, p):
    r = rng.random()
    if r < 0.05: return ('exact', F(1))
    if r < 0.08: return ('exact', F(-1))
    if r < 0.12: return ('exact', F(rng.randint(2, 40), 1) * F(p) ** rng.randint(-2, 2))
    if r < 0.40:
        a = 1 + p * rng.randint(-60, 60) if rng.random() < 0.8 else rng.randint(-60, 60)
        b = 1 + p * rng.randint(0, 20) if rng.random() < 0.7 else rng.randint(1, 30)
        if b % p == 0: b += 1
        if a == 0: a = 1
        return ('exact', F(a, b))
    if r < 0.45: return ('ball', F(0), rng.randint(-2, 3))
    A = rng.randint(-1, 7 if p <= 3 else 4)
    c = 1 + p * rng.randint(0, p ** 6) if rng.random() < 0.85 else rng.randint(0, p ** 6)
    if p == 2 and rng.random() < 0.5: c = 3 + 4 * rng.randint(0, 2 ** 6)
    return ('ball', F(c), A)

def gen_s(rng, p):
    r = rng.random()
    if r < 0.06: return ('exact', F(0))
    if r < 0.30:
        a = rng.randint(-50, 50); b = rng.randint(1, 12)
        if rng.random() < 0.8:
            while b % p == 0: b += 1
        return ('exact', F(a, b) * F(p) ** rng.randint(0, 3))
    if r < 0.42: return ('ball', F(0), rng.randint(-2, 5))
    B = rng.randint(-1, 6 if p <= 3 else 4)
    c = rng.randint(0, p ** 5) * F(p) ** rng.choice((0, 0, 0, 1, 2, -1))
    return ('ball', F(c), B)

def fields(p, xf):
    return fields_exact(p, xf[1]) if xf[0] == 'exact' else fields_ball(p, xf[1], xf[2])

def main(count, sd, primes=(2, 3, 5, 7, 13)):
    rng = random.Random(sd)
    cases, lines = [], []
    for _ in range(count):
        p = rng.choice(primes)
        uf, sf = gen_u(rng, p), gen_s(rng, p)
        if rng.random() < 0.03: sf = uf
        N = rng.randint(-2, 9 if p <= 3 else 6) if rng.random() < 0.4 else 40
        cases.append((p, uf, sf, N))
        lines.append('W %d %s %s %d' % (p, fields(p, uf), fields(p, sf), N))
    out = Harness().run(lines)
    bad = 0; sts = {}
    for cs, o in zip(cases, out):
        sts[o.split()[0]] = sts.get(o.split()[0], 0) + 1
        r = check(*cs, o)
        if r:
            bad += 1
            if bad <= 25: print('FAIL', cs, '->', r)
    print('cases', len(cases), 'failures', bad, sts, STATS)

if __name__ == '__main__':
    main(int(sys.argv[1]), int(sys.argv[2]))
