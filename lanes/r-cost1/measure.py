"""One bounded batch of the real-root benchmark, pinned to CPU 2."""
import json
import re
import subprocess
import sys
import time

binary, output, mode, family, e, degree, prec, repetitions = sys.argv[1:]
for repetition in range(int(repetitions)):
    command = ["timeout", "170", "taskset", "-c", "2", binary, mode, family, e, degree, prec]
    start = time.time()
    result = subprocess.run(command, capture_output=True, text=True)
    row = dict(command=command, repetition=repetition, exit=result.returncode,
               stdout=result.stdout, stderr=result.stderr, start=start, end=time.time())
    with open(output, "a") as stream:
        stream.write(json.dumps(row) + "\n")
    seconds = re.findall(r"(?:adf|count|eval2): ([0-9.]+)", result.stdout)
    print(f"{mode} {family} {e} {degree} {prec} run {repetition}: exit {result.returncode}, {seconds}",
          flush=True)
    if result.returncode:
        break
