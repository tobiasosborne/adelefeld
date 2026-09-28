"""Final read-only artifact checks; write the result log inside the review directory."""
import ast
import hashlib
from pathlib import Path

P = Path("docs/reviews/m1/dump/checks")
documents = [P.parent/"review.md", Path("lanes/m1-review-dump/report.md"), P/"commands.md", P/"README.md"]
long_lines = [(str(p), i, len(line)) for p in documents
              for i, line in enumerate(p.read_text().splitlines(), 1) if len(line) > 116]
syntax = []
for p in sorted(P.glob("*.py")):
    ast.parse(p.read_text())
    syntax.append(p.name)
changed = []
for line in (P/"reviewed-files.sha256").read_text().splitlines():
    digest, path = line.split(maxsplit=1)
    if hashlib.sha256(Path(path).read_bytes()).hexdigest() != digest:
        changed.append(path)
binaries = []
for p in P.rglob("*"):
    if p.is_file():
        with p.open("rb") as f:
            magic = f.read(8)
        if magic.startswith(b"\x7fELF") or magic == b"!<arch>\n":
            binaries.append(str(p))
missing = [str(p) for p in documents if not p.is_file()]
text = (f"documents={len(documents)} missing={len(missing)} lines_over_116={len(long_lines)}\n"
        f"python_scripts_parsed={len(syntax)} syntax_errors=0\n"
        f"reviewed_source_hashes=8 changed_sources={len(changed)}\n"
        f"remaining_build_directories={sum((P/n).exists() for n in ('build', 'san'))} "
        f"remaining_binaries={len(binaries)}\n"
        f"long_lines={long_lines!r} changed={changed!r} binaries={binaries!r}\n")
(P/"final-audit.log").write_text(text)
print(text, end="")
assert not (missing or long_lines or changed or binaries)
