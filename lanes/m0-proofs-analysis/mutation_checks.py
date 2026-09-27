#!/usr/bin/env python3
"""Deliberate convention mutations; all modified copies stay in this lane."""
import os
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
lane = root / 'lanes/m0-proofs-analysis'
source = (root / 'proto/analysis_checks.py').read_text()
mutants = [
    ('finite_sign', 'values[j] * phase(-mp.mpf(j*k)/L)',
     'values[j] * phase(mp.mpf(j*k)/L)', 'check_finite_fourier'),
    ('finite_weight', 'for j in range(L)) / M\n', 'for j in range(L)) / D\n',
     'check_finite_fourier'),
    ('real_sign', 'z = B + 2j * mp.pi * y', 'z = B - 2j * mp.pi * y', 'check_real_transform'),
    ('ramified_sign', 'mp.conj(eta(u))*phase(-mp.mpf(u)/p**a)',
     'mp.conj(eta(u))*phase(mp.mpf(u)/p**a)', 'check_local_gamma'),
    ('root_parity', 'W=gauss(chi,C)/((1j)**parity(chi)*mp.sqrt(C))',
     'W=gauss(chi,C)*(1j)**parity(chi)/mp.sqrt(C)', 'check_functional_equation'),
    ('conjugate', 'right=W*completed_reference(1-s,lambda n:mp.conj(chi(n)),C)',
     'right=W*completed_reference(1-s,chi,C)', 'check_functional_equation'),
    ('conductor', '(mp.mpf(C)/mp.pi)**((s+e)/2)',
     '(1/mp.pi)**((s+e)/2)', 'check_functional_equation'),
    ('pole_sign', 'result+=1/(s-1)-1/s', 'result+=1/(s-1)+1/s', 'check_continuation'),
]
failures = 0
for name, old, new, check in mutants:
    if source.count(old) != 1:
        print(f'{name}: substitution_matches={source.count(old)} expected=1')
        failures += 1
        continue
    path = lane / ('mutant_' + name + '.py')
    path.write_text(source.replace(old, new))
    env = dict(os.environ, OPENBLAS_NUM_THREADS='1', OMP_NUM_THREADS='1')
    result = subprocess.run([sys.executable, '-B', str(path), '--only', check],
                            capture_output=True, text=True, timeout=45, env=env)
    (lane / ('mutant_' + name + '.txt')).write_text(result.stdout + result.stderr)
    killed = result.returncode != 0 and 'AssertionError' in result.stdout
    failures += not killed
    print(f'{name}: killed={int(killed)} returncode={result.returncode}', flush=True)
print(f'TOTAL mutations={len(mutants)} killed={len(mutants)-failures} survivors={failures}')
sys.exit(bool(failures))
