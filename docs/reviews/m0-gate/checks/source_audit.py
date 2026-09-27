#!/usr/bin/env python3
"""Open the local source ranges attached to every substantive SPEC [quoted] label.

This extracts evidence, not an automated judgement of its mathematical meaning.
"""
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
ranges = {
    "hertogh-thesis/thesis.txt": [(231, 233), (374, 394), (849, 866), (1525, 1538)],
    "milne-cft/CFT.txt": [(9373, 9376), (9853, 9858), (9883, 9886), (9904, 9908), (1307, 1308),
                         (3162, 3166)],
    "tate-poonen/notes.txt": [(693, 701), (733, 740), (933, 937), (1014, 1016),
                              (1053, 1055), (1720, 1725), (1733, 1733)],
    "tate-kudla/kudla-1.txt": [(705, 706), (900, 905)],
    "tate-warwick/tatesthesis_notes.txt": [(493, 512), (518, 521)],
    "flint-3.0.1/ulong_extras.rst": [(456, 458)],
    "flint-3.0.1/arb.rst": [(285, 298)],
    "flint-3.0.1/acb_dirichlet.rst": [(360, 365)],
    "pari-doc/usersch3.tex": [(9266, 9271), (19478, 19485), (19556, 19558)],
    "hilbert-bristol/lecture19.txt": [(7, 9), (46, 56), (89, 91)],
}
count = 0
for relative, parts in ranges.items():
    path = ROOT / "refs/src" / relative
    raw = path.read_bytes()
    # PDF extractions contain form feeds and C0 characters. Only LF counts as a physical line.
    lines = raw.decode().split("\n")
    print(f"FILE {path.relative_to(ROOT)} sha256={hashlib.sha256(raw).hexdigest()}")
    for lo, hi in parts:
        assert 1 <= lo <= hi <= len(lines)
        print(f"RANGE {lo}-{hi}")
        for i in range(lo, hi + 1):
            print(f"{i}: {lines[i - 1]}")
        count += 1
print(f"source_files={len(ranges)} opened_ranges={count}")
