"""Measure coverage of the exact reference fixtures, independently of the C test counter."""
import importlib.util
import json
import sys
from fractions import Fraction as F
from pathlib import Path

root = Path(__file__).resolve().parents[2]
s = importlib.util.spec_from_file_location('quotient3', root/'proto/quotient3_checks.py')
o = importlib.util.module_from_spec(s); sys.modules[s.name] = o; s.loader.exec_module(o)
rows = labels = size = lost = carry = 0
counts = set()
for path in sorted((root/'tests/ref/vectors/q-slice2').glob('*.jsonl')):
    size += path.stat().st_size
    for line in path.read_text().splitlines():
        v = json.loads(line); rows += 1; labels += len(v['points'])
        assert len(v['points']) == 40
        assert len({(F(a), F(b)) for a, b, _ in v['points']}) == 40
        if path.stem == 'integer':
            counts.add(v['raw']-1)
        for l, h, _, _ in v['exact']:
            l, h = F(l), F(h)
            m = o.round_binary((l+h)/2, max(v['prec'], 2)); d = max(m-l, h-m)
            if not d:
                continue
            u = o.round_binary(d, 30, True)
            lost += o.round_binary(d, 30) < d
            power = (u.numerator & (u.numerator-1) == 0 and
                     u.denominator & (u.denominator-1) == 0)
            carry += power and u > d
assert {0, 1, 2, 5} <= counts
assert rows == 152 and labels == 6080 and size < 500000 and lost and carry
print('records', rows, 'labels', labels, 'bytes', size)
print('integer shifted-interior counts', sorted(counts))
print('RN30 loses an endpoint without successor', lost, 'pieces; RU30 binade carries', carry)
