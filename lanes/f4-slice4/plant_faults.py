#!/usr/bin/env python3
"""Bounded named faults in scratch source copies. The working source is never changed."""
import argparse
import json
import os
from pathlib import Path
import subprocess

p = argparse.ArgumentParser()
p.add_argument('--san', action='store_true')
args = p.parse_args()
lane = Path('lanes/f4-slice4')
scratch = lane / 'faults'
scratch.mkdir(exist_ok=True)
base = lane / ('combined' if args.san else 'build')
original = Path('src/ffun.c').read_text()
start = original.index('/* Slice 4b.')
head, body = original[:start], original[start:]


def replace(old, new, group='all', whole=False):
    text = original if whole else body
    assert old in text, old
    return (text.replace(old, new, 1) if whole else head + text.replace(old, new, 1)), group


faults = {}
for dim in ['D', 'M']:
    call = f'ffun_lcm(&{dim},x->{dim},y->{dim})'
    added = '''static int fault_max(ulong *out, ulong a, ulong b)
{
    *out=FLINT_MAX(a,b); return *out>ADF_FFUN_ITEMS_MAX ? ADF_LIMIT : ADF_OK;
}
'''
    mutated, group = replace(call, f'fault_max(&{dim},x->{dim},y->{dim})', 'mul')
    faults['max_' + dim] = (mutated.replace('int adf_ffun_mul(', added + 'int adf_ffun_mul(', 1), group)
faults['wrong_translation'] = replace('(k+L-shift)%L', '(k+shift)%L', 'translate')
faults['refinement_holes'] = replace('if (k%r==0) acb_set', 'if (1) acb_set', 'all', whole=True)
faults['wrong_reflection'] = replace('(k ? L-k : 0)', '(k ? L-k-1 : 0)', 'reflect')
faults['negative_without_reflection'] = replace('int negative=fmpq_sgn(q)<0;', 'int negative=0;', 'dilate')
faults['dilation_holes'] = replace('if (k%t==0) {', 'if (1) {', 'dilate')
mutated, group = replace('if (++count>1) return ADF_NOT_DETERMINED;', '++count;', 'idele')
mutated = mutated.replace('if (count!=1) return ADF_NOT_DETERMINED;',
                          'if (count==0) return ADF_NOT_DETERMINED;', 1)
faults['ambiguous_unit_accepted'] = (mutated, group)
faults['unit_ignored'] = replace('ffun_dilate_cells(t,x,a->r,unit);',
                                'ffun_dilate_cells(t,x,a->r,1); (void)unit;', 'idele')
faults['conjugate_omitted'] = replace('acb_conj(t->f+k,x->f+k);', 'acb_set(t->f+k,x->f+k);', 'conj')
faults['early_output_write'] = replace('if (!enumerate) unit=fmpz_fdiv_ui(a->u.c,L);',
                                      'adf_ffun_set(y,x);\n    if (!enumerate) unit=fmpz_fdiv_ui(a->u.c,L);',
                                      'idele')
faults['mul_radius_dropped'] = replace('acb_mul(t->f+k,a,b,p);',
                                      'acb_mul(t->f+k,a,b,p); acb_get_mid(t->f+k,t->f+k);', 'mul')
# Invert q/content inside their own scratch public functions; every successful temporary is cleared.
for name, function, argument, group in [('q_inverted', 'adf_ffun_dilate_rat', 'q->q', 'dilate'),
                                       ('content_inverted', 'adf_ffun_dilate_idele', 'a->r', 'idele')]:
    text = body
    a = text.index('int ' + function + '(')
    b = text.index('\n}', a) + 2
    block = text[a:b]
    block = block.replace('    adf_ffun_t t;',
                          '    adf_ffun_t t; fmpq_t inverse; fmpq_init(inverse);\n' +
                          f'    fmpq_inv(inverse,{argument});', 1)
    block = block.replace('x,' + argument, 'x,inverse')
    block = block.replace('adf_ffun_swap(y,t);', 'fmpq_clear(inverse); adf_ffun_swap(y,t);', 1)
    faults[name] = (head + text[:a] + block + text[b:], group)

if args.san:
    faults['missing_idele_output_entry'] = replace(
        'ADF_INV_FFUN(x); ADF_INV_FFUN(y); ADF_FFUN_INV_IDELE(a);',
        'ADF_INV_FFUN(x); ADF_FFUN_INV_IDELE(a);', 'inv')

if args.san:
    faults['fourier_before_array_read'] = replace('acb_is_finite(t->f+k)) { st=ADF_NOT_DETERMINED;',
                                                 'acb_is_finite(t->f-k)) { st=ADF_NOT_DETERMINED;',
                                                 'guard', whole=True)
    faults['identity_skips_cell_zero'] = replace(
        'for (ulong j=0;j<x->D*x->M;j++) if (!acb_equal(x->f+j,y->f+j)) return 0;',
        'for (ulong j=1;j<x->D*x->M;j++) if (!acb_equal(x->f+j,y->f+j)) return 0;', 'mul', whole=True)
    faults['lcm_overflow_preflight'] = replace('a>ADF_FFUN_ITEMS_MAX/b',
                                               'a>ADF_FFUN_ITEMS_MAX*b', 'caps', whole=True)

results = []
env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
flags = ['-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-Iinclude', '-Isrc', '-Itests']
if args.san:
    flags += ['-DADF_CHECK_INVARIANTS', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
for name, (source, group) in faults.items():
    path = scratch / (name + '.c')
    path.write_text(source)
    binary = scratch / name
    command = ['timeout', '30', 'clang' if args.san else 'cc', *flags,
               'tests/test_ffun_algebra.c', str(path),
               *[str(q) for q in sorted((base / 'support').glob('*.o'))], str(base / 'libadelefeld.a'),
               '-lflint', '-lgmp', '-lm', '-pthread', '-o', str(binary)]
    built = subprocess.run(command, capture_output=True, text=True)
    record = dict(name=name, compile=built.returncode, group=group)
    if built.returncode:
        record['diagnostic'] = built.stderr[-1500:]
    else:
        run = subprocess.run(['timeout', '30', str(binary), group], env=env, capture_output=True, text=True)
        record['run'] = run.returncode
        record['diagnostic'] = run.stderr[-800:]
    results.append(record)
    print(name, record['compile'], record.get('run', 'not run'), flush=True)
(lane / ('fault-results-san.json' if args.san else 'fault-results.json')).write_text(
    json.dumps(results, indent=2) + '\n')
assert all(v['compile'] == 0 and v['run'] not in (0, 124) for v in results), results
print('killed', len(results), 'compiled', len(results))
