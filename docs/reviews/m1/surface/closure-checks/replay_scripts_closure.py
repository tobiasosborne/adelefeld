#!/usr/bin/env python3
"""Run the review oracle on the original driver scripts after three new scripts were added."""
from pathlib import Path
import sys

sys.path.insert(0, "docs/reviews/m1/surface/checks")
import oracle  # noqa: E402

total = agreed = different = unmodelled = 0
for command_file in sorted(Path("tests/driver").glob("*.cmd")):
    if command_file.name.startswith("09") or command_file.name[:2] not in [
        "01", "02", "03", "04", "05", "06", "07", "08", "10"
    ]:
        continue
    expected = command_file.with_suffix(".out").read_text().splitlines()
    commands = [line for line in command_file.read_text().splitlines()
                if line.strip() and not line.lstrip().startswith("#")]
    assert len(commands) == len(expected), command_file
    for command, want in zip(commands, expected):
        total += 1
        try:
            got = oracle.run(command)
        except Exception:
            got = None
        if got is None:
            unmodelled += 1
        elif got == want:
            agreed += 1
        else:
            different += 1
            print(f"DIFFERENT {command_file}: {command[:80]}")
print(f"lines {total}, agree {agreed}, differ {different}, not modelled {unmodelled}")
raise SystemExit(bool(different))
