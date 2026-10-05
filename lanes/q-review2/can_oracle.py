#!/usr/bin/env python3
"""lanes/q-review2/can_oracle.py: independent verdict of the adf_qclass storage invariant
(conventions 5.10 with CV-45, docs/api-3.md section 1), compared with adf_qclass_is_canonical.

Exact rationals are used throughout: dyadics are (mantissa, exponent) pairs and are compared
without materialising huge powers of two. A piece description is a dict; contexts match the
table of can_eval.c.

Usage: timeout 170 python3 can_oracle.py <seed> <ncases>
Prints one line per disagreement and a summary. Exit 1 on any disagreement.
"""
import functools
import os
import random
import subprocess
import sys
from math import gcd

CTX = [((2, 3), 6), ((5, 7), 35), ((3, 5, 7), 105), ((243, 2 ** 32), 243 * 2 ** 32)]
BIGEXP = 2 ** 60
ALIGN_MAX = 1 << 16           # refuse to align dyadics further apart than this


class Bad(Exception):
    pass


# ---------------- dyadics ----------------

def dnorm(v):
    m, e = v
    if m == 0:
        return (0, 0)
    while m % 2 == 0:
        m //= 2
        e += 1
    return (m, e)


def dcmp(x, y):
    """-1, 0, 1 for x < y: exact, without building huge integers."""
    m1, e1 = x
    m2, e2 = y
    if m1 == 0 and m2 == 0:
        return 0
    if m1 == 0:
        return -1 if m2 > 0 else 1
    if m2 == 0:
        return 1 if m1 > 0 else -1
    if (m1 > 0) != (m2 > 0):
        return 1 if m1 > 0 else -1
    a, b = abs(m1), abs(m2)
    l1, l2 = a.bit_length() + e1, b.bit_length() + e2
    if l1 != l2:
        r = 1 if l1 > l2 else -1
        return r if m1 > 0 else -r
    if e1 <= e2:
        c = a - (b << (e2 - e1))
    else:
        c = (a << (e1 - e2)) - b
    if c == 0:
        return 0
    r = 1 if c > 0 else -1
    return r if m1 > 0 else -r


def dadd(x, y):
    m1, e1 = x
    m2, e2 = y
    if m1 == 0:
        return dnorm((m2, e2))
    if m2 == 0:
        return dnorm((m1, e1))
    e = min(e1, e2)
    if abs(e1 - e) > ALIGN_MAX or abs(e2 - e) > ALIGN_MAX:
        raise Bad("dyadic alignment too wide")
    return dnorm(((m1 << (e1 - e)) + (m2 << (e2 - e)), e))


ZERO = (0, 0)
ONE = (1, 0)


# ---------------- the invariant ----------------

def adele_canonical(p):
    if p['mid'][0] == 'inf':
        return None
    return fin_canonical(p)


def fin_predicate_ok(p):
    """Predicate G or L of conventions 5.2 / 5.3 on the raw stored fields."""
    t, A, H, d, ctx, res = p['ftype'], p['A'], p['H'], p['d'], p['ctx'], p['res']
    if t in (0, 2, 5):
        if t != 0:
            return False                        # mctx != NULL or res != NULL
        if d <= 0 or H < 0:
            return False
        if H == 0:
            return gcd(abs(A), d) == 1
        return 0 <= A < H and gcd(gcd(abs(A), abs(H)), d) == 1
    if t in (1, 4):
        if t != 1 or ctx is None or ctx < 0:
            return False
        q, K = CTX[ctx]
        if not q or A != 0 or H != K or d < 1:
            return False
        return len(res) >= len(q) and all(0 <= r < qq for r, qq in zip(res, q))
    return False                                # backend tag 2


def fin_canonical(p):
    if not fin_predicate_ok(p):
        return None
    if p['ftype'] in (0, 2, 5):
        return (p['A'], p['H'], p['d'])
    q, K = CTX[p['ctx']]
    A0 = 0
    for r, qq in zip(p['res'], q):
        A0 += r * (K // qq) * pow(K // qq, -1, qq)
    A0 %= K
    g = gcd(gcd(A0, K), p['d'])
    return (A0 // g, K // g, p['d'] // g)


def piece_key(p):
    """(lo, hi, H, A) with exact dyadic endpoints: conventions 5.10 order."""
    tri = fin_canonical(p)
    if tri is None:
        raise Bad("key of a non-canonical piece")
    lo = dadd(p['mid'], (-p['rad'][0], p['rad'][1]))
    hi = dadd(p['mid'], (p['rad'][0], p['rad'][1]))
    return (lo, hi, tri[1], tri[0])


def keycmp(a, b):
    ka, kb = piece_key(a), piece_key(b)
    for i in range(4):
        c = dcmp(ka[i], kb[i]) if i < 2 else (-1 if ka[i] < kb[i] else (1 if ka[i] > kb[i] else 0))
        if c:
            return c
    return 0


def verdict(form, length, nullflag, pieces):
    """1 exactly when every clause of docs/api-3.md section 1 holds."""
    if form not in (0, 1):
        return 0, "tag"
    if length < 1:
        return 0, "len"
    if nullflag:
        return 0, "null array"
    if form == 0:
        if length != 1:
            return 0, "lift len != 1"
        if not len(pieces):
            return 0, "lift member missing"
        if adele_canonical(pieces[0]) is None:
            return 0, "lift member not canonical"
        return 1, ""
    keys = []
    for p in pieces[:length]:
        tri = adele_canonical(p)
        if tri is None:
            return 0, "member not canonical"
        if dcmp(p['mid'], ZERO) < 0 or dcmp(p['mid'], ONE) > 0:
            return 0, "midpoint outside [0,1]"
        if tri[2] != 1:
            return 0, "d != 1"
        if tri[1] < 0:
            return 0, "H < 0"
        if tri[1] > 0 and not (0 <= tri[0] < tri[1]):
            return 0, "A outside [0,H)"
        keys.append(keycmp(p, p))          # placeholder, replaced below
    keys = [piece_key(p) for p in pieces[:length]]
    for i in range(1, len(keys)):
        a, b = keys[i - 1], keys[i]
        c = 0
        for j in range(4):
            c = dcmp(a[j], b[j]) if j < 2 else (-1 if a[j] < b[j] else (1 if a[j] > b[j] else 0))
            if c:
                break
        if c >= 0:
            return 0, "order"
    return 1, ""


# ---------------- generation ----------------

def dy(rng, expo_lo=-8, expo_hi=0, bits=6):
    """A dyadic with an odd mantissa of at most bits bits, exponent in the given range."""
    m = rng.randrange(1, 2 ** bits) | 1
    if rng.random() < 0.15:
        m = -m
    return (m, rng.randrange(expo_lo, expo_hi + 1))


def real_ball(rng):
    m = rng.randrange(0, 2 ** 12)
    mid = (m, -rng.randrange(0, 10))
    rad = (rng.randrange(0, 2 ** 8) | 1, -rng.randrange(0, 8)) if rng.random() < 0.6 else (0, 0)
    return mid, rad


def mid_in_unit(rng):
    """A dyadic midpoint in [0,1], sometimes exactly 0 or 1."""
    r = rng.random()
    if r < 0.08:
        return ZERO
    if r < 0.16:
        return ONE
    if r < 0.24:
        return (1, -rng.randrange(1, 70))                    # 2^-k
    if r < 0.32:
        return (2 ** rng.randrange(0, 40) * 2 - 1, -rng.randrange(1, 40))
    num = rng.randrange(0, 2 ** rng.randrange(1, 30))
    e = rng.randrange(0, 30)
    v = dnorm((num, -e))
    while dcmp(v, ONE) > 0:
        v = dnorm((v[0], v[1] - 1))
    return v


def fin_ball(rng):
    """A canonical finite ball of a random backend."""
    t = rng.random()
    if t < 0.22:                                            # global exact integer
        return dict(ftype=0, A=rng.randrange(-20, 20), H=0, d=1, ctx=-1, res=[])
    if t < 0.32:                                            # global exact rational
        d = rng.randrange(2, 8)
        a = rng.randrange(-20, 20) * d + rng.choice([1, -1])
        g = gcd(abs(a), d)
        return dict(ftype=0, A=a // g, H=0, d=d // g, ctx=-1, res=[])
    if t < 0.82:                                            # global with radius, d = 1
        H = rng.randrange(1, 30)
        return dict(ftype=0, A=rng.randrange(0, H), H=H, d=1, ctx=-1, res=[])
    if t < 0.87:                                            # global with radius, d > 1
        d = rng.randrange(2, 5)
        H = rng.randrange(1, 12) * d
        A = rng.randrange(0, H)
        g = gcd(gcd(A, H), d)
        return dict(ftype=0, A=A // g, H=H // g, d=d // g, ctx=-1, res=[])
    ci = rng.randrange(0, 4)                                # local, d = 1
    q, K = CTX[ci]
    return dict(ftype=1, A=0, H=K, d=1, ctx=ci, res=[rng.randrange(0, qq) for qq in q])


def piece(rng, mid=None, fin=None):
    mid = mid_in_unit(rng) if mid is None else mid
    rad = (rng.randrange(0, 2 ** 6) | 1, -rng.randrange(0, 5)) if rng.random() < 0.5 else (0, 0)
    return dict(mid=mid, rad=rad, **fin_ball(rng) if fin is None else fin)


def emit(p):
    mid, rad = p['mid'], p['rad']
    res = ' '.join(str(r) for r in p['res']) if p['res'] else '0'
    return "PIECE %d %d %d %d %d %d %d %d %d %s" % (
        mid[0], mid[1], rad[0], rad[1], p['ftype'], p['A'], p['H'], p['d'], p['ctx'], res)


def value(form, length, nullflag, pieces):
    out = ["EVAL %d %d %d %d" % (form, length, nullflag, len(pieces))]
    out += [emit(p) for p in pieces]
    out.append("END")
    return out


# ---------------- the run ----------------

def run(seed, ncases):
    rng = random.Random(seed)
    cases = []
    while len(cases) < ncases:
        c = make_case(rng)
        if c is not None:
            cases.append(c)
    text = '\n'.join('\n'.join(value(*c)) for c in cases) + '\n'
    exe = os.environ.get('CAN_EVAL', './lanes/q-review2/can_eval')
    proc = subprocess.run([exe], input=text, capture_output=True,
                          text=True, timeout=160)
    if proc.returncode != 0:
        print("can_eval exit", proc.returncode, proc.stderr[:400])
        return 1
    out = proc.stdout.split('\n')
    bad = 0
    counts = {}
    for i, c in enumerate(cases):
        form, length, nullflag, pieces = c
        want, why = verdict(form, length, nullflag, pieces)
        counts[(want, why)] = counts.get((want, why), 0) + 1
        tok = out[i].split()
        got = int(tok[1])
        DEFAULT = dict(mid=ZERO, rad=(0, 0), ftype=0, A=0, H=0, d=1, ctx=-1, res=[])
        stored = list(pieces) + [DEFAULT] * max(0, length - len(pieces))
        # cross-check the canonical triples read back from C
        j = 3
        for p in stored[:length]:
            flag = int(tok[j]); j += 1
            mine = fin_canonical(p)
            if flag == 1:
                if mine is None:
                    print("TRIPLE MISMATCH case %d: C canonical, oracle not" % i)
                    bad += 1
                    continue
                got3 = (int(tok[j]), int(tok[j + 1]), int(tok[j + 2])); j += 3
                if got3 != mine:
                    print("TRIPLE MISMATCH case %d: C %s oracle %s" % (i, got3, mine))
                    bad += 1
            elif mine is not None:
                print("TRIPLE MISMATCH case %d: C not canonical, oracle %s" % (i, mine))
                bad += 1
        if j != len(tok):
            print("PARSE MISMATCH case %d: %d tokens left" % (i, len(tok) - j))
            bad += 1
        if got != want:
            bad += 1
            if bad <= 20:
                print("MISMATCH case %d: C %d, oracle %d (%s)" % (i, got, want, why))
                print('\n'.join(value(*c)))
    print("cases %d  mismatches %d" % (len(cases), bad))
    for k in sorted(counts, key=lambda z: -counts[z])[:20]:
        print("   oracle verdict %d (%s): %d" % (k[0], k[1], counts[k]))
    return 1 if bad else 0


def make_case(rng):
    r = rng.random()
    n = rng.randrange(1, 13)
    if r < 0.45:                                             # random list, sorted or not
        ps = [piece(rng) for _ in range(n)]
        if rng.random() < 0.5:
            ps.sort(key=functools.cmp_to_key(keycmp))
            keys = [piece_key(p) for p in ps]
            ps = dedup(ps)
        return (1, len(ps), 0, ps)
    if r < 0.50:                                             # a lift
        return (0, 1, 0, [piece(rng)])
    if r < 0.53:                                             # bad tag
        return (rng.choice([2, 3, -1, 1000]), 1, 0, [piece(rng)])
    if r < 0.57:                                             # len 0 or negative
        return (rng.choice([0, 1]), rng.choice([0, -1, -2, -100]), 0, [piece(rng)])
    if r < 0.60:                                             # lift with len 2
        return (0, 2, 0, [piece(rng), piece(rng)])
    if r < 0.63:                                             # NULL array
        return (1, rng.randrange(1, 4), 1, [piece(rng)])
    if r < 0.72:                                             # the midpoint clause
        kind = rng.randrange(0, 9)
        if kind == 0:
            mid = (-1, -60)
        elif kind == 1:
            mid = (3, -60)                                  # 1 + 2^-60
        elif kind == 2:
            mid = ZERO
        elif kind == 3:
            mid = ONE
        elif kind == 4:
            mid = (2 ** 60 - 1, -60)                        # 1 - 2^-60
        elif kind == 5:
            mid = (1, -BIGEXP)                              # 1 - 2^-2^60
        elif kind == 6:
            mid = (1, -BIGEXP + 1)                          # 1 + 2^-2^60
        elif kind == 7:
            mid = (-1, -BIGEXP)
        else:
            mid = (1, BIGEXP)
        rad = (0, 0) if kind in (2, 3) else (rng.randrange(1, 4), -rng.randrange(0, 3))
        if kind in (5, 6, 7, 8):
            rad = (rng.randrange(1, 4), -BIGEXP + rng.randrange(0, 3))
        p = piece(rng, mid=mid)
        p['rad'] = rad
        return (1, 1, 0, [p])
    if r < 0.80:                                             # finite part faults
        ps = [piece(rng) for _ in range(rng.randrange(1, 4))]
        ps.sort(key=functools.cmp_to_key(keycmp))
        ps = dedup(ps)
        victim = ps[rng.randrange(0, len(ps))]
        q, K = CTX[1]
        kind = rng.randrange(0, 12)
        if kind == 0:
            victim.update(ftype=0, A=3, H=0, d=0, ctx=-1, res=[])          # d = 0
        elif kind == 1:
            victim.update(ftype=0, A=3, H=-5, d=1, ctx=-1, res=[])         # H < 0
        elif kind == 2:
            victim.update(ftype=0, A=7, H=5, d=1, ctx=-1, res=[])          # A >= H
        elif kind == 3:
            victim.update(ftype=0, A=-2, H=5, d=1, ctx=-1, res=[])         # A < 0
        elif kind == 4:
            victim.update(ftype=0, A=2, H=4, d=2, ctx=-1, res=[])          # gcd > 1
        elif kind == 5:
            victim.update(ftype=2, ctx=0)                                   # global with mctx
        elif kind == 6:
            victim.update(ftype=5, ctx=0)                                   # global with res
        elif kind == 7:
            victim.update(ftype=3)                                          # backend tag 2
        elif kind == 8:
            victim.update(ftype=1, ctx=1, H=K, d=1, A=0,
                          res=[q[i] for i in range(len(q))])                # res == q
        elif kind == 9:
            victim.update(ftype=1, ctx=1, H=K, d=1, A=0,
                          res=[q[i] + 1 for i in range(len(q))])
        elif kind == 10:
            victim.update(ftype=4, ctx=-1, H=K, d=1, A=0, res=[0] * len(q))  # local, no ctx
        else:
            victim.update(ftype=1, ctx=1, H=K + 1, d=1, A=0, res=[0] * len(q))
        return (1, len(ps), 0, ps)
    if r < 0.90:                                             # order and duplicates
        base = []
        for _ in range(rng.randrange(2, 5)):
            lo = mid_in_unit(rng)
            base.append((lo, lo))
        ps = [piece(rng, mid=lo) for lo, _ in base]
        kind = rng.randrange(0, 7)
        if kind == 0:                                        # equal pieces
            twin = dict(ps[0])
            twin['rad'] = (0, 0) if ps[0]['rad'] == (0, 0) else ps[0]['rad']
            ps.append(dict(twin))
            ps.sort(key=functools.cmp_to_key(keycmp))
            return (1, len(ps), 0, ps)
        if kind == 1:                                        # equal keys, other backend
            g = dict(ftype=0, A=1, H=6, d=1, ctx=-1, res=[])
            loc = dict(ftype=1, A=0, H=6, d=1, ctx=0, res=[1, 1])
            ps = [dict(ps[0], **g), dict(ps[0], **loc)]
            ps.sort(key=functools.cmp_to_key(keycmp))
            return (1, 2, 0, ps)
        # two pieces differing in one key position only
        same_real = rng.random() < 0.5
        p1 = piece(rng)
        p2 = dict(p1)
        if same_real:
            key = rng.randrange(0, 4)
            if key <= 1:                                     # differ in H or A
                f2 = fin_ball(rng)
                if key == 2:
                    f2 = dict(ftype=0, A=p1['A'], H=p1['H'] + 1 if p1['ftype'] == 0 else 1,
                              d=1, ctx=-1, res=[])
                    f2 = dict(ftype=0, A=0, H=p1['H'] + 1, d=1, ctx=-1, res=[])
                else:
                    q2, K2 = CTX[p1['ctx']] if p1['ftype'] == 1 else ((2, 3), 6)
                    f2 = dict(ftype=0, A=(p1['A'] + 1) % max(p1['H'], 1) if p1['H'] > 0 else p1['A'] + 1,
                              H=p1['H'], d=1, ctx=-1, res=[])
                    if p1['H'] == 0:
                        f2 = dict(ftype=0, A=p1['A'] + 1, H=0, d=1, ctx=-1, res=[])
                p2.update(f2)
            else:                                            # differ in hi only: widen
                p2 = dict(p1)
                p2['rad'] = (p1['rad'][0] * 3 | 1, p1['rad'][1])
        else:
            p2 = piece(rng)
        ps = [p1, p2]
        if rng.random() < 0.5:
            ps.sort(key=functools.cmp_to_key(keycmp))
        else:
            ps.reverse()
        return (1, 2, 0, ps)
    if r < 0.95:
        # dyadics at one huge binary exponent: keys of 2 to 4 pieces, both orders
        E = rng.choice([BIGEXP, -BIGEXP, BIGEXP - 3, -BIGEXP + 3, 2 ** 40, -(2 ** 40)])
        npc = rng.randrange(2, 5)
        ps = []
        for _ in range(npc):
            if rng.random() < 0.2:
                mid = ZERO
            else:
                mid = (rng.randrange(-40, 41), E)
            rad = (rng.randrange(0, 9), E + rng.choice([0, 0, 2, -2, 3])) if rng.random() < 0.7 else ZERO
            ps.append(piece(rng, mid=mid))
            ps[-1]['rad'] = rad
        ps.sort(key=functools.cmp_to_key(keycmp))
        ps = dedup(ps)
        if rng.random() < 0.5:
            ps.reverse()
        return (1, len(ps), 0, ps)
    # a sorted list of many pieces, every backend, some local
    ps = [piece(rng) for _ in range(rng.randrange(1, 13))]
    ps.sort(key=functools.cmp_to_key(keycmp))
    ps = dedup(ps)
    return (1, len(ps), 0, ps)


def dedup(ps):
    keep = []
    for p in ps:
        if keep:
            a, b = piece_key(keep[-1]), piece_key(p)
            if [dcmp(a[0], b[0]), dcmp(a[1], b[1]), (a[2] > b[2]) - (a[2] < b[2]),
                (a[3] > b[3]) - (a[3] < b[3])] == [0, 0, 0, 0]:
                continue
        keep.append(p)
    return keep


if __name__ == '__main__':
    sys.exit(run(int(sys.argv[1]), int(sys.argv[2])))
