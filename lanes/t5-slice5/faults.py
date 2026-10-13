#!/usr/bin/env python3
"""The six faults of design section 7 for 5.4 (docs/api-5.md:346-348), planted one at a time in scratch copies of
the test's side evaluation (fe_sides of tests/test_tate_fe.c) and of the driver command (adf_drv_tate_fe of
tools/adf/adf.c). Each must make the test fail (exit status not 0) and the fixture tests/driver/tate-fe differ.

    timeout 1500 python3 -B lanes/t5-slice5/faults.py      (needs lanes/t5-slice5/build/libadelefeld.a and the
                                                           support objects; writes lanes/t5-slice5/faults.out)

For each fault: the full test (the first failing line), the section `values` alone (the acceptance cases: both
sides against each other and against the FLINT and mpmath references), and for the copy the section
`independence` (the sentinel test, api-5.md:350-352); then the driver fixture."""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
B = os.path.join(HERE, 'build')
S = os.path.join(HERE, 'scratch')
CFLAGS = ['-std=c11', '-O2', '-g', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-Iinclude', '-Itests']
LIBS = [os.path.join(B, 'support', 'jsonl.o'), os.path.join(B, 'support', 'golden.o'),
        os.path.join(B, 'libadelefeld.a'), '-lflint', '-lgmp', '-lm']

# (name, the test's text, its replacement, the driver's text, its replacement)
FAULTS = [
    ('no conjugate: B = Lambda(1 - s, chi)',
     'st = adf_tate_completed(B, cc, t, bits + 4, prec);', 'st = adf_tate_completed(B, chi, t, bits + 4, prec);',
     'status = adf_tate_completed(B, cx, t, bits + 4, st->prec);',
     'status = adf_tate_completed(B, x, t, bits + 4, st->prec);'),
    ('inverse W: R = B / W',
     'acb_mul(R, W, B, wp);', 'acb_div(R, B, W, wp);',
     'acb_mul(R, W, B, wp);', 'acb_div(R, B, W, wp);'),
    ('negative tau: R = (-tau/(i^e sqrt C)) B',
     'acb_mul(R, W, B, wp);', 'acb_mul(R, W, B, wp); acb_neg(R, R);',
     'acb_mul(R, W, B, wp);', 'acb_mul(R, W, B, wp); acb_neg(R, R);'),
    ('incomplete completion: B = I(1 - s, conj chi) without C^z\'',
     'st = adf_tate_completed(B, cc, t, bits + 4, prec);', 'st = adf_tate_continue(B, cc, t, bits + 4, prec);',
     'status = adf_tate_completed(B, cx, t, bits + 4, st->prec);',
     'status = adf_tate_continue(B, cx, t, bits + 4, st->prec);'),
    ('-s instead of 1 - s',
     'acb_neg(t, s); acb_add_ui(t, t, 1, prec);', 'acb_neg(t, s);',
     'acb_neg(t, s); acb_add_ui(t, t, 1, st->prec);', 'acb_neg(t, s);'),
    ('one side copied from the other: R = A',
     'acb_mul(R, W, B, wp);', 'acb_set(R, A);',
     'acb_mul(R, W, B, wp);', 'acb_set(R, A);'),
]


def run(cmd, **kw):
    return subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, **kw)


def plant(src, old, new, region_start):
    """Replace old by new once, inside the function that starts at region_start."""
    i = src.index(region_start)
    j = src.index('\n}\n', i)
    body = src[i:j]
    assert body.count(old) == 1, (old, region_start)
    return src[:i] + body.replace(old, new) + src[j:]


def first_failure(text):
    m = re.search(r'(test_tate_fe[^:\n]*\.c:\d+: [^\n]*)', text)
    return m.group(1) if m else (text.strip().splitlines() or ['(no output)'])[-1][:200]


def main():
    os.makedirs(S, exist_ok=True)
    test = open(os.path.join(ROOT, 'tests', 'test_tate_fe.c')).read()
    drv = open(os.path.join(ROOT, 'tools', 'adf', 'adf.c')).read()
    cmd = os.path.join(ROOT, 'tests', 'driver', 'tate-fe.cmd')
    want = open(os.path.join(ROOT, 'tests', 'driver', 'tate-fe.out')).read()
    lines = []
    detected = 0
    for k, (name, told, tnew, dold, dnew) in enumerate(FAULTS, 1):
        src = plant(test, told, tnew, 'static int fe_sides(')
        path = os.path.join(S, f'test_tate_fe_f{k}.c')
        exe = os.path.join(S, f'test_f{k}')
        open(path, 'w').write(src)
        r = run(['cc'] + CFLAGS + [path] + LIBS + ['-o', exe])
        assert r.returncode == 0, r.stderr
        full = run(['timeout', '300', exe])
        vals = run(['timeout', '300', exe, 'values'])
        extra = run(['timeout', '300', exe, 'independence']) if 'copied' in name else None
        dsrc = plant(drv, dold, dnew, 'adf_drv_tate_fe(FILE *out')
        dpath = os.path.join(S, f'adf_f{k}.c')
        dexe = os.path.join(S, f'adf_f{k}')
        open(dpath, 'w').write(dsrc)
        r = run(['cc'] + CFLAGS + [dpath, os.path.join(B, 'libadelefeld.a'), '-lflint', '-lgmp', '-lm', '-o', dexe])
        assert r.returncode == 0, r.stderr
        with open(cmd) as fh:
            d = run(['timeout', '300', dexe], stdin=fh)
        got = d.stdout
        diff = [i + 1 for i, (a, b) in enumerate(zip(want.splitlines(), got.splitlines())) if a != b]
        ok = full.returncode != 0 and got != want
        detected += ok
        lines.append(f'{k}. {name}')
        lines.append(f'   test (full): exit {full.returncode}: {first_failure(full.stderr)}')
        lines.append(f'   test values: exit {vals.returncode}: {first_failure(vals.stderr)}')
        if extra is not None:
            lines.append(f'   test independence: exit {extra.returncode}: {first_failure(extra.stderr)}')
        lines.append(f'   driver fixture: {"differs" if got != want else "EQUAL"}, output lines {diff}')
        for p in (path, exe, dpath, dexe):
            os.remove(p)
    lines.append(f'{detected} of {len(FAULTS)} faults detected by both the test and the driver fixture')
    out = '\n'.join(x if len(x) <= 116 else x[:113] + '...' for x in lines) + '\n'
    open(os.path.join(HERE, 'faults.out'), 'w').write(out)
    sys.stdout.write(out)


if __name__ == '__main__':
    main()
