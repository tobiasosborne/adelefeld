"""Summarise raw bounded benchmark runs; keep exclusions explicit."""
import collections
import json
import re
import statistics
from pathlib import Path

lane = Path("lanes/r-cost1")
groups = collections.defaultdict(list)
for name in ("before.jsonl", "before-serial.jsonl", "counts.jsonl", "after.jsonl", "paired.jsonl",
             "references.jsonl"):
    path = lane / name
    if not path.exists():
        continue
    for index, line in enumerate(path.read_text().splitlines()):
        row = json.loads(line)
        command = row["command"]
        binary, mode, family, e, degree, prec = command[-6:]
        if name == "before.jsonl":
            if family == "random" and index == 0:
                continue
            if family in ("cluster", "sqrt2", "xn2") and prec != "262144":
                continue
            if family == "xn2" and prec == "262144":
                # These three serial runs were before the repeat batch.
                pass
        phase = "after" if "build-after" in binary else "before"
        if name == "paired.jsonl":
            phase = "paired " + phase
        if name == "counts.jsonl":
            phase = "count experiment"
        if name == "references.jsonl":
            phase = "reference"
        match = re.search(r"(?:adf|count|eval2): ([0-9.]+)", row["stdout"])
        seconds = float(match.group(1)) if match else None
        groups[(phase, mode, family, e, degree, prec)].append((seconds, row["exit"]))

lines = ["| Phase | Mode/input | e | d | prec | Completed/attempts | Median s | Range s |",
         "| --- | --- | ---: | ---: | ---: | ---: | ---: | --- |"]
for key, rows in groups.items():
    phase, mode, family, e, degree, prec = key
    times = [t for t, exit_code in rows if t is not None and exit_code == 0]
    if phase.startswith("paired") or phase == "reference":
        times = times[-5:]
    if times:
        median = f"{statistics.median(times):.9f}"
        span = f"{min(times):.6f} to {max(times):.6f}"
    else:
        median = ">170 (censored)" if all(code == 124 for _, code in rows) else "unavailable"
        span = "-"
    lines.append(f"| {phase} | {mode}/{family} | {e} | {degree} | {prec} | "
                 f"{len(times)}/{len(rows)} | {median} | {span} |")
lane.joinpath("tables.md").write_text("\n".join(lines) + "\n")
print("\n".join(lines))
