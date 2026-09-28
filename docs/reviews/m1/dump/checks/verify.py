"""Run reviewed checks sequentially, recording exact commands and exit codes. Run at repository root."""
import json
import os
from pathlib import Path
import shlex
import signal
import subprocess
import sys
import time

P = Path("docs/reviews/m1/dump/checks")
phase = sys.argv[1]
jobs = []

def add(name, args, timeout=165):
    jobs.append((name, args, timeout))

if phase == "build":
    add("build", ["make", "-j2", f"BUILD={P}/build",
        "CFLAGS=-std=c11 -O2 -g -fPIC -Wall -Wextra -Wpedantic -Werror",
        *[f"{P}/build/{t}" for t in ("test_dump", "test_dump_ctx", "test_dump_golden")]])
    for source, output, extra in [("bridge.c", "bridge.so", ["-fPIC", "-shared"]),
                                  ("bridge.c", "bridge", []), ("roundtrip.c", "roundtrip", [])]:
        add(f"compile-{output}", ["cc", "-std=c11", "-O2", "-g", "-Wall", "-Wextra", "-Werror", "-Iinclude",
            *extra, str(P/source), f"{P}/build/libadelefeld.a", "-lflint", "-lgmp", "-lm", "-o", str(P/output)])
elif phase == "unit":
    for t in ("test_dump", "test_dump_ctx", "test_dump_golden"):
        add(t, [f"{P}/build/{t}"])
    for t in ("bridge", "roundtrip"):
        add(t, [str(P/t)])
    add("status", ["python3", "-B", str(P/"status_findings.py")])
elif phase == "diff":
    add("differential", ["python3", "-B", str(P/"differential.py")])
elif phase == "memory":
    for t in ("bridge", "roundtrip"):
        add(f"valgrind-{t}", ["valgrind", "--leak-check=full",
            "--errors-for-leak-kinds=definite,indirect", "--error-exitcode=99", str(P/t)])
    add("fuzz-build", ["cc", "-std=c11", "-O0", "-g", "-I.", "-Iinclude", str(P/"fuzz_driver.c"),
        f"{P}/build/libadelefeld.a", "-lflint", "-lgmp", "-lm", "-o", str(P/"fuzz-gcc-o0")])
    add("fuzz-seed", ["valgrind", "--track-origins=yes", "--error-exitcode=99", str(P/"fuzz-gcc-o0")])
elif phase == "san":
    add("san-build", ["make", "-j2", f"BUILD={P}/san", "SAN=1",
        *[f"{P}/san/{t}" for t in ("test_dump", "test_dump_ctx", "test_dump_golden")]])
    for t in ("bridge", "roundtrip"):
        add(f"{t}-san-build", ["cc", "-std=c11", "-O1", "-g", "-Iinclude",
            "-fsanitize=address,undefined", "-fno-omit-frame-pointer", str(P/f"{t}.c"),
            f"{P}/san/libadelefeld.a", "-lflint", "-lgmp", "-lm", "-o", str(P/f"{t}-san")])
    for t in ("test_dump", "test_dump_ctx", "test_dump_golden", "bridge", "roundtrip"):
        exe = f"{P}/san/{t}" if t.startswith("test") else str(P/f"{t}-san")
        add(f"san-{t}", ["env", "ASAN_OPTIONS=detect_leaks=0", "UBSAN_OPTIONS=halt_on_error=1", exe])
elif phase == "cost":
    add("cost", ["python3", "-B", str(P/"cost.py"), "65537"])
elif phase in ("near-modctx", "near-qclass"):
    add(phase, ["python3", "-B", str(P/"cost_near_limit.py"), phase.split("-")[1]])
else:
    raise SystemExit("unknown phase")

for name, args, timeout in jobs:
    start = time.monotonic()
    with (P/f"verified-{name}.log").open("wb") as log:
        try:
            process = subprocess.Popen(args, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
            code = process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.wait()
            code = "timeout"
    row = dict(name=name, command=shlex.join(args), exit=code, seconds=round(time.monotonic()-start, 6))
    with (P/"verified-commands.jsonl").open("a") as f:
        f.write(json.dumps(row)+"\n")
    print(json.dumps(row), flush=True)
