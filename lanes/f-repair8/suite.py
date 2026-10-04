"""Run the mandatory whole suite in this lane, keeping fixed build/ paths in lane storage."""
from pathlib import Path
import shutil
import subprocess
import json
import time

root = Path.cwd()
lane = root / 'lanes/f-repair8'
suite = lane / 'build/suite'
suite.mkdir(exist_ok=True)
for name in ('src', 'include', 'tests', 'refs', 'proto', 'docs', 'lanes'):
    (suite / name).symlink_to(root / name, target_is_directory=True)
shutil.copytree(root / 'tools', suite / 'tools', ignore=shutil.ignore_patterns('__pycache__'))
makefile = (root / 'Makefile').read_text()
makefile = makefile.replace('./$$t || fail=1;', 'timeout 170 ./$$t || fail=1;')
for cmd in ('sh tests/test_driver.sh', 'sh tests/test_exports.sh', 'sh tests/test_julia.sh',
            'python3 tools/mutate/selftest.py', 'python3 tools/memcheck/selftest.py'):
    makefile = makefile.replace(cmd + ';', 'timeout 170 ' + cmd + ';')
(suite / 'Makefile').write_text(makefile)
start = time.monotonic()
with (lane / 'check-all.log').open('w') as log:
    result = subprocess.run(['timeout', '1700', 'make', '-j2', 'check-all'], cwd=suite,
                            stdout=log, stderr=subprocess.STDOUT)
record = dict(command='timeout 1700 make -j2 check-all', cwd=str(suite), exit=result.returncode,
              wall=round(time.monotonic() - start, 6))
(lane / 'check-all.json').write_text(json.dumps(record, indent=2) + '\n')
print(json.dumps(record))
raise SystemExit(result.returncode)
