"""f-repair3: compare the residues of pins.txt with the exact Fraction oracle proto/lfunc_trig_checks.point."""
import sys
from fractions import Fraction
sys.path.insert(0, 'proto')
from lfunc_trig_checks import point
bad = n = 0
for line in open('lanes/f-repair3/pins.txt'):
    w = line.split()
    kv = dict(t.split('=') for t in w[1:])
    want = point(w[0], int(kv['p']), Fraction(kv['x']), int(kv['N']))
    n += 1
    if want != int(kv['residue']):
        bad += 1
        print('MISMATCH', line.strip(), want)
print(f'{n} pinned residues compared with the Fraction oracle, {bad} mismatches')
sys.exit(bad != 0)
