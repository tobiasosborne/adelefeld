#!/usr/bin/env python3
"""Lane q-review3 (Claude Opus): hunts 1 and 2 of the brief. Generates lifts and PIECES inputs, runs the
harness, and checks for every case:
  (a) exact containment of the input class in the stored union (model.contains; no algorithm R),
  (b) membership of sampled rational points of the input class (model.point_in),
  (c) the stored list against an own algorithm R + Q1 (model.expected_store), exactly,
  (d) Q1 bound: rho - d <= 2^-28 d against the exact constructed piece, midpoint in [0,1],
  (e) storage order: strictly increasing (lo, hi, H, A), d = 1, 0 <= A < H or H = 0,
  (f) status: limit = construction count gives OK; limit = count - 1 gives LIMIT, y untouched.
Usage: python3 check.py --cases N --seed S [--harness PATH] [--points K] [--prec P]
"""
import argparse, os, random, subprocess, sys, time
from fractions import Fraction as F

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from model import q1, canon_fin, expected_store, R_count, contains, point_in  # noqa: E402

FRACS = [F(0), F(1), F(2), F(3), F(12), F(360), F(1, 2), F(1, 3), F(2, 3), F(3, 2), F(5, 4),
         F(7, 360)]
DENS = [1, 1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 60, 360]


def dy(v):
    """(mantissa, exponent) of a dyadic rational."""
    v = F(v)
    n, d = v.numerator, v.denominator
    assert d & (d - 1) == 0
    return n, -(d.bit_length() - 1)


def rand_dyadic_rad(rng, maxw):
    """A dyadic radius r >= 0 with an odd mantissa below 2^30 and 2r <= about maxw."""
    if maxw == 0 or rng.random() < 0.1:
        return F(0)
    k = rng.choice([1, 2, 3, 5, 8, 15, 29, 30])
    man = rng.randrange(1, 1 << k)
    # scale so that man * 2^e <= maxw / 2
    e = 0
    while F(man) * F(2) ** e > F(maxw, 2):
        e -= 1
    e -= rng.choice([0, 0, 1, 3, 10, 40, 120, 200])
    return F(man) * F(2) ** e


def rand_finite(rng, big=False):
    N = rng.choice(FRACS)
    den = rng.choice(DENS) if rng.random() < 0.8 else rng.randrange(1, 361)
    if big:
        num = rng.randrange(-(1 << 2000), 1 << 2000)
    else:
        num = rng.randrange(-5000, 5001)
    return F(num, den), N


def gen_lift(rng):
    kind = rng.choice(['gen', 'gen', 'ends', 'ends', 'tiny', 'big', 'local', 'wide'])
    bk = 0
    if kind in ('gen', 'big', 'local', 'wide'):
        e = rng.randrange(-200, 201)
        k = rng.choice([1, 2, 5, 10, 30, 53, 64, 100])
        man = rng.randrange(-(1 << k), 1 << k)
        mid = F(man) * F(2) ** e
        if kind == 'wide':
            mid = F(rng.randrange(-8, 9), rng.choice([1, 2, 4]))
        maxw = rng.choice([0, 1, 2, 3, 5, 10, 20, 40]) if kind != 'wide' else 40
        rad = rand_dyadic_rad(rng, maxw)
    elif kind == 'ends':
        lo = F(rng.randrange(-60, 61), rng.choice([1, 2]))
        hi = lo + F(rng.randrange(0, 41), rng.choice([1, 2]))
        if rng.random() < 0.2:
            hi = lo
        mid, rad = (lo + hi) / 2, (hi - lo) / 2
        if rad and rad.denominator > 1 << 29:
            rad = F(0)
    else:  # tiny
        e = rng.randrange(-260, -150)
        mid = F(rng.randrange(-(1 << 20), 1 << 20)) * F(2) ** e
        rad = F(rng.randrange(0, 1 << 20)) * F(2) ** (e - rng.randrange(0, 5))
    c, N = rand_finite(rng, big=(kind == 'big'))
    if kind == 'ends' and rng.random() < 0.5:
        c = F(rng.randrange(-20, 21), rng.choice([1, 2]))
    if kind == 'local':
        d = rng.choice([1, 2, 3, 5, 7, 11, 13, 720])
        c = F(rng.randrange(-1000, 1000), d)
        N = F(360, d)
        bk = 1
    # keep the work bounded: B fibres of about (width + 1) pieces each
    while N and N.denominator * (2 * rad / N.numerator + 2) > 3000:
        rad = rad / 4
        if rad < F(1, 1 << 200):
            rad = F(0)
    return (mid, rad, c, N, bk)


def fin_set(c, N):
    """(a, N) with a the canonical centre."""
    A, H, d = canon_fin(c, N)
    return F(A, d), F(H, d)


def piece_tokens(mid, rad, c, N, bk):
    mm, me = dy(mid)
    if rad:
        rm, re = dy(rad)
    else:
        rm, re = 0, 0
    return "%d %d %d %d %d %d %d %d %d" % (mm, me, rm, re, c.numerator, c.denominator,
                                           N.numerator, N.denominator, bk)


def case_line(form, limit, prec, digits, alias, pieces):
    return "C %d %d %d %d %d %d %s" % (form, limit, prec, digits, alias, len(pieces),
                                       " ".join(piece_tokens(*p) for p in pieces))


def run(harness, lines, timeout=170):
    t0 = time.time()
    r = subprocess.run([harness], input="\n".join(lines) + "\n", capture_output=True, text=True,
                       timeout=timeout)
    if r.returncode != 0:
        print("harness exit", r.returncode, r.stderr[-3000:])
    return r.stdout.splitlines(), time.time() - t0


def parse(out):
    recs, cur = [], None
    for ln in out:
        t = ln.split(' ', 1)
        if t[0] == 'X':
            recs.append({'bad': ln})
            cur = None
        elif t[0] == 'I':
            if cur is None or 'R' in cur:
                cur = {'I': [], 'S': [], 'T': None}
                recs.append(cur)
            cur['I'].append([int(v) for v in t[1].split()])
        elif t[0] == 'R':
            cur['R'] = [int(float(v)) for v in t[1].split()]
        elif t[0] == 'S':
            cur['S'].append([int(v) for v in t[1].split()])
        elif t[0] == 'T':
            cur['T'] = t[1]
    return recs


def stored_of(rec):
    """Stored pieces as (L, U, c, H) and as (m, rho, A, H, d)."""
    out, raw = [], []
    for s in rec['S']:
        m, rho = F(s[0], s[1]), F(s[2], s[3])
        A, H, d = s[4], s[5], s[6]
        out.append((m - rho, m + rho, F(A, d), F(H, d)))
        raw.append((m, rho, A, H, d))
    return out, raw


def input_of(rec):
    res = []
    for i in rec['I']:
        m, r = F(i[0], i[1]), F(i[2], i[3])
        A, H, d = i[4], i[5], i[6]
        res.append((m - r, m + r, F(A, d), F(H, d)))
    return res


def sample_points(rng, lo, hi, a, N, k):
    pts = [lo, hi]
    w = hi - lo
    if w:
        pts += [lo + w / 10 ** 9, hi - w / 10 ** 9, lo + w / 2]
    # the glued boundary: real coordinates that are integers inside [lo, hi]
    from math import ceil, floor
    for n in range(ceil(lo), min(floor(hi), ceil(lo) + 3) + 1):
        pts.append(F(n))
    while len(pts) < k:
        pts.append(lo + w * F(rng.randrange(0, 10 ** 6 + 1), 10 ** 6))
    res = []
    for s in pts[:k]:
        if N == 0:
            z = a
        else:
            z = a + N * rng.randrange(-50, 51)
        q = F(rng.randrange(-10 ** 6, 10 ** 6), rng.randrange(1, 1000))
        res.append((s + q, z + q))
    return res


def check_case(rec, prec, limit, count, rng, npoints, findings, tag):
    R = rec['R']
    st, form, ln, canon, untouched, alias_ok = R[:6]
    ins = input_of(rec)
    want = 0 if (limit >= 1 and count <= limit) else 10
    if st != want:
        findings.append((tag, 'status', st, want, count, limit))
        return
    if not alias_ok:
        findings.append((tag, 'alias', st))
    if st != 0:
        if not untouched:
            findings.append((tag, 'touched-on-LIMIT'))
        return
    out, raw = stored_of(rec)
    if form != 1 or not canon or ln != len(raw):
        findings.append((tag, 'form/canon', form, canon, ln))
    # (e) storage predicate, own check
    prev = None
    for (m, rho, A, H, d), (L, U, c, Hh) in zip(raw, out):
        if d != 1 or H < 0 or (H > 0 and not 0 <= A < H) or not 0 <= m <= 1:
            findings.append((tag, 'piece-predicate', m, rho, A, H, d))
        key = (L, U, H, A)
        if prev is not None and not prev < key:
            findings.append((tag, 'order', prev, key))
        prev = key
    # (a) exact containment
    for (lo, hi, a, N) in ins:
        ok, wit = contains(out, lo, hi, a, N)
        if not ok:
            findings.append((tag, 'ENCLOSURE-LOST', (lo, hi, a, N), wit))
        # (b) points
        if npoints:
            k = max(5, npoints // len(ins))
            for (s, z) in sample_points(rng, lo, hi, a, N, k):
                if not point_in(out, s, z):
                    findings.append((tag, 'POINT-LOST', (lo, hi, a, N), (s, z)))
                    break
    # (c) against own R + Q1
    cnt, exp, exact = expected_store(ins, prec)
    if cnt != count:
        findings.append((tag, 'count-mismatch-model', cnt, count))
    if exp != raw:
        findings.append((tag, 'stored-differs-from-model', len(exp), len(raw)))
    # (d) Q1 bound against each exact piece this stored piece stands for
    for (m, rho, A, H, d), lhs in zip(raw, exact):
        for (l, h) in lhs:
            if not (m - rho <= l and h <= m + rho):
                findings.append((tag, 'Q1-no-enclosure', m, rho, l, h))
            dd = max(m - l, h - m)
            if rho - dd > F(1, 1 << 28) * dd:
                findings.append((tag, 'Q1-too-wide', m, rho, l, h))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cases', type=int, default=200)
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--harness', default=os.path.join(HERE, 'harness'))
    ap.add_argument('--points', type=int, default=200)
    ap.add_argument('--batch', type=int, default=500)
    ap.add_argument('--pieces', action='store_true', help='PIECES inputs instead of lifts')
    ap.add_argument('--twice', action='store_true', help='PIECES inputs: the output of a reduction')
    args = ap.parse_args()
    rng = random.Random(args.seed)
    findings = []
    nruns = nok = nlim = nbad = 0
    npieces = 0
    ndedup = 0
    t0 = time.time()
    done = 0
    while done < args.cases:
        nb = min(args.batch, args.cases - done)
        lines, meta = [], []
        for _ in range(nb):
            prec = rng.choice([2, 3, 10, 20, 30, 53, 64, 128, 300])
            if args.twice:
                form, pcs = 1, None
            elif args.pieces:
                form, pcs = gen_pieces(rng)
            else:
                form, pcs = 0, [gen_lift(rng)]
            if pcs is None:
                pcs = twice_input(args.harness, rng, prec)
                if pcs is None:
                    continue
            count = sum(R_count(p[0] - p[1], p[0] + p[1], *fin_set(p[2], p[3])) for p in pcs)
            alias = 1 if rng.random() < 0.2 else 0
            for lim in (count, count - 1):
                lines.append(case_line(form, lim, prec, 0, alias, pcs))
                meta.append((prec, lim, count, (form, pcs)))
        out, dt = run(args.harness, lines)
        recs = parse(out)
        if len(recs) != len(lines):
            print('answered', len(recs), 'of', len(lines))
            return 2
        for rec, (prec, lim, count, inp) in zip(recs, meta):
            nruns += 1
            if 'bad' in rec:
                nbad += 1
                continue
            o = check_case(rec, prec, lim, count, rng, args.points if lim == count else 0,
                           findings, (args.seed, nruns))
            if rec['R'][0] == 0:
                nok += 1
                npieces += len(rec['S'])
                if len(rec['S']) < count:
                    ndedup += 1
            else:
                nlim += 1
            if findings and len(findings) < 5 and findings[-1][0] == (args.seed, nruns):
                print('FINDING', findings[-1], case_line(inp[0], lim, prec, 0, 0, inp[1])[:3000])
        done += nb
    print('seed %d: %d calls (%d OK, %d LIMIT, %d inputs refused by harness), %d stored pieces checked,'
          ' %d findings, %.1f s; OK calls where deduplication removed pieces: %d' % (args.seed, nruns, nok, nlim,
          nbad, npieces, len(findings),
                                    time.time() - t0, ndedup))
    kinds = {}
    for f in findings:
        kinds[f[1]] = kinds.get(f[1], 0) + 1
    print('finding kinds:', kinds)
    return 1 if findings else 0


# ------------------------------------------------------------------ PIECES inputs

def twice_input(harness, rng, prec):
    """Reduce a random lift; return its stored output as a PIECES input (or None)."""
    lift = gen_lift(rng)
    out, _ = run(harness, [case_line(0, 5000, prec, 0, 0, [lift])])
    recs = parse(out)
    if not recs or 'bad' in recs[0] or recs[0]['R'][0] != 0:
        return None
    pcs = []
    for (m, rho, A, H, d) in stored_of(recs[0])[1]:
        pcs.append((m, rho, F(A, d), F(H, d), 0))
    return pcs


def key_of(mid, rad, c, N):
    A, H, d = canon_fin(c, N)
    return (mid - rad, mid + rad, H, A)


def gen_pieces(rng):
    """A canonical PIECES input: 1 to 4 pieces, midpoints in [0,1], integer finite radius, d = 1."""
    pcs = []
    n = rng.choice([1, 1, 2, 3, 4])
    for _ in range(n):
        kind = rng.choice(['spill', 'spill', 'zero', 'one', 'plain'])
        if kind == 'zero':
            mid = F(0)
        elif kind == 'one':
            mid = F(1)
        else:
            mid = F(rng.randrange(0, 1 << 20), 1 << 20)
        if kind == 'spill':
            rad = max(mid, 1 - mid) + F(rng.randrange(1, 1 << 10), 1 << rng.choice([10, 20, 30]))
        elif kind == 'plain':
            rad = min(mid, 1 - mid) * F(rng.randrange(0, 1 << 10), 1 << 10)
        else:
            rad = F(rng.randrange(0, 1 << 12), 1 << rng.choice([10, 12, 20, 40]))
        if rad and rad.numerator >= 1 << 30:
            rad = F(rad.numerator >> 12, rad.denominator >> 12) if rad.denominator >= 1 << 12 else F(1)
        H = rng.choice([0, 1, 2, 3, 4, 6, 12])
        c = F(rng.randrange(0, H)) if H else F(rng.randrange(-10, 11))
        pcs.append((mid, rad, c, F(H), 0))
    pcs.sort(key=lambda p: key_of(*p[:4]))
    ded = []
    for p in pcs:
        if not ded or key_of(*ded[-1][:4]) != key_of(*p[:4]):
            ded.append(p)
    return 1, ded


if __name__ == '__main__':
    sys.exit(main())
