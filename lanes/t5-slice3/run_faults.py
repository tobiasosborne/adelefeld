#!/usr/bin/env python3
"""Scratch faults of slice 5b: timeout 600 python3 -B lanes/t5-slice3/run_faults.py

Each fault edits lines of a scratch copy of src/localfactor.c (each edit is checked to match exactly once at
its line), compiles it, replaces localfactor.o in a copy of the plain library archive of
lanes/t5-slice3/build-plain, links tests/test_tate_gamma.c and runs it from the repository root. A fault is
killed when the test program exits non-zero; the failed-check count is read from its summary line.
The scratch directory lanes/t5-slice3/faults is removed at the end.
"""
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LANE = ROOT / 'lanes/t5-slice3'
BASE = LANE / 'build-plain'
SCR = LANE / 'faults'
CFLAGS = ['-std=c11', '-O2', '-g', '-Iinclude']   # no -Werror: a fault may leave a parameter unused

GAMMA_TAIL = ('    if (st == ADF_OK) acb_swap(z, t);\n    acb_clear(t);\n'
              '    return st == ADF_OK ? ADF_OK : fail(st, where, v);\n}\n\n/* api-5.md:114-118')
FAULTS = [
    ('5.1 inverse omitted: eta0 for eta0^-1 in G_minus',
     [('fmpq_set_ui(add, m, eta0->q);', 'fmpq_set_ui(add, m, eta0->q); fmpq_neg(theta, theta);')]),
    ('5.1 G_minus positive kernel E(+u/q)',
     [('fmpq_set_ui(add, m, eta0->q);', 'fmpq_set_si(add, -(slong) m, eta0->q);')]),
    ('5.1 alpha omitted (ramified alpha^a)',
     [('acb_pow_ui(out, alpha, a, w);', 'acb_one(out);')]),
    ('5.1 alpha omitted (unramified quotient)',
     [('acb_mul(num, alpha, t, w);', 'acb_set(num, t);'),
      ('acb_sub(den, alpha, t, w);', 'acb_sub_ui(den, t, 1, w); acb_neg(den, den);'),
      ('acb_mul(num, num, alpha, w);', ';')]),
    ('5.1 exponent a = 1 at conductor 4 (a forced to 1)',
     [('acb_mul_ui(t, t, a, w);', 'acb_mul_ui(t, t, 1, w);'),
      ('acb_pow_ui(out, alpha, a, w);', 'acb_pow_ui(out, alpha, 1, w);')]),
    ('5.1 parity omitted at infinity',
     [('st = gamma_real(t, s, eta0->parity, prec);', 'st = gamma_real(t, s, 0, prec);')]),
    ('5.1 unit volume doubled (ramified constant times 2)',
     [('acb_mul(out, out, g, w);', 'acb_mul(out, out, g, w); acb_mul_2exp_si(out, out, 1);')]),
    ('integral pole list for gamma (real: -e-2k)',
     [('if (odd != e && arf_cmp_si(x, 1 + e) >= 0) return ADF_DOMAIN;',
       'if (odd == e && arf_cmp_si(x, -e) <= 0) return ADF_DOMAIN;')]),
    ('integral pole list for gamma (finite: alpha = p^s)',
     [('if (tate_exact_finite_pole(s1, alpha, p))', 'if (tate_exact_finite_pole(s, alpha, p))')]),
    ('Gamma/Gamma division instead of rgamma',
     [('rgamma_shifted(den, den, w);', 'acb_gamma(den, den, w); acb_inv(den, den, w);')]),
    ('i^e dropped at infinity',
     [('if (e) acb_mul_onei(num, num);', ';')]),
    ('T^a for T^-a (ramified)',
     [('acb_mul_ui(t, t, a, w);\n        acb_neg(t, t);', 'acb_mul_ui(t, t, a, w);\n        ;')]),
    ('where not written (gamma)',
     [(GAMMA_TAIL, GAMMA_TAIL.replace('return st == ADF_OK ? ADF_OK : fail(st, where, v);', 'return st;'))]),
    ('z written before the pole check (gamma)',
     [(GAMMA_TAIL, GAMMA_TAIL.replace('if (st == ADF_OK) acb_swap(z, t);', 'acb_swap(z, t);'))]),
    ('epsilon rejects gamma poles (unramified)',
     [('if (a == 0) acb_one(t);',
       'if (a == 0) { st = gamma_unramified(t, s, p, alpha, prec); if (st == ADF_OK) acb_one(t); }')]),
    ('epsilon rejects gamma poles (real)',
     [('        if (eta0->parity) acb_onei(t);\n        else acb_one(t);\n        st = ADF_OK;',
       '        st = gamma_real(t, s, eta0->parity, prec);\n        if (eta0->parity) acb_onei(t);\n'
       '        else acb_one(t);')]),
    ('computed zero-containing denominator returned as DOMAIN',
     [('if (!acb_is_finite(den) || acb_contains_zero(den)) goto done;',
       'if (!acb_is_finite(den) || acb_contains_zero(den)) { st = ADF_DOMAIN; goto done; }')]),
]


def run(cmd, **kw):
    return subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, **kw)


def main():
    source = (ROOT / 'src/localfactor.c').read_text()
    marker = source.index('/* Slice 5b: the local gamma and epsilon factors')
    if SCR.exists():
        shutil.rmtree(SCR)
    SCR.mkdir()
    r = run(['timeout', '120', 'cc', *CFLAGS, '-Itests', '-c', 'tests/test_tate_gamma.c',
             '-o', str(SCR / 'test.o')])
    assert r.returncode == 0, r.stderr
    support = sorted(str(p) for p in (BASE / 'support').glob('*.o'))
    rows, killed = [], 0
    for name, edits in FAULTS:
        text = source
        for old, new in edits:
            assert text.count(old) == 1 and text.index(old) > marker, (name, old)
            text = text.replace(old, new)
        (SCR / 'src').mkdir(exist_ok=True)
        (SCR / 'src/localfactor.c').write_text(text)
        shutil.copy(ROOT / 'src/invariants.h', SCR / 'src/invariants.h')
        shutil.copy(BASE / 'libadelefeld.a', SCR / 'lib.a')
        r = run(['timeout', '120', 'cc', *CFLAGS, '-c', str(SCR / 'src/localfactor.c'),
                 '-o', str(SCR / 'localfactor.o')])
        if r.returncode != 0:
            rows.append((name, 'compile error', r.stderr[:300]))
            continue
        run(['ar', 'r', str(SCR / 'lib.a'), str(SCR / 'localfactor.o')])
        r = run(['timeout', '120', 'cc', str(SCR / 'test.o'), *support, str(SCR / 'lib.a'),
                 '-lflint', '-lgmp', '-lm', '-o', str(SCR / 'test')])
        assert r.returncode == 0, r.stderr
        r = run(['timeout', '170', str(SCR / 'test')])
        m = re.search(r'(\d+) tests, (\d+) checks, (\d+) failed checks', r.stdout)
        failed = m.group(3) if m else f'no summary (exit {r.returncode})'
        tests = sorted(set(re.findall(r'^FAIL (\w+) \(', r.stdout, re.M)))
        if r.returncode != 0:
            killed += 1
        verdict = 'killed' if r.returncode != 0 else 'SURVIVED'
        rows.append((name, verdict, f'{failed} failed; {", ".join(tests)}'))
    shutil.rmtree(SCR)
    for row in rows:
        print(' | '.join(row))
    print(f'{killed} of {len(FAULTS)} faults killed')
    return 0 if killed == len(FAULTS) else 1


if __name__ == '__main__':
    sys.exit(main())
