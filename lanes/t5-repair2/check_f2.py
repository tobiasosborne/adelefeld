"""Check the two repaired S12 statements and the conductor-1 example."""
from pathlib import Path
import mpmath as m
m.mp.dps = 60
text = Path('docs/api-5b.md').read_text()
checks = 0
failures = 0

def check(ok, name):
    global checks, failures
    checks += 1
    if not ok:
        failures += 1
        print('FAIL', name)

s2 = text.split('S2. The value and the split.', 1)[1].split('S3.', 1)[0]
s12 = text.split('S12. The trivial zeros.', 1)[1].split('S13.', 1)[0]
check('poles at 0 and 1 for `C = 1`' in s2, 'S2 conductor-1 qualification')
check('regular at the permitted `s0`' in s12, 'S12 local regularity')
check("2 (-1)^k (pi/C)^k L'(s0)/k!" in s12, 'S12 completion factor')
naked = -2 * m.zeta(-2, 1, 1)
completed = m.pi * naked
check(abs(completed - m.mpf('0.1913132980155851711250765701204')) < m.mpf('1e-30'), 'completed value')
check(abs(completed - naked) > m.mpf('0.13'), 'omitted factor changes value')
print('naked =', m.nstr(naked, 50))
print('completed =', m.nstr(completed, 50))
print(f'{checks} checks, {failures} failures')
raise SystemExit(bool(failures))
