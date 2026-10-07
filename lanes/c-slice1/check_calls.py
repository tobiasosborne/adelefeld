#!/usr/bin/env python3
"""Direct CLI examples, with independently specified exact expected lines."""
import subprocess

cases = [
    (['char', 'char(q=5, n=2, s=(0) + (0)*i)'], 'char(q=5, n=2, s=(0) + (0)*i)\n'),
    (['chi', 'char(q=5, n=2, s=(0) + (0)*i)', 'with', '2'], '(0) + (1)*i\n'),
    (['chi', 'char(q=5, n=2, s=(0) + (0)*i)', '2'], '(0) + (1)*i\n'),
    (['gauss', 'char(q=8, n=7, s=(0) + (0)*i)'], 'e=1 tau=(0) + (2)*i W=(1) + (0)*i\n'),
]
for args, expected in cases:
    result = subprocess.run(['timeout', '30', 'build/adf', *args], text=True, capture_output=True)
    assert result.returncode == 0, result.stderr
    assert result.stdout == expected, (args, result.stdout, expected)
print('direct CLI: 4 calls, 4 expected lines')
