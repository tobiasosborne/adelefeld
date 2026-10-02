from pathlib import Path
import os
import subprocess

lane = Path("lanes/n-review2")
commands = ["prec 20000", "digits 1"]
cases = []
for digits in (1000, 1500):
    real = "1 +/- 0." + "9" * digits
    for kind in ("idele", "class"):
        text = f"({real} ; 1 * [1])" if kind == "idele" else f"<{real} ; [1]>"
        commands.append("show " + text)
        cases.append((digits, kind))
(lane / "driver-limit.cmd").write_text("\n".join(commands) + "\n")
r = subprocess.run(["timeout", "45", str(lane / "adf-san"), str(lane / "driver-limit.cmd")],
                   capture_output=True, text=True, timeout=50,
                   env=dict(os.environ, ASAN_OPTIONS="detect_leaks=0"))
(lane / "driver-limit.out").write_text(r.stdout)
(lane / "driver-limit.err").write_text(r.stderr)
lines = r.stdout.splitlines()
fail = int(len(lines) != 4 or r.returncode != 1 or bool(r.stderr))
for (digits, kind), line in zip(cases, lines):
    if digits == 1500:
        fail += line != "error: LIMIT"
    else:
        fail += line.startswith("error:")
    print(f"nines={digits} kind={kind} length={len(line)} limit={line == 'error: LIMIT'}")
print(f"driver_exit={r.returncode} stderr_bytes={len(r.stderr)} failures={fail}")
raise SystemExit(bool(fail))
