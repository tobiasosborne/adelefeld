"""Self-test of the oracle: shrink or drop stored pieces of real outputs; containment must fail."""
import random, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from fractions import Fraction as F
from check import gen_lift, fin_set, case_line, run, parse, stored_of, input_of, HERE
from model import contains, point_in
rng = random.Random(7)
lines = []
for _ in range(400):
    lines.append(case_line(0, 10**9, 53, 0, 0, [gen_lift(rng)]))
out, _ = run(os.path.join(HERE, 'harness'), lines)
recs = parse(out)
tot = det_drop = det_shrink = 0
for rec in recs:
    if 'bad' in rec or rec['R'][0] != 0: continue
    st, _ = stored_of(rec); (lo, hi, a, N), = input_of(rec)
    if len(st) < 2 or hi - lo < F(1, 1000): continue
    tot += 1
    i = rng.randrange(len(st))
    if not contains(st[:i] + st[i+1:], lo, hi, a, N)[0]: det_drop += 1
    L, U, c, H = st[i]
    st2 = st[:i] + [(L + (U - L) / 3, U, c, H)] + st[i+1:]
    if not contains(st2, lo, hi, a, N)[0]: det_shrink += 1
print('cases', tot, 'drop detected', det_drop, 'shrink detected', det_shrink)
# show a few undetected drops
shown = 0
for rec in recs:
    if 'bad' in rec or rec['R'][0] != 0: continue
    st, _ = stored_of(rec); (lo, hi, a, N), = input_of(rec)
    if len(st) < 2 or hi - lo < F(1, 1000): continue
    for i in range(len(st)):
        if contains(st[:i] + st[i+1:], lo, hi, a, N)[0]:
            L, U, c, H = st[i]
            print('input', float(lo), float(hi), a, N, 'npieces', len(st), 'dropped', float(L), float(U), c, H)
            shown += 1
            break
    if shown >= 6: break
