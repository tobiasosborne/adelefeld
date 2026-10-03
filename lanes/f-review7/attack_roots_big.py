"""Attack B2: roots at p = 65537 and p = 2^64 - 59: every listed identifier t has t^n = U mod p, the list is sorted,
distinct, of length gcd(n, p - 1) (so complete); each ball contains the certified root (sampled)."""
import sys, random, time
from oracle import *
from attack_roots import parse_roots

def run(p, n, U, N, rng, H, cap=2000000, sample=20):
    s = vp(n, p); d = math.gcd(n, p - 1)
    line = 'A %d %s %d %d %d' % (p, fields_exact(p, U), n, N, cap)
    t0 = time.time(); o = H.run([line], timeout=110)[0]; dt = time.time() - t0
    st, ln, br, touched = parse_roots(o)
    ok = branch_exists(p, n, U, mod_unit(U, p, 1)) if False else None
    exists = any(True for _ in [0])  # existence is checked through the listed roots below
    if st != 'OK':
        return 'p=%d n=%d d=%d status %s (%.2fs)' % (p, n, d, o[:80], dt), dt
    ids = [b[0] for b in br]
    Um = mod_unit(U, p, 1)
    if ln != d or len(ids) != d: return 'count %d expected %d' % (ln, d), dt
    if ids != sorted(ids) or len(set(ids)) != d: return 'not sorted/distinct', dt
    badt = [t for t in ids if pow(t, n, p) != Um]
    if badt: return 'non-roots listed: %s' % badt[:5], dt
    for t, res in rng.sample(br, min(sample, len(br))):
        if res[0] == 1: continue
        K = res[2]
        beta = nroot(p, n, U, t, K + 1)
        if not in_ball(p, (0, beta, K + 1), None, res): return 'enclosure t=%d' % t, dt
    return None, dt

def main(sd):
    rng = random.Random(sd); H = Harness()
    P1, P2 = 65537, 2 ** 64 - 59
    jobs = []
    for d in (2, 4, 16, 256, 65536):
        for k in (1, 3):
            jobs.append((P1, d * k if d * k <= 65536 * 3 else d))
    for d in (2, 4, 11, 22, 44, 137, 547, 6028, 24068, 149878, 299756, 824329):
        jobs.append((P2, d))
        jobs.append((P2, d * 3 if d * 3 < 2**40 else d))
    bad = 0
    for (p, n) in jobs:
        for rep in range(2):
            t = rng.randrange(2, p)
            U = F(pow(t, n, p) + p * rng.randrange(1, 1000)) if rep == 0 else F(pow(t, n, p ** 3), 1 + p * rng.randrange(1, 9))
            r, dt = run(p, n, U, 3, rng, H)
            print('p=%d n=%d d=%d rep=%d %.2fs %s' % (p, n, math.gcd(n, p - 1), rep, dt, r or 'ok'))
            if r: bad += 1
    print('failures', bad)

if __name__ == '__main__':
    main(int(sys.argv[1]))
