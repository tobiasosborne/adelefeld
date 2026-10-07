#!/usr/bin/env python3
"""Remove only this lane's named build trees and scratch fault copies."""
from pathlib import Path
import shutil

lane = Path(__file__).resolve().parent
for name in ('build', 'san', 'inv', 'clang', 'san-inv', 'faults', 'survivors'):
    path = lane / name
    if path.exists():
        shutil.rmtree(path)
        print('removed', name)
