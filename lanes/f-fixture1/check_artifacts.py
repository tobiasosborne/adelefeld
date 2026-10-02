#!/usr/bin/env python3
"""Audit committed fixture inputs, distribution, size, and the lane's readable source line lengths."""
from collections import Counter
import json
from pathlib import Path
import sys

from gen_stored_large import ROOT, LANE, PRIMES, cases, route

sys.set_int_max_str_digits(0)
path = ROOT / 'tests/ref/vectors/f-slice5/stored_large.jsonl'
rows = [json.loads(s) for s in path.read_text().splitlines()]
expected = cases(64)
assert len(rows) == len(expected) == 465
assert path.stat().st_size < 1000000
assert [dict(f=r['f'], N=r['N'], x=r['x']) for r in rows] == expected
print('rows=465 input mismatches=0 bytes=' + str(path.stat().st_size))
print('functions=' + str(dict(Counter(r['f'] for r in rows))))
print('exact=' + str(sum(r['x']['exact'] for r in rows)),
      'balls=' + str(sum(not r['x']['exact'] for r in rows)),
      'negative_Log=' + str(sum(r['f'] == 'Log' and r['x']['v'] < 0 for r in rows)))
for p in PRIMES:
    part = [r for r in rows if r['x']['p'] == p]
    info = [route(r, 64) for r in part]
    print(f'p={p} functions={dict(Counter(r["f"] for r in part))} '
          f'exact={sum(r["x"]["exact"] for r in part)} '
          f'negative_Log={sum(r["f"] == "Log" and r["x"]["v"] < 0 for r in part)}')
long_lines = []
for path in LANE.iterdir():
    if path.suffix in ('.py', '.c', '.sh', '.md'):
        for n, s in enumerate(path.read_text().splitlines(), 1):
            if len(s) > 116:
                long_lines.append((path.name, n, len(s)))
print(f'lines_over_116={long_lines}')
assert not long_lines
