"""Build or run the six required tests; retain commands, exits and output."""
from pathlib import Path
import json
import subprocess
import sys
import time
lane = Path('lanes/t5-repair2')
names = ['test_tate_local', 'test_tate_gamma', 'test_localfactor', 'test_tate',
         'test_tate_continue', 'test_tate_fe']
action, mode = sys.argv[1:3]
if len(sys.argv) > 3:
    selected = sys.argv[3:]
    assert all(name in names for name in selected)
    names = selected
build = lane / ('build-' + mode)
options = {'plain': [], 'san': ['SAN=1'], 'inv': ['INV=1'],
           'clang': ['CC=clang'], 'san-inv': ['SAN=1', 'INV=1']}
commands = ([['make', '-j2', 'BUILD=' + str(build), *options[mode],
              *[str(build / n) for n in names]]] if action == 'build'
            else [[str(build / n)] for n in names])
failed = 0
for command in commands:
    wrapped = ['timeout', '170', *command]
    start = time.monotonic()
    result = subprocess.run(wrapped, text=True, capture_output=True)
    elapsed = time.monotonic() - start
    label = 'build' if action == 'build' else Path(command[0]).name
    log = lane / (mode + '-' + label + '.log')
    log.write_text(result.stdout + result.stderr)
    record = dict(command=wrapped, exit=result.returncode, seconds=elapsed, log=str(log))
    with (lane / 'checks.jsonl').open('a') as stream:
        stream.write(json.dumps(record) + '\n')
    print(json.dumps(record), flush=True)
    if action != 'build':
        print('\n'.join(result.stdout.splitlines()[-3:]), flush=True)
    failed += result.returncode != 0
raise SystemExit(bool(failed))
