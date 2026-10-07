#!/usr/bin/env python3
"""Compile named faults in owned scratch copies, using the SAN+INV lane archive.

Each executable runs under timeout. No source in src/ is changed.
"""
import os
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path.cwd()
LANE = ROOT/'lanes/q-slice6'
SOURCE = (ROOT/'src/qclass_sets.c').read_text()
FAULTS = [
    ('one-residue', 'while (r < L) { f->residues[r] = 1; if (step >= L-r) break; r += step; }',
     'if (r < L) f->residues[r] = 1;'),
    ('one-real-sample', 'for (i = 0; i+1 < unique; i++) {',
     'for (i = 0; i < 1; i++) {'),
    ('point-comparison', 'kind == 0 ? xy && yx : (kind == 1 ? xy : overlap)',
     'kind == 0 ? ((void) yx, adf_fball_compare(&x->piece[0].fin, &y->piece[0].fin) == ADF_CMP_EQUAL) : '
     '(kind == 1 ? xy : overlap)'),
    ('glue-plus', 'fmpz_sub_ui(shifted, p[i].m, 1);', 'fmpz_add_ui(shifted, p[i].m, 1);'),
    ('no-endpoint-glue', 'if (fmpq_is_zero(s) && fmpq_is_one(p[i].hi)) {', 'if (0) {'),
    ('contains-reversed', 'return qs_query(truth, x, y, work_limit, 1);',
     'return qs_query(truth, y, x, work_limit, 1);'),
    ('coset-as-sample', 'if (a->residues[i] && !b->residues[i]) return 0;',
     'if (a->residues[i] && !b->residues[i]) { slong j; int found = 0; '
     'for (j = 0; j < b->len; j++) if (fmpz_fdiv_ui(b->points+j, (ulong) L) == (ulong) i) found = 1; '
     'if (!found) return 0; }'),
    ('allocation-before-budget', 'if (!qs_preflight(&K, &L, x, y, limit)) return ADF_LIMIT;',
     '{ void *premature = flint_malloc(1); flint_free(premature); } '
     'if (!qs_preflight(&K, &L, x, y, limit)) return ADF_LIMIT;'),
    ('no-saturation', 'if (fmpz_cmp(total, cap) > 0) fmpz_set(total, cap);',
     'if (0) fmpz_set(total, cap);'),
    ('word-overflow', 'fmpz_add(total, total, count);',
     'fmpz_set_si(total, fmpz_get_si(total)+fmpz_get_si(count));'),
    ('truth-on-limit', 'if (!qs_preflight(&K, &L, x, y, limit)) return ADF_LIMIT;',
     'if (!qs_preflight(&K, &L, x, y, limit)) { *truth = 0; return ADF_LIMIT; }'),
]


def run_cases(cases, results_name='fault-results.tsv', expected_survivors=()):
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0', UBSAN_OPTIONS='halt_on_error=1')
    scratch = LANE/'fault-scratch'
    results = []
    try:
        for name, before, after in cases:
            assert SOURCE.count(before) == 1, name
            tree = scratch/name
            (tree/'src').mkdir(parents=True)
            (tree/'tests').mkdir()
            (tree/'src/qclass_sets.c').write_text(SOURCE.replace(before, after))
            shutil.copyfile(ROOT/'tests/test_qclass_sets.c', tree/'tests/test_qclass_sets.c')
            binary = tree/'test_qclass_sets'
            build = LANE/'clang-san-inv'
            cmd = ['timeout', '60', 'clang', '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                   '-DADF_CHECK_INVARIANTS', '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                   '-Iinclude', '-Itests', '-Isrc', str(tree/'tests/test_qclass_sets.c'),
                   str(build/'support/jsonl.o'), str(build/'support/golden.o'),
                   str(build/'libadelefeld.a'), '-lflint', '-lgmp', '-lm', '-pthread', '-o', str(binary)]
            t0 = time.monotonic()
            comp = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env)
            if comp.returncode:
                print(name, 'COMPILE FAILURE', comp.stdout[:1000], flush=True)
                results.append((name, comp.returncode, 'not compiled'))
                continue
            run = subprocess.run(['timeout', '15', str(binary)], text=True, stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT, env=env)
            detail = next((s for s in run.stdout.splitlines() if 'sets line' in s or 'runtime error' in s),
                          run.stdout.splitlines()[0] if run.stdout else '(no output)')
            print(name, 'exit', run.returncode, detail, f'{time.monotonic()-t0:.2f} s', flush=True)
            results.append((name, run.returncode, detail))
            shutil.rmtree(tree)
    finally:
        shutil.rmtree(scratch, ignore_errors=True)
    (LANE/results_name).write_text(''.join(f'{a}\t{b}\t{c}\n' for a, b, c in results))
    assert len(results) == len(cases)
    assert all(detail != 'not compiled' and (code == 0) == (name in expected_survivors)
               for name, code, detail in results)
    print(f'{sum(code != 0 for _, code, _ in results)} killed, '
          f'{sum(code == 0 for _, code, _ in results)} survived, 0 compile failures')


def main():
    run_cases(FAULTS)


if __name__ == '__main__':
    main()
