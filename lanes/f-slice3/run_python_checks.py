"""Runs the f-slice3 section of proto/functions_checks.py only (python3 -B lanes/f-slice3/run_python_checks.py)."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "proto"))
import functions_checks as R  # noqa: E402

R.slice3_main()
