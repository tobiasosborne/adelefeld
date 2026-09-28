#!/usr/bin/env python3
"""Show that the R1 selftest catches removal of its new first-use condition.

Run from the scratch repository root. The source file is restored before exit.
"""
from pathlib import Path
import subprocess

source = Path("tools/memcheck/check_uninit.py")
original = source.read_text()
old = "if var.init_line is None and var.use_line is None:"
assert original.count(old) == 1
command = ["python3", "tools/memcheck/check_uninit.py", "--category", "use-before-init",
           "docs/reviews/m1/surface/checks/memcheck/s1_use_then_init.c"]
try:
    green = subprocess.run(command, capture_output=True, text=True, check=False)
    source.write_text(original.replace(old, "if False:  # scratch reversal of the R1 check"))
    red = subprocess.run(command, capture_output=True, text=True, check=False)
finally:
    source.write_text(original)

print(f"R1 current: exit {green.returncode}, findings {len(green.stdout.splitlines())}")
print(f"R1 reversed: exit {red.returncode}, findings {len(red.stdout.splitlines())}")
raise SystemExit(not (green.returncode == 1 and len(green.stdout.splitlines()) == 1
                      and red.returncode == 0 and len(red.stdout.splitlines()) == 0))
