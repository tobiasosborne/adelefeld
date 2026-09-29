#!/usr/bin/env python3
import sys
sys.dont_write_bytecode = True
from attack import Probe, mul, trim, planted, Q

u, v = [1], [0, 1]
for n in range(2, 51):
    h = [0] + [2 * x for x in v]
    for i, x in enumerate(u):
        h[i] -= x
    u, v = v, trim(h)

p = Probe("lanes/r-review1/probe")
p.nested(v, [2, 53])
assert p.roots == 100
for d in (20, 40):
    roots = list(map(Q, range(1, d + 1)))
    p.nested(planted(roots), [2, 53], roots)
for e in (10000, 100000):
    roots = [Q(2**e), Q(2**e + 1)]
    p.nested(planted(roots, -1234567), [2, 53], roots)
p.close()
print(f"calls={p.calls} roots={p.roots} nested_pairs={p.pairs} oracle_checks={p.checks} errors=0")
