#!/usr/bin/env python3
"""Run the stock driver script without creating build files outside this lane."""
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'lanes/f4-slice7/driverroot'
if OUT.exists():
    shutil.rmtree(OUT)
for name in ('src', 'include', 'tools/adf', 'tests/driver'):
    shutil.copytree(ROOT / name, OUT / name)
shutil.copy2(ROOT / 'Makefile', OUT / 'Makefile')
shutil.copy2(ROOT / 'tests/test_driver.sh', OUT / 'tests/test_driver.sh')
print('Driver sources and all fixtures copied; build stays under lanes/f4-slice7/driverroot.')
