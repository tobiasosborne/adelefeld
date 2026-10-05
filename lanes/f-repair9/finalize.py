"""Audit retained results and remove only this lane's scratch builds. Report is written separately."""
import hashlib
import json
import re
import shutil
from pathlib import Path

lane = Path(__file__).resolve().parent
root = lane.parents[1]
configurations = {}
for name in ('plain', 'san', 'inv', 'clang'):
    data = json.loads((lane / f'check-{name}.json').read_text())
    want = ['14', '306749', '0', '0'] if name == 'inv' else ['13', '306730', '0', '0']
    assert data['exit'] == 0 and data['summary'] == [want]
    configurations[name] = data
assert configurations['plain']['seconds'] < 30
faults = [json.loads(s) for s in (lane / 'fault-table.log').read_text().splitlines()]
assert len(faults) == 13 and all(r['build_exit'] == 0 for r in faults)
assert all(r['test_exit'] == (0 if r['fault'] in ('no-logpi-B', 'no-M0-B') else 1) for r in faults)
assert all(re.search(r'13 tests, \d+ checks, \d+ failed checks, \d+ failed tests', r['totals']) for r in faults)
f1 = (lane / 'mutants/half-B.log').read_text()
assert 'near-pole box 0 sample 2 prec=16: outside' in f1
assert 'near-pole box 1 sample 2 prec=256: outside' in f1
f2 = (lane / 'mutants/lower-E.log').read_text()
assert all(f'closed zero p=2 e=0 sx=1 sy=0 prec={p}: status 0' in f2 for p in (2, 128))
f3 = (lane / 'mutants/open-integer.log').read_text()
assert 'closed pole n=64 width=2^0 side=1 prec=2: status 10' in f3
assert 'closed pole n=100 width=2^0 side=1 prec=64: status 10' in f3
fixture = root / 'tests/ref/vectors/f-repair9/near-poles.jsonl'
lines = fixture.read_text().splitlines()
assert len(lines) == 6728 and max(map(len, lines)) <= 116 and fixture.stat().st_size <= 300000
assert sum('"s":' in s for s in lines) == 1346
assert len(lines)-1346 == 5382
cross = json.loads((lane / 'crosscheck.json').read_text())
assert (cross['digits'], cross['samples'], cross['failures']) == (400, 20, 0)
assert 'failures=0' in (lane / 'compare-guard.log').read_text()
assert '0 failures' in (lane / 'active-check.log').read_text()
assert '1 check, 0 failures' in (lane / 'bound-final.log').read_text()
for seed in (9041, 9042):
    assert 'misses=0 bad_B=0' in (lane / f'search-final-{seed}.log').read_text()
sources = [root / p for p in ('src/localfactor.c', 'tests/test_localfactor.c', 'docs/api-1f9.md')]
sources += sorted(lane.glob('*.py'))+sorted(lane.glob('*.c'))+[lane / 'f5-analysis.md', fixture]
for path in sources:
    data = path.read_bytes()
    assert data.endswith(b'\n'), path
    assert all(len(line) <= 116 for line in data.decode().splitlines()), path
old_hash = hashlib.sha256((lane / 'old-localfactor.c').read_bytes()).hexdigest()
assert old_hash == 'e5efca95e7cb11c9c90139af41d62f79debb63bcb85286cad82ccb9f08f01dba'
removed = []
for name in ('build', 'build-san', 'build-inv', 'build-clang', '__pycache__'):
    path = lane / name
    if path.exists():
        shutil.rmtree(path)
        removed.append(name)
for pole in (0, -2, -4, -16):
    path = lane / f'pole{pole}.json'
    path.unlink()
    removed.append(path.name)
assert not any(lane.glob('build/'))
audit = dict(configurations=configurations, fault_count=13, detected=11, surviving=2,
             fixture_bytes=fixture.stat().st_size, fixture_records=len(lines), fixture_boxes=1346,
             fixture_samples=5382, longest_fixture_line=max(map(len, lines)), old_source_sha256=old_hash,
             line_checked_files=len(sources), removed=removed)
(lane / 'audit.json').write_text(json.dumps(audit, indent=2)+'\n')
print('audit: 4 configurations, 13 faults, 11 detected, 2 surviving, 0 assertion failures')
print('cleanup:', len(removed), 'owned scratch paths removed')
