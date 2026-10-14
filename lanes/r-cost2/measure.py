#!/usr/bin/env python3
"""Five serial benchmark samples, each bounded to 170 seconds on CPU 2."""
import json
import pathlib
import re
import statistics
import subprocess
import sys

phase, family = sys.argv[1:3]
modes = sys.argv[3:] or ["adf", "count"]
suffix = "-" + modes[0] if len(modes) == 1 else ""
inputs = {
    "cluster": (190, 50), "scaled": (150, 50), "tiny": (150, 50),
    "random": (10000, 50), "deg": (190, 50), "small": (0, 50),
}
e, d = inputs[family]
bench = f"lanes/r-cost2/build-{phase}/bench_roots_real"
records = []
for mode in modes:
    values = []
    for repeat in range(5):
        cmd = ["timeout", "170", "taskset", "-c", "2", bench, mode, family, str(e), str(d), "2"]
        result = subprocess.run(cmd, capture_output=True, text=True)
        match = re.search(r"(?:adf|count): ([0-9.]+) s", result.stdout)
        seconds = float(match.group(1)) if match else None
        records.append({"command": cmd, "repeat": repeat, "exit": result.returncode,
                        "seconds": seconds, "stdout": result.stdout.splitlines(),
                        "stderr": result.stderr.splitlines()})
        pathlib.Path(f"lanes/r-cost2/{phase}-{family}{suffix}.json").write_text(
            json.dumps(records, indent=2) + "\n")
        print(phase, family, mode, repeat, result.returncode, seconds, flush=True)
        if result.returncode != 0 or seconds is None:
            sys.exit(1)
        values.append(seconds)
    print("median", phase, family, mode, f"{statistics.median(values):.6f}", flush=True)
