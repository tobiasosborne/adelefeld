"""Reproduce remaining builder examples and quantify the drive findings."""
from fractions import Fraction
import json
from pathlib import Path
import re
import subprocess

lane = Path('lanes/ux-drive-atlas')
cases = [
    ('logminus1', '-1', 'log', [2, 3, 5]),
    ('fractionroot', '1/25', 'roots 2', [5]),
    ('ballroot', '1 mod 16', 'roots 2', [2]),
    ('coarse', '0 mod 1', 'identity', [2, 3, 5]),
    ('sqrt2', '2', 'roots 2', [2, 3, 5, 7]),
    ('exp0', '0', 'exp', [2, 3, 5]),
    ('log1', '1', 'log', [2, 3, 5]),
]
for name, value, lens, primes in cases:
    command = ['timeout', '130', 'python3', 'ux/atlas/scene.py', value, '--lens', lens,
               '--primes', *map(str, primes), '--driver', str(lane / 'build/adf'),
               '--output', str(lane / (name + '.json'))]
    run = subprocess.run(command, text=True, capture_output=True)
    assert run.returncode == 0, (name, run.stdout, run.stderr)
    check = subprocess.run(['timeout', '30', 'python3', str(lane / 'check_scene.py'),
                            str(lane / (name + '.json'))], text=True, capture_output=True)
    (lane / (name + '-check.log')).write_text(check.stdout + check.stderr)
    assert check.returncode == 0, (name, check.stdout, check.stderr)
    dom = subprocess.run(['timeout', '30', 'node', str(lane / 'check_dom.cjs'),
                          str(lane / (name + '.json')), str(lane / (name + '-dom.json'))],
                         text=True, capture_output=True)
    (lane / (name + '-dom.log')).write_text(dom.stdout + dom.stderr)
    assert dom.returncode == 0, (name, dom.stdout, dom.stderr)
    scene = json.loads((lane / (name + '.json')).read_text())
    print(name, 'records', len(scene['provenance']['records']), 'real', scene['ruler']['status'],
          'trees', [(t['prime'], t['depths']['5']['status'], t['depths']['5']['lit_path_count'])
                    for t in scene['trees']])
    print(check.stdout.splitlines()[-1])

checks = 0
for name, modulus in [('idempotents30', 30), ('idempotents36', 36)]:
    path = Path('ux/atlas/scenes/idempotents30.json') if modulus == 30 else lane / (name + '.json')
    scene = json.loads(path.read_text())
    for row in scene['modular_solutions']:
        assert int(row['square']) == int(row['residue']) ** 2
        checks += 1
for k, a, b in [(8, 280182, 110443), (12, 123327057, 120813568)]:
    assert (a*a+1) % 5**k == (b*b+1) % 5**k == 0
    assert (a+b) % 5**k == 0 and a % 5 == 2 and b % 5 == 3
    checks += 4
assert 123327057 % 5**8 == 280182 and 120813568 % 5**8 == 110443
checks += 2
scene = json.loads((lane / 'onefifth.json').read_text())
ball = scene['ruler']['balls'][0]
mid, rad = Fraction(ball['midpoint_printed']), Fraction(ball['radius_printed'])
assert mid-rad <= Fraction(1, 5) <= mid+rad
checks += 1
print('supplementary_arithmetic_checks', checks)

dom = json.loads((lane / 'prime101-dom.json').read_text())
row = [e for e in dom['elements'] if e['tag'] == 'circle' and e.get('cy') == '286']
assert len(row) == 101
left, right = sorted(row, key=lambda e: float(e['cx']))[:2]
distance = float(right['cx'])-float(left['cx'])
diameter = float(left['r'])+float(right['r'])
assert distance < diameter
print('D1: first-row circles', len(row), 'centre_distance', distance, 'radii_sum', diameter)

names = re.findall(r'^#define ADF_(\w+)[ \t]+\d+[ \t]+/\*',
                   Path('include/adelefeld/status.h').read_text(), re.M)
names = [name for name in names if not name.startswith('CMP_')]
readme = Path('ux/atlas/README.md').read_text()
assert len(names) == 11 and all(name in readme for name in names)
print('status_names', len(names), 'missing', 0)

for name, expected in [('exp0', '1'), ('log1', '0')]:
    scene = json.loads((lane / (name + '.json')).read_text())
    assert [t['value'] for t in scene['threads']] == [expected]
scene = json.loads((lane / 'fractionroot.json').read_text())
assert {t['value'] for t in scene['threads']} == {'-1/5', '1/5'}
scene = json.loads((lane / 'sqrt2.json').read_text())
assert not scene['threads'] and all(b['dyadic_endpoints'] == 'not given' for b in scene['ruler']['balls'])
print('additional_builder_claims', 4, 'failures', 0)
