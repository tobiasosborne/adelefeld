#!/usr/bin/env python3
"""Lane m3-review1, hunt 7: ten-plus own faults in scratch copies; each is built into a copy of the normal archive
and the lanes' tests for that file are linked and run; then my own checkers are run against the faulty archive
to classify the fault (does it lose enclosure / change a truth or a sign?). Scratch trees are removed."""
import os
import shutil
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
LANE = os.path.join(ROOT, 'lanes', 'm3-review1')
B = os.path.join(LANE, 'build')
CF = ['-std=c11', '-O2', '-g', '-Wall', '-Wextra', '-Wpedantic', '-I' + os.path.join(ROOT, 'include'),
      '-I' + os.path.join(ROOT, 'src'), '-I' + os.path.join(ROOT, 'tests')]

FAULTS = [
    ('F1', 'psi.c', 'if (!psi_sub(b, a, m)) goto done;', 'fmpq_add(b, a, m);',
     'real sign reversed in psi_read: E(a+m)', ['test_psi', 'test_psi_class'], 'psi'),
    ('F2', 'psi.c', 'if (!psi_sub(d, u, r)) goto done;', 'fmpq_set(d, u);',
     'Q4 distance ignores the real radius r', ['test_psi', 'test_psi_class'], 'psi'),
    ('F3', 'psi.c', 'psi_fold(ext, upper, i == 0);', 'psi_fold(ext, upper, 1);',
     'class hull = hull of the last entry only', ['test_psi_class'], 'psi'),
    ('F4', 'psi.c', 'if (strict && frac) { st = ADF_NOT_DETERMINED; goto done; }',
     'if (strict && frac && x->len > 1) { st = ADF_NOT_DETERMINED; goto done; }',
     'class strict passes a fractional LIFT', ['test_psi_class'], 'psi'),
    ('F5', 'psi.c', 'fmpq_neg(b, b); psi_mod1(b);', 'psi_mod1(b);',
     'psi_at infinity E(+m) instead of E(-m)', ['test_psi_local', 'test_psi_class'], 'local'),
    ('F6', 'psi.c', 'fmpz_mul(t, t, n); fmpz_mod(t, t, M);', 'fmpz_mod(t, n, M);',
     'fp_p without the inverse of the prime-to-p denominator', ['test_psi_local', 'test_psi_class'], 'local'),
    ('F7', 'qclass_sets.c', 'if (fmpq_is_zero(s) && fmpq_is_one(p[i].hi)) {',
     'if (0 && fmpq_is_zero(s) && fmpq_is_one(p[i].hi)) {',
     'set queries drop the glue (1,m)~(0,m-1) for positive cosets', ['test_qclass_sets'], 'sets'),
    ('F8', 'qclass_sets.c', 'fmpz_mul_ui(budget, E, 2); fmpz_add_ui(budget, budget, 1);',
     'fmpz_mul_ui(budget, E, 1);',
     'budget (2E+1)KL replaced by E K L', ['test_qclass_sets'], 'sets'),
    ('F9', 'qclass_arith.c', 'qa_add(s->a->q, s->a->q, s->a2->q, 0) && qa_gcd(s->N->q, s->N->q, s->N2->q);',
     'qa_add(s->a->q, s->a->q, s->a2->q, 0);',
     'sum radius N1 instead of gcd(N1,N2)', ['test_qclass_arith'], 'arith'),
    ('F10', 'qclass_arith.c', 'fmpq_neg(s->lo, s->hi2); fmpq_neg(s->hi, s->lo2); fmpq_neg(s->a->q, s->a->q);',
     'fmpq_neg(s->lo, s->hi2); fmpq_neg(s->hi, s->lo2);',
     'negation keeps the finite centre', ['test_qclass_arith'], 'arith'),
    ('F11', 'qclass.c', 'return c ? c : (p->ordinal > q->ordinal) - (p->ordinal < q->ordinal);',
     'return c ? c : (p->ordinal < q->ordinal) - (p->ordinal > q->ordinal);',
     'set_pieces keeps the last of equal keys', ['test_qclass_text'], None),
    ('F12', 'dump.c', '            if (cmp >= 0)\n                keys->unordered = 1;',
     '            if (cmp > 0)\n                keys->unordered = 1;',
     'dump loader accepts two equal keys (duplicate pieces)', ['test_qclass_dump'], 'dump'),
    ('F13', 'qclass_sets.c', 'if (a->residues[i] && !b->residues[i]) return 0;',
     'if (a->residues[i] && !b->residues[i] && i) return 0;',
     'inclusion skips residue 0', ['test_qclass_sets'], 'sets'),
    ('F14', 'psi.c', '        v = fmpz_get_ui(man)+1;\n        fmpz_set(MAG_EXPREF(arb_radref(z)), ARF_EXPREF(r));',
     '        v = fmpz_get_ui(man);\n        fmpz_set(MAG_EXPREF(arb_radref(z)), ARF_EXPREF(r));',
     'hull radius RU30 without the successor (kernel of 3.3/Q1)', ['test_psi', 'test_psi_class'], 'psi'),
    ('F15', 'psi.c', 'return arb_is_finite(x) && mag_cmp_2exp_si(arb_radref(x), -p-1) <= 0;',
     'return arb_is_finite(x) && mag_cmp_2exp_si(arb_radref(x), -p+12) <= 0;',
     'cosine certificate 2^(13-p) instead of 2^-p (3.3 excess)', ['test_psi', 'test_psi_class', 'test_psi_local'], 'psi'),
    ('F16', 'qclass_arith.c', 'arf_div(arb_midref(z), u, den, FLINT_MAX(prec, 2), ARF_RND_NEAR);',
     'arf_div(arb_midref(z), u, den, FLINT_MAX(prec, 2), ARF_RND_DOWN);',
     'Q1 midpoint rounded down, not to nearest', ['test_qclass_arith'], 'arith'),
]

CHECK = {
    'psi': ['python3', 'psi_check.py', '41', '300'],
    'local': ['python3', 'psi_local.py', '41', '300'],
    'sets': ['python3', 'sets_check.py', '41', '1500'],
    'arith': ['python3', 'arith_check.py', '41', '300'],
    'dump': ['python3', 'dump_check.py', '41', '400'],
}


def sh(cmd, cwd=ROOT, timeout=170):
    try:
        r = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=timeout)
        return r.returncode, r.stdout + r.stderr
    except subprocess.TimeoutExpired:
        return 'timeout', ''


def main():
    only = sys.argv[1:]
    work = os.path.join(LANE, 'fault')
    rows = []
    for (fid, fname, old, new, what, tests, chk) in FAULTS:
        if only and fid not in only:
            continue
        shutil.rmtree(work, ignore_errors=True)
        os.makedirs(os.path.join(work, 'src'))
        os.makedirs(os.path.join(work, 'tests'))
        src = open(os.path.join(ROOT, 'src', fname)).read()
        assert src.count(old) == 1, (fid, src.count(old))
        open(os.path.join(work, 'src', fname), 'w').write(src.replace(old, new))
        obj = os.path.join(work, fname[:-2] + '.o')
        rc, out = sh(['cc'] + CF + ['-c', os.path.join(work, 'src', fname), '-o', obj])
        if rc != 0:
            rows.append((fid, what, 'COMPILE FAIL', out[:200], ''))
            continue
        lib = os.path.join(work, 'libadelefeld.a')
        shutil.copy(os.path.join(B, 'libadelefeld.a'), lib)
        sh(['ar', 'r', lib, obj], cwd=work)
        res = []
        for t in tests:
            exe = os.path.join(work, t)
            # tests that #include "../src/X.c" see the faulty copy (work/tests/../src)
            shutil.copy(os.path.join(ROOT, 'tests', t + '.c'), os.path.join(work, 'tests', t + '.c'))
            rc, out = sh(['cc'] + CF + [os.path.join(work, 'tests', t + '.c'), os.path.join(B, 'support', 'golden.o'),
                                        os.path.join(B, 'support', 'jsonl.o'), lib, '-lflint', '-lgmp', '-lm',
                                        '-pthread', '-o', exe])
            if rc != 0:
                res.append('%s:link-fail' % t)
                continue
            rc, out = sh([exe])
            res.append('%s:%s' % (t, 'DETECTED' if rc != 0 else 'passes'))
        own = ''
        if chk:
            hexe = os.path.join(work, 'h')
            rc, out = sh(['cc'] + CF + [os.path.join(LANE, 'h.c'), lib, '-lflint', '-lgmp', '-lm', '-ldl', '-o', hexe])
            rel = os.path.relpath(hexe, LANE)
            rc, out = sh(CHECK[chk] + [rel], cwd=LANE)
            lines = [l for l in out.splitlines() if 'FINDING' in l]
            own = 'own checker: %d findings; first: %s' % (len(lines), (lines[0][:160] if lines else '-'))
            if rc not in (0,):
                own += ' (rc %s)' % rc
        rows.append((fid, what, ' '.join(res), own))
        print(rows[-1], flush=True)
    shutil.rmtree(work, ignore_errors=True)


main()
