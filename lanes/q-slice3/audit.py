#!/usr/bin/env python3
"""Static checks of this slice's source/prose and shared-file owned sections."""
from pathlib import Path
import ast

paths = [Path(p) for p in
         ('include/adelefeld/psi.h', 'src/psi.c', 'tests/test_psi.c', 'docs/api-3b.md',
          'tests/julia/psi.jl')]
paths += sorted(Path('tests/driver').glob('psi-*.cmd'))
paths += sorted(Path('tests/driver').glob('psi-*.out'))
paths += sorted(Path('lanes/q-slice3').glob('*.py'))
paths += sorted(Path('lanes/q-slice3').glob('*.c'))
paths += [Path('lanes/q-slice3/redgreen.md')]
if Path('lanes/q-slice3/report.md').exists():
    paths += [Path('lanes/q-slice3/report.md')]
lines = parses = 0
for path in paths:
    text = path.read_text()
    assert text.endswith('\n'), path
    for n, line in enumerate(text.splitlines(), 1):
        assert len(line) <= 116, (path, n, len(line))
        lines += 1
    if path.suffix == '.py':
        ast.parse(text, filename=str(path))
        parses += 1
shared = [('tools/adf/README.md', '### Tate additive character', None),
          ('tests/test_julia.sh', '# Slice 3.2-a:', '# Slice 3.1-a:'),
          ('tools/adf/adf.c', '/* Slice 3.2-a:', '/* WP 1F.9: exact scalar')]
for filename, start, end in shared:
    text = Path(filename).read_text()
    assert text.endswith('\n'), filename
    section = text[text.index(start):]
    if end is not None:
        section = section[:section.index(end)]
    for line in section.splitlines():
        assert len(line) <= 116, (filename, line)
        lines += 1
assert Path('include/adelefeld.h').read_text().count('#include "adelefeld/psi.h"') == 1
declarations = Path('include/adelefeld/psi.h').read_text()
assert declarations.count('\nint adf_') == 5
assert 'int adf_qclass_psi' not in declarations and 'int adf_lball_psi' not in declarations
vectors = list(Path('tests/ref/vectors/q-slice3').iterdir())
assert sum(p.stat().st_size for p in vectors) <= 400*1024
assert not Path('run_faults.py').exists()
print(f'{len(paths)} files, {lines} lines <=116, {parses} Python parses, 5 declarations')
print(f'{len(vectors)} vector files, {sum(p.stat().st_size for p in vectors)} bytes')
