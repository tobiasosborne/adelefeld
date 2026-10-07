#!/usr/bin/env python3
"""Lane m3-review1, hunt 4: dump forms. Random classes (LIFT, PIECES; global, ctx1, ctx2 backends):
dump -> load identical, re-dump byte identical, inspect count = local occurrences. Mutations of the bytes:
a letter in a count, truncation, count 0, two pieces swapped, a duplicated piece, count + 1, random byte edits;
every non-OK leaves x untouched; no FLINT string loader is called on PARSE/LIMIT/UNSUPPORTED (interposed
fmpz_set_str/arb_load_str/arf_load_str/mag_load_str in h.c); every OK text re-dumps byte-identically."""
import random
import sys
from fractions import Fraction as F
from common import Piece, enc_class, run

seed = int(sys.argv[1]) if len(sys.argv) > 1 else 1
NC = int(sys.argv[2]) if len(sys.argv) > 2 else 1000
EXE = sys.argv[3] if len(sys.argv) > 3 else 'h'
rng = random.Random(seed)


def rpiece(pieces, bkset):
    bk = rng.choice(bkset)
    if pieces:
        mid = F(rng.randint(0, 2 ** 12), 2 ** 12)
        rad = rng.choice([F(0), F(rng.randint(1, 2 ** 20), 2 ** rng.randint(10, 40)), F(3, 2)])
        if bk == 1:
            return Piece(mid, rad, F(rng.randint(0, 359)), F(360), 1)
        if bk == 2:
            return Piece(mid, rad, F(rng.randint(0, 7 * 11 * 16 - 1)), F(7 * 11 * 16), 2)
        H = rng.choice([0, 1, 2, 12])
        return Piece(mid, rad, F(rng.randint(-9, 9) if H == 0 else rng.randint(0, H - 1)), F(H), 0)
    mid = F(rng.randint(-2 ** 40, 2 ** 40)) * F(2) ** rng.randint(-2000, 2000)
    rad = rng.choice([F(0), F(rng.randint(1, 2 ** 29)) * F(2) ** rng.randint(-3000, 100)])
    if bk == 1:
        d = rng.choice([1, 2, 7])
        return Piece(mid, rad, F(rng.randint(-10 ** 6, 10 ** 6), d), F(360, d), 1)
    if bk == 2:
        return Piece(mid, rad, F(rng.randint(-10 ** 6, 10 ** 6), 3), F(1232, 3), 2)
    return Piece(mid, rad, F(rng.randint(-2 ** 200, 2 ** 200), rng.choice([1, 3, 360])),
                 rng.choice([F(0), F(5), F(2, 7)]), 0)


def mutations(t):
    toks = t.split(' ')
    out = []
    if toks[3] == 'pieces':
        h = int(toks[4])
        m = list(toks); m[4] = 'x'; out.append(('letter', ' '.join(m), 'bad'))
        m = list(toks); m[4] = '0'; out.append(('zero', ' '.join(m), 'bad'))
        m = list(toks); m[4] = str(h + 1); out.append(('plus1', ' '.join(m), 'bad'))
        # pieces: split body by the arch count token '1' at piece starts: rebuild from dump grammar positions
        body = toks[5:]
        pcs, i = [], 0
        while i < len(body):
            j = i + 1 + 4   # '1' m e rm re
            if body[j] == 'g':
                j += 4
            else:
                k = int(body[j + 3], 16)
                j += 4 + 2 * k
            pcs.append(body[i:j]); i = j
        if len(pcs) >= 2:
            a, b = rng.sample(range(len(pcs)), 2)
            p2 = list(pcs); p2[a], p2[b] = p2[b], p2[a]
            out.append(('swap', ' '.join(toks[:5] + sum(p2, [])), 'bad'))
        p2 = list(pcs); k = rng.randrange(len(pcs)); p2.insert(k, pcs[k])
        m = toks[:4] + [str(h + 1)]
        out.append(('dup', ' '.join(m + sum(p2, [])), 'bad'))
    cut = rng.randrange(len(t) - 1)
    out.append(('trunc', t[:cut], 'any'))
    for _ in range(3):
        b = list(t)
        k = rng.randrange(len(b))
        b[k] = rng.choice('0123456789abcdefgl -x')
        out.append(('edit', ''.join(b), 'any'))
    return out


def main():
    lines, cases = [], []
    for _ in range(NC):
        if rng.random() < 0.5:
            form, pcs = 0, [rpiece(False, [0, 1, 2])]
        else:
            form, pcs = 1, [rpiece(True, [0, 0, 1, 2]) for _ in range(rng.randint(1, 7))]
        lines.append('D ' + enc_class(form, pcs))
        cases.append((form, pcs))
    out, _ = run(lines, EXE)
    bad = 0
    muts = []
    i = 0
    nok = 0
    for form, pcs in cases:
        r = out[i].split(); i += 1
        if r[0] == 'X':
            continue
        T = out[i][2:]; i += 1
        st, ident, same, ist, nctx, nb = [int(v) for v in r[1:7]]
        if st != 0 or ident != 1 or same != 1 or ist != 0 or nctx != nb:
            print('FINDING roundtrip', r, T); bad += 1
            continue
        nok += 1
        only_ctx1 = all(p.bk in (0, 1) for p in pcs)
        if only_ctx1:
            for kind, m, exp in mutations(T):
                muts.append((kind, m, exp, nb))
    lines = ['M %d %s' % (nb, m) for (kind, m, exp, nb) in muts]
    out, _ = run(lines, EXE)
    i = 0
    stats = {}
    for kind, m, exp, nb in muts:
        r = out[i].split(); i += 1
        st, flag, calls, ist = int(r[1]), int(r[2]), int(r[3]), int(r[4])
        T = None
        if i < len(out) and out[i].startswith('T '):
            T = out[i]; i += 1
        stats[(kind, st)] = stats.get((kind, st), 0) + 1
        if st == 0:
            if exp == 'bad':
                print('FINDING accepted', kind, m); bad += 1
            elif T is None or T.split(' ', 2)[1] != '1':
                print('FINDING OK text not byte-identical on re-dump', kind, m, T); bad += 1
            if ist != 0:
                print('FINDING inspect disagrees', kind, st, ist, m); bad += 1
        else:
            if flag != 1:
                print('FINDING not untouched', kind, st, m); bad += 1
            if st in (8, 9, 10) and calls:
                print('FINDING FLINT string call before validation', kind, st, calls, m); bad += 1
            if calls >= 1000:
                print('FINDING arb/arf/mag_load_str called', kind, st, m); bad += 1
            if ist != st and not (st == 7 and ist == 0):
                print('NOTE inspect status differs', kind, st, ist, m[:80])
    print('classes', len(cases), 'roundtrips ok', nok, 'mutants', len(muts), 'findings', bad)
    print(sorted(stats.items()))


main()
