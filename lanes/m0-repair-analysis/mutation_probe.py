#!/usr/bin/env python3
"""Require the three bounds mutants from the review to fail the author's checks."""

import importlib.util
from pathlib import Path

root = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('analysis_checks', root / 'proto/analysis_checks.py')
checks = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checks)

original_lattice = checks.lattice_bound
original_error = checks.continuation_error
mutants = [
    ('lattice_factor_2', 'lattice_bound',
     lambda *args: original_lattice(*args)/2, checks.check_tail_bounds),
    ('error_times_1e-6', 'continuation_error',
     lambda *args, **kwargs: original_error(*args, **kwargs)*checks.mp.mpf('1e-6'),
     checks.check_continuation),
    ('conductor_1_bound', 'continuation_error',
     lambda s,chi,C,N=24,R=420: original_error(s,chi,1,N,R),
     checks.check_continuation),
]

failures = 0
for label, target, mutant, check in mutants:
    original = getattr(checks, target)
    setattr(checks, target, mutant)
    checks.COUNTS.clear()
    checks.ERRORS.clear()
    try:
        try:
            check()
        except AssertionError as exc:
            print(f'{label}: KILLED by {check.__name__} assertion {checks.COUNTS[check.__name__]}')
        else:
            failures += 1
            print(f'{label}: SURVIVED')
    finally:
        setattr(checks, target, original)
print(f'TOTAL mutants=3 survived={failures}')
raise SystemExit(bool(failures))
