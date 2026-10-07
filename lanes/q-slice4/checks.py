#!/usr/bin/env python3
"""Sequential final checks. Two compiler jobs, explicit timeouts, concise per-command records."""
from pathlib import Path
import os
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
LANE = ROOT / "lanes/q-slice4"
records = []
logname = "own-final-checks.log" if "--own-only" in sys.argv else "final-checks.log"


def run(cmd, seconds, env=None):
    process = subprocess.run(["timeout", str(seconds)] + cmd, cwd=ROOT, env=env,
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    data = process.stdout
    record = "$ " + " ".join(cmd) + f"\nexit {process.returncode}\n" + data
    records.append(record)
    (LANE / logname).write_text("\n".join(records))
    print("exit", process.returncode, " ".join(cmd[:4]), flush=True)
    if data:
        print(data[-1500:], end="", flush=True)
    return process.returncode


for config, options in (("build", []), ("san", ["SAN=1"]), ("inv", ["INV=1"]),
                        ("clang", ["CC=clang"]), ("san-inv", ["CC=clang", "SAN=1", "INV=1"])):
    binaries = [f"lanes/q-slice4/{config}/test_qclass_{kind}" for kind in ("text", "dump")]
    assert run(["make", "-s", "-j2", f"BUILD=lanes/q-slice4/{config}"] + options + binaries, 180) == 0
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
    for binary in binaries:
        assert run([binary], 60, env) == 0

if "--own-only" in sys.argv:
    raise SystemExit(0)

sources = sorted({p for pattern in ("test_dump*.c", "test_text*.c", "test_qclass*.c")
                  for p in (ROOT / "tests").glob(pattern)})
binaries = ["lanes/q-slice4/build/" + p.stem for p in sources]
assert run(["make", "-s", "-j2", "BUILD=lanes/q-slice4/build"] + binaries, 180) == 0
failed = []
for binary in binaries:
    if run([binary], 60):
        failed.append(binary)
print("regression programs:", len(binaries), "failed:", len(failed), flush=True)
(LANE / "regression-results.txt").write_text(
    f"{len(binaries)} programs; {len(binaries)-len(failed)} exit 0; {len(failed)} nonzero\n" +
    "\n".join(failed) + "\n")
run(["sh", "tests/test_driver.sh"], 180)
assert run(["cc", "-shared", "-fPIC", "-Iinclude", "-std=c11", "-O1", "-g", "-Wall", "-Wextra"] +
           [str(p.relative_to(ROOT)) for p in sorted((ROOT / "src").glob("*.c"))] +
           ["-o", "lanes/q-slice4/build/libadelefeld.so", "-lflint", "-lgmp", "-lm"], 180) == 0
env = dict(os.environ, JULIA_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1",
           LD_PRELOAD="/lib/x86_64-linux-gnu/libgmp.so.10")
assert run(["julia", "--startup-file=no", "tests/julia/qclass_text.jl",
            "lanes/q-slice4/build/libadelefeld.so"], 60, env) == 0
assert (LANE / "final-checks.log").stat().st_size < 100000
