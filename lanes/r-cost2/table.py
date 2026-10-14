#!/usr/bin/env python3
"""Verify benchmark results and emit the requested median table."""
import json
import pathlib
import re
import statistics

root = pathlib.Path("lanes/r-cost2")
families = [("cluster", 190), ("scaled", 150), ("tiny", 150),
            ("random", 10000), ("deg", 190), ("small", 0)]
rows = ["| Family / e / d | adf before s | adf after s | count before s | count after s |",
        "| --- | ---: | ---: | ---: | ---: |"]
for family, e in families:
    values = {}
    for phase in ("before", "after"):
        records = []
        for path in root.glob(f"{phase}-{family}*.json"):
            records.extend(json.loads(path.read_text()))
        assert len(records) == 10, (phase, family, len(records))
        for mode in ("adf", "count"):
            samples = [r for r in records if r["command"][6] == mode]
            assert len(samples) == 5
            for record in samples:
                assert record["exit"] == 0
                out = record["stdout"]
                if isinstance(out, list):
                    out = "\n".join(out)
                expected = 12 if family == "random" else 50
                if mode == "adf":
                    assert re.search(rf"status 0, balls {expected},", out), out
                else:
                    assert re.search(rf"n = {expected}\b", out), out
            values[phase, mode] = statistics.median(r["seconds"] for r in samples)
    row = f"| {family} / {e} / 50 |"
    for phase, mode in (("before", "adf"), ("after", "adf"), ("before", "count"), ("after", "count")):
        row += f" {values[phase, mode]:.6f} |"
    rows.append(row)
text = "\n".join(rows) + "\n"
(root / "table.md").write_text(text)
print(text, end="")
print("120 benchmark calls, 0 failed statuses or counts")
