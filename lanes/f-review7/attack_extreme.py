"""Attack A5: extreme words under ASan/UBSan: n near 2^64, e = LONG_MIN/LONG_MAX, N = LONG_MIN/LONG_MAX, seeds near
2^64; statuses against the oracle where the value is small, and no sanitizer report anywhere."""
import sys, random
from oracle import *
import attack_powrat as AP

def main(sd, count):
    rng = random.Random(sd); random.seed(sd)
    big_n = [2**64 - 1, 2**63, 2**63 + 1, 2**62, 3**40, 2**64 - 59, 2**32 + 15]
    big_e = [-2**63, 2**63 - 1, -(2**63 - 1), 2**62, -2**62, 3**39]
    Ns = [-2**63, 2**63 - 1, -2**60, 2**60, 0, 5, 40]
    seeds = [0, 1, 2, 3, 2**64 - 1, 2**63, 2**64 - 60]
    lines, cases = [], []
    for _ in range(count):
        p = rng.choice((2, 3, 5, 7, 2**64 - 59))
        xf = AP.gen(rng, p) if p < 100 else ('exact', F(rng.randrange(1, 10**30)))
        r = rng.random()
        if r < 0.4: n = rng.choice(big_n); e = rng.randint(-5, 5)
        elif r < 0.7: n = rng.choice((1, 2, 3, 4, 6)); e = rng.choice(big_e)
        else: n = rng.choice(big_n); e = rng.choice(big_e)
        seed = rng.choice(seeds + [rng.randrange(0, 8)])
        N = rng.choice(Ns)
        lines.append('P %d %s %d %d %d %d' % (p, AP.xfields(p, xf), e, n, seed, N))
        cases.append((p, xf, e, n, seed, N))
        if rng.random() < 0.3:
            lines.append('A %d %s %d %d %d' % (p, AP.xfields(p, xf), n, N, 64)); cases.append(None)
    if os.environ.get('DUMP'): open(os.environ['DUMP'], 'w').write('\n'.join(lines) + '\n'); return
    out = Harness(os.path.join(HERE, 'hsan')).run(lines, timeout=110)
    bad = 0; checked = 0; sts = {}
    for cs, o in zip(cases, out):
        sts[o.split()[0]] = sts.get(o.split()[0], 0) + 1
        if 'TOUCHED' in o.replace('UNTOUCHED', '') or 'ALIASDIFF' in o:
            bad += 1; print('FAIL touched/alias', cs, o[:200]); continue
        if cs is None: continue
        p, xf, e, n, seed, N = cs
        g = math.gcd(abs(e), n); e1 = e // g; n1 = n // g
        # value checks only where the result is small: |e1| <= 5, |N| <= 40
        if abs(e1) <= 5 and abs(N) <= 40 and n1 >= 2:
            exp = AP.expected_powrat(p, xf, e, n, seed, N)
            r = AP.check(p, xf, e, n, seed, N, o, None) if exp[0] != 'POWSI' else None
            checked += 1
            if r: bad += 1; print('FAIL', cs, r[:200])
    print('lines', len(lines), 'value-checked', checked, 'failures', bad, sts)

if __name__ == '__main__':
    main(int(sys.argv[1]), int(sys.argv[2]))
