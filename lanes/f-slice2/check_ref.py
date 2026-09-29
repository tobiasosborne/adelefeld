"""Runs the three checks of the section f-slice2 of proto/functions_checks.py (run from the repository root)."""
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "proto"))
import functions_checks as R  # noqa: E402

t = time.time()
R.sball_main()
print(f"seconds={time.time() - t:.1f}")
