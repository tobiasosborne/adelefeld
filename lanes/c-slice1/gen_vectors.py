#!/usr/bin/env python3
"""Slice a vectors from the independent character and text oracles."""
import json
from fractions import Fraction as F
from math import gcd
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'proto'))
import char_checks as oracle
import text_grammar as text
import flint

OUT = ROOT / 'tests/ref/vectors/c-slice1'
OUT.mkdir(parents=True, exist_ok=True)


def write(name, rows):
    path = OUT / (name + '.jsonl')
    with path.open('w') as stream:
        for row in rows:
            stream.write(json.dumps(row, separators=(',', ':')) + '\n')
    print(name, len(rows), path.stat().st_size)


def operands(C):
    return list(range(-17, 15)) + [C, -C, 2*C, -2*C, 2**2000, -2**2000,
                                  2**2000+1, -2**2000-1]


write('inputs', [{'a': list(range(-17, 15)) + ['C', '-C', '2C', '-2C',
                  str(2**2000), str(-2**2000), str(2**2000+1), str(-2**2000-1)]}])
write('lower', [{'q': q, 'n': n, 'lower': list(oracle.lower(q, n))}
                for q in range(1, 81) for n in range(1, q+1) if gcd(q, n) == 1])
write('phases', [{'C': C, 'n': n, 'phases': ['zero' if t is None else str(t)
                                         for a in operands(C)
                                         for t in [oracle.raw_phases(C, n)[a % C]]]}
                 for C, n in oracle.primitive_pairs(40)])
pairs = oracle.primitive_pairs(64)
selected = [pairs[i*(len(pairs)-1)//44] for i in range(45)]
rows = []
for C, n in selected:
    tau, error = oracle.gauss(C, n)
    W = oracle.root_from_input(C, n)
    # Certify W independently by dividing the certified tau rectangle in python-flint.
    with flint.ctx.workprec(256):
        phases = oracle.raw_phases(C, n)
        ball = sum((oracle.phase_acb(t + F(a, C)) for a, t in enumerate(phases)
                    if t is not None), flint.acb(0))
        e = oracle.lower(C, n)[2]
        root = ball / (flint.acb(0, 1)**e * flint.arb(C).sqrt())
        we = F(0)
        for coord, part in zip((W.real, W.imag), (root.real, root.imag)):
            lo, hi = oracle.arb_interval(part)
            v = oracle.exact_mpf(coord)
            we += max(abs(v-lo), abs(v-hi))
    rows.append({'C': C, 'n': n, 'tau': [str(oracle.exact_mpf(tau.real)),
                                       str(oracle.exact_mpf(tau.imag)), str(error)],
                 'W': [str(oracle.exact_mpf(W.real)), str(oracle.exact_mpf(W.imag)), str(we)]})
write('gauss', rows)
texts = [line.split('\t')[0] for line in (ROOT / 'tests/golden/char.tsv').read_text().splitlines()
         if line and not line.startswith('#')]
texts += ['char(q=8, n=7, s=(-1) + (0.5)*i)', 'char(q=65536, n=1, s=(0) + (0)*i)',
          'char(q=65537, n=1, s=(0) + (0)*i)', 'char(q=1, n=' + str(2**2000) + ', s=(0) + (0)*i)',
          'char(q=5, n=' + str(2**2000+1) + ', s=(0) + (0)*i)',
          'char(q=5, n=2, s=(1.) + (0)*i)', 'char(q=5, n=2, s=(1e) + (0)*i)',
          'char(q=5, n=2, s=(0 +/- -1) + (0)*i)', 'char(q=5, n=2, s=(0) - (0)*i)',
          'char(q=5, n=2, s=(0) + (0)*j)', 'char(q=5, n=2, s=(0) + (0)*i) tail',
          'char(q=5, n=2, s=(0) + (0)*i)\0', 'char(q=5, n=2, s=(0) + (0)*i)\u2212']
rows = []
for s in texts:
    answer = text.canonical('char', s)
    if s.startswith('char(q=65537,'):
        answer = '!LIMIT'  # D1 belongs to C, not the reference's older cap.
    row = {'input': s, 'expected': answer}
    if not answer.startswith('!'):
        parsed = text._syntax('char', s)
        row['s_bounds'] = [str(v) for mid, rad in text._complex(parsed[3])
                           for v in (mid-rad, mid+rad)]
    rows.append(row)
write('texts', rows)
total = sum(p.stat().st_size for p in OUT.glob('*.jsonl'))
assert total <= 400*1024
print('TOTAL_BYTES', total)
