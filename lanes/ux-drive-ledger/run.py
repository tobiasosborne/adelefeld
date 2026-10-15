import argparse
import pathlib
import shlex
import subprocess
import time

lane = pathlib.Path(__file__).resolve().parent
p = argparse.ArgumentParser()
p.add_argument("name")
p.add_argument("expected")
p.add_argument("command", nargs=argparse.REMAINDER)
a = p.parse_args()
timestamp = subprocess.check_output(["timeout", "5", "date", "-Iseconds"], text=True).strip()
started = time.monotonic()
r = subprocess.run(["timeout", "175", *a.command], text=True, stdout=subprocess.PIPE,
                   stderr=subprocess.STDOUT)
duration = time.monotonic() - started
(lane / (a.name + ".log")).write_text(r.stdout)
with (lane / "drive.md").open("a") as f:
    f.write(f"\n### {a.name} at {timestamp}\n\nExpected: {a.expected}\n\nCommand:\n\n```sh\n")
    f.write(shlex.join(["timeout", "175", *a.command]) + "\n```\n\n")
    f.write(f"Exit: {r.returncode}. Elapsed: {duration:.3f} s. Full output: `{a.name}.log`.\n\n")
    f.write("```text\n" + r.stdout + "\n```\n")
print(f"{a.name}: exit={r.returncode}, elapsed={duration:.3f}s")
print(r.stdout[-18000:])
