"""Run one closure command with a 170-second process-group limit and a retained log."""
import json
import os
from pathlib import Path
import shlex
import signal
import subprocess
import sys
import time
import resource

HERE = Path(__file__).resolve().parent
name, *args = sys.argv[1:]
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
start = time.monotonic()
with (HERE / (name + ".log")).open("wb") as out:
    proc = subprocess.Popen(args, stdout=out, stderr=subprocess.STDOUT, start_new_session=True,
                            env={**os.environ, "PYTHONDONTWRITEBYTECODE": "1",
                                 "OMP_NUM_THREADS": "1", "OPENBLAS_NUM_THREADS": "1"})
    try:
        code = proc.wait(timeout=170)
    except subprocess.TimeoutExpired:
        os.killpg(proc.pid, signal.SIGKILL)
        proc.wait()
        code = 124
row = dict(name=name, command=shlex.join(args), exit=code, seconds=round(time.monotonic()-start, 6))
with (HERE / "commands.jsonl").open("a") as out:
    out.write(json.dumps(row) + "\n")
print(json.dumps(row), flush=True)
print((HERE / (name + ".log")).read_text(errors="replace")[-5000:], end="")
sys.exit(code)
