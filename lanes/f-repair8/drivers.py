"""Four idele Log driver cases, linked explicitly to the repaired archive."""
import subprocess
import json
from pathlib import Path

lane = Path('lanes/f-repair8')
exe = lane / 'build/plain/adf'
command = ['timeout', '170', 'cc', '-Iinclude', '-std=c11', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
           'tools/adf/adf.c', str(lane / 'build/plain/libadelefeld.a'), '-lflint', '-lgmp', '-lm',
           '-o', str(exe)]
result = subprocess.run(command, capture_output=True, text=True)
assert result.returncode == 0, result.stderr
records = [dict(command=command, exit=result.returncode)]
for suffix in ('values', 'status', 'refine-values', 'refine-status'):
    name = 'gfunc-log-' + suffix
    source = Path('tests/driver') / (name + '.cmd')
    expected = source.with_suffix('.out').read_text()
    want = 1 if source.read_text().startswith('#!exit 1') else 0
    with source.open() as f:
        result = subprocess.run(['timeout', '60', str(exe)], stdin=f, capture_output=True, text=True)
    record = dict(command='timeout 60 ' + str(exe) + ' < ' + str(source), exit=result.returncode,
                  expected_exit=want, lines=len(result.stdout.splitlines()),
                  differences=int(result.stdout != expected))
    records.append(record)
    assert result.returncode == want and record['differences'] == 0, (record, result.stderr)
(lane / 'drivers.json').write_text(json.dumps(records, indent=2) + '\n')
print(json.dumps(records))
