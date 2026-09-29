#!/usr/bin/env python3
"""Run two incorrect target variants against the author's own tests in child processes.

No imported author functions. Mutated target scripts exist only in subprocess memory.
The original design and checks remain unchanged.
"""
from pathlib import Path
import os
import subprocess
import sys

root = Path(__file__).resolve().parents[4]
source = (root / 'proto/solvers_checks.py').read_text().rsplit('if __name__ == "__main__":', 1)[0]
mutants = {
    'limit_discards_two_found_solutions': '''
original_recon = recon_partial
changed = 0
def recon_partial(m, c, A, B, limit):
    global changed
    status, solutions, cert = original_recon(m, c, A, B, limit)
    if status == NOT_UNIQUE and cert is not None and B // abs(cert[3]) > max(limit, 0):
        changed += 1
        return NOT_DETERMINED, [], cert
    return status, solutions, cert
check_s3_limit()
print('changed_statuses:', changed)
print('witness:', recon_partial(2, 1, 1, 3, 1))
assert changed > 0 and not FAILURES
''',
    'real_ignores_requested_precision': '''
original_real = real_roots_ref
def real_roots_ref(f, bits):
    return original_real(f, 1)
check_s2_real_completeness()
status, count, balls = real_roots_ref([-2, 0, 1], 20)
violations = sum(hi-lo > F(1, 2**20) for lo,hi in balls)
print('precision_violations_x2_minus_2:', violations)
assert violations == 2 and not FAILURES
''',
}
for name, mutant in mutants.items():
    result = subprocess.run([sys.executable, '-c', source + mutant], text=True, capture_output=True,
                            timeout=60, env=dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1'))
    print(name + ': exit=' + str(result.returncode))
    print(result.stdout, end='')
    if result.stderr:
        print(result.stderr, end='')
    if result.returncode:
        sys.exit(1)
print('total: surviving_mutants=2, reproduction_failures=0')
