#!/usr/bin/env python3
"""Compare direct shell operands with the prewritten hand-derived fixture."""
from pathlib import Path
import subprocess
lines = [s for s in Path('tests/driver/ffun-algebra.cmd').read_text().splitlines()
         if s and not s.startswith('#')]
expected = Path('tests/driver/ffun-algebra.out').read_text().splitlines()
checks = 0
for index in [0, 1, 3, 4, 9, 10, 7]:
    name, text = lines[index].split(' ', 1)
    operands = text.split(' with ')
    variants = [operands] if len(operands) == 1 else [[operands[0], 'with', operands[1]], operands]
    for arguments in variants:
        result = subprocess.run(['timeout', '10', 'build/adf', name, *arguments],
                                capture_output=True, text=True)
        assert result.returncode == (1 if index == 7 else 0), result
        assert result.stdout == expected[index] + '\n', result
        checks += 1
print('direct shell calls:', checks, 'passed')
