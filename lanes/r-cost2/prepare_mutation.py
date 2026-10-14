#!/usr/bin/env python3
"""Copy the needed source tree into lane-owned scratch storage."""
import pathlib
import shutil

root = pathlib.Path("lanes/r-cost2/build-mutation-root")
root.mkdir(parents=True, exist_ok=True)
for name in ("Makefile", "include", "src", "tests"):
    source = pathlib.Path(name)
    dest = root / name
    if source.is_dir():
        shutil.copytree(source, dest, dirs_exist_ok=True,
                        ignore=shutil.ignore_patterns("__pycache__"))
    else:
        shutil.copy2(source, dest)
lane = root / "lanes/r-cost2"
lane.mkdir(parents=True, exist_ok=True)
shutil.copy2("lanes/r-cost2/mutation-check.sh", lane / "mutation-check.sh")
print("copied Makefile, include, src, tests, mutation-check.sh")
