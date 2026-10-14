"""Preserve log text, wrapping retained checks at 116 columns. Leave live harness transcripts alone."""
from pathlib import Path
import textwrap

logs = [p for p in Path("lanes/j-slice4").glob("*.log")
        if p.name not in ("stdout.log", "stderr.log", "lane.log")]
changed = 0
for p in logs:
    text = p.read_text()
    lines = []
    for line in text.splitlines():
        lines.extend(textwrap.wrap(line, width=116, replace_whitespace=False, drop_whitespace=False,
                                   break_long_words=True, break_on_hyphens=False) if len(line) > 116 else [line])
    wrapped = "\n".join(lines) + "\n"
    if text != wrapped:
        p.write_text(wrapped)
        changed += 1
print(f"LOGS={len(logs)} WRAPPED={changed}")
