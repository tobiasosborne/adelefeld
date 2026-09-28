"""Inventory executed commands and remove only review-owned build products. Run after all checks finish."""
import json
from pathlib import Path
import shlex
import shutil

HERE = Path(__file__).resolve().parent
rows = [json.loads(line) for line in (HERE/"verified-commands.jsonl").read_text().splitlines()]
out = ["# Executed check commands", "", "Run from the repository root; checks were sequential.", ""]
for i, row in enumerate(rows, 1):
    out += [f"{i}. {row['name']}: exit {row['exit']}; elapsed {row['seconds']} seconds.", "", "```sh"]
    line = ""
    for token in shlex.split(row["command"]):
        token = shlex.quote(token)
        if line and len(line)+len(token)+1 > 110:
            out.append(line+" \\")
            line = "    "+token
        else:
            line += (" " if line else "")+token
    out += [line, "```", ""]
(HERE/"commands.md").write_text("\n".join(out))

removed_dirs = []
for name in ("build", "san"):
    path = HERE/name
    if path.is_dir():
        shutil.rmtree(path)
        removed_dirs.append(name)
removed_files = []
for path in sorted(HERE.iterdir()):
    if path.is_file():
        with path.open("rb") as f:
            magic = f.read(8)
        if magic.startswith(b"\x7fELF") or magic == b"!<arch>\n":
            path.unlink()
            removed_files.append(path.name)
left = []
for path in HERE.rglob("*"):
    if path.is_file():
        with path.open("rb") as f:
            magic = f.read(8)
        if magic.startswith(b"\x7fELF") or magic == b"!<arch>\n":
            left.append(str(path.relative_to(HERE)))
result = dict(command="python3 -B docs/reviews/m1/dump/checks/finish.py", command_records=len(rows),
              removed_build_directories=removed_dirs, removed_binaries=removed_files, remaining_binaries=left)
(HERE/"cleanup.log").write_text(json.dumps(result, indent=2)+"\n")
print(json.dumps(result))
assert not left
