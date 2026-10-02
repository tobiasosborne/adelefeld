"""Check new driver commands against local probe outputs and hostile typed operands."""
from collections import Counter
from fractions import Fraction
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parent
names = ['sin_at', 'cos_at', 'sinh_at', 'cosh_at']
cmds, wants = [], []
counts = Counter()
status = {1: 'NOT_DETERMINED', 7: 'DOMAIN', 10: 'LIMIT'}
for line, output in zip((root/'local.in').read_text().splitlines(),
                        (root/'local.out').read_text().splitlines()):
    f, p, un, ud, v, M, exact, N = map(int, line.split())
    if N < 2 or max(abs(un).bit_length(), ud.bit_length()) > 400:
        continue
    key = p, f, exact
    if counts[key] >= 4:
        continue
    counts[key] += 1
    q = Fraction(un, ud)*p**v
    operand = str(q) if exact else f'(* ; {q} mod {p**M})'
    cmds.extend([f'prec {N}', f'{names[f]} {operand} with {p}'])
    s, op, ou, ov, oM, oe = output.split()
    s, op, ov, oM, oe = map(int, [s, op, ov, oM, oe])
    if s:
        wants.append(f'error: {status[s]}')
    else:
        centre = Fraction(ou)*op**ov
        wants.append(f'{op}: {centre}' + ('' if oe else f' + O({op}^{oM})'))
good = len(wants)
types = ['[1]', '[5 mod 6]', '(5 ; 5 * [1])', '(1 ; 1 * [1])', '<1 ; [1]>',
         '((1) + (2)*i ; 5)', '[p=5: 1 + O(5^2)]']
for name in names:
    for operand in types:
        for place in ['2', '5', 'real']:
            cmds.append(f'{name} {operand} with {place}')
            wants.append('error: UNSUPPORTED')
    for place, error in [('+5', 'PARSE'), ('05', 'PARSE'), ('2 3', 'PARSE'), ('4', 'DOMAIN'),
                         ('18446744073709551616', 'DOMAIN')]:
        cmds.append(f'{name} 0 with {place}')
        wants.append(f'error: {error}')
    cmds.append(f'{name} (* ; 0 mod 4) with real')
    wants.append('error: DOMAIN')
    cmds.append(f'{name} 2 with 2')
    wants.append('error: DOMAIN')
    cmds.append(f'{name} (* ; 0 mod 2) with 2')
    wants.append('error: NOT_DETERMINED')
text = '\n'.join(cmds)+'\n'
(root/'driver.cmd').write_text(text)
(root/'driver.expected').write_text('\n'.join(wants)+'\n')
r = subprocess.run(['timeout', '40', str(root/'build/adf')], input=text,
                   capture_output=True, text=True, timeout=45)
(root/'driver.out').write_text(r.stdout)
(root/'driver.stderr').write_text(r.stderr)
actual = r.stdout.splitlines()
assert r.returncode == 1, r.returncode
assert len(actual) == len(wants), (len(actual), len(wants))
faults = [(i+1, a, b) for i, (a, b) in enumerate(zip(actual, wants)) if a != b]
print(f'library_comparisons={good} hostile_commands={len(wants)-good} lines={len(wants)} '
      f'driver_exit={r.returncode} failures={len(faults)}')
for fault in faults:
    print(fault)
assert not faults
