"""Plant the faults of brief step F in scratch copies; each must make test_qclass_arith fail.

Run from the worktree root: timeout 1500 python3 lanes/q-slice7/plant_faults.py SCRATCH
A scratch copy of Makefile, include, src, tests and the lane build tree (timestamps kept) is made
once; each fault replaces one exact text of src/qclass_arith.c, rebuilds the test, runs it under
timeout 120, and restores the file. Results go to lanes/q-slice7/fault-results.tsv.
"""
import shutil
import subprocess
import sys
from pathlib import Path

root = Path.cwd()
scratch = Path(sys.argv[1])
SRC = 'src/qclass_arith.c'

FAULTS = [
    ('gcd-lcm', 'wrong finite gcd: lcm of the radii',
     '    fmpq_gcd(g, x, y);\n',
     '    fmpq_gcd(g, x, y);\n    if (fmpq_is_zero(x) || fmpq_is_zero(y)) fmpq_zero(g);\n'
     '    else { fmpq_t p; fmpq_init(p); fmpq_mul(p, x, y); fmpq_div(g, p, g); fmpq_abs(g, g);'
     ' fmpq_clear(p); }\n'),
    ('gcd-N-alone', 'wrong finite gcd: the radius of x alone',
     'qa_gcd(s->N->q, s->N->q, s->N2->q);', 'qa_gcd(s->N->q, s->N->q, s->N->q);'),
    ('neg-shift-sign', 'sign of the finite shift under negation',
     ' fmpq_neg(s->a->q, s->a->q);\n', '\n'),
    ('early-real-round', 'arb_add of the real balls at 53 bits before R',
     '    return qa_add(s->lo, s->lo, s->lo2, 0) && qa_add(s->hi, s->hi, s->hi2, 0) &&\n',
     '    { arb_t t; arf_t r; fmpq_t m; arb_init(t); arf_init(r); fmpq_init(m);\n'
     '      arb_add(t, x->piece[k/y->len].inf, u->inf, 53); arf_get_fmpq(m, arb_midref(t));\n'
     '      arf_set_mag(r, arb_radref(t)); arf_get_fmpq(s->hi, r); fmpq_sub(s->lo, m, s->hi);\n'
     '      fmpq_add(s->hi, m, s->hi); arb_clear(t); arf_clear(r); fmpq_clear(m); }\n'
     '    return 1 &&\n'),
    ('count-after-dedup', 'limit compared with the count after deduplication',
     '                if (fmpz_cmp_si(total, piece_limit) > 0) goto done;\n'
     '                if (pass) {',
     '                if (pass) {'),
    ('count-after-dedup-b', '(second half of the same fault, applied together)',
     '    out.len = keep; adf_qclass_swap(z, &out);',
     '    out.len = keep; if (keep > piece_limit) goto done; adf_qclass_swap(z, &out);'),
    ('z-written-early', 'z written before the last allocation',
     '    for (pass = 0; pass < 2; pass++) {\n',
     '    z->form = ADF_QCLASS_PIECES;\n    for (pass = 0; pass < 2; pass++) {\n'),
    ('neg-ends-not-reversed', 'end points not reversed under negation',
     'fmpq_neg(s->lo, s->hi2); fmpq_neg(s->hi, s->lo2);',
     'fmpq_neg(s->lo, s->lo2); fmpq_neg(s->hi, s->hi2);'),
    ('pairs-len-x', 'pair loop over len(x) only',
     'return qa_construct(z, x, y, x->len*y->len, piece_limit, prec);',
     'return qa_construct(z, x, y, x->len, piece_limit, prec);'),
    ('pairs-index', 'pair index uses len(x) for the row (k/len(x))',
     'u = x->piece+k/y->len;', 'u = x->piece+k/x->len;'),
]
# Faults applied together (one fault in two places).
GROUPS = [['gcd-lcm'], ['gcd-N-alone'], ['neg-shift-sign'], ['early-real-round'],
          ['count-after-dedup', 'count-after-dedup-b'], ['z-written-early'],
          ['neg-ends-not-reversed'], ['pairs-len-x'], ['pairs-index']]

if scratch.exists():
    shutil.rmtree(scratch)
scratch.mkdir(parents=True)
for d in ('include', 'src', 'tests'):
    shutil.copytree(root/d, scratch/d, symlinks=True)
shutil.copy2(root/'Makefile', scratch/'Makefile')
shutil.copytree(root/'lanes/q-slice7/build', scratch/'build', symlinks=True)
orig = (scratch/SRC).read_text()
byname = {f[0]: f for f in FAULTS}
rows = []
for group in GROUPS:
    text = orig
    for name in group:
        _, what, old, new = byname[name]
        assert text.count(old) == 1, (name, text.count(old))
        text = text.replace(old, new)
    (scratch/SRC).write_text(text)
    b = subprocess.run(['timeout', '600', 'make', '-s', '-j2', 'BUILD=build', 'build/test_qclass_arith'],
                       cwd=scratch, capture_output=True, text=True)
    if b.returncode:
        rows.append((group[0], byname[group[0]][1], 'not compiled', b.stderr.strip()[-200:]))
        continue
    r = subprocess.run(['timeout', '120', './build/test_qclass_arith'], cwd=scratch,
                       capture_output=True, text=True)
    err = (r.stderr.strip().splitlines() or [''])[0][:150]
    rows.append((group[0], byname[group[0]][1], 'rejected' if r.returncode else 'SURVIVED',
                 f'exit {r.returncode}: {err}'))
    print(rows[-1], flush=True)
(scratch/SRC).write_text(orig)
out = root/'lanes/q-slice7/fault-results.tsv'
out.write_text('fault\twhat\tresult\tfirst failure\n' +
               ''.join('\t'.join(r)+'\n' for r in rows))
print(f'{sum(r[2] == "rejected" for r in rows)} of {len(rows)} rejected')
