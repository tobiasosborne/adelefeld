#!/usr/bin/env python3
"""lanes/q-review2/mutate.py: plant ten faults of my own in a scratch copy of src/qclass.c and of
the qclass part of src/text.c, build tests/test_qclass.c against each, and record which are caught.

Nothing outside lanes/q-review2/mut is written. Run: timeout 170 python3 lanes/q-review2/mutate.py
"""
import os
import subprocess
import sys

ROOT = os.path.abspath('.')
MUT = 'lanes/q-review2/mut'
CC = ['cc', '-Iinclude', '-Isrc', '-Itests', '-std=c11', '-O1', '-g', '-Wall', '-Wextra']

FAULTS = [
    ('F1 midpoint test >= 0 made > 0', 'qclass', [
        ('arf_sgn(arb_midref(a->inf)) < 0 ||', 'arf_sgn(arb_midref(a->inf)) <= 0 ||')]),
    ('F2 order compared on the first key only', 'qclass', [
        ('            if (!c) c = end_cmp(x->piece[i-1].inf, a->inf, 1);\n'
         '            if (!c) c = fmpz_cmp(prevH, H);\n'
         '            if (!c) c = fmpz_cmp(prevA, A);\n', '')]),
    ('F3 the duplicate test dropped', 'qclass', [
        ('if (c >= 0) { ok = 0; break; }', 'if (c > 0) { ok = 0; break; }')]),
    ('F4 set copies len - 1 pieces', 'qclass', [
        ('for (i = 0; i < t.len; i++) {', 'for (i = 0; i + 1 < t.len; i++) {')]),
    ('F5 add_rat translates the real part only', 'qclass', [
        ('    (void) q;\n    adf_qclass_set(y, x);\n}',
         '    adf_qclass_set(y, x);\n'
         '    { arb_t t; arb_init(t);\n'
         '      arb_set_fmpq(t, q->q, 53);\n'
         '      arb_add(y->piece[0].inf, y->piece[0].inf, t, 53);\n'
         '      arb_clear(t); }\n}')]),
    ('F6 set_rat gives the lift of a non-integer', 'qclass', [
        ('    (void) q;\n    adf_qclass_init(t);\n    adf_qclass_swap(y, t);',
         '    adf_qclass_init(t);\n'
         '    if (!fmpq_is_one(q->q)) adf_adele_set_rat(t->piece, q, 53);\n'
         '    adf_qclass_swap(y, t);')]),
    ('F7 the union refused before the syntax check', 'text', [
        ('    is_union = TX_KW(&c, "union");\n',
         '    is_union = TX_KW(&c, "union");\n    if (is_union) return ADF_UNSUPPORTED;\n')]),
    ('F8 the printer omits " + Q" when the radius is zero', 'text', [
        ('    tx_puts(&b, " + Q");',
         '    if (!mag_is_zero(arb_radref(x->piece[0].inf))) tx_puts(&b, " + Q");')]),
    ('F9 get_piece accepts i == len', 'qclass', [
        ('    if (i < 0 || i >= x->len) return ADF_DOMAIN;', '    if (i < 0 || i > x->len) return ADF_DOMAIN;')]),
    ('F10 swap exchanges two of the three fields', 'qclass', [
        ('    adf_qclass_struct t = *x;\n    *x = *y;\n    *y = t;',
         '    int f = x->form; slong l = x->len;\n'
         '    x->form = y->form; x->len = y->len;\n'
         '    y->form = f; y->len = l;')]),
    ('F11 the d = 1 clause dropped', 'qclass', [
        ('if (!fmpz_is_one(d) || fmpz_sgn(H) < 0 ||', 'if (fmpz_sgn(H) < 0 ||')]),
    ('F12 the H >= 0 clause dropped', 'qclass', [
        ('if (!fmpz_is_one(d) || fmpz_sgn(H) < 0 ||\n'
         '            (fmpz_sgn(H) > 0 && (fmpz_sgn(A) < 0 || fmpz_cmp(A, H) >= 0))) { ok = 0; break; }',
         'if (!fmpz_is_one(d)) { ok = 0; break; }')]),
    ('F13 the centre range of 5.2 dropped', 'qclass', [
        ('(fmpz_sgn(H) > 0 && (fmpz_sgn(A) < 0 || fmpz_cmp(A, H) >= 0))) { ok = 0; break; }',
         '0) { ok = 0; break; }')]),
    ('F14 the upper end compared before the lower end', 'qclass', [
        ('            c = end_cmp(x->piece[i-1].inf, a->inf, 0);\n'
         '            if (!c) c = end_cmp(x->piece[i-1].inf, a->inf, 1);',
         '            c = end_cmp(x->piece[i-1].inf, a->inf, 1);\n'
         '            if (!c) c = end_cmp(x->piece[i-1].inf, a->inf, 0);')]),
    ('F15 the midpoint test accepts 1 + 2^-60', 'qclass', [
        ('arf_cmp_ui(arb_midref(a->inf), 1) > 0', 'arf_cmp_ui(arb_midref(a->inf), 1) >= 0')]),
    ('F16 identical compares the first piece only', 'qclass', [
        ('    for (i = 0; i < x->len; i++)\n'
         '        if (!adf_adele_identical(x->piece + i, y->piece + i)) return 0;',
         '    if (!adf_adele_identical(x->piece, y->piece)) return 0;')]),
    ('F17 set_adele shares the member instead of copying', 'qclass', [
        ('    adf_qclass_init(t);\n    adf_adele_set(t->piece, x);',
         '    adf_qclass_init(t);\n    adf_adele_swap(t->piece, (adf_adele_ptr) x);')]),
    ('F18 the reader drops the stage 6 finite check', 'text', [
        ('    if (tx_fin_domain(s, &f)) return ADF_DOMAIN;\n'
         '    adf_adele_init(a);',
         '    adf_adele_init(a);')]),
    ('F19 the union count check after the refusal', 'text', [
        ('    if (over || (is_union && (lim->max_items < 1 || count > (size_t) lim->max_items)))\n'
         '        return ADF_LIMIT;\n    if (is_union) return ADF_UNSUPPORTED;',
         '    if (is_union) return ADF_UNSUPPORTED;\n'
         '    if (over) return ADF_LIMIT;')]),
    ('F20 the printer never writes " + Q"', 'text', [
        ('    tx_puts(&b, " + Q");', '    /* no marker */')]),
]


def run(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, timeout=160, **kw)


def main():
    os.makedirs(MUT, exist_ok=True)
    src = {'qclass': 'src/qclass.c', 'text': 'src/text.c'}
    print("%-52s %-8s %-8s %s" % ("fault", "build", "test", "note"))
    rows = []
    for name, which, patches in FAULTS:
        path = os.path.join(MUT, name.split()[0] + '_' + which + '.c')
        text = open(src[which]).read()
        ok = True
        for a, b in patches:
            if a not in text:
                print("%-52s PATCH NOT FOUND: %r" % (name, a[:40]))
                ok = False
                break
            text = text.replace(a, b, 1)
        if not ok:
            rows.append((name, 'PATCHFAIL', '-', ''))
            continue
        open(path, 'w').write(text)
        obj = path[:-2] + '.o'
        r = run(CC + ['-c', path, '-o', obj])
        if r.returncode != 0:
            print("%-52s COMPILE FAIL" % name)
            print(r.stderr[:600])
            rows.append((name, 'CFAIL', '-', r.stderr.strip().split('\n')[0][:60]))
            continue
        exe = os.path.join(MUT, 'test_' + name.split()[0])
        r = run(CC + ['tests/test_qclass.c', 'lanes/q-review2/build/support/golden.o',
                      'lanes/q-review2/build/support/jsonl.o', obj,
                      'lanes/q-review2/build/libadelefeld.a', '-lflint', '-lgmp', '-lm', '-o', exe])
        if r.returncode != 0:
            print("%-52s LINK FAIL" % name)
            rows.append((name, 'LFAIL', '-', r.stderr.strip().split('\n')[0][:60]))
            continue
        r = run(['timeout', '150', exe])
        note = r.stdout.strip().split('\n')[-1][:70] if r.stdout.strip() else ''
        killed = r.returncode != 0
        print("%-52s %-8s %-8s %s" % (name, 'ok', 'FAIL' if killed else 'PASS',
                                      note if not killed else r.stdout.strip().split('\n')[-1][:70]))
        rows.append((name, 'ok', 'FAIL' if killed else 'PASS', note))
    print()
    print("survivors:", [n for n, b, t, _ in rows if t == 'PASS'])
    return 0


if __name__ == '__main__':
    sys.exit(main())
