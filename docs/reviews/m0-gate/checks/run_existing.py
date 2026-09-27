#!/usr/bin/env python3
"""Run read-only suites in sequence, with a 170-second limit per process."""
import os
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
commands = {
    "text": [sys.executable, "-B", "-m", "unittest", "proto/test_text_grammar.py"],
    "ref": [sys.executable, "-B", "-m", "unittest", "discover", "-s", "tests/ref/tests"],
    "mutants": [sys.executable, "-B", "tests/ref/mutants.py"],
    "analysis": [sys.executable, "-B", "proto/analysis_checks.py"],
    "seams": [sys.executable, "-B", "proto/seams_checks.py"],
}
env = dict(os.environ, PYTHONDONTWRITEBYTECODE="1", OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1")
for name in sys.argv[1:]:
    command = commands[name]
    start = time.monotonic()
    result = subprocess.run(command, cwd=ROOT, env=env, capture_output=True, text=True, timeout=170)
    log = "$ " + " ".join(command) + "\n" + result.stdout + result.stderr
    log += f"exit={result.returncode} seconds={time.monotonic() - start:.3f}\n"
    (HERE / ("existing_" + name + ".out")).write_text(log)
    print(name, log, flush=True)
    if result.returncode:
        raise SystemExit(result.returncode)
