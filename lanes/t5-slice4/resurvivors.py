#!/usr/bin/env python3
"""Re-runs mutation survivors of src/tate.c that were gaps of the tests, after the tests were added:
timeout 900 python3 -B lanes/t5-slice4/resurvivors.py (needs lanes/t5-slice4/build). One textual replacement each,
in a scratch copy linked before the library archive, as faults.py; the scratch directory is removed at the end."""
import os
import shutil
import subprocess

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
B = os.path.join(HERE, 'build')
S = os.path.join(HERE, 'scratch-re')
CFLAGS = ['-std=c11', '-O2', '-g', '-Wall', '-Wextra', '-Wpedantic', '-Werror']
MUTANTS = [
    ('388 nz == 0 -> nz == 1', '        if (nz == 0)                                      /* every coefficient',
     '        if (nz == 1)                                      /* every coefficient'),
    ('579 C * C -> C + C', 'st = tt_charge(work, C * C + 2', 'st = tt_charge(work, C + C + 2'),
]


def main():
    src = open(os.path.join(ROOT, 'src', 'tate.c')).read()
    os.makedirs(S, exist_ok=True)
    sup = [os.path.join(B, 'support', f) for f in ('golden.o', 'jsonl.o')]
    for k, (name, old, new) in enumerate(MUTANTS):
        assert src.count(old) == 1, name
        c, o, exe = (os.path.join(S, f'm{k}{x}') for x in ('.c', '.o', '.exe'))
        open(c, 'w').write(src.replace(old, new))
        subprocess.run(['cc', '-Iinclude', '-Isrc'] + CFLAGS + ['-c', c, '-o', o], cwd=ROOT, check=True)
        subprocess.run(['cc', '-Iinclude', '-Itests'] + CFLAGS + ['tests/test_tate_continue.c'] + sup +
                       [o, os.path.join(B, 'libadelefeld.a'), '-lflint', '-lgmp', '-lm', '-o', exe], cwd=ROOT,
                       check=True)
        r = subprocess.run(['timeout', '300', exe], cwd=ROOT, capture_output=True, text=True)
        fail = [x for x in r.stderr.splitlines() if 'test_tate_continue.c:' in x]
        print(name + ':', 'killed (exit %d) %s' % (r.returncode, fail[0] if fail else '') if r.returncode
              else 'SURVIVED', flush=True)
    shutil.rmtree(S)


if __name__ == '__main__':
    main()
