#!/usr/bin/env python3
"""Exact keys and text/dump expectations. No C library is used."""
from fractions import Fraction as F
from pathlib import Path
import json
import random
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "proto"))
import text_grammar as text
import quotient3_checks as quotient

OUT = ROOT / "tests/ref/vectors/q-slice4"
OUT.mkdir(exist_ok=True)


def write(name, rows):
    data = "".join(json.dumps(r, separators=(",", ":")) + "\n" for r in rows)
    (OUT / name).write_text(data)
    print(name, len(rows), len(data.encode()))


def decimal(q):
    return text.print_real(q, F(0), digits=20)


def piece(m, r, a, h):
    return f"({decimal(m)}" + (f" +/- {decimal(r)}" if r else "") + f" ; {a}" + (
        f" mod {h}" if h else "") + ")"


rng = random.Random(310407)
rows = []
for i in range(100):
    values = [(F(rng.randrange(17), 16), F(rng.randrange(5), 16), rng.randrange(-12, 12),
               rng.randrange(8)) for _ in range(1 + i % 7)]
    if i % 3 == 0:
        values.append(values[0])
    inputs = [piece(*v) for v in values]
    keys = [quotient.Piece(m-r, m+r, a, h) for m, r, a, h in values]
    order = sorted(set(keys), key=quotient.piece_key)
    # These storage keys deliberately do not reduce spill or identify glued boundary points.
    canonical = [piece((p.lo+p.hi)/2, (p.hi-p.lo)/2, p.m, p.N) for p in order]
    source = "union(" + ", ".join(inputs) + ") + Q"
    expected = "union(" + ", ".join(canonical) + ") + Q"
    rows.append(dict(input=source, pieces=inputs, raw=len(inputs), count=len(order),
                     keys=[[str(k) for k in quotient.piece_key(p)] for p in order],
                     canonical=expected, printed=text.canonical("qclass", expected)))
write("pieces.jsonl", rows)

rows = []
for line in (ROOT / "tests/golden/qclass.tsv").read_text().splitlines():
    if not line or line.startswith("#"):
        continue
    source, expected = line.split("\t")
    assert text.canonical("qclass", source) == expected
    rec = dict(input=source, canonical=expected)
    if not expected.startswith("!"):
        members, printed = [], []
        union = source.startswith("union")
        for real, finite in re.findall(r"\(([^();]+);([^();]+)\)", source):
            parts = real.split("+/-")
            m, r = F(parts[0]), F(parts[1]) if len(parts) > 1 else F(0)
            parts = finite.split("mod")
            a, h = F(parts[0]), F(parts[1]) if len(parts) > 1 else F(0)
            a = a % h if h else a
            members.append(dict(lo=str(m-r), hi=str(m+r), a=str(a), h=str(h)))
            sm = quotient.round_binary(m, 128)
            if not union and sm != m:
                # The existing lift reader rounds toward zero.
                e = abs(m).numerator.bit_length() - abs(m).denominator.bit_length() - 127
                step = F(2)**e
                while abs(m) / step < 2**127:
                    step /= 2
                sm = (abs(m) // step) * step * (1 if m >= 0 else -1)
            d = r + abs(sm-m)
            if d:
                e = d.numerator.bit_length() - d.denominator.bit_length() - 29
                step = F(2)**e
                sr = -(-d // step) * step
            else:
                sr = F(0)
            realtext = text.print_real(sm, sr, digits=2 if union else 6)
            pm, pr = text.read_real(realtext)
            printed.append(((pm, pr, h, a), f"({realtext} ; {a}" + (f" mod {h}" if h else "") + ")"))
        strings = [s for _, s in sorted(set(printed))]
        rec.update(members=members, c_print=("union(" + ", ".join(strings) + ") + Q"
                                            if union else strings[0] + " + Q"))
    rows.append(rec)
write("text.jsonl", rows)

rows = []
for n in (1, 2, 7):
    for backend in ("global", "one", "several"):
        entries = []
        for i in range(n):
            m = F(i+1, 8)
            num, e = m.numerator, -(m.denominator.bit_length()-1)
            arch = f"1 {num:x} {e:x} 0 0"
            if backend == "global":
                fb = f"g {i % 3:x} 3 1"
            elif backend == "one" or i % 2 == 0:
                fb = "l 2 6 2 2 3 0 0"
            else:
                fb = "l 1 5 1 5 0"
            entries.append(arch + " " + fb)
        s = f"adf1 Q qclass pieces {n:x} " + " ".join(entries)
        assert text.dump_roundtrip(s) == s
        rows.append(dict(input=s, canonical=s, nctx=len(text.dump_contexts(s))))
        lift = "adf1 Q qclass lift " + entries[0]
        rows.append(dict(input=lift, canonical=lift, nctx=len(text.dump_contexts(lift))))
write("dump.jsonl", rows)
assert sum(p.stat().st_size for p in OUT.iterdir()) <= 400000
