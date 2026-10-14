#!/usr/bin/env python3
"""Isolate the degree-family timing comparison from shared-machine clock drift."""
import json
import pathlib
import re
import statistics
import subprocess

records = []
for repeat in range(5):
    for phase in ("before", "after"):
        cmd = ["timeout", "170", "taskset", "-c", "2",
               f"lanes/r-cost2/build-{phase}/bench_roots_real", "adf", "deg", "190", "50", "2"]
        result = subprocess.run(cmd, capture_output=True, text=True)
        assert result.returncode == 0
        assert "status 0, balls 50," in result.stdout
        seconds = float(re.search(r"adf: ([0-9.]+) s", result.stdout).group(1))
        records.append({"command": cmd, "phase": phase, "repeat": repeat,
                        "seconds": seconds, "exit": result.returncode,
                        "stdout": result.stdout.splitlines()})
for phase in ("before", "after"):
    samples = [r["seconds"] for r in records if r["phase"] == phase]
    print(phase, "deg paired median", f"{statistics.median(samples):.6f}", "s")
pathlib.Path("lanes/r-cost2/paired-deg.json").write_text(json.dumps(records, indent=2) + "\n")
