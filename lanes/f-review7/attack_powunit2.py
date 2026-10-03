"""Attack A2b: p = 2 only, every combination of A in {exact, 1, 2, 3, 4, 6} with u0 = 1 or 3 mod 4 (and -1, 1
exact), s exact (odd/even/zero) or B in {0, 1, 2, 3, 5} with odd/even/zero centres, N in {1, 2, 3, 5, 9, 40}:
enumeration modulo 2^(K+1) (enclosure, tightness when K < N); the hull cases counted."""
import sys, itertools, random
from oracle import *
import attack_powunit as W

def main():
    us = [('exact', F(1)), ('exact', F(-1)), ('exact', F(5)), ('exact', F(3)), ('exact', F(-5, 3)), ('exact', F(7, 9))]
    for A in (1, 2, 3, 4, 6):
        for c in (1, 3, 5, 7, 13, 23, 41):
            us.append(('ball', F(c), A))
    ss = [('exact', F(0)), ('exact', F(1)), ('exact', F(2)), ('exact', F(-3)), ('exact', F(4, 3)), ('exact', F(12)),
          ('exact', F(1, 3))]
    for B in (0, 1, 2, 3, 5):
        for c in (0, 1, 2, 3, 4, 6, 8, 12):
            ss.append(('ball', F(c), B))
    lines, cases = [], []
    for uf, sf, N in itertools.product(us, ss, (1, 2, 3, 5, 9, 40)):
        cases.append((2, uf, sf, N))
        lines.append('W 2 %s %s %d' % (W.fields(2, uf), W.fields(2, sf), N))
    out = Harness().run(lines)
    bad = 0; hull = 0
    for cs, o in zip(cases, out):
        if o.startswith('OK 0 0 1 1') and cs[3] > 1: hull += 1
        r = W.check(*cs, o)
        if r:
            bad += 1
            if bad < 20: print('FAIL', cs, r)
    print('cases', len(cases), 'failures', bad, 'hull results with N > 1:', hull, W.STATS)

main()
