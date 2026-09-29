#!/usr/bin/env python3
"""Check statuses and the non-OK preservation results of the fixed memory replay."""
import collections
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
filename = sys.argv[1] if len(sys.argv) > 1 else 'memory-final.outputs'
inputs = (HERE / 'memory.inputs').read_text().splitlines()
outputs = (HERE / filename).read_text().splitlines()
assert len(inputs) == len(outputs) == 232
counts = collections.Counter()
for entry, result in zip(inputs, outputs):
    mode, p, prec, depth, length, *coeff = entry.split()
    p, prec, depth, length = map(int, (p, prec, depth, length))
    data = result.split()
    if mode == 'M':
        assert data[0] == 'M'
        expected = sorted({0, p - 1, (2**1024 + 3) % p}) if length == 4 else []
        assert list(map(int, data[1:])) == [len(expected)] + expected
        counts['modp'] += 1
        continue
    st, sp, unchanged = map(int, data[1:4])
    if prec < 1 or depth < 0 or all(int(c) == 0 for c in coeff):
        want = (7, 7)
    elif prec == 99999999 or mode == 'C' and length == 6:
        want = (10, 10)
    elif length == 6 and depth < 3:
        want = (1, 0)
    else:
        want = (0, 0)
    assert (st, sp) == want and unchanged == 1, (entry, result, want)
    if length == 9:
        assert data[10] == '1', ('reduced', result)
    counts['status_' + str(st)] += 1
print('memory_replay:', dict(sorted(counts.items())), '232 rows; failures=0')
