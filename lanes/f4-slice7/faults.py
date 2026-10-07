#!/usr/bin/env python3
"""Named faults in scratch copies only. Each child build and test has its own timeout."""
from pathlib import Path
import json
import os
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'lanes/f4-slice7/mutroot'
source = (OUT / 'src/dump.c').read_text()
unchanged = (OUT / 'src/dump_unchanged.h').read_text()
env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0:abort_on_error=1', UBSAN_OPTIONS='halt_on_error=1')
results = []


def replace(text, old, new):
    assert old in text, old
    return text.replace(old, new, 1)


faults = []
faults.append(('reorder terms', 'rfun', replace(source,
    'st = adf_rfun_is_canonical(&out) ? ADF_OK : ADF_DOMAIN;',
    'if (out.len > 1) { adf_rterm_struct q = out.term[0]; '
    'out.term[0] = out.term[out.len-1]; out.term[out.len-1] = q; }\n'
    '    st = adf_rfun_is_canonical(&out) ? ADF_OK : ADF_DOMAIN;'), unchanged, True))
faults.append(('trim exact trailing zero', 'rfun', replace(replace(source,
    'dp_zero(a[1].m) && dp_zero(a[1].rm)) return ADF_DOMAIN;',
    'dp_zero(a[1].m) && dp_zero(a[1].rm)) { /* faulty acceptance */ }'),
    '_acb_poly_set_length(t->P, (slong) L);',
    '_acb_poly_set_length(t->P, (slong) L); _acb_poly_normalise(t->P);'), unchanged, True))
faults.append(('FLINT before byte validation', 'ffun', replace(source,
    'int st = dp_validate(P, s, n, lim, kind);',
    'arb_t bad; int st; arb_init(bad); (void) arb_load_str(bad, s); arb_clear(bad);\n'
    '    st = dp_validate(P, s, n, lim, kind);'), unchanged, True))
faults.append(('count cap after reading values', 'ffun', replace(replace(source,
    'D > ADF_FFUN_ITEMS_MAX / M', 'D > UWORD_MAX / M'),
    'st = adf_ffun_is_canonical(&out) ? ADF_OK : ADF_DOMAIN;',
    'if (count > ADF_FFUN_ITEMS_MAX) { adf_ffun_clear(&out); return ADF_LIMIT; }\n'
    '    st = adf_ffun_is_canonical(&out) ? ADF_OK : ADF_DOMAIN;'), unchanged, False))
badgrammar = replace(unchanged, 'bad = fmpz_cmp_ui(M, (ulong) c->left) > 0;', 'bad = 0;')
badgrammar = replace(badgrammar, 'if (mode == DP_SYNTAX && c.left != 0)', 'if (0)')
faults.append(('D M count mismatch ignored', 'ffun', source, badgrammar, True))
faults.append(('Re(A) predicate omitted', 'rfun', replace(replace(source,
    'if (j == L && dp_arb_sign(a) != 1) return ADF_DOMAIN;', 'if (0) return ADF_DOMAIN;'),
    'st = adf_rfun_is_canonical(&out) ? ADF_OK : ADF_DOMAIN;', 'st = ADF_OK;'), unchanged, True))
faults.append(('cap off by one', 'ffun', replace(source,
    'D > ADF_FFUN_ITEMS_MAX / M', 'D > (ADF_FFUN_ITEMS_MAX - 1) / M'), unchanged, False))
faults.append(('x written before final validation', 'ffun', replace(source,
    'int st = dp_fn_validate(&P, DP_FFUN, s, n, lim);',
    'int st; acb_one(x->f); st = dp_fn_validate(&P, DP_FFUN, s, n, lim);'), unchanged, True))
faults.append(('nctx not written on OK', 'ffun', replace(source,
    'if (st == ADF_OK) *nctx = 0;', 'if (st == ADF_OK) { /* omitted write */ }'), unchanged, True))

try:
    for name, kind, text, prefix, quick in faults:
        (OUT / 'src/dump.c').write_text(text)
        (OUT / 'src/dump_unchanged.h').write_text(prefix)
        command = ['timeout', '90', 'make', '-s', '-j2', 'SAN=1', 'INV=1', 'test_' + kind + '_dump']
        built = subprocess.run(command, cwd=OUT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if built.returncode:
            print(name, 'BUILD FAILED', built.stdout.decode()[-1000:], flush=True)
            results.append({'fault': name, 'build': built.returncode, 'test': None})
            continue
        command = ['timeout', '90', './test_' + kind + '_dump'] + (['--quick'] if quick else [])
        test = subprocess.run(command, cwd=OUT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        output = test.stdout.decode(errors='replace')
        print(name, 'test exit', test.returncode, '|', ' | '.join(output.splitlines()[-3:]), flush=True)
        results.append({'fault': name, 'build': 0, 'test': test.returncode,
                        'witness': output.splitlines()[-1] if output.splitlines() else ''})
finally:
    (OUT / 'src/dump.c').write_text(source)
    (OUT / 'src/dump_unchanged.h').write_text(unchanged)
    (ROOT / 'lanes/f4-slice7/fault-results.json').write_text(json.dumps(results, indent=2) + '\n')
sys.exit(any(r['build'] or r['test'] in (None, 0, 124) for r in results))
