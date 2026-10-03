import sys, random, re
sys.path.insert(0, "lanes/f-review10/w")
from o1 import *
random.seed(11)
primes = [1000003, 2305843009213693951, 18446744073709551557, 9223372036854775783, 101, 65537, 4294967291]
cases = []; meta = []
for _ in range(120):
    p = random.choice(primes); k = random.choice([1, 2, 3]); N = random.choice([1, 2, 3, 4, 5])
    other = random.choice([1, 3, 7, 10])
    M = p ** k * other
    c = random.randrange(2, M)
    while gcd(c, M) != 1: c = random.randrange(2, M)
    m = random.choice([-2, 0, 1, 3])
    rn, rd = random.choice([1, 2, 5, 12, 99]), random.choice([1, 3, 7])
    if m > 0: rn *= p ** m
    if m < 0: rd *= p ** (-m)
    g = gcd(rn, rd); rn //= g; rd //= g
    rs = str(rn) if rd == 1 else "%d/%d" % (rn, rd)
    cases.append("AT %d %d 5 (1 ; %s * [%d mod %d])" % (p, N, rs, c, M)); meta.append((p, k, N, rn, rd, c))
out, rc = run(cases)
bad = 0
for o, (p, k, N, rn, rd, c) in zip(out, meta):
    mm = re.match(r"st=(\d+) c=(\S+) N=(-?\d+) exact=(\d)", o)
    if not mm: print("OUT", o); bad += 1; continue
    K = min(N, k); H = K
    assert int(mm.group(3)) == K, (o, K)
    m = vp(rn, p) - vp(rd, p)
    ru = Fraction(rn, rd) / Fraction(p) ** m
    mod = p ** H
    x = (ru.numerator * c * pow(ru.denominator, -1, mod)) % mod
    want = logp(x, p, H)
    got = int(mm.group(2))
    if (got - want) % p ** H: print("MISMATCH", o, want, p, k, N); bad += 1
print("cases", len(cases), "bad", bad)
