"""Re-run the 10 survivors of the mutation run against the extended test (scratch copy, as plant_faults.py).

Run: timeout 900 python3 lanes/q-slice7/check_survivors.py SCRATCH. Uses the plain lane build tree.
"""
import shutil
import subprocess
import sys
from pathlib import Path

root = Path.cwd()
scratch = Path(sys.argv[1])
SRC = 'src/qclass_arith.c'
M = [
    ('236 / -> *', 'out.len > SIZE_MAX/sizeof', 'out.len > SIZE_MAX*sizeof'),
    ('184 swap hi', 'qa_add(s->hi, s->hi, s->hi2, 0)', 'qa_add(s->hi, s->hi2, s->hi, 0)'),
    ('59 <= -> <', 'FLINT_MAX(e, b) <= ADF_QCLASS_BITS_MAX', 'FLINT_MAX(e, b) < ADF_QCLASS_BITS_MAX'),
    ('231 < -> <=', 'if (j+1 < fibres && !qa_add(centre', 'if (j+1 <= fibres && !qa_add(centre'),
    ('81 > -> >=', '+1 > ADF_QCLASS_BITS_MAX || xd > ADF_QCLASS_BITS_MAX) return 0;',
     '+1 > ADF_QCLASS_BITS_MAX || xd >= ADF_QCLASS_BITS_MAX) return 0;'),
    ('136 1 -> 0', 'if (v == (UWORD(1) << 30))', 'if (v == (UWORD(0) << 30))'),
    ('87 swap add', '    if (subtract) fmpq_sub(z, x, y); else fmpq_add(z, x, y);\n    return 1;\n}\n/* Read',
     '    if (subtract) fmpq_sub(z, x, y); else fmpq_add(z, y, x);\n    return 1;\n}\n/* Read'),
    ('114 ok 0 -> 1', 'ulong v; int ok = 0;', 'ulong v; int ok = 1;'),
    ('81 || -> &&', '+1 > ADF_QCLASS_BITS_MAX || xd > ADF_QCLASS_BITS_MAX) return 0;',
     '+1 > ADF_QCLASS_BITS_MAX && xd > ADF_QCLASS_BITS_MAX) return 0;'),
    ('60 + -> -', '(e >= b ? 1 : b-e+1)', '(e >= b ? 1 : b-e-1)'),
]
if scratch.exists():
    shutil.rmtree(scratch)
scratch.mkdir(parents=True)
for d in ('include', 'src', 'tests'):
    shutil.copytree(root/d, scratch/d, symlinks=True)
shutil.copy2(root/'Makefile', scratch/'Makefile')
shutil.copytree(root/'lanes/q-slice7/build', scratch/'build', symlinks=True)
orig = (scratch/SRC).read_text()
for name, old, new in M:
    assert orig.count(old) == 1, name
    (scratch/SRC).write_text(orig.replace(old, new))
    b = subprocess.run(['timeout', '600', 'make', '-s', '-j2', 'BUILD=build', 'build/test_qclass_arith'],
                       cwd=scratch, capture_output=True, text=True)
    if b.returncode:
        print(f'{name}\tnot compiled', flush=True)
        continue
    r = subprocess.run(['timeout', '120', './build/test_qclass_arith'], cwd=scratch, capture_output=True, text=True)
    err = (r.stderr.strip().splitlines() or [''])[0][:110]
    print(f'{name}\t{"killed" if r.returncode else "SURVIVED"}\texit {r.returncode}: {err}', flush=True)
