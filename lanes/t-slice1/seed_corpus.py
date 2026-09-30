#!/usr/bin/env python3
"""lanes/t-slice1/seed_corpus.py: seeds for tests/fuzz/fuzz_text_idele.c, written to build/fuzz/corpus/text_idele/.

Every seed is two control bytes (prec, digits) and a text: the rows of tests/golden/{ucoset,idele,idclass}.tsv and 300
texts of tests/ref/vectors/t-slice1/text_idele.jsonl, each with a few control bytes.  Nothing is written under
tests/ (that directory is not this lane's).

    python3 lanes/t-slice1/seed_corpus.py          (from the repository root)
"""
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "lanes", "m1-text"))
import gen_text_vectors as g  # noqa: E402

OUT = os.path.join(ROOT, "build", "fuzz", "corpus", "text_idele")
os.makedirs(OUT, exist_ok=True)
n = 0


def put(data, ctl):
    global n
    with open(os.path.join(OUT, "seed%04d" % n), "wb") as fh:
        fh.write(bytes(ctl) + data)
    n += 1


for name in ("ucoset", "idele", "idclass"):
    for _, data, _, gen in g.golden_rows(name):
        if not gen:
            for ctl in ((128, 19), (0, 0), (30, 3)):
                put(data, ctl)
i = 0
with open(os.path.join(ROOT, "tests", "ref", "vectors", "t-slice1", "text_idele.jsonl")) as fh:
    for line in fh:
        i += 1
        if i % 7 == 0:
            r = json.loads(line)
            put(bytes.fromhex(r["hex"]), (64 + i % 100, i % 30))
put(b"(1 +/- 0.99999999999 ; 1 * [1])", (30, 19))
put(b"(1 ; 1 * [1])", (255, 19))
print("%d seeds in %s" % (n, os.path.relpath(OUT, ROOT)))
