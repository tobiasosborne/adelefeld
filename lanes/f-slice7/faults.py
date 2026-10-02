"""Five deliberate faults in scratch copies only; no mutation tool and no fuzz target.
Run from the repository root under timeout 180. Each compile/test has its own timeout.
"""
from pathlib import Path
import re
import subprocess

build = Path('lanes/f-slice7/build')
src = Path('src/lfunc.c').read_text()
start = src.index('static int\nparity_coefficient')
base, parity = src[:start], src[start:]
faults = [
    ('one_term_short', 'D = val_fac((ulong) L, p);', 'if (L >= 3) L -= 2;\n    D = val_fac((ulong) L, p);'),
    ('input_exponent', 'K = x->exact || N < E ? N : E;', 'K = x->exact ? N : x->N;\n    (void) E;'),
    ('uncertain_zero_exact', 'if (x->exact && fmpq_is_zero(x->u))', 'if (fmpq_is_zero(x->u))'),
    ('alternating_sign', 'return alternating && (k / 2) % 2 ? -1 : 1;',
     '(void) alternating;\n    return 1;'),
    ('guard_digit', 'W = K + D;', 'W = K + D - (D > 0);'),
]
for name, old, new in faults:
    assert parity.count(old) == 1, (name, parity.count(old))
    dest = build/'faults'/name
    dest.mkdir(parents=True, exist_ok=True)
    (dest/'lfunc.c').write_text(base+parity.replace(old, new))
    command = ['timeout','45','cc','-std=c11','-O2','-Wall','-Wextra','-Wpedantic','-Werror',
               '-Iinclude','-Isrc','-Itests','tests/test_lfunc_trig.c',str(dest/'lfunc.c'),
               str(build/'support/jsonl.o'),str(build/'libadelefeld.a'),
               '-lflint','-lgmp','-lm','-o',str(dest/'test')]
    with (dest/'build.log').open('w') as log:
        cp = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT)
    assert cp.returncode == 0, name
    with (dest/'test.log').open('w') as log:
        cp = subprocess.run(['timeout','30',str(dest/'test')], stdout=log, stderr=subprocess.STDOUT)
    lines = (dest/'test.log').read_text().splitlines()
    summary = lines[-1] if lines else 'NO OUTPUT'
    assert cp.returncode == 1 and re.search(r'[1-9][0-9]* failed checks', summary), (name, cp.returncode, summary)
    print(name, 'exit', cp.returncode, summary, flush=True)
print('five planted faults: 5 rejected, 0 survived')
