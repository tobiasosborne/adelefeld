#!/usr/bin/env python3
"""Verify final check records, remove lane builds, and audit retained artifacts."""
import json
import pathlib
import re
import shutil
import textwrap

lane = pathlib.Path("lanes/r-cost2")
for config in ("after", "san", "inv", "clang"):
    suffix = "-nolsan" if config == "san" else ""
    records = json.loads((lane / f"check-{config}{suffix}.json").read_text())
    assert len(records) == 2
    for record in records:
        assert record["exit"] == 0, record
        content = pathlib.Path(record["log"]).read_text()
        if "isolate" in record["log"]:
            expected = "17 tests, 19363 checks" if config == "inv" else "15 tests, 19320 checks"
        else:
            expected = "12 tests, 10885 checks"
        assert expected + ", 0 failed checks, 0 failed tests" in content
assert "24 killed, 1 survived, 5 not compiled, 0 timed out, 0 excused" in (
    lane / "mutation.log").read_text()
assert "status: killed" in (lane / "replay-survivor.log").read_text()
assert "driver: 2 cases, 29 expected lines, 0 differences" in (lane / "driver.log").read_text()
for path in lane.glob("*.json"):
    records = json.loads(path.read_text())
    if isinstance(records, list):
        for record in records:
            for key in ("stdout", "stderr"):
                if isinstance(record.get(key), str):
                    record[key] = record[key].splitlines()
    path.write_text(json.dumps(records, indent=2) + "\n")
for path in list(lane.iterdir()):
    if path.is_dir() and path.name.startswith("build"):
        shutil.rmtree(path)
for path in lane.glob("*.log"):
    content = path.read_text(errors="replace")
    if path.name == "stdout.log":
        content = content[-6000:]
    lines = []
    for line in content.splitlines():
        lines.extend(textwrap.wrap(line, width=116, break_long_words=True,
                                   replace_whitespace=False) or [""])
    path.write_text("\n".join(lines) + "\n")
large = [str(p) for p in lane.rglob("*") if p.is_file() and p.stat().st_size > 100000]
long_lines = []
missing_newline = []
for path in lane.rglob("*"):
    if not path.is_file():
        continue
    data = path.read_bytes()
    if not data.endswith(b"\n"):
        missing_newline.append(str(path))
    if path.name in ("brief.md", "lane.log"):
        continue
    for line, text in enumerate(data.decode("utf-8").splitlines(), 1):
        if len(text) > 116:
            long_lines.append((str(path), line, len(text)))
roots = pathlib.Path("src/roots.c").read_text()
start = roots.index("/* Count on a small rational translate")
stop = roots.index("/* Algorithm RR", start)
regions = [("real_count", roots[start:stop]),
           ("appended tests", pathlib.Path("tests/test_roots_real.c").read_text().split(
               "/* R9 count regressions (lane r-cost2).", 1)[1]),
           ("appended design", pathlib.Path("docs/design/real-roots.md").read_text().split(
               "### r-cost2\n", 1)[1])]
for name, content in regions:
    for line, text in enumerate(content.splitlines(), 1):
        if len(text) > 116:
            long_lines.append((name, line, len(text)))
assert not large, large
assert not long_lines, long_lines
assert not missing_newline, missing_newline
print("Final check records: 8 programs, 0 nonzero exits")
print("Mutation: 30 sampled, 24 killed, 1 survived, 5 compile failures; replay: 1 killed")
print("Driver: 2 cases, 29 lines, 0 differences")
print("Artifacts: 0 files above 100 KB, 0 long written lines, 0 missing newlines, 0 build trees")
print("Prose audit excludes the pre-existing brief.md and lane.log")
