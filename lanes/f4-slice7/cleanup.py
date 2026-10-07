#!/usr/bin/env python3
"""Remove this lane's build trees, binaries and mutation scratch; retain sources and check summaries."""
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[2]
LANE = ROOT / 'lanes/f4-slice7'
paths = [LANE / name for name in ('plain', 'san', 'inv', 'clang', 'combined', 'mutroot', 'driverroot',
                                 'adf', 'adf-red', 'layout_probe', 'libadelefeld.so', 'stdout.log')]
paths.append(Path('/tmp/adf-f4-slice7-mutate'))
removed = 0
for p in paths:
    if p.is_dir():
        shutil.rmtree(p)
        removed += 1
    elif p.exists():
        p.unlink()
        removed += 1
print('Removed', removed, 'build/scratch/binary/log paths; remaining build directories:',
      sum((LANE / name).exists() for name in ('plain', 'san', 'inv', 'clang', 'combined', 'mutroot', 'driverroot')))
print('Remaining mutation scratch:', int(Path('/tmp/adf-f4-slice7-mutate').exists()))
