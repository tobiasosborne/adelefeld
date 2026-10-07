#!/usr/bin/env python3
"""Lane m3-review1, hunt 3: the union reader. Own printer of union texts (conventions 9.2 grammar qclass_v,
9.4 templates); exact intervals by conventions 9.5 Reading. Checks: OK; canonical; every text piece enclosed
by a stored piece with the same canonical finite (A mod H, H); stored len <= distinct text pieces; max_items at
the count (OK) and one below (LIMIT, untouched); get_str rereads to an enclosing class."""
import random
import sys
from fractions import Fraction as F
from common import run

seed = int(sys.argv[1]) if len(sys.argv) > 1 else 1
NC = int(sys.argv[2]) if len(sys.argv) > 2 else 1000
EXE = sys.argv[3] if len(sys.argv) > 3 else 'h'
rng = random.Random(seed)


def rdec():
    t = rng.random()
    if t < 0.15:
        return rng.choice(['0', '1', '0.5', '0.25', '1.0', '0.000'])
    if t < 0.5:
        k = rng.randint(1, 30)
        v = rng.randint(0, 10 ** k)
        s = str(v).rjust(k, '0')
        return '0.' + s
    if t < 0.7:
        return '%de-%d' % (rng.randint(1, 9), rng.randint(1, 40))
    if t < 0.85:
        return '%d.%de-%d' % (rng.randint(1, 9), rng.randint(0, 999), rng.randint(1, 5))
    return '0.' + '3' * rng.randint(1, 60)


def rrad():
    t = rng.random()
    if t < 0.3:
        return None
    if t < 0.6:
        return '%de-%d' % (rng.randint(1, 9), rng.randint(0, 30))
    if t < 0.8:
        return rng.choice(['0.05', '0.5', '1', '2.5', '0.125', '0.1', '0'])
    return '0.' + str(rng.randint(1, 10 ** 20))


def rfin():
    H = rng.choice([0, 0, 1, 2, 3, 6, 12, 360, 7])
    a = rng.randint(-1000, 1000) if rng.random() < 0.9 else rng.randint(-2 ** 300, 2 ** 300)
    if H == 0:
        return str(a), a, 0
    return '%d mod %d' % (a, H), a % H, H


def piece():
    m, r = rdec(), rrad()
    while F(m) > 1:
        m = rdec()
    ftxt, A, H = rfin()
    real = m if r is None else '%s +/- %s' % (m, r)
    lo = F(m) - (F(r) if r else 0)
    hi = F(m) + (F(r) if r else 0)
    return '(%s ; %s)' % (real, ftxt), (lo, hi, A, H)


def main():
    lines, cases = [], []
    for _ in range(NC):
        n = rng.randint(1, 6)
        ps = [piece() for _ in range(n)]
        if rng.random() < 0.3:
            ps.append(rng.choice(ps))          # an exact duplicate
        rng.shuffle(ps)
        txt = 'union(' + ', '.join(p[0] for p in ps) + ') + Q'
        prec = rng.choice([2, 20, 53, 128])
        cnt = len(ps)
        lines.append('U %d %d 10 %s' % (prec, cnt, txt))
        lines.append('U %d %d 10 %s' % (prec, cnt - 1, txt))
        cases.append((txt, ps, prec))
    out, _ = run(lines, EXE)
    i = 0
    bad = 0
    nok = 0
    reread = []
    for txt, ps, prec in cases:
        r = out[i].split(); i += 1
        st, canon, form, ln = [int(v) for v in r[1:5]]
        S = []
        if st == 0:
            for _ in range(ln):
                S.append(out[i].split()); i += 1
            T = out[i]; i += 1
        r2 = out[i].split(); i += 1
        if r2[1] != '10' or r2[2] != '1':
            if len(ps) > 1 or r2[1] != '0':
                print('FINDING max_items-1', r2, txt); bad += 1
        if st != 0:
            print('FINDING status', st, txt); bad += 1
            continue
        nok += 1
        if not canon or form != 1:
            print('FINDING canon', canon, form, txt); bad += 1
        stored = []
        for s in S:
            mn, md, rn, rd, A, H, d = [int(v) for v in s[1:8]]
            mid, rad = F(mn, md), F(rn, rd)
            stored.append((mid - rad, mid + rad, A, H, d, mid))
            if not (0 <= mid <= 1) or d != 1:
                print('FINDING stored midpoint/d', s, txt); bad += 1
        distinct = {p[1] for p in ps}
        if ln > len(distinct):
            print('FINDING len', ln, len(distinct), txt); bad += 1
        for (_, (lo, hi, A, H)) in ps:
            if not any(sl <= lo and hi <= sh and sA == A and sH == H for (sl, sh, sA, sH, d, m) in stored):
                print('FINDING enclosure', (lo, hi, A, H), txt, S); bad += 1
                break
        if T[2:] != 'NULL':
            reread.append((T[2:], stored))
    # get_str text reread: each stored piece enclosed by a piece of the reread class
    lines = ['U 128 1000 10 %s' % t for t, _ in reread]
    out, _ = run(lines, EXE)
    i = 0
    for t, stored in reread:
        r = out[i].split(); i += 1
        if r[1] != '0':
            print('FINDING reread status', r, t); bad += 1
            continue
        S2 = []
        for _ in range(int(r[4])):
            s = out[i].split(); i += 1
            mn, md, rn, rd, A, H, d = [int(v) for v in s[1:8]]
            S2.append((F(mn, md) - F(rn, rd), F(mn, md) + F(rn, rd), A, H))
        i += 1
        for (lo, hi, A, H, d, m) in stored:
            if not any(a <= lo and hi <= b and A == A2 and H == H2 for (a, b, A2, H2) in S2):
                print('FINDING get_str does not enclose', (lo, hi, A, H), t); bad += 1
                break
    print('texts', len(cases), 'ok', nok, 'findings', bad)


main()
