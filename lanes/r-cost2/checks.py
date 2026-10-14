#!/usr/bin/env python3
"""Run whole real-root suites with per-program timeouts and explicit exit records."""
import json
import pathlib
import subprocess
import sys

config = sys.argv[1]
root = pathlib.Path("lanes/r-cost2")
records = []
for name in ("test_roots_real", "test_roots_real_isolate"):
    cmd = ["timeout", "170", "taskset", "-c", "2"]
    if len(sys.argv) > 2:
        cmd += ["env", "ASAN_OPTIONS=detect_leaks=0"]
    cmd += [str(root / f"build-{config}" / name)]
    result = subprocess.run(cmd, capture_output=True, text=True)
    log = root / f"check-{config}-{name}{'-nolsan' if len(sys.argv) > 2 else ''}.log"
    log.write_text(result.stdout + result.stderr)
    records.append({"command": cmd, "exit": result.returncode, "log": str(log)})
    print(config, name, "exit", result.returncode, flush=True)
    print("\n".join(result.stdout.splitlines()[-4:]), flush=True)
(root / f"check-{config}{'-nolsan' if len(sys.argv) > 2 else ''}.json").write_text(
    json.dumps(records, indent=2) + "\n")
sys.exit(int(any(record["exit"] for record in records)))
