"""Attack A1: adf_lball_powrat against the own oracle (statuses, exponent, centre, enclosure, tightness)."""
import sys, random
from oracle import *

LIM = 2 ** 60
STATS = {}

def expected_powrat(p, xf, e, n, seed, N):
    """xf: ('exact', q) or ('ball', centre, M). Returns ('ST',) or ('OK', kind, data) or ('POWSI', e1)."""
    if n == 0: return ('DOMAIN',)
    g = math.gcd(abs(e), n); e1 = e // g; n1 = n // g
    if n1 == 1: return ('POWSI', e1)
    c = cfor(p); s = vp(n1, p)
    if xf[0] == 'exact':
        q = F(xf[1])
        if q == 0:
            if seed != 0: return ('DOMAIN',)
            return ('NOT_UNIT',) if e1 < 0 else ('OKEXACT', F(0))
        m, U = unit_part(q, p)
    else:
        cen, M = F(xf[1]), xf[2]
        if cen == 0 or vq(cen, p) >= M: return ('NOT_DETERMINED',)
        m, U = unit_part(cen, p)
        if M - m < c + s: return ('NOT_DETERMINED',)
    if m % n1: return ('DOMAIN',)
    if p == 2:
        if seed not in (1, 3): return ('DOMAIN',)
    elif not (1 <= seed <= p - 1): return ('DOMAIN',)
    if not branch_exists(p, n1, U, seed): return ('DOMAIN',)
    j = m // n1
    ve = vp(abs(e1), p)
    if xf[0] == 'exact':
        # rational branch?
        a, b = U.numerator, U.denominator
        ra, rb = iroot(abs(a), n1), iroot(b, n1)
        if ra is not None and rb is not None and (a > 0 or n1 % 2 == 1):
            for sg in ((1, -1) if n1 % 2 == 0 else ((1 if a > 0 else -1),)):
                qq = F(sg * ra, rb)
                if mod_unit(qq, p, c) == seed % p ** c:
                    return ('OKEXACT', (F(p) ** j * qq) ** e1)
        K = N
    else:
        Ep = e1 * j + (M - m) - s + ve
        K = min(N, Ep)
    return ('OKBALL', K, j, e1, n1, U, ve)

def point_value(p, n1, e1, j, Ut, seed, K):
    """b^e' for the root b = p^j beta of p^(n1 j) Ut on the branch seed, as (a, w, h)"""
    ve = vp(abs(e1), p); c = cfor(p)
    h = max(K - e1 * j - ve, c) + 1
    beta = nroot(p, n1, Ut, seed, h)
    P = p ** (h + ve)
    w = pow(beta, e1, P)
    return (e1 * j, w, h + ve)

def check(p, xf, e, n, seed, N, out, pow_si_out=None):
    exp = expected_powrat(p, xf, e, n, seed, N)
    st, res, line = parse(out)
    if exp[0] == 'POWSI':
        return None if out.replace(' ALIASDIFF', '') == pow_si_out and 'ALIASDIFF' not in out else \
            'n1=1 differs from pow_si: %s vs %s' % (out, pow_si_out)
    if 'ALIASDIFF' in out: return 'aliasing differs: ' + out
    if exp[0] in ('DOMAIN', 'NOT_DETERMINED', 'NOT_UNIT'):
        if st != exp[0] or 'UNTOUCHED' not in out: return 'expected %s got %s' % (exp[0], out)
        return None
    if exp[0] == 'OKEXACT':
        if st != 'OK' or res[0] != 1: return 'expected exact %s got %s' % (exp[1], out)
        val = F(p) ** res[1] * res[3]
        return None if val == exp[1] else 'exact value %s expected %s' % (val, exp[1])
    _, K, j, e1, n1, U, ve = exp
    if st != 'OK': return 'expected OK K=%d got %s' % (K, out)
    STATS['ballx' if xf[0] == 'ball' else 'exactx'] = STATS.get('ballx' if xf[0] == 'ball' else 'exactx', 0) + 1
    if K <= e1 * j: STATS['zeroball'] = STATS.get('zeroball', 0) + 1
    if res[0] != 0 or res[2] != K: return 'expected ball K=%d got %s' % (K, out)
    # enclosure of the centre's image point, and of random points of the ball
    pts = [U]
    if xf[0] == 'ball':
        M = xf[2]; m = n1 * j
        for _ in range(3):
            w = random.randrange(0, p ** 3)
            pts.append(U + F(p) ** (M - m) * w)
    for Ut in pts:
        val = point_value(p, n1, e1, j, Ut, seed, K)
        if not in_ball(p, val, None, res): return 'point %s (Ut=%s) not in %s' % (val, Ut, out)
    # tightness when the image is claimed (ball, K = E' <= N): two points at distance exactly p^K
    if xf[0] == 'ball' and K > e1 * j:
        M = xf[2]; m = n1 * j
        Ep = e1 * j + (M - m) - vp(n1, p) + ve
        if K == Ep:
            STATS['tight'] = STATS.get('tight', 0) + 1
            P = p ** (K + 1 - e1 * j)
            digs = set()
            for w in range(min(p, 13)):
                vw = point_value(p, n1, e1, j, U + F(p) ** (M - m) * w, seed, K + 1)
                digs.add(vw[1] % P)
            if len(digs) != min(p, 13) or len({d % p ** (K - e1 * j) for d in digs}) != 1:
                return 'image not the full ball of exponent %d: %d classes mod p^(K+1) (%s)' % (K, len(digs), out)
    return None

def gen(rng, p):
    kind = rng.random()
    if kind < 0.08: return ('exact', 0)
    if kind < 0.14: return ('ball', 0, rng.randint(-4, 6))
    m = rng.randint(-6, 6)
    if kind < 0.45:
        a = rng.randint(1, 3000) * rng.choice((1, -1))
        while a % p == 0: a //= p
        b = rng.randint(1, 50)
        while b % p == 0: b = rng.randint(1, 50)
        if rng.random() < 0.3:   # perfect powers
            r = rng.randint(1, 40) * rng.choice((1, -1))
            while r % p == 0: r += 1
            a = r ** rng.choice((2, 3, 4, 6)); b = 1
            if a % p == 0: a += p * 0 + 1
        q = F(a, b) * F(p) ** m
        if a % p == 0: q = F(1) * F(p) ** m
        return ('exact', q)
    r = rng.randint(1, 9)
    U = rng.randint(1, p ** r - 1)
    while U % p == 0: U = rng.randint(1, p ** r - 1)
    if rng.random() < 0.4:   # a perfect power centre
        t = rng.randint(1, p ** r)
        while t % p == 0: t += 1
        U = pow(t, rng.choice((2, 3, 4, 6, 8, 9)), p ** r)
    return ('ball', F(p) ** m * U, m + r)

def xfields(p, xf):
    return fields_exact(p, xf[1]) if xf[0] == 'exact' else fields_ball(p, xf[1], xf[2])

def main(count, seedv):
    rng = random.Random(seedv); random.seed(seedv)
    H = Harness()
    cases = []
    for _ in range(count):
        p = rng.choice((2, 3, 5, 7, 13))
        xf = gen(rng, p)
        e = rng.randint(-9, 9)
        n = rng.choice((1, 2, 2, 3, 4, 6, 8, 9, 12, 5, 7, 10, 13, 25, 26, 27, 16))
        if rng.random() < 0.15: e = e * n   # unreduced multiples
        seed = rng.randint(0, p if p > 2 else 4)
        g0 = math.gcd(abs(e), n); n1 = n // g0
        if rng.random() < 0.8 and n1 >= 2 and not (xf[0] == 'exact' and xf[1] == 0) and not (xf[0] == 'ball' and xf[1] == 0):
            # make the valuation divisible and pick a valid branch when one exists
            cen = F(xf[1]); m, U = unit_part(cen, p)
            m2 = (m // n1) * n1
            if xf[0] == 'exact': xf = ('exact', U * F(p) ** m2)
            else: xf = ('ball', U * F(p) ** m2, xf[2] - m + m2)
            ids = [t for t in ((1, 3) if p == 2 else range(1, p)) if branch_exists(p, n1, U, t)]
            if ids: seed = rng.choice(ids)
        N = rng.randint(-8, 14)
        cases.append((p, xf, e, n, seed, N))
    lines = []
    for (p, xf, e, n, seed, N) in cases:
        f = xfields(p, xf)
        lines.append('P %d %s %d %d %d %d' % (p, f, e, n, seed, N))
        g = math.gcd(abs(e), n) if n else 1
        lines.append('I %d %s %d' % (p, f, e // g if n else 0))
    out = H.run(lines)
    bad = 0; stats = {}
    for i, cs in enumerate(cases):
        o, ps = out[2 * i], out[2 * i + 1]
        if o.startswith('NONCANON'): print('NONCANON', cs); bad += 1; continue
        r = check(*cs, o, ps)
        k = expected_powrat(*cs)[0]; stats[k] = stats.get(k, 0) + 1
        if r:
            bad += 1
            if bad <= 25: print('FAIL', cs, '->', r)
    print('cases', len(cases), 'failures', bad, stats, STATS)

if __name__ == '__main__':
    main(int(sys.argv[1]), int(sys.argv[2]))
