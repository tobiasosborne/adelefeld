"""Instrument only owned scratch sources for the bound search and comparisons."""
import sys
from pathlib import Path

sys.dont_write_bytecode = True
from mutations import faults, lane

source = faults['no-M0-B']
source = source.replace('mag_mul(B, sum, f);',
                        'mag_mul(B, sum, f); mag_set(search_B, B); active = 1; search_shift = n; search_cb = cb;')
source = source.replace('real_derivative_bound(B, z, lo, hi, logpi, n, w);',
                        'acb_set(search_candidate, cand); real_derivative_bound(B, z, lo, hi, logpi, n, w);')
needle = 'acb_add_error_mag(ym, B);'
assert source.count(needle) == 1
source = source.replace(needle, needle+'''
                changed = mag_cmp(arb_radref(acb_realref(ym)), arb_radref(acb_realref(cand))) < 0
                       || mag_cmp(arb_radref(acb_imagref(ym)), arb_radref(acb_imagref(cand))) < 0;''')
(lane / 'build/search-source.c').write_text(source)
print('search: no-M0-B, three trace sites')
