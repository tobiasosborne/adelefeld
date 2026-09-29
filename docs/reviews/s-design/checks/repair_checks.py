#!/usr/bin/env python3
"""Validate the proposed float-free reference repair without editing the design."""
from fractions import Fraction as Q
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[4]
source = (root / 'proto/solvers_checks.py').read_text().rsplit('if __name__ == "__main__":', 1)[0]
old = 'while B <= 1 + max(abs(c) for c in g[:-1]) / abs(g[-1]):'
new = 'while B * abs(g[-1]) <= abs(g[-1]) + max(abs(c) for c in g[:-1]):'
assert source.count(old) == 1
checks = '''
for sign_ in (-1, 1):
    for denominator in (1, 3):
        f = [-sign_ * 10**400, denominator]
        want = F(sign_ * 10**400, denominator)
        st, count, balls = real_roots_ref(f, 6)
        assert st == OK and count == 1 and len(balls) == 1
        lo, hi = balls[0]
        assert lo <= want <= hi and hi-lo <= F(1, 64)
print('integer_bound_repair: huge_linear_polynomials=4, failures=0')
'''
result = subprocess.run([sys.executable, '-c', source.replace(old, new) + checks],
                        text=True, capture_output=True, timeout=30)
print(result.stdout, end='')
print(result.stderr, end='')
assert result.returncode == 0
# An enclosure satisfying the incoming FLINT accuracy promise can need widening.
# This is a logical contract counterexample, not an observed FLINT output for X-1.
prec = 8
radius = Q(3, 2**(prec+2))
lo, hi = Q(1), Q(1)+2*radius
assert (lo-1)*(hi-1) == 0
lo2, hi2 = lo-radius, hi+radius
assert (lo2-1)*(hi2-1) < 0
assert (hi2-lo2)/2 > Q(1, 2**prec)
print('RR_widening_contract: prec=8, input_radius=3/1024, output_radius=3/512, promised_max=1/256')
print('total: repair_cases=4, widening_counterexamples=1, failures=0')
