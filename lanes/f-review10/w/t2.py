import random, sys, re
sys.path.insert(0, "lanes/f-review10/w")
from o1 import *

random.seed(int(sys.argv[1]) if len(sys.argv) > 1 else 1)
count = int(sys.argv[2]) if len(sys.argv) > 2 else 100
PR = [2, 3, 5, 7, 11, 13]
bad = 0; checked = 0
for _ in range(count):
    ps = random.sample(PR, random.randint(0, 4))
    ps2 = list(ps); random.shuffle(ps2)
    Mf = random.choice([0, 0, 1, 2, 4, 8, 9, 27, 25, 36, 72, 45, 100, 7 * 9, 49 * 4, 3 ** 4 * 2, 5 * 7 * 2, 2 * 3 * 11])
    N = random.choice([-1, 0, 1, 2, 3, 4, 5])
    rn = random.choice([1, 2, 3, 4, 9, 5, 7, 12, 25, 11])
    rd = random.choice([1, 1, 2, 3, 9, 5])
    rs = "%d/%d" % (rn, rd) if rd != 1 else str(rn)
    if Mf == 0:
        u = random.choice(["[1]", "[-1]"]); c = None
    else:
        while True:
            c = random.randrange(1, Mf + 1)
            if gcd(c, Mf) == 1: break
        u = "[%d mod %d]" % (c, Mf)
    idl = "(1 ; %s * %s)" % (rs, u)
    lines = ["REF %d 20 %s %s" % (N, ",".join(map(str, ps2)) or "-", idl)]
    # per-prime single places for ps and 2 and others
    for p in set(ps) | {2, 3, 5, 7, 11, 13}:
        lines.append("AT %d %d 20 %s" % (p, N, idl))
    out, rc = run(lines)
    mm = re.match(r"st=(\d+)(?: A=(\d+) H=(\d+))?", out[0])
    if int(mm.group(1)) != 0:
        print("STATUS", out[0], lines[0]); bad += 1; continue
    A = int(mm.group(2)); R = int(mm.group(3))
    per = {}
    for l, o in zip(lines[1:], out[1:]):
        p = int(l.split()[1])
        m2 = re.match(r"st=0(?: scanon=\d)? c=(\S+) N=(-?\d+) exact=(\d)", o)
        per[p] = (Fraction(m2.group(1)), int(m2.group(2)), int(m2.group(3)))
    expR = 1
    # baseline
    ex2 = 2
    for p in ps:
        c_, K, ex = per[p]
        if ex: K = N
        if p == 2: ex2 = max(2, K)
        elif K > 0: expR *= p ** K
    expR *= 2 ** ex2
    ok = True
    if R != expR: ok = False; print("R", R, expR, lines[0])
    if A % 4: ok = False; print("A not 0 mod 4", A, lines[0])
    for p in ps:
        c_, K, ex = per[p]
        if ex: c_ = 0; K = N
        if K > 0 or (p == 2 and K > 0):
            if (A - int(c_)) % p ** K: ok = False; print("A vs centre at", p, A, c_, K, lines[0])
    if not (0 <= A < R): ok = False; print("A out of range", A, R, lines[0])
    if not ok: bad += 1
    checked += 1
print("checked", checked, "bad", bad)
