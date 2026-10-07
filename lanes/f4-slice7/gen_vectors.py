#!/usr/bin/env python3
"""Canonical dump bytes from proto/text_grammar.py; no C implementation is imported."""
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "proto"))
import text_grammar as ref

OUT = ROOT / "tests/ref/vectors/f4-slice7"
OUT.mkdir(exist_ok=True)
ZERO = ("arb", 0, 0, 0, 0)
Z = ("acb", ZERO, ZERO)
ONE = ("acb", ("arb", 1, 0, 0, 0), ZERO)


def ball(i, large=False):
    m = (1 << 1999) + 2 * i + 1 if large else 2 * i + 1
    e = [-2000, -30, -1, 0, 30, 2000][i % 6]
    rm = [0, 1, 3, (1 << 29) - 1, (1 << 30) - 1][i % 5]
    rexp = [-2000, -31, -1, 0, 31, 2000][(i // 5) % 6] if rm else 0
    return ("arb", -m if i % 3 == 0 else m, e, rm, rexp)


def value(i, large=False):
    return ("acb", ball(i, large), ZERO if i % 4 == 0 else ball(i + 1, large))


def record(text, binds=0):
    result = ref.dump_roundtrip(text)
    status = result[1:] if result.startswith("!") else "OK"
    if status == "OK":
        assert result == text and ref.dump_contexts(text) == []
    if binds:
        status = ref.dump_load_check(text, [None] * binds).lstrip("!")
    return {"text": text, "status": status, "binds": binds}


for kind in ("ffun", "rfun"):
    rows = []
    for i in range(40):
        if kind == "ffun":
            D, M = [(1, 1), (2, 3), (3, 2), (6, 6), (12, 5), (1, 1024)][i % 6]
            vals = [value(i + j, large=(i % 7 == 0 and D * M < 100)) for j in range(D * M)]
            node = (kind, D, M, vals)
        else:
            terms = []
            for j in range(i % 5):
                coeffs = [] if (i + j) % 9 == 0 else [value(i + k, i % 7 == 0)
                                                     for k in range((i + j) % 7 + 1)]
                if i == 1 and j == 0:
                    coeffs[-1] = ("acb", ZERO, ("arb", 1, 0, 0, 0))
                if i == 2 and j == 0:
                    coeffs[-1] = ("acb", ZERO, ("arb", 0, 0, 1, 0))
                A = ("acb", ("arb", 3, 0, 1, -2), ball(i + j))
                terms.append((coeffs, A, value(i + j), value(i + j + 2)))
            node = (kind, terms)
        rows.append(record(ref._dump_print(node)))
    valid = rows[0]["text"]
    bad = [valid + " l 1 2 1 2 0", valid + " 0", valid + "\x80",
           valid + "\x00", valid.replace("adf1", "adf2"), valid.replace(" Q ", " K ")]
    if kind == "ffun":
        bad += ["adf1 Q ffun z 1", "adf1 Q ffun 1", "adf1 Q ffun 0 1",
                "adf1 Q ffun 1 2 " + ref._p_acb(ONE), "adf1 Q ffun 100001 1",
                ref._dump_print((kind, 1, 1, [("acb", ("arb", 0, 5, 0, 0), ZERO)]))]
    else:
        bad += ["adf1 Q rfun z", "adf1 Q rfun 1", "adf1 Q rfun 10001",
                ref._dump_print((kind, [([ONE, Z], ONE, Z, Z)])),
                ref._dump_print((kind, [([ONE], Z, Z, Z)])),
                ref._dump_print((kind, [([ONE], ("acb", ("arb", 1, 0, 1, 0), ZERO), Z, Z)]))]
    rows += [record(s) for s in bad]
    rows.append(record(valid, binds=1))
    path = OUT / (kind + ".jsonl")
    path.write_text("".join(json.dumps(r, separators=(",", ":")) + "\n" for r in rows))
    print(kind, len(rows), "rows; 40 valid values;", path.stat().st_size, "bytes")
assert sum(p.stat().st_size for p in OUT.glob("*.jsonl")) <= 400000
