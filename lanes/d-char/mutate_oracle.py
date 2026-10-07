#!/usr/bin/env python3
"""Twelve bounded mutations of the design reference, not of future C implementation.

Each mutation changes executed code and reruns a relevant group. Only an assertion
failure counts as killed; timeouts, syntax errors and other exceptions are failures
of this harness. No production or reference file is modified.
"""
import os
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT/'proto/char_checks.py'
MUTATIONS = [
    ('33_lower_n_mod_C', 'return C, label, parity, order',
     'return C, (1 if C == 1 else n % C), parity, order', 'check_lowering'),
    ('33_parity_from_n', 'parity = int(2*prim[-1 % C])',
     'parity = label % 2', 'check_lowering'),
    ('33_zero_as_phase', 'return None if theta is None else int(theta*order)',
     'return 0 if theta is None else int(theta*order)', 'check_characters'),
    ('33_strict_write', "if len(vals) != 1:\n        return 'NOT_DETERMINED'",
     "if len(vals) != 1:\n        out[0] = vals\n        return 'NOT_DETERMINED'", 'check_faults_findings'),
    ('33_hull_endpoints', 'return (-cs(F(1, 2)), cs(F(0)), -cs(F(3, 4)), cs(F(1, 4)))',
     'zs = [E(t) for t in (min(coset_values(C,n,c,N)), max(coset_values(C,n,c,N)))]\n'
     '    return (min(z.real for z in zs), max(z.real for z in zs),\n'
     '            min(z.imag for z in zs), max(z.imag for z in zs))', 'check_cosets'),
    ('33_missing_real_sign', 'u = c if x_inf > 0 else -c', 'u = c', 'check_evaluation'),
    ('34_negative_sign', 'phases = [(F(k, order)+F(a, C)) % 1',
     'phases = [(F(k, order)-F(a, C)) % 1', 'check_gauss'),
    ('34_conjugate_chi', 'phases = [(F(k, order)+F(a, C)) % 1',
     'phases = [(-F(k, order)+F(a, C)) % 1', 'check_gauss'),
    ('34_drop_C1', 'phases = [(F(k, order)+F(a, C)) % 1 for a in range(C)',
     'phases = [(F(k, order)+F(a, C)) % 1 for a in range(1,C)', 'check_gauss'),
    ('34_omit_i_parity', 'return gauss(C, label)[0]/(mp.j**parity*mp.sqrt(C))',
     'return gauss(C, label)[0]/mp.sqrt(C)', 'check_goldens'),
    ('34_use_input_modulus', 'return gauss(C, label)[0]/(mp.j**parity*mp.sqrt(C))',
     'return gauss(C, label)[0]/(mp.j**parity*mp.sqrt(q))', 'check_goldens'),
    ('34_wrong_exponent_order', 'F(int(ch.chi_exponent(a)), L)',
     'F(int(ch.chi_exponent(a)), int(ch.order()))', 'check_lowering'),
]


def child(index):
    name, old, new, group = MUTATIONS[index]
    source = SOURCE.read_text()
    assert source.count(old) == 1, (name, 'ambiguous mutation site')
    namespace = {'__name__': 'char_reference_mutant', '__file__': str(SOURCE)}
    sys.path.insert(0, str(ROOT/'proto'))
    exec(compile(source.replace(old, new, 1), str(SOURCE), 'exec'), namespace)
    try:
        namespace[group]()
    except AssertionError as exc:
        print(f'{name}: killed by {group}: {exc}')
        return 10
    print(f'{name}: SURVIVED')
    return 0


def main():
    if len(sys.argv) == 2:
        return child(int(sys.argv[1]))
    killed = 0
    for i, mutation in enumerate(MUTATIONS):
        result = subprocess.run([sys.executable, '-B', str(Path(__file__)), str(i)],
                                capture_output=True, text=True, timeout=12,
                                env={**os.environ, 'PYTHONDONTWRITEBYTECODE': '1'})
        print(result.stdout.strip(), flush=True)
        if result.returncode != 10:
            print(result.stderr, flush=True)
            raise AssertionError((mutation[0], result.returncode))
        killed += 1
    print(f'TOTAL: {killed} killed, 0 survived, 0 harness errors', flush=True)
    return 0


if __name__ == '__main__':
    sys.exit(main())
