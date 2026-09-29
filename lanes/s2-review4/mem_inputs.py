#!/usr/bin/env python3
"""Create a bounded replay for allocations, cleanup and output preservation on error paths."""
from pathlib import Path
from oracle import mul, prod, nonresidue

HERE = Path(__file__).resolve().parent
lines = []
for p in (131, 4294967291, 9223372036854775783, 18446744073709551557):
    roots = [-1, -1 + p**3, 2, 2 + p**2, 5]
    cases = [('P', prod(roots), 5, d) for d in range(5)]
    cases += [('P', [0], 3, 0), ('P', [1, 1], 0, 0), ('P', [1, 1], 3, -1),
              ('P', [1, 1], 99999999, 0), ('P', [-nonresidue(p), 0, 1], 3, 0),
              ('P', [-1, p], 3, 0), ('M', prod([0, p - 1, 2**1024 + 3]), 0, 0),
              ('M', [1, p*2**4096], 0, 0)]
    if p.bit_length() == 64:
        cases += [('C', prod(roots), 1, 3), ('C', prod([1, 1 + p]), 1, 1)]
    roots2 = [-1, 0, p**2 - 1, p - 2]
    f = mul(mul(prod(roots2 + roots2[:1]), [-1, p]), [-nonresidue(p), 0, 1])
    cases += [('P', [-p**3 * 2**2048 * c for c in f], 2, 2)]
    for repeat in range(4):
        for mode, f, prec, depth in cases:
            lines.append(f'{mode} {p} {prec} {depth} {len(f)} ' + ' '.join(map(str, f)))
(HERE / 'memory.inputs').write_text('\n'.join(lines) + '\n')
print('memory_inputs=', len(lines))
