#!/usr/bin/env python3
"""Lane q-review3, hunt 7: plant faults in a scratch copy of src/qclass.c (lanes/q-review3/fault/src),
build tests/test_qclass_reduce.c (copied; it includes ../src/qclass.c) and the lane harness against each,
run the lane's test and the own checker. Run from the worktree root:
  python3 lanes/q-review3/faults.py [--only F1,F2] [--cases 300]"""
import argparse, os, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
L = os.path.join(ROOT, 'lanes', 'q-review3')
SRC = open(os.path.join(ROOT, 'src', 'qclass.c')).read()

FAULTS = {
    'F1': ("upper end min(h, n+1) replaced by n+1",
           "if (fmpq_cmp(right, one) > 0) fmpq_one(right);",
           "fmpq_one(right);"),
    'F2': ("last integer dropped when h = integer + tiny (frac(h) < 2^-100)",
           "else fmpz_cdiv_q(stop, fmpq_numref(h), fmpq_denref(h));",
           "else { fmpz_cdiv_q(stop, fmpq_numref(h), fmpq_denref(h));\n"
           "    { fmpq_t fr; fmpz_t fl; fmpq_init(fr); fmpz_init(fl);\n"
           "      fmpz_fdiv_q(fl, fmpq_numref(h), fmpq_denref(h)); fmpq_set_fmpz(fr, fl);\n"
           "      fmpq_sub(fr, h, fr); fmpq_mul_2exp(fr, fr, 100);\n"
           "      if (fmpq_sgn(fr) > 0 && fmpz_cmp(fmpq_numref(fr), fmpq_denref(fr)) < 0 &&\n"
           "          fmpz_cmp(stop, first) > 1 - 1 && fmpz_cmp(fl, first) > 0) fmpz_set(stop, fl);\n"
           "      fmpq_clear(fr); fmpz_clear(fl); } }"),
    'F3': ("fibres 0 <= j <= B",
           "for (j = 0; j < fibres; j++) {",
           "for (j = 0; j <= fibres; j++) {"),
    'F4': ("shift -n reduced modulo A with the wrong sign for negative n",
           "fmpz_neg(fin, n); if (!fmpz_is_zero(A)) fmpz_mod(fin, fin, A);",
           "fmpz_neg(fin, n); if (fmpz_sgn(n) < 0) fmpz_neg(fin, fin);\n"
           "                        if (!fmpz_is_zero(A)) fmpz_mod(fin, fin, A);"),
    'F5': ("Q1 successor omitted when the radius u is a power of two",
           "v = fmpz_get_ui(mant)+1;",
           "v = fmpz_get_ui(mant) == (UWORD(1) << 29) ? fmpz_get_ui(mant) : fmpz_get_ui(mant)+1;"),
    'F6': ("deduplication on the real key only",
           "        if (q_piece_cmp(out.piece+keep-1, out.piece+i)) {",
           "        if (end_cmp(out.piece[keep-1].inf, out.piece[i].inf, 0) ||\n"
           "            end_cmp(out.piece[keep-1].inf, out.piece[i].inf, 1)) {"),
    'F7': ("limit compared with the count after deduplication",
           "if (fmpz_cmp_si(total, piece_limit) > 0) goto done;",
           "if (fmpz_cmp_si(total, WORD_MAX/2) > 0) goto done;",
           "out.len = keep; adf_qclass_swap(y, &out); status = ADF_OK;",
           "out.len = keep; if (keep > piece_limit) goto done;\n"
           "    adf_qclass_swap(y, &out); status = ADF_OK;"),
    'F8': ("y written before the last allocation succeeds (y->form set at the start of pass 1)",
           "        fmpz_zero(total);\n",
           "        fmpz_zero(total); if (pass) y->form = ADF_QCLASS_PIECES;\n"),
    'F9': ("midpoint clamped into [0,1] instead of the piece being built there",
           "                        if (fmpq_sgn(left) < 0) fmpq_zero(left);\n"
           "                        if (fmpq_cmp(right, one) > 0) fmpq_one(right);\n"
           "                        if (!q_round(out.piece[pos].inf, left, right, prec)) goto done;",
           "                        if (!q_round(out.piece[pos].inf, left, right, prec)) goto done;\n"
           "                        if (arf_sgn(arb_midref(out.piece[pos].inf)) < 0)\n"
           "                            arf_zero(arb_midref(out.piece[pos].inf));\n"
           "                        if (arf_cmp_ui(arb_midref(out.piece[pos].inf), 1) > 0)\n"
           "                            arf_one(arb_midref(out.piece[pos].inf));"),
    'F10': ("exponent bound off by one",
            "fmpz_cmp_si(ARF_EXPREF(a), ADF_QCLASS_EXP_MAX) > 0) return 0;",
            "fmpz_cmp_si(ARF_EXPREF(a), ADF_QCLASS_EXP_MAX+1) > 0) return 0;"),
    # own additional faults
    'F11': ("count check after a fibre omitted for the second and later fibres (total only at end)",
            "                if (fmpz_cmp_si(total, piece_limit) > 0) goto done;\n",
            "                if (j == 0 && fmpz_cmp_si(total, piece_limit) > 0) goto done;\n"),
    'F12': ("equal-endpoint case uses ceil(h) (zero pieces at an integer point)",
            "if (fmpq_equal(l, h)) fmpz_add_ui(stop, first, 1);",
            "if (0) fmpz_add_ui(stop, first, 1);"),
    'F13': ("Q1 midpoint rounded toward zero instead of to nearest",
            "arf_div(arb_midref(z), u, den, FLINT_MAX(prec, 2), ARF_RND_NEAR);",
            "arf_div(arb_midref(z), u, den, FLINT_MAX(prec, 2), ARF_RND_DOWN);"),
}


def sh(cmd, timeout=170):
    try:
        r = subprocess.run(cmd, shell=True, cwd=ROOT, capture_output=True, text=True, timeout=timeout)
        return r.returncode, r.stdout + r.stderr
    except subprocess.TimeoutExpired:
        return 'timeout', ''


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--only', default='')
    ap.add_argument('--cases', type=int, default=300)
    args = ap.parse_args()
    names = args.only.split(',') if args.only else list(FAULTS)
    lib = os.path.join(L, 'build', 'libadelefeld.a')
    for d in ('src', 'tests', 'bin'):
        os.makedirs(os.path.join(L, 'fault', d), exist_ok=True)
    open(os.path.join(L, 'fault', 'tests', 'test_qclass_reduce.c'), 'w').write(
        open(os.path.join(ROOT, 'tests', 'test_qclass_reduce.c')).read())
    for name in names:
        spec = FAULTS[name]
        what, reps = spec[0], list(zip(spec[1::2], spec[2::2]))
        src = SRC
        if any(src.count(o) != 1 for o, n in reps):
            print(name, 'PATTERN NOT UNIQUE')
            continue
        for o, n in reps:
            src = src.replace(o, n)
        open(os.path.join(L, 'fault', 'src', 'qclass.c'), 'w').write(src)
        tbin = os.path.join(L, 'fault', 'bin', 'test_' + name)
        hbin = os.path.join(L, 'fault', 'bin', 'harness_' + name)
        rc, out = sh('gcc -std=c11 -O2 -w -Iinclude -Isrc -Itests %s/fault/tests/test_qclass_reduce.c '
                     'tests/support/jsonl.c tests/support/golden.c %s -lflint -lgmp -lm -o %s'
                     % (L, lib, tbin))
        if rc != 0:
            print(name, 'TEST BUILD FAILED', out[-500:])
            continue
        rc2, out2 = sh('gcc -std=c11 -O2 -w -c -Iinclude -Isrc %s/fault/src/qclass.c -o %s/fault/bin/q.o && '
                       'gcc -std=c11 -O2 -w -Iinclude %s/harness.c %s/fault/bin/q.o %s -lflint -lgmp -lm -o %s'
                       % (L, L, L, L, lib, hbin))
        rc, out = sh('timeout 170 %s' % tbin)
        lane = 'PASSES (not detected)' if rc == 0 else 'fails (rc %s): %s' % (
            rc, ' | '.join(l for l in out.splitlines() if 'line' in l or 'Abort' in l)[:150])
        own = 'not built'
        if rc2 == 0:
            rc3, out3 = sh('timeout 170 python3 %s/check.py --cases %d --seed 900 --points 20 --harness %s'
                           % (L, args.cases, hbin))
            last = [l for l in out3.splitlines() if l.startswith('seed') or l.startswith('finding kinds')]
            own = ' '.join(last)[-300:] if last else ('rc %s %s' % (rc3, out3[-300:]))
        print('%s: %s\n  lane test: %s\n  own check: %s' % (name, what, lane, own))
        sys.stdout.flush()


if __name__ == '__main__':
    main()
