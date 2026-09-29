#!/usr/bin/env python3
"""Expose the unchecked smaller-count case. The altered gcd exists only in a lane copy."""
import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LANE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('author_diff', ROOT / 'tests/fuzz/diff_roots_padic.py')
D = importlib.util.module_from_spec(spec)
spec.loader.exec_module(D)
for name in ('original', 'small_gcd', 'missing_root'):
    br = D.Bridge(str(LANE / f'{name}.so'))
    got = br.run([0, -1, 1], 18446744073709551557, 3, 0)
    p = got['partial']
    print(name, 'status=', got['status'], 'complete=', p['complete'], 'certs=', p['certs'],
          'entries=', p['verify'], 'verify_complete=', p['vc'])
    assert got['status'] == 0 and p['complete'] == 1 and p['verify'] == p['vc'] == 1
    assert len(p['certs']) == {'original': 2, 'small_gcd': 0, 'missing_root': 1}[name]
