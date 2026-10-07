#!/usr/bin/env python3
"""Lane q-repair1: red runs. Plants the review's faults F2 and F8 (patches copied from
lanes/q-review3/faults.py) in a scratch copy of src/qclass.c under lanes/q-repair1/fault/src, builds
tests/test_qclass_reduce.c (copied beside it; it includes ../src/qclass.c) against the plain archive
lanes/q-repair1/build/libadelefeld.a, and runs it from the worktree root. NONE builds the unmodified copy.
  timeout 600 python3 lanes/q-repair1/faults.py"""
import os, subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
L = os.path.join(ROOT, 'lanes', 'q-repair1')
SRC = open(os.path.join(ROOT, 'src', 'qclass.c')).read()
FAULTS = {
    'NONE': ("unmodified source", []),
    'F2': ("last integer dropped when h = integer + tiny (frac(h) < 2^-100)",
           [("else fmpz_cdiv_q(stop, fmpq_numref(h), fmpq_denref(h));",
             "else { fmpz_cdiv_q(stop, fmpq_numref(h), fmpq_denref(h));\n"
             "    { fmpq_t fr; fmpz_t fl; fmpq_init(fr); fmpz_init(fl);\n"
             "      fmpz_fdiv_q(fl, fmpq_numref(h), fmpq_denref(h)); fmpq_set_fmpz(fr, fl);\n"
             "      fmpq_sub(fr, h, fr); fmpq_mul_2exp(fr, fr, 100);\n"
             "      if (fmpq_sgn(fr) > 0 && fmpz_cmp(fmpq_numref(fr), fmpq_denref(fr)) < 0 &&\n"
             "          fmpz_cmp(stop, first) > 1 - 1 && fmpz_cmp(fl, first) > 0) fmpz_set(stop, fl);\n"
             "      fmpq_clear(fr); fmpz_clear(fl); } }")]),
    'F8': ("y->form written at the start of the second pass, before its last LIMIT exit",
           [("        fmpz_zero(total);\n",
             "        fmpz_zero(total); if (pass) y->form = ADF_QCLASS_PIECES;\n")]),
}


def sh(cmd, timeout=170):
    try:
        r = subprocess.run(cmd, shell=True, cwd=ROOT, capture_output=True, text=True, timeout=timeout)
        return r.returncode, r.stdout + r.stderr
    except subprocess.TimeoutExpired:
        return 'timeout', ''


def main():
    lib = os.path.join(L, 'build', 'libadelefeld.a')
    for d in ('src', 'tests', 'bin'):
        os.makedirs(os.path.join(L, 'fault', d), exist_ok=True)
    open(os.path.join(L, 'fault', 'tests', 'test_qclass_reduce.c'), 'w').write(
        open(os.path.join(ROOT, 'tests', 'test_qclass_reduce.c')).read())
    for name, (what, reps) in FAULTS.items():
        src = SRC
        for o, n in reps:
            assert src.count(o) == 1, name
            src = src.replace(o, n)
        open(os.path.join(L, 'fault', 'src', 'qclass.c'), 'w').write(src)
        tbin = os.path.join(L, 'fault', 'bin', 'test_' + name)
        rc, out = sh('gcc -std=c11 -O2 -w -Iinclude -Isrc -Itests %s/fault/tests/test_qclass_reduce.c '
                     'tests/support/jsonl.c tests/support/golden.c %s -lflint -lgmp -lm -o %s'
                     % (L, lib, tbin))
        if rc != 0:
            print(name, 'BUILD FAILED', out[-800:])
            continue
        print('%s: %s' % (name, what))
        for part in ('', 'repair-vectors', 'repair-count', 'repair-r1', 'repair-r2'):
            rc, out = sh('timeout 170 %s %s' % (tbin, part))
            lines = [l for l in out.splitlines() if 'reduce line' in l or 'checks' in l]
            print('  %-15s exit %s; %s' % (part or 'whole file', rc, ' | '.join(lines)[:200]))


if __name__ == '__main__':
    main()
