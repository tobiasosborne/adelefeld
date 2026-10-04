#!/usr/bin/env python3
"""Plant the eight faults of docs/design/local-zeta.md section 4 (:437-450) one at a time in a scratch copy of the
tree and run test_localfactor against each (lane f-slice14, slice F).

The scratch copy is lanes/f-slice14/faults/tree (Makefile, include/, src/, tests/support, tests/test_runner.h,
tests/test_localfactor.c, tests/ref/vectors/f-slice14). Each fault is a textual replacement in src/localfactor.c
that must match exactly once; the build is `make -s -j2 BUILD=build build/test_localfactor` in the copy, the run
`timeout 300 ./build/test_localfactor`. A fault counts as caught when the test exits non-zero; the first failing
assertions are printed. The working tree is never written.
"""
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
TREE = os.path.join(HERE, 'faults', 'tree')

FAULTS = [
    ('1', 'finite poles tested only at k = 0 (the denominator test replaced by a test of s against 0)',
     [('    if (acb_contains_zero(d0)) goto done;\n', '    if (acb_contains_zero(s)) goto done;\n')]),
    ('1+7', 'fault 1 together with fault 7 (without the final finiteness check, 1 alone is masked by it)',
     [('    if (acb_contains_zero(d0)) goto done;\n', '    if (acb_contains_zero(s)) goto done;\n'),
      ('    if (!acb_is_finite(out)) goto done;\n    acb_set_round(out, out, prec);\n'
       '    if (!acb_is_finite(out)) goto done;\n', '    acb_set_round(out, out, prec);\n')]),
    ('2', 'DOMAIN on a positive-radius pole ball (at p: a denominator that contains 0)',
     [('    if (acb_contains_zero(d0)) goto done;\n',
       '    if (acb_contains_zero(d0)) { st = ADF_DOMAIN; goto done; }\n')]),
    ('2r', 'DOMAIN on a positive-radius pole ball (at infinity: the geometry)',
     [('            st = ADF_NOT_DETERMINED;\n            goto done;\n',
       '            st = ADF_DOMAIN;\n            goto done;\n')]),
    ('3', 'the exponent sign reversed, the stable ratio unchanged',
     [('    if (neg) arb_set(b, a);\n    else arb_neg(b, a);\n', '    if (neg) arb_neg(b, a);\n    else arb_set(b, a);\n')]),
    ('4', 'Gamma(s) instead of Gamma(s/2)',
     [('    acb_gamma(g, z, w);\n', '    acb_mul_2exp_si(g, z, 1);\n    acb_gamma(g, g, w);\n')]),
    ('5', 'pi^(-s) instead of pi^(-s/2)',
     [('    acb_mul_arb(pref, z, logpi, w);\n', '    acb_mul_arb(pref, z, logpi, w);\n    acb_mul_2exp_si(pref, pref, 1);\n')]),
    ('6', 'y written before the status is decided',
     [('    if (st == ADF_OK) acb_swap(y, t);\n', '    acb_swap(y, t);\n')]),
    ('7', 'the final finiteness checks omitted (at p both, at infinity the candidate and the rounded value)',
     [('    if (!acb_is_finite(out)) goto done;\n    acb_set_round(out, out, prec);\n'
       '    if (!acb_is_finite(out)) goto done;\n', '    acb_set_round(out, out, prec);\n'),
      ('        if (acb_is_finite(g)) acb_swap(y, g);\n        else st = ADF_NOT_DETERMINED;\n',
       '        acb_swap(y, g);\n'),
      ('    if (!acb_is_finite(cand)) st = ADF_NOT_DETERMINED;\n    else acb_swap(res, cand);\n',
       '    acb_swap(res, cand);\n')]),
    ('8', 'where not written on failure',
     [('    if (where != NULL) *where = v;\n', '    (void) where; (void) v;\n')]),
    ('8b', 'where written on OK',
     [('    if (st == ADF_OK) acb_swap(y, t);\n',
       '    if (st == ADF_OK) acb_swap(y, t);\n    if (st == ADF_OK && where != NULL) *where = v;\n')]),
    ('R1', 'mutation survivor 313: the midpoint refinement never applied (not one of the eight faults)',
     [('    if (!acb_is_exact(s) && n <= ADF_LOCAL_ZETA_SHIFT_MAX && arf_cmp_si(h, 64) <= 0)\n',
       '    if (!(!acb_is_exact(s) && n <= ADF_LOCAL_ZETA_SHIFT_MAX && arf_cmp_si(h, 64) <= 0))\n')]),
    ('R2', 'mutation survivor 329: the real part of the refinement not intersected (not one of the eight faults)',
     [('if (arb_intersection(c, acb_realref(cand), acb_realref(ym), w)) arb_swap',
       'if (!arb_intersection(c, acb_realref(cand), acb_realref(ym), w)) arb_swap')]),
]


def copy_tree():
    if os.path.exists(TREE):
        shutil.rmtree(TREE)
    os.makedirs(os.path.join(TREE, 'tests', 'ref', 'vectors'))
    shutil.copy(os.path.join(ROOT, 'Makefile'), TREE)
    shutil.copytree(os.path.join(ROOT, 'include'), os.path.join(TREE, 'include'))
    shutil.copytree(os.path.join(ROOT, 'src'), os.path.join(TREE, 'src'))
    shutil.copytree(os.path.join(ROOT, 'tests', 'support'), os.path.join(TREE, 'tests', 'support'))
    shutil.copy(os.path.join(ROOT, 'tests', 'test_runner.h'), os.path.join(TREE, 'tests'))
    shutil.copy(os.path.join(ROOT, 'tests', 'test_localfactor.c'), os.path.join(TREE, 'tests'))
    shutil.copytree(os.path.join(ROOT, 'tests', 'ref', 'vectors', 'f-slice14'),
                    os.path.join(TREE, 'tests', 'ref', 'vectors', 'f-slice14'))


def run(name, what, edits, original):
    text = original
    for old, new in edits:
        assert text.count(old) == 1, (name, old)
        text = text.replace(old, new)
    with open(os.path.join(TREE, 'src', 'localfactor.c'), 'w') as f:
        f.write(text)
    b = subprocess.run(['make', '-s', '-j2', 'BUILD=build', 'build/test_localfactor'], cwd=TREE,
                       capture_output=True, text=True)
    if b.returncode != 0:
        print(f'fault {name}: BUILD FAILED\n{b.stderr[-2000:]}')
        return False
    r = subprocess.run(['timeout', '300', './build/test_localfactor'], cwd=TREE, capture_output=True, text=True)
    fails = [ln for ln in r.stdout.splitlines() if ln.startswith('FAIL tests/')]
    tests = [ln for ln in r.stdout.splitlines() if ln.startswith('FAIL ') and not ln.startswith('FAIL tests/')]
    summary = [ln for ln in r.stdout.splitlines() if ' tests, ' in ln]
    caught = r.returncode != 0
    print(f'fault {name}: {what}\n  exit {r.returncode}, caught: {caught}; {summary[-1] if summary else "(no summary)"}')
    for ln in tests:
        print('  ' + ln)
    for ln in fails[:2]:
        print('  first: ' + ln[:220])
    for ln in r.stdout.splitlines():
        if ln.startswith('width target missed') or 'width missed' in ln:
            print('  ' + ln[:220])
    return caught


def main():
    copy_tree()
    with open(os.path.join(ROOT, 'src', 'localfactor.c')) as f:
        original = f.read()
    wanted = sys.argv[1:]
    results = {}
    for name, what, edits in FAULTS:
        if wanted and name not in wanted:
            continue
        results[name] = run(name, what, edits, original)
    with open(os.path.join(TREE, 'src', 'localfactor.c'), 'w') as f:
        f.write(original)
    print('caught:', sum(results.values()), 'of', len(results), results)


if __name__ == '__main__':
    main()
