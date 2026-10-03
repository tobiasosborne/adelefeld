import random, sys, re
sys.path.insert(0, "lanes/f-review10/w")
from o1 import *

random.seed(int(sys.argv[1]) if len(sys.argv) > 1 else 1)
count = int(sys.argv[2]) if len(sys.argv) > 2 else 300
cases = []; meta = []
for _ in range(count):
    p = random.choice([2, 3, 5, 7])
    H = 4 if p == 7 else 5
    k = random.choice([0, 0, 1, 1, 2, 2, 3, 4]) if p != 7 else random.choice([0, 1, 2, 3])
    other = random.choice([1, 1, 3, 5, 7, 9, 11, 13, 15, 25])
    other = other if other % p else 1
    exactM = random.random() < 0.15
    M = 0 if exactM else p ** k * other
    if M == 1 and random.random() < 0.5: pass
    # content
    pm = random.choice([-3, -1, 0, 0, 1, 2, 4])
    numf = random.choice([1, 1, 2, 3, 5, 7, 11, 13, 4, 9, 15, 21, 22, 25, 49])
    denf = random.choice([1, 1, 1, 2, 3, 5, 7, 13, 6, 10])
    rn, rd = numf, denf
    if pm > 0: rn *= p ** pm
    if pm < 0: rd *= p ** (-pm)
    rn, rd = Fraction(rn, rd).numerator, Fraction(rn, rd).denominator
    if M == 0:
        c = random.choice([1, -1]); ustr = "[%d]" % c; cval = c
    else:
        while True:
            c = random.randrange(1, M + 1) if M > 1 else 1
            if gcd(c, M) == 1 and (c % 2 or M % 2 == 0 or True): break
        if M == 1:
            ustr = random.choice(["[1 mod 1]", "[1]"]) if False else "[1 mod 1]"
        else:
            ustr = "[%d mod %d]" % (c, M)
        cval = c
    E = max(k, 2 if p == 2 else 1)
    N = random.choice([-2, 0, 1, 2, 3, 4, 5, 6, 7])
    rs = str(rn) if rd == 1 else "%d/%d" % (rn, rd)
    s = "AT %d %d 20 (1 ; %s * %s)" % (p, N, rs, ustr)
    cases.append(s); meta.append((p, H, k, M, rn, rd, cval, N, E, s))
out, rc = run(cases)
assert len(out) == len(cases), (len(out), rc)
bad = 0; checked = 0; exactz = 0
for o, (p, H, k, M, rn, rd, c, N, E, s) in zip(out, meta):
    m = re.match(r"st=(\d+)(?: scanon=\d)?(?: c=(\S+) N=(-?\d+) exact=(\d))?", o)
    st = int(m.group(1))
    if st != 0:
        print("STATUS", o, s); bad += 1; continue
    if m.group(4) == "1":
        exactz += 1
        S = local_image(rn, rd, M, c, p, H)
        if S != {0}: print("EXACT ZERO WRONG", o, s, S); bad += 1
        continue
    cen = Fraction(m.group(2)); K = int(m.group(3))
    S = local_image(rn, rd, M, c, p, H)
    assert cen.denominator == 1
    cen = int(cen)
    kk = min(K, H)
    ok = all((x - cen) % p ** kk == 0 for x in S) if kk > 0 else True
    if not ok:
        print("BLOCKER not contained", o, s, sorted(S)[:5]); bad += 1; continue
    if M != 0:
        _, j = smallest_ball(S, p, H)
        expK = min(N, j)
        if j >= H and k < H: print("note j>=H", s)
        if K != expK:
            print("TIGHTNESS", o, s, "true j", j, "expect K", expK); bad += 1
    else:
        if K != N and N <= H: print("EXACT-N", o, s); bad += 1
    checked += 1
print("cases", len(cases), "checked", checked, "exactzero", exactz, "bad", bad)
