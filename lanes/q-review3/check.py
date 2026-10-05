#!/usr/bin/env python3
"""Lane q-review3, part 1: the driver.  See model.py for the exact model."""
import argparse, os, random, subprocess, sys
from fractions import Fraction as F

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from model import (R_raw, round_piece, canon, bf_member, gen_case, line, parse, fam_of,
                   expected)

def q(f):
    return F(f[0], f[1]) if isinstance(f, (list, tuple)) else f

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cases', type=int, default=2000)
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--harness', default=os.path.join(os.path.dirname(os.path.abspath(__file__)), 'harness'))
    ap.add_argument('--work', default=os.path.join(os.path.dirname(os.path.abspath(__file__)), 'work'))
    ap.add_argument('--prec', type=int, default=53)
    ap.add_argument('--limitgap', action='store_true', help='also run with limit = count - 1')
    ap.add_argument('--points', type=int, default=200)
    args = ap.parse_args()
    os.makedirs(args.work, exist_ok=True)
    rng = random.Random(args.seed)
    os.makedirs(args.work, exist_ok=True)
    cases = []
    for i in range(args.cases):
        kind = rng.choice(['real', 'real', 'real', 'wide', 'bigexp'])
        c = gen_case(rng, kind)
        cases.append(c)

    # the model needs the input as the library reads it; the first pass writes the cases
    lines = []
    for c in cases:
        pc = [c]
        lines.append(line(10 ** 9, args.prec, pc))
    path = os.path.join(args.work, 'cases.txt')
    with open(path, 'w') as f:
        f.write('\n'.join(lines) + '\n')
    out = run(args.harness, path)
    recs = parse(out)
    inrecs = [r for r in recs if 'I' in r]
    if len(inrecs) != len(cases):
        print('harness answered %d of %d cases' % (len(inrecs), len(cases)))
        return 2

    # Now build the real batch: limit = count and (with --limitgap) limit = count - 1.
    lines = []
    meta = []
    for i, (c, r) in enumerate(zip(cases, inrecs)):
        fam = fam_of([r])
        lo, hi, a, N = fam[0]
        raw = R_raw(lo, hi, a, N)
        cnt = len(raw)
        lines.append(line(cnt, args.prec, [c])); meta.append((i, cnt, 0))
        if args.limitgap:
            lines.append(line(cnt - 1, args.prec, [c])); meta.append((i, cnt, -1))
    path2 = os.path.join(args.work, 'cases2.txt')
    with open(path2, 'w') as f:
        f.write('\n'.join(lines) + '\n')
    out = run(args.harness, path2)
    recs2 = parse(out)

    findings = []
    nchecked = 0
    for idx, (ci, cnt, gap) in enumerate(meta):
        r = recs2[idx]
        if 'I' not in r:
            findings.append(('no answer', ci, cnt, gap, None))
            continue
        fam = fam_of([r])
        lo, hi, a, N = fam[0]
        want_status = 0 if cnt + gap >= 1 else 10
        # only count >= 1 always holds; piece_limit >= 1 required
        if cnt + gap >= 1:
            want = 0 if cnt <= cnt + gap else 10
        else:
            want = 10
        st = r['status']
        if st not in (0, 10):
            findings.append(('status not OK/LIMIT', ci, cnt, gap, r))
            continue
        if st != want:
            findings.append(('wrong status', ci, cnt, gap, r))
            continue
        if st == 10:
            if r['untouched'] != 1:
                findings.append(('LIMIT changed y', ci, cnt, gap, r))
            continue
        nchecked += 1
        if r['form'] != 1:
            findings.append(('form not PIECES', ci, cnt, gap, r)); continue
        if r['canon'] != 1:
            findings.append(('not canonical', ci, cnt, gap, r)); continue
        if r['len'] != len(r['S']):
            findings.append(('len mismatch', ci, cnt, gap, r)); continue
        # expected pieces
        c0 = len(raw_expect(r, args.prec))
        if r['len'] != c0:
            findings.append(('piece count', ci, cnt, gap, r)); continue
        # the stored pieces, exact
        got = []
        for s in r['S']:
            mid = F(s[0], s[1]); rad = F(s[2], s[3]); A = s[4]; H = s[5]; d = s[6]
            got.append((mid, rad, A, H, d))
        want_set = raw_expect(r, args.prec)
        if sorted(got) != sorted(want_set):
            findings.append(('pieces differ', ci, cnt, gap, (got, want_set)))
            continue
        # every midpoint in [0,1]
        for g in got:
            if not (0 <= g[0] <= 1):
                findings.append(('midpoint out of [0,1]', ci, cnt, gap, r)); break
        # 200 sampled points of the input class must be in the output family
        outfam = [(g[0] - g[1], g[0] + g[1], g[2], g[3]) for g in got]
        bad = 0
        for (s, w) in sample_points(fam, outfam, args.points, rng, cnt):
            if not bf_member(outfam, s, w):
                bad += 1
                if bad == 1:
                    findings.append(('point lost', ci, cnt, gap, (s, w, fam, outfam)))
                break
    print('cases %d, checked %d, findings %d' % (len(meta), nchecked, len(findings)))
    for f in findings[:20]:
        print('---', f[0], 'case', f[1], 'count', f[2], 'gap', f[3])
        print('   ', str(f[4])[:600])
    return 1 if findings else 0

def raw_expect(r, prec):
    """The expected stored list of (mid, rad, A, H, d) for the case read in r."""
    fam = fam_of([r])
    lo, hi, a, N = fam[0]
    raw = R_raw(lo, hi, a, N)
    got = []
    for (l, h, m, A) in raw:
        mid, rad = round_piece(l, h, prec)
        got.append((mid - rad, mid + rad, A, canon(m, A), A))
    got.sort()
    ded = []
    for g in got:
        if not ded or ded[-1] != g:
            ded.append(g)
    return [( (g[0] + g[1]) / 2, (g[1] - g[0]) / 2, g[3], g[2], 1) for g in ded]

def sample_points(fam, outfam, n, rng, cnt):
    """Points (s, w) of the input class: the end points, near-end points, t=0 and t=1,
    the glued boundary, and points of the ball translated by rationals."""
    pts = []
    for (lo, hi, a, N) in fam:
        cands = [lo, hi, (lo + hi) / 2, F(0), F(1), lo - F(1, 3), hi + F(1, 7),
                 F(1, 2), F(-1, 2), F(0), F(1), F(1) - F(1, 2 ** 40)]
        cands.append(lo + F(1, 2 ** 30))
        cands.append(hi - F(1, 2 ** 30))
        ws = [a]
        if N > 0:
            A, B = F(N).numerator, F(N).denominator
            ws = [a + F(j * A, B) for j in range(min(B, 8))]
            ws += [a + F(k) for k in range(-3, 4)]
        for s in cands:
            for w in ws:
                pts.append((s, w))
    while len(pts) < n:
        lo, hi, a, N = fam[rng.randrange(len(fam))]
        s = lo + (hi - lo) * F(rng.randrange(0, 10 ** 6), 10 ** 6)
        w = a + (F(rng.randrange(-10 ** 6, 10 ** 6), 10 ** 6) if N == 0 else
                 F(rng.randrange(-40, 40)) * N)
        pts.append((s, w))
    return pts

def run(harness, path):
    with open(path, 'rb') as f:
        r = subprocess.run(['timeout', '170', harness], stdin=f, capture_output=True)
    if r.returncode != 0:
        print('harness exit %d' % r.returncode, r.stderr[:400])
    return r.stdout.decode().splitlines()

if __name__ == '__main__':
    sys.exit(main())