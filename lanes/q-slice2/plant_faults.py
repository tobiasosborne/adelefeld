"""Plant the named faults in scratch source files; never mutate the working source."""
from pathlib import Path
import json
import subprocess
import time
import argparse

root = Path(__file__).resolve().parents[2]
lane = root/'lanes/q-slice2'
parser = argparse.ArgumentParser()
parser.add_argument('--survivors', action='store_true')
parser.add_argument('--printer', action='store_true')
parser.add_argument('--name')
parser.add_argument('--build', default='build', choices=['build', 'clang-san-inv'])
parser.add_argument('--cc', default='cc', choices=['cc', 'clang'])
parser.add_argument('--san', action='store_true')
parser.add_argument('--inv', action='store_true')
args = parser.parse_args()
out = lane/('faults-saninV' if args.san else 'faults')
out.mkdir(exist_ok=True)
source = (root/'src/qclass.c').read_text()
faults = [
    ('D1_drop_last_piece', 'if (pass) {', 'if (pass) { fmpz_sub_ui(stop, stop, 1);'),
    ('D2_upper_singleton',
     'else fmpz_cdiv_q(stop, fmpq_numref(h), fmpq_denref(h));',
     'else { fmpz_fdiv_q(stop, fmpq_numref(h), fmpq_denref(h)); fmpz_add_ui(stop, stop, 1); }'),
    ('D3_wrong_boundary_carry', 'fmpz_neg(fin, n); if (!fmpz_is_zero(A))',
     'if (fmpz_is_one(n)) fmpz_set(fin, n);\n'
     '                        else fmpz_neg(fin, n);\n'
     '                        if (!fmpz_is_zero(A))'),
    ('D4_one_fractional_fibre', 'fibres = fmpz_get_si(B);', 'fibres = 1;'),
    ('D5_refinement_analogue', 'fmpz_set(A, fmpq_numref(N->q));',
     'fmpz_set(A, fmpq_numref(N->q)); if (!fmpz_is_zero(A)) fmpz_mul_2exp(A, A, 1);'),
    ('D6_finite_point_widened', 'fmpz_set(A, fmpq_numref(N->q));',
     'fmpz_set(A, fmpq_numref(N->q)); if (fmpz_is_zero(A)) fmpz_one(A);'),
    ('O1_count_k', 'fmpz_sub(count, stop, first);',
     'fmpz_sub(count, stop, first); fmpz_sub_ui(count, count, 1);'),
    ('O2_no_finite_shift', 'fmpz_neg(fin, n);', 'fmpz_zero(fin);'),
    ('O3_radius_nearest', 'arf_div(u, u, den, 30, ARF_RND_CEIL);',
     'arf_div(u, u, den, 30, ARF_RND_NEAR);'),
    ('O4_limit_after_allocation', 'if (fmpz_cmp_si(total, piece_limit) > 0) goto done;',
     'if (pass && fmpz_cmp_si(total, piece_limit) > 0) goto done;'),
    ('spill_clipped',
     'if (!q_read(lo, hi, a, N, x->piece[i].inf, &x->piece[i].fin)) goto done;',
     'if (!q_read(lo, hi, a, N, x->piece[i].inf, &x->piece[i].fin)) goto done;\n'
     '            if (x->form == ADF_QCLASS_PIECES) {\n'
     '                if (fmpq_sgn(lo) < 0) fmpq_zero(lo);\n'
     '                if (fmpq_cmp(hi, one) > 0) fmpq_one(hi);\n'
     '            }'),
    ('S1_equal_denominator_cap', '|| xd > ADF_QCLASS_BITS_MAX)', '|| xd >= ADF_QCLASS_BITS_MAX)'),
    ('S2_denominator_leading_bit', '(e >= b ? 1 : b-e+1)', '(e >= b ? 1 : b-e+0)'),
    ('S3_denominator_cap_strict',
     '(e >= b ? 1 : b-e+1) <= ADF_QCLASS_BITS_MAX',
     '(e >= b ? 1 : b-e+1) < ADF_QCLASS_BITS_MAX'),
    ('S4_remaining_not_subtracted', 'fmpz_sub(remaining, remaining, total);', ''),
]
if args.survivors:
    faults = [f for f in faults if f[0].startswith('S')]
if args.printer:
    source = (root/'src/text.c').read_text()
    faults = [('P1_printed_order', 'qsort(p, (size_t) x->len, sizeof(*p), tq_print_cmp);', '')]
if args.name:
    faults = [f for f in faults if f[0] == args.name]
    assert len(faults) == 1
objects = sorted((lane/args.build).glob('*.o'))
objects = [str(p) for p in objects if p.name != 'qclass.o']
if args.printer:
    objects = [p for p in objects if Path(p).name != 'text.o']
objects += [str(p) for p in sorted((lane/args.build/'support').glob('*.o'))]
flags = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if args.san else []
if args.inv:
    flags += ['-DADF_CHECK_INVARIANTS']
results = []
for name, old, new in faults:
    assert source.count(old) == 1, name
    mutated = source.replace(old, new)
    if name == 'O4_limit_after_allocation':
        at = 'for (i = 0; i < out.len; i++) adf_adele_init(out.piece+i);'
        mutated = mutated.replace(at, at+'\n            if (fmpz_cmp_si(total, piece_limit) > 0) goto done;')
    src, obj, exe = out/f'{name}.c', out/f'{name}.o', out/name
    src.write_text(mutated)
    test_src = out/f'test_{name}.c'
    include = str(root/'src/qclass.c') if args.printer else f'{name}.c'
    test_src.write_text((root/'tests/test_qclass_reduce.c').read_text().replace(
        '#include "../src/qclass.c"', f'#include "{include}"'))
    commands = [
        ['timeout', '30', args.cc, '-Iinclude', '-Isrc', '-std=c11', '-O2', '-g', *flags,
         '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-c', str(src), '-o', str(obj)],
        ['timeout', '30', args.cc, '-Iinclude', '-Itests', '-Isrc', '-std=c11', '-O2', '-g', *flags,
         str(test_src), *([str(obj)] if args.printer else []), *objects,
         '-lflint', '-lgmp', '-lm', '-pthread', '-o', str(exe)],
        ['timeout', '30', str(exe)],
    ]
    start = time.monotonic()
    with (out/f'{name}.log').open('w') as log:
        for index, command in enumerate(commands):
            r = subprocess.run(command, cwd=root, stdout=log, stderr=log, timeout=35)
            if r.returncode:
                break
    result = dict(name=name, stage=index, exit=r.returncode, seconds=round(time.monotonic()-start, 3))
    results.append(result)
    print(result, flush=True)
    assert index == 2 and r.returncode != 0, result
    obj.unlink(missing_ok=True)
    exe.unlink(missing_ok=True)
    test_src.unlink(missing_ok=True)
result_name = f'results-{args.name}.json' if args.name else (
    'results-printer.json' if args.printer else 'results.json')
(out/result_name).write_text(json.dumps(results, indent=2)+'\n')
