#!/usr/bin/env python3
"""Repeat the eight semantic changes that survived the first 40-mutant sweep."""
from plant_faults import run_cases

cases = [
    ('endpoint-tie', 'fmpq_cmp(t, e->next) < 0', 'fmpq_cmp(t, e->next) <= 0'),
    ('partial-point-init', 'for (i = 0; i < 2*K; i++) fmpz_init(points[i].centre);',
     'for (i = 0; i < 2+K; i++) fmpz_init(points[i].centre);'),
    ('read-size-or', 'ok = qs_size(a->q) && qs_size(N->q);', 'ok = qs_size(a->q) || qs_size(N->q);'),
    ('denominator-off-one', '(e >= b ? 1 : b-e+1)', '(e >= b ? 1 : b-e+0)'),
    ('capacity-ok', 'return ADF_LIMIT;\n    return ADF_OK;', 'return ADF_OK;\n    return ADF_OK;'),
    ('skip-side-cleanup', 'for (side = 0; side < 2; side++) {\n'
     '        for (i = 0; i < 2*K; i++) fmpz_clear(f[side].points+i);',
     'for (side = 1; side < 2; side++) {\n'
     '        for (i = 0; i < 2*K; i++) fmpz_clear(f[side].points+i);'),
    ('endpoint-byte-product', '(size_t) (2*K+2) > SIZE_MAX/sizeof(fmpq)',
     '(size_t) (2/K+2) > SIZE_MAX/sizeof(fmpq)'),
    ('partial-integer-clear', 'for (i = 0; i < 2*K; i++) fmpz_clear(f[side].points+i);',
     'for (i = 0; i < 2/K; i++) fmpz_clear(f[side].points+i);'),
]

if __name__ == '__main__':
    run_cases(cases, 'survivor-results.tsv', ('endpoint-tie', 'read-size-or', 'endpoint-byte-product'))
