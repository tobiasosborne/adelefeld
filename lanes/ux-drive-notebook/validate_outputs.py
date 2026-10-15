from pathlib import Path
import hashlib
import json

lane = Path(__file__).resolve().parent
checks = json.loads((lane/'checks.json').read_text())
assert len(checks) == 29
assert all((lane/c['output']).is_file() for c in checks)
for name in ['drive.md', 'report.md']:
    lines = (lane/name).read_text().splitlines()
    bad = [i for i, line in enumerate(lines, 1) if len(line) > 116]
    assert not bad, (name, bad)
    print(name, 'lines', len(lines), 'lines above 116:', len(bad))
hashes = json.loads((lane/'slice-sha256.json').read_text())
for p in (lane/'observed').iterdir():
    assert hashlib.sha256(p.read_bytes()).hexdigest() == hashes['out/'+p.name]
print('4 observed files match retained slice hashes; 29 command outputs exist')
assert not (lane/'work').exists()
assert not list(lane.glob('firefox-*'))
print('0 temporary work trees; 0 Firefox profiles')
