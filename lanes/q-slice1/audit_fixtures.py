#!/usr/bin/env python3
"""Read-only audit of old qclass placeholders. No fixture is edited."""
from pathlib import Path
import subprocess

rows = []
for name in ('06_pairs', '07_status', '08_show_type', '12_status_order', '13_dump', 'f-places-hostile'):
    path = Path(f'tests/driver/{name}.cmd')
    commands = [(n, s) for n, s in enumerate(path.read_text().splitlines(), 1)
                if s.strip() and not s.startswith('#')]
    result = subprocess.run(['build/adf'], input=path.read_bytes(), stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, timeout=120)
    got = result.stdout.decode().splitlines()
    old = path.with_suffix('.out').read_text().splitlines()
    # Successful settings have no line of output. All settings in 07_status are invalid.
    commands = [(n, s) for n, s in commands
                if not (s.startswith(('prec ', 'digits ')) and name != '07_status')]
    assert len(old) == len(got) == len(commands), (name, len(old), len(got), len(commands))
    for (n, cmd), before, after in zip(commands, old, got):
        if '(0.5 ; 0) + Q' in cmd:
            rows.append((name, n, before, after))
        else:
            assert before == after, (name, n, cmd, before, after)
print('| Fixture .cmd | Line | Old output | New output |')
print('|---|---:|---|---|')
for name, n, old, new in rows:
    print(f'| {name} | {n} | `{old}` | `{new}` |')
print(f'\n{len(rows)} placeholder commands; {sum(a != b for _, _, a, b in rows)} changed outputs.')
