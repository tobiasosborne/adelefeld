from pathlib import Path
import subprocess
import re

binary = 'lanes/n-review1/adf'
count = 0
bad = 0
for p in sorted(Path('tests/driver').glob('[if]-*.cmd')):
    expected = p.with_suffix('.out').read_text()
    code = re.search(r'^#!exit (\d+)$', p.read_text(), re.M)
    want_code = int(code[1]) if code else 0
    run = subprocess.run(['timeout', '30', binary, str(p)], capture_output=True, text=True, timeout=32)
    mismatch = sum(a != b for a, b in zip(run.stdout.splitlines(), expected.splitlines()))
    mismatch += abs(len(run.stdout.splitlines())-len(expected.splitlines()))
    bad += mismatch + (run.returncode != want_code)
    count += len(expected.splitlines())
    print(p.name, 'lines', len(expected.splitlines()), 'mismatches', mismatch,
          'exit', run.returncode, 'expected_exit', want_code)
print('fixture_lines', count, 'failures', bad)

commands = []
expected = []
for op in ('project', 'exp_at', 'log_at'):
    for value in ('[1]', '[5 mod 6]', '(5 ; 5 * [1])', '(1 ; 1 * [1])', '<1 ; [1]>'):
        commands.append(f'{op} {value} with 5')
        expected.append('error: UNSUPPORTED')
commands += ['prec 2', 'project 5 with 5', 'exp_at 5 with 5', 'log_at 6 with 5',
             'project (1 ; 1) with 2 real', 'type [p=5: 1 + O(5^2)]']
expected += ['5: 5', '5: 6 + O(5^2)', '5: 5 + O(5^2)', 'real: 1; 2: 1', 'lball']
Path('lanes/n-review1/driver-cross.cmd').write_text('\n'.join(commands)+'\n')
Path('lanes/n-review1/driver-cross.expected').write_text('\n'.join(expected)+'\n')
run = subprocess.run(['timeout', '10', binary, 'lanes/n-review1/driver-cross.cmd'],
                     capture_output=True, text=True, timeout=12)
Path('lanes/n-review1/driver-cross.actual').write_text(run.stdout)
diffs = []
cmds = [c for c in commands if not c.startswith('prec ')]
for c, a, w in zip(cmds, run.stdout.splitlines(), expected):
    if a != w:
        diffs.append((c, a, w))
        print('CROSS_FAIL', c, 'actual', a, 'expected', w)
print('cross_lines', len(expected), 'mismatches', len(diffs), 'exit', run.returncode)
raise SystemExit(bool(bad or len(diffs) != 15))
