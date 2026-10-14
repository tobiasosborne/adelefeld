#!/usr/bin/env python3
"""Fault replants of lane t5-repair1: timeout 900 python3 -B lanes/t5-repair1/replant.py [NAME[:SECTION][@TEST] ...]

Each mutant is the exact text change of tools/mutate/mutate.py (the line and the old text are asserted), written
into a scratch copy of the one source file under lanes/t5-repair1/replant/NAME/, compiled with the flags of the
Makefile and linked before the plain archive lanes/t5-repair1/build/libadelefeld.a (built first with
make -s -j2 BUILD=lanes/t5-repair1/build <the test programs>), so that the mutated object replaces the archive's
member. The test (one section, or the whole program) is run under timeout; the exit code and the last line of its
error output are printed. The scratch directory is removed afterwards. Run from the root of the worktree.
"""
import os
import shutil
import subprocess
import sys

B = 'lanes/t5-repair1/build'
FLAGS = ['-std=c11', '-O2', '-g', '-Wall', '-Wextra', '-Wpedantic', '-Werror']

# name: (file, line, old text of the line, new text of the line, test, argv)
MUTANTS = {
    # item 1: L14's second branch (t5-slice4 survivor 247) and the dropped upper bound of Re(z') (t5-slice2 551)
    'm247': ('src/tate.c', 247, '        arb_mul(u, bb, R0, TT_TP);\n', '', 'test_tate_continue', ['jbound']),
    'm551': ('src/tate.c', 623, '    arb_get_ubound_arf(r2, acb_realref(zd), TT_TP);\n', '',
             'test_tate_continue', ['jbound']),
    # item 2: tt_series charges 0 per Lemma 6 step (t5-slice4 survivor 161)
    'm161': ('src/tate.c', 161, '        if (!tt_charge(w, 1))\n', '        if (!tt_charge(w, 0))\n',
             'test_tate', ['work']),
    'm161c': ('src/tate.c', 161, '        if (!tt_charge(w, 1))\n', '        if (!tt_charge(w, 0))\n',
              'test_tate_continue', ['jbound']),
}


# item 3: the survivors of lanes t5-slice1 and t5-slice3 in src/localfactor.c (the same table as probe.py)
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from probe import M as LF  # noqa: E402
for k, (ln, o, n) in LF.items():
    MUTANTS['lf' + k] = ('src/localfactor.c', ln, o, n, 'test_localfactor', [])


def run(cmd, timeout):
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout, text=True)
    return p.returncode, p.stdout, p.stderr


def replant(spec):
    spec, _at, other = spec.partition('@')                # NAME@TEST runs another test program
    name, _, sect = spec.partition(':')                   # NAME:SECTION runs another section ('' all)
    src, line, old, new, test, argv = MUTANTS[name]
    if _at:
        test = other
    if _:
        argv = [sect] if sect else []
    scratch = os.path.join('lanes', 't5-repair1', 'replant', name)
    os.makedirs(scratch, exist_ok=True)
    try:
        with open(src) as f:
            lines = f.readlines()
        assert lines[line - 1] == old, (name, lines[line - 1])
        lines[line - 1] = new
        mfile = os.path.join(scratch, os.path.basename(src))
        with open(mfile, 'w') as f:
            f.writelines(lines)
        obj = os.path.join(scratch, 'm.o')
        exe = os.path.join(scratch, 't')
        rc, _, err = run(['cc', '-Iinclude', '-Isrc'] + FLAGS + ['-c', mfile, '-o', obj], 300)
        assert rc == 0, err
        rc, _, err = run(['cc', '-Iinclude', '-Itests'] + FLAGS + ['tests/%s.c' % test, B + '/support/golden.o',
                          B + '/support/jsonl.o', obj, B + '/libadelefeld.a', '-lflint', '-lgmp', '-lm', '-o', exe],
                         300)
        assert rc == 0, err
        try:
            rc, out, err = run([exe] + argv, 600)
        except subprocess.TimeoutExpired:
            rc, out, err = 124, '', 'timeout'
        fails = [x for x in out.splitlines() if x.startswith('FAIL ')]
        tail = fails[0] if fails else (err.strip().splitlines() or ['(no error output)'])[-1]
        if len(fails) > 1:
            tail += ' (+%d more FAIL lines)' % (len(fails) - 1)
        print('%s %s:%d %s %s: exit %d: %s' % (name, src, line, test, ' '.join(argv), rc, tail))
        return rc
    finally:
        shutil.rmtree(scratch)


def main():
    names = sys.argv[1:] or list(MUTANTS)
    for n in names:
        replant(n)
    replant_dir = os.path.join('lanes', 't5-repair1', 'replant')
    if os.path.isdir(replant_dir) and not os.listdir(replant_dir):
        os.rmdir(replant_dir)


if __name__ == '__main__':
    main()
