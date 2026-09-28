#!/usr/bin/env python3
"""lanes/m1-text/seed_text_corpus.py: the seed corpus of the fuzz target tests/fuzz/fuzz_text.c.

One file per input under tests/fuzz/corpus/text/, named by the SHA-1 of its bytes: every input (and every
canonical output) of the golden files rat, fball, adele, cadele, dispatch and realball_read (the real balls
wrapped as "(X ; 0)"), one text of each of the other nine types, and a few hand-written hostile inputs.
Inputs longer than 4096 bytes are left out (libFuzzer's default -max_len). The golden files are read with
the same escapes as the C reader (docs/conventions.md 11.1).

    python3 lanes/m1-text/seed_text_corpus.py        (from the repository root)
"""
import hashlib
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "lanes", "m1-text"))
from gen_text_vectors import golden_rows  # noqa: E402

OUT = os.path.join(ROOT, "tests", "fuzz", "corpus", "text")

EXTRA = [
    b"", b" ", b"\x00", b"\x80", b"7\x00", b"(", b"((((", b"-", b"1e", b"1.", b"+/-",
    b"[5 mod 6]", b"(2.5 +/- 1e-9 ; 3/2 * [5 mod 36])", b"<1.25 +/- 1e-30 ; [5 mod 36]>", b"[p=5: 3 + O(5^4)]",
    b"{inf: 1; p=5: 2}", b"union((0.5 ; 0 mod 1)) + Q", b"ffun(D=1, M=1; (0) + (0)*i)",
    b"rfun(term(P=[], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))", b"char(q=5, n=2, s=(0) + (0)*i)",
    b"(1e5000 ; 0)", b"(1e5001 ; 0)", b"(1e-5000 +/- 1e-5000 ; 1/3 mod 5/7)",
    b"((1e-300 +/- 1e-301) + (-1e300)*i ; 0 mod 1)", b"(0.1 +/- 0.13 ; 0)",
]


def main():
    os.makedirs(OUT, exist_ok=True)
    inputs = list(EXTRA)
    for name in ("rat", "fball", "adele", "cadele", "dispatch"):
        for _, data, exp, _ in golden_rows(name):
            inputs.append(data)
            if not exp.startswith("!") and name != "dispatch":
                inputs.append(exp.encode("ascii"))
    for _, data, _, _ in golden_rows("realball_read"):
        inputs.append(b"(" + data + b" ; 0)")
    written = 0
    for data in inputs:
        if len(data) > 4096:
            continue
        path = os.path.join(OUT, hashlib.sha1(data).hexdigest())
        if not os.path.exists(path):
            with open(path, "wb") as fh:
                fh.write(data)
            written += 1
    print("%s: %d files written, %d in all" % (os.path.relpath(OUT, ROOT), written, len(os.listdir(OUT))))


if __name__ == "__main__":
    main()
