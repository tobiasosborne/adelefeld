#!/usr/bin/env python3
"""Seeds of the fuzzer tests/fuzz/fuzz_dump.c (lane m1-dump): every input of tests/golden/dump.tsv (escapes
decoded as in docs/conventions.md 11.1; the @gen line of 1048577 bytes is left out) and every seventh text
of tests/ref/vectors/m1-dump/dump_ref.jsonl shorter than 400 bytes. One file per input, named by the first
16 hexadecimal digits of its SHA-1. Run from the repository root:

    python3 lanes/m1-dump/gen_fuzz_seeds.py
"""
import hashlib
import json
import os

OUT = os.path.join("tests", "fuzz", "corpus", "dump")


def unescape(s):
    out = bytearray()
    i = 0
    while i < len(s):
        c = s[i]
        if c != "\\":
            out += c.encode("ascii")
            i += 1
            continue
        n = s[i + 1]
        if n == "x":
            out.append(int(s[i + 2:i + 4], 16))
            i += 4
        else:
            out += {"\\": b"\\", "t": b"\t", "n": b"\n", "r": b"\r"}[n]
            i += 2
    return bytes(out)


def main():
    os.makedirs(OUT, exist_ok=True)
    seeds = set()
    for line in open(os.path.join("tests", "golden", "dump.tsv")):
        line = line.rstrip("\n")
        if not line or line.startswith("#") or line.startswith("@gen:"):
            continue
        seeds.add(unescape(line.split("\t")[0]))
    for i, line in enumerate(open(os.path.join("tests", "ref", "vectors", "m1-dump", "dump_ref.jsonl"))):
        t = json.loads(line)["text"].encode("ascii")
        if i % 7 == 0 and len(t) < 400:
            seeds.add(t)
    for s in sorted(seeds):
        name = hashlib.sha1(s).hexdigest()[:16]
        with open(os.path.join(OUT, name), "wb") as f:
            f.write(s)
    print("%d seeds in %s" % (len(seeds), OUT))


if __name__ == "__main__":
    main()
