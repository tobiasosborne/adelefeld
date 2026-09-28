#!/usr/bin/env python3
"""Check text keys on current source and after a line shift in an owned scratch tree.

Run from the scratch repository root. The source file is restored before exit.
"""
from pathlib import Path
import subprocess

source = Path("src/scaled.c")
original = source.read_bytes()
command = ["python3", "tools/mutate/check_equivalent.py"]
try:
    first = subprocess.run(command, capture_output=True, text=True, check=False)
    source.write_bytes(b"\n" + original)
    shifted = subprocess.run(command, capture_output=True, text=True, check=False)
finally:
    source.write_bytes(original)

for label, result in (("current", first), ("shifted by one line", shifted)):
    print(f"{label}: exit {result.returncode}, {result.stdout.strip()}")
good = first.returncode == shifted.returncode == 0
good &= "108 entries" in first.stdout and "108 entries" in shifted.stdout
raise SystemExit(not good)
