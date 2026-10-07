#!/usr/bin/env python3
"""Plant the ten brief faults and a universal square in scratch copies. Run under timeout.

Each source mutation is compiled against a copied pristine archive. Every compiler,
archiver, and test has its own timeout. Scratch files are removed even on failure.
The baseline archive and support objects are supplied as the first argument.
"""
from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile

root = Path.cwd()
base = root / sys.argv[1]
source = (root / 'src/psi.c').read_text()
env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
cc = os.environ.get('FAULT_CC', 'cc')
flags = ['-I' + str(root / 'include'), '-I' + str(root / 'src'), '-I' + str(root / 'tests'),
         '-DADF_CHECK_INVARIANTS', '-std=c11', '-O2', '-g', '-Wall', '-Wextra', '-Wpedantic',
         '-Werror', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
faults = [
    ('D1 finite sign', 'psi_mod1(a); psi_mod1(m);',
     'psi_mod1(a); fmpq_neg(a, a); psi_mod1(a); psi_mod1(m);'),
    ('D2 numerator roots', 'fmpq_denref(N), targets[j]',
     '(fmpq_is_zero(N) ? fmpq_denref(N) : fmpq_numref(N)), targets[j]'),
    ('D3 strict writes', 'if (strict && !fmpz_is_one(fmpq_denref(N))) goto done;',
     'if (strict && !fmpz_is_one(fmpq_denref(N))) { acb_zero(z); goto done; }'),
    ('D4 getter ignores real radius',
     'if (!fmpq_is_zero(r) || !fmpz_is_one(fmpq_denref(N))) goto done;',
     'if (!fmpz_is_one(fmpq_denref(N))) goto done;'),
    ('D5 getter chooses one finite phase', 'if (!fmpz_is_one(fmpq_denref(N))) goto done;',
     '/* Wrong: one phase is returned for a finite family. */'),
    ('D6 pi instead of 2pi', 'fmpq_mul_2exp(twice, theta, 1);', 'fmpq_set(twice, theta);'),
    ('D7 universal square',
     'arf_neg(lo, upper[1]); psi_round(acb_realref(out), lo, upper[0], p);\n'
     '    arf_neg(lo, upper[3]); psi_round(acb_imagref(out), lo, upper[2], p);',
     'acb_zero(out); mag_one(arb_radref(acb_realref(out)));\n'
     '    mag_one(arb_radref(acb_imagref(out)));'),
    ('O1 real sign matches finite sign', 'if (!psi_sub(b, a, m)) goto done;',
     'fmpq_neg(m, m); if (!psi_sub(b, a, m)) goto done;'),
    ('O2 phase not reduced', 'psi_mod1(b); ok = 1;', 'ok = 1;'),
    ('O3 endpoint hull misses interior extrema', 'if (fmpq_sgn(d) < 0) fmpq_zero(d);',
     'if (fmpq_sgn(d) < 0) fmpq_neg(d, d);'),
    ('O4 finite radius ignored', 'if (!psi_read(b, r, N, x)) goto done;',
     'if (!psi_read(b, r, N, x)) goto done; fmpq_zero(N);'),
]


def run(args, seconds=150):
    return subprocess.run(['timeout', str(seconds), *map(str, args)], cwd=root, env=env,
                          text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


baseline = run([base / 'test_psi'])
print('baseline', baseline.returncode, baseline.stdout.strip(), flush=True)
assert baseline.returncode == 0
results = []
with tempfile.TemporaryDirectory(prefix='faults-', dir=root / 'lanes/q-slice3') as tmp:
    scratch = Path(tmp)
    for name, old, new in faults:
        # The last read call occurs in the numerical adele helper, not the phase getter.
        if name.startswith('O4'):
            position = source.rfind(old)
            assert position >= 0
            mutant = source[:position] + source[position:].replace(old, new, 1)
        else:
            assert source.count(old) == 1, (name, source.count(old))
            mutant = source.replace(old, new, 1)
        (scratch / 'psi.c').write_text(mutant)
        shutil.copy2(base / 'libadelefeld.a', scratch / 'libadelefeld.a')
        compiled = run([cc, *flags, '-c', scratch / 'psi.c', '-o', scratch / 'psi.o'])
        assert compiled.returncode == 0, (name, compiled.stdout)
        archived = run(['ar', 'rcs', scratch / 'libadelefeld.a', scratch / 'psi.o'])
        assert archived.returncode == 0, archived.stdout
        linked = run([cc, *flags, root / 'tests/test_psi.c',
                      base / 'support/jsonl.o', base / 'support/golden.o',
                      scratch / 'libadelefeld.a', '-lflint', '-lgmp', '-lm', '-pthread',
                      '-o', scratch / 'test_psi'])
        assert linked.returncode == 0, (name, linked.stdout)
        tested = run([scratch / 'test_psi'])
        # A startup crash is not evidence that an assertion detects the planted fault.
        assertion = next((line for line in tested.stdout.splitlines()
                          if 'tests/test_psi.c:' in line), '')
        print(name, 'exit', tested.returncode, assertion, flush=True)
        assert tested.returncode == 1 and assertion, (name, tested.stdout)
        results.append((name, tested.returncode, assertion))
with (root / 'lanes/q-slice3/fault-results.tsv').open('w') as out:
    for row in results:
        out.write('\t'.join(map(str, row)) + '\n')
print(len(results), 'planted faults rejected by assertions', flush=True)
