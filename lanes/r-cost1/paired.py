"""Five interleaved before/after calls on one CPU, bounded individually."""
import json
import re
import subprocess
import sys
import time
from pathlib import Path

family, e, degree, prec = sys.argv[1:5]
repetitions = int(sys.argv[5]) if len(sys.argv) > 5 else 5
lane = Path("lanes/r-cost1")
for repetition in range(repetitions):
    for phase in ("before", "after"):
        binary = lane / f"build-{phase}/bench_roots_real"
        command = ["timeout", "170", "taskset", "-c", "2", str(binary), "adf", family, e, degree, prec]
        start = time.time()
        result = subprocess.run(command, capture_output=True, text=True)
        row = dict(command=command, repetition=repetition, exit=result.returncode,
                   stdout=result.stdout, stderr=result.stderr, start=start, end=time.time())
        with (lane / "paired.jsonl").open("a") as stream:
            stream.write(json.dumps(row) + "\n")
        times = re.findall(r"adf: ([0-9.]+)", result.stdout)
        print(phase, family, prec, repetition, "exit", result.returncode, times, flush=True)
