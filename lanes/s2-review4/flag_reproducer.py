#!/usr/bin/env python3
"""A deterministic call to the unchanged check_big against the lane's reduced-flag fault."""
import importlib.util
from pathlib import Path
import random

ROOT = Path(__file__).resolve().parents[2]
LANE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('author_diff', ROOT / 'tests/fuzz/diff_roots_padic.py')
D = importlib.util.module_from_spec(spec)
spec.loader.exec_module(D)
D.draw_big_prime = lambda rng: 18446744073709551557
roots = [2**99 + j for j in range(6)]
f = D.S.pfrom_roots(roots + roots[:1])
D.draw_planted = lambda rng, p: (f, roots, 0)
D.draw_prec = lambda rng: 2
for name in ('original', 'shared_reduced'):
    br = D.Bridge(str(LANE / f'{name}.so'))
    fails, where, status, info = D.check_big(br, random.Random(294), 1)
    got = br.run(f, 18446744073709551557, 2, 0)
    print(name, 'status=', got['status'], 'reduced=', got['partial']['reduced'],
          'expected_reduced=1', 'check_big_failures=', len(fails), 'details=', fails)
    assert not fails
    assert got['partial']['reduced'] == (1 if name == 'original' else 0)
