#!/usr/bin/env python3
"""tools/mutate/check_equivalent.py: does every entry of equivalent.txt match exactly one mutant?

An entry of tools/mutate/equivalent.txt names a mutant by file, kind, the text of the line and the
change (mutate.py, Mutant.set_key). After an edit of a source the mutant may be gone or may have
become another one; an entry that matches no mutant excuses nothing and hides nothing, but it is
false, and it stays in the file for ever unless something says so. This tool says so:

    python3 tools/mutate/check_equivalent.py [--root .] [--equivalent FILE] [--files src/a.c ...]

It lists the mutants of every file the entries name (or of --files; by default every src/*.c that
exists), and prints every entry that matches no mutant, every entry that stands twice, and every line
that is not an entry (the old form FILE:LINE:KIND is one). The exit status is 0 when all entries
match exactly one mutant and 1 otherwise. It builds and runs nothing.
"""
import argparse
import glob
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import mutate  # noqa: E402


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--root", default=".")
    parser.add_argument("--equivalent", default=os.path.join(HERE, "equivalent.txt"))
    parser.add_argument("--files", nargs="*", default=None,
                        help="the source files whose mutants are listed (default: every src/*.c)")
    args = parser.parse_args(argv)
    root = os.path.abspath(args.root)

    files = args.files
    if files is None:
        files = sorted(os.path.relpath(p, root) for p in glob.glob(os.path.join(root, "src", "*.c")))
    keys = {}
    for name in files:
        with open(os.path.join(root, name)) as fh:
            text = fh.read()
        for m in mutate.mutants_of(name, text, root):
            keys.setdefault(m.key, []).append(m)

    bad = 0
    seen = {}
    count = 0
    with open(args.equivalent) as fh:
        for number, raw in enumerate(fh, start=1):
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            head, sep, reason = line.rpartition(" | ")
            parts = mutate.split_entry(head) if sep else None
            if parts is None:
                print("%s:%d: not an entry: %s" % (args.equivalent, number, line[:120]))
                bad += 1
                continue
            count += 1
            key = mutate.entry_key(*parts)
            if key in seen:
                print("%s:%d: the entry stands twice (first at line %d): %s"
                      % (args.equivalent, number, seen[key], key))
                bad += 1
                continue
            seen[key] = number
            if len(keys.get(key, [])) != 1:
                print("%s:%d: the entry matches %d mutants, not one: %s (%s)"
                      % (args.equivalent, number, len(keys.get(key, [])), key, reason[:80]))
                bad += 1
    if bad:
        print("check_equivalent: FAILED: %d problem(s) in %d entries" % (bad, count))
        return 1
    print("check_equivalent: passed: %d entries, each matches exactly one mutant of %d file(s)"
          % (count, len(files)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
