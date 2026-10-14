"""Run the unchanged driver test in a lane scratch tree, with optional F3 fault."""
from pathlib import Path
import os
import shutil
import subprocess
import sys
root = Path(__file__).resolve().parents[2]
lane = root / 'lanes/t5-repair2'
stage = lane / 'build-driver'
if not stage.exists():
    (stage / 'tools/adf').mkdir(parents=True)
    (stage / 'build').mkdir()
    shutil.copy2(root / 'Makefile', stage / 'Makefile')
    shutil.copy2(root / 'tools/adf/Makefile', stage / 'tools/adf/Makefile')
    for name in ('src', 'include', 'tests'):
        (stage / name).symlink_to(root / name, target_is_directory=True)
    (stage / 'build/drv-plain').symlink_to(lane / 'build-plain', target_is_directory=True)
source = (root / 'tools/adf/adf.c').read_text()
old = 'acb_mul(R, W, B, wp);'
assert source.count(old) == 1
fault = sys.argv[1] == 'fault'
if fault:
    source = source.replace(old, 'acb_set(R, A);')
(stage / 'tools/adf/adf.c').write_text(source)
env = dict(os.environ, MAKEFLAGS='-j2')
p = subprocess.run(['timeout', '170', 'sh', 'tests/test_driver.sh'], cwd=stage,
                   env=env, text=True, capture_output=True)
name = 'fault' if fault else 'baseline'
(lane / ('f3-driver-' + name + '.log')).write_text(p.stdout + p.stderr)
print(p.stdout, end='')
print('driver test exit =', p.returncode)
# For the fault this script succeeds only when the unchanged test rejects it.
expected = p.returncode == (1 if fault else 0)
if fault:
    expected = expected and 'test_driver: tate-fe: the output differs' in p.stdout
    expected = expected and '1.5e-27' in p.stdout and '2.1e-26' in p.stdout
raise SystemExit(0 if expected else 1)
