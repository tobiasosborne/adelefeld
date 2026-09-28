#!/usr/bin/env python3
"""tests/fuzz/seed_corpus.py: write the committed seed corpus of the fuzz targets.

The corpus of a target is tests/fuzz/corpus/<name>/. `make fuzz` copies it into
build/fuzz/corpus/<name>/ before the run, so a run never adds files to the working tree, and
libFuzzer writes the inputs it keeps to that copy.

The seeds of a target are small excerpts of the files of the repository, the vector files of
tests/ref/vectors/ and the golden files of tests/golden/, cut at a line boundary, plus a few
hand-written cases that reach the corners a random input reaches only after a long run: an
@gen: line with a count that must be refused, a lone backslash, a \\u escape, a line with two
TABs, and a lone high byte.

    python3 tests/fuzz/seed_corpus.py            write the seeds that are missing
    python3 tests/fuzz/seed_corpus.py --check    report what is missing, write nothing
    python3 tests/fuzz/seed_corpus.py --force    rewrite every seed file

An existing file is never overwritten without --force: a seed that libFuzzer has grown into a
useful input is worth keeping.
"""

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CORPUS = os.path.join(ROOT, "tests", "fuzz", "corpus")

# Bytes taken from the head of a file, cut at the last newline so that the seed is a whole
# number of records.
EXCERPTS = {
    "add.jsonl": 700,
    "recon.jsonl": 700,
    "policies.jsonl": 700,
    "rat.tsv": 500,
    "dump.tsv": 700,
    "realball_read.tsv": 500,
    "psi_phases.tsv": 500,
}

# The hand-written seeds, per target. Each is one file of the corpus.
LITERALS = {
    "support": [
        b"{}\n",
        b"[]\n",
        b"\n",
        b'{"a": 1}\n',
        b'{"a": {"A": 0, "H": 0, "d": 1}, "op": "neg"}\n',
        b'{"a": 1, "a": 2}\n',
        b'{"a": 01}\n',
        b'{"a": 1.5}\n',
        b'{"a": "x\\u0041\\ud83d\\ude00"}\n',
        b'{"a": "\xc3\xa9\xff"}\n',
        b'{"a": [[[[[[[[[1]]]]]]]]]}\n',
        b"7/3\t7/3\n",
        b"7/3\t!PARSE\n",
        b"7/3\t7/3\t7/3\n",
        b"\\q\t7/3\n",
        b"@gen:a|b|3|c\t7/3\n",
        b"@gen:a|b|99999999999999999999|c\t7/3\n",
        b"@gen:|(\x7c|4|\t!PARSE\n",
        b"# a comment\n\n7/3\t7/3\n",
        b"(1 ; 0)\\x00\t!PARSE\n",
        b"3.14 \\xc2\\xb1 0.1 ; 0\t!PARSE\n",
    ],
}


def head_of(path, limit):
    with open(path, "rb") as fh:
        data = fh.read(limit)
    cut = data.rfind(b"\n")
    if cut > 0:
        data = data[: cut + 1]
    return data


def write(path, data, force, check_only):
    if os.path.exists(path) and not force:
        return "kept"
    if check_only:
        return "missing"
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as fh:
        fh.write(data)
    return "written"


def main(argv):
    check_only = "--check" in argv
    force = "--force" in argv
    if not os.path.isdir(CORPUS) and not check_only:
        os.makedirs(CORPUS, exist_ok=True)

    written = kept = missing = 0
    for target, literals in sorted(LITERALS.items()):
        for index, data in enumerate(literals):
            name = os.path.join(CORPUS, target, "lit_%03d" % index)
            state = write(name, data, force, check_only)
            written += state == "written"
            kept += state == "kept"
            missing += state == "missing"
        for base, limit in sorted(EXCERPTS.items()):
            for sub in ("tests/ref/vectors", "tests/golden"):
                source = os.path.join(ROOT, sub, base)
                if not os.path.isfile(source):
                    continue
                name = os.path.join(CORPUS, target, "%s_%d" % (base, limit))
                state = write(name, head_of(source, limit), force, check_only)
                written += state == "written"
                kept += state == "kept"
                missing += state == "missing"

    print("corpus of tests/fuzz/corpus: %d written, %d kept, %d missing"
          % (written, kept, missing))
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
