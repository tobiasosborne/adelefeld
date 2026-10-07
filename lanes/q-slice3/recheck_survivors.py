#!/usr/bin/env python3
"""Recheck the original sweep's survivors with the added tests and instrumentation.

Run under timeout after the 60-mutant sweep has finished. This reuses a copied pristine
SAN+INV+clang archive. It tries no additional mutation candidates. All children have timeouts.
"""
from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile

root = Path.cwd()
base = Path(sys.argv[1]).resolve()
timeouts_only = len(sys.argv) > 2 and sys.argv[2] == 'timed-out'
source = (root / 'src/psi.c').read_text()
env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
flags = ['-I' + str(root / 'include'), '-I' + str(root / 'src'), '-I' + str(root / 'tests'),
         '-DADF_CHECK_INVARIANTS', '-std=c11', '-O2', '-g', '-Wall', '-Wextra', '-Wpedantic',
         '-Werror', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
wrap = ['-Wl,--wrap=' + name for name in
        ('arb_sin_cos_pi_fmpq', 'arb_cos_pi_fmpq', 'fmpq_mul_fmpz', 'fmpq_div_fmpz')]
sys.path.insert(0, str(root))
from tools.mutate.mutate import mutants_of
selected = []
for line in (root / 'lanes/q-slice3/mutate.log').read_text().splitlines():
    prefix = 'TIMED OUT ' if timeouts_only else 'SURVIVED '
    if line.startswith(prefix):
        selected.append(line.removeprefix(prefix))
all_mutants = {str(m): m for m in mutants_of('src/psi.c', source, str(root))}


def run(args):
    return subprocess.run(['timeout', '150', *map(str, args)], cwd=root, env=env, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


def execute(scratch, label, source_path):
    rows = []
    for name, test, extra in [('public', root / 'tests/test_psi.c', []),
                              ('instrumented', root / 'lanes/q-slice3/test_retry.c', wrap),
                              ('lifecycle', root / 'lanes/q-slice3/test_lifecycle.c',
                               ['-DADF_PSI_SOURCE="' + str(source_path) + '"'])]:
        if timeouts_only and name == 'public':
            continue
        exe = scratch / ('test_' + name)
        result = run(['clang', *flags, test, base / 'support/jsonl.o', base / 'support/golden.o',
                      scratch / 'libadelefeld.a', *extra, '-lflint', '-lgmp', '-lm', '-pthread',
                      '-o', exe])
        assert result.returncode == 0, (label, result.stdout)
        result = run([exe])
        assert result.returncode in (0, 1), (label, result.stdout)
        rows.append((result.returncode, result.stdout.strip()))
    print(label, *(code for code, _ in rows), flush=True)
    for _, output in rows:
        print(output, flush=True)
    return rows


results = []
with tempfile.TemporaryDirectory(prefix='recheck-', dir=root / 'lanes/q-slice3') as tmp:
    scratch = Path(tmp)
    shutil.copy2(base / 'libadelefeld.a', scratch / 'libadelefeld.a')
    baseline = execute(scratch, 'baseline', root / 'src/psi.c')
    assert all(code == 0 for code, _ in baseline)
    for key in selected:
        mutant = all_mutants[key]
        (scratch / 'psi.c').write_text(mutant.apply(source))
        compiled = run(['clang', *flags, '-c', scratch / 'psi.c', '-o', scratch / 'psi.o'])
        assert compiled.returncode == 0, (key, compiled.stdout)
        shutil.copy2(base / 'libadelefeld.a', scratch / 'libadelefeld.a')
        archived = run(['ar', 'rcs', scratch / 'libadelefeld.a', scratch / 'psi.o'])
        assert archived.returncode == 0, archived.stdout
        tests = execute(scratch, key, scratch / 'psi.c')
        status = 'killed' if any(code for code, _ in tests) else 'survived'
        results.append((key, status))
    shutil.copy2(base / 'libadelefeld.a', scratch / 'libadelefeld.a')
    assert all(code == 0 for code, _ in execute(scratch, 'final pristine', root / 'src/psi.c'))
output_file = 'timeout-results.tsv' if timeouts_only else 'survivor-results.tsv'
with (root / 'lanes/q-slice3' / output_file).open('w') as out:
    for row in results:
        out.write('\t'.join(row) + '\n')
print('rechecks', len(results), 'killed', sum(status == 'killed' for _, status in results),
      'survived', sum(status == 'survived' for _, status in results), flush=True)
