#!/usr/bin/env python3
"""Audit the author's checker, kept separate from the independent checks file."""
import contextlib
import io
from pathlib import Path


source = Path('proto/quotient_checks.py').read_text()
mutations = (
    ('drop mixed-modulus residue refinements',
     'for j in range(Np // Ni):', 'for j in range(1):'),
    ('wrong boundary glue sign',
     "T[(m - 1) % Np].append(('pt', F(0), F(0)))",
     "T[(m + 1) % Np].append(('pt', F(0), F(0)))"),
)
survived = killed = 0
for label, old, new in mutations:
    assert source.count(old) == 1
    namespace = {'__name__': 'review_mutant'}
    output = io.StringIO()
    with contextlib.redirect_stdout(output):
        exec(compile(source.replace(old, new), '<review mutant>', 'exec'), namespace)
        namespace['check_translation']()
    failed = bool(namespace['FAILURES'])
    killed += failed
    survived += not failed
    print(('KILLED' if failed else 'SURVIVED') + ': ' + label)
    print(output.getvalue().strip())
assert (survived, killed) == (1, 1)
print(f'TOTAL mutants=2; survived={survived}; killed={killed}')
