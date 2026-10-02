"""Expected driver lines from the exact oracle, never from driver output."""
from pathlib import Path
import sys
sys.path.insert(0, 'proto')
from lfunc_trig_checks import FUNCS, result
from lfunc_checks import lb_exact, lb_ball, lb_value

cmd = ['# 1F.7; expected values: proto/lfunc_trig_checks.py, exact sums with a proved tail.', '#!exit 1']
out = []
for f in FUNCS:
    for p, a, M, N in [(5,5,None,20), (2,4,None,8), (3,3,None,5), (5,1,None,8),
                        (2,2,None,8), (5,0,None,8), (2,0,1,8), (2,0,2,8),
                        (5,0,3,8), (5,0,3,2), (5,5,3,8), (3,3,2,1),
                        (7,-7,None,8), (13,13,None,8)]:
        arg = str(a) if M is None else f'(* ; {a} mod {p**M})'
        cmd.extend([f'prec {N}', f'{f}_at {arg} with {p}'])
        st, y = result(f, lb_exact(p,a) if M is None else lb_ball(p,a,M), N)
        if st != 'OK':
            out.append(f'error: {st}')
        else:
            text = f'{p}: {lb_value(y)}'
            out.append(text if y['exact'] else text + f' + O({p}^{y["N"]})')
Path('tests/driver/trig-prime.cmd').write_text('\n'.join(cmd)+'\n')
Path('tests/driver/trig-prime.out').write_text('\n'.join(out)+'\n')
# Exact real zeros avoid uncertain printed rounding in the golden file; C tests check nonzero arb values.
cmd = ['# 1F.7 real constant terms, exact real component of an adele, and absent real place.', '#!exit 1']
out = []
for f in FUNCS:
    cmd.extend([f'{f}_at 0 with real', f'{f}_at (0 ; 5) with real', f'{f}_at (* ; 0 mod 25) with real'])
    v = int(f in ('cos', 'cosh'))
    out.extend([f'real: {v}', f'real: {v}', 'error: DOMAIN'])
Path('tests/driver/trig-real.cmd').write_text('\n'.join(cmd)+'\n')
Path('tests/driver/trig-real.out').write_text('\n'.join(out)+'\n')
cmd = ['# 1F.7: same arity, place syntax, and accepted kinds as exp_at.', '#!exit 1']
out = []
for f in FUNCS:
    for args, st in [('5', 'PARSE'), ('5 with', 'PARSE'), ('5 with 5 7', 'PARSE'),
                     ('5 with +5', 'PARSE'), ('5 with 4', 'DOMAIN'),
                     ('[1] with 5', 'UNSUPPORTED'), ('((1) + (2)*i ; 5) with real', 'UNSUPPORTED')]:
        cmd.append(f'{f}_at {args}')
        out.append(f'error: {st}')
Path('tests/driver/trig-hostile.cmd').write_text('\n'.join(cmd)+'\n')
Path('tests/driver/trig-hostile.out').write_text('\n'.join(out)+'\n')
print(f'driver fixtures: 3 files, {56+12+28} expected lines')
