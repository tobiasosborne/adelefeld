#!/usr/bin/env python3
"""Prefix every committed text-fuzz seed with the two control bytes of fuzz_text.c (finding R9).

The control bytes of a seed are the first two bytes of the SHA-1 of its old content, so they vary
over seeds, and the new file name is the SHA-1 of the new content (libFuzzer's own naming rule).
Run from the repository root: python3 lanes/m1-repair-text/prefix_corpus.py
"""
import hashlib
import sys
from pathlib import Path

CORPUS = Path("tests/fuzz/corpus/text")


def main():
    if not CORPUS.is_dir():
        sys.exit("no corpus at %s" % CORPUS)
    old = sorted(p for p in CORPUS.iterdir() if p.is_file())
    for p in old:
        data = p.read_bytes()
        ctrl = hashlib.sha1(data).digest()[:2]
        new = ctrl + data
        name = hashlib.sha1(new).hexdigest()
        p.unlink()
        (CORPUS / name).write_bytes(new)
    print("prefixed %d seeds" % len(old))


if __name__ == "__main__":
    main()
