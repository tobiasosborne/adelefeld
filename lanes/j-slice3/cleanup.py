#!/usr/bin/env python3
"""Remove only the generated paths explicitly named by the lane brief."""
from pathlib import Path
import shutil

assert Path("julia/Adelefeld/src/Adelefeld.jl").is_file()
targets = [Path("build"), Path("lanes/j-slice3/depot"), Path("lanes/j-slice3/tmp"),
           Path("lanes/j-slice3/scratch"), Path("julia/Adelefeld/Manifest.toml")]
removed = 0
for p in targets:
    if p.is_symlink() or p.is_file():
        p.unlink()
        removed += 1
    elif p.is_dir():
        shutil.rmtree(p)
        removed += 1
remaining = sum(p.exists() or p.is_symlink() for p in targets)
print(f"cleanup targets: {len(targets)}; removed: {removed}; remaining: {remaining}")
assert remaining == 0
