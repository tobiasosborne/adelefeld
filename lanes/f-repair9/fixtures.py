"""Export certified dyadic cells from proto/zeta_checks.py's Z8 Gamma integral.

Each [k,e] encloses a component in [k*2^e,(k+1)*2^e]; null means exact zero.
Cells have 44 significant bits. No acb Gamma value is used as a reference.
Run four bounded batches, then assemble a file smaller than 300 KB.
"""
import argparse
import json
import sys
from fractions import Fraction as F
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LANE = Path(__file__).resolve().parent
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / 'proto'))
import zeta_checks as Z


def dyad(k, e):
    return F(k * 2**e) if e >= 0 else F(k, 2**-e)


def points(s):
    x, y, rx, ry = dyad(s[0], s[1]), dyad(s[2], s[3]), dyad(1, s[4]), F(0)
    if s[5] is not None:
        ry = dyad(1, s[5])
    if ry == 0:
        return [(x-rx, y), (x, y), (x+rx, y)]
    return [(x-rx, y-ry), (x-rx, y+ry), (x, y), (x+rx, y-ry), (x+rx, y+ry)]


def cell(a, zero=False):
    if zero:
        assert a == 0
        return None
    lo, hi = Z.aq(a.lower()), Z.aq(a.upper())
    scale = max(abs(lo), abs(hi))
    e = scale.numerator.bit_length() - scale.denominator.bit_length() - 44
    unit = dyad(1, e)
    k = lo // unit
    assert k*unit <= lo <= hi <= (k+1)*unit, (lo, hi, k, e)
    return [k, e]


def row(s):
    lo = (dyad(s[0], s[1])-dyad(1, s[4]))/2
    hi = (dyad(s[0], s[1])+dyad(1, s[4]))/2
    n = max(0, 1-lo.numerator//lo.denominator)
    assert n <= 64 and hi+n <= 64
    refs = []
    with Z.ctx.workprec(Z.CERT_BITS):
        for pt in points(s):
            box = Z.point_enclosure('real', *pt)
            refs.append([cell(box[0]), cell(box[1], pt[1] == 0)])
    return dict(s=s, v=refs)


def family(pole):
    # Real intervals lie to the left; complex boxes lie to the right, with midpoint distance d.
    # Complex boxes are thin vertically:
    # rx=d/2^k and ry=rx/16. The full product keeps the Z4 refinement admitted.
    for e in range(1, 13):
        for k in range(1, 15):
            for complex_box in (False, True):
                s = [pole*2**e+(1 if complex_box else -1), -e, 0, 0, -e-k,
                     -e-k-4 if complex_box else None]
                yield row(s)


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--pole', type=int, choices=(0, -2, -4, -16))
    p.add_argument('--assemble', action='store_true')
    a = p.parse_args()
    if a.assemble:
        rows = [row([-1, -4, 0, 0, -8, None]), row([-1, -4, 0, 0, -14, None])]
        for pole in (0, -2, -4, -16):
            rows += json.loads((LANE / f'pole{pole}.json').read_text())
        dest = ROOT / 'tests/ref/vectors/f-repair9/near-poles.jsonl'
        dest.parent.mkdir(parents=True, exist_ok=True)
        records = []
        for r in rows:
            records.append(dict(s=r['s'], n=len(r['v'])))
            records += [dict(v=v) for v in r['v']]
        lines = [json.dumps(r, separators=(',', ':')) for r in records]
        assert max(map(len, lines)) <= 116
        dest.write_text(''.join(s+'\n' for s in lines))
        assert dest.stat().st_size <= 300000
        print('boxes', len(rows), 'records', len(records), 'samples', sum(len(r['v']) for r in rows),
              'bytes', dest.stat().st_size)
    else:
        assert a.pole is not None
        rows = list(family(a.pole))
        (LANE / f'pole{a.pole}.json').write_text(json.dumps(rows, separators=(',', ':'))+'\n')
        print('pole', a.pole, 'rows', len(rows), 'samples', sum(len(r['v']) for r in rows))


if __name__ == '__main__':
    main()
