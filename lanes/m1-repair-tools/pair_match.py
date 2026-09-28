#!/usr/bin/env python3
"""pair_match.py: for every old entry, the mutants of the tree that its reason describes.

The old keys of tools/mutate/equivalent.txt were (file, line, kind) and the sources have moved
a long way since they were written, so the key of an entry is taken from the reason instead: a
reason of a swap quotes the call before the exchange and the call after it (`A against B`), a
reason of a comparison or of a zero quotes the line before the change and the line after it
(`A as B`), a reason of a dropped call quotes the statement that is dropped.  A mutant is a
candidate of an entry when the text the reason quotes is the text the mutant changes.

For every entry this prints the candidates, so that the key can be read off and the reason can
be checked against the source as it stands.  It writes nothing.

Usage:  python3 lanes/m1-repair-tools/pair_match.py ENTRYFILE...
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tools", "mutate"))
import mutate  # noqa: E402

MUTATED = []          # (name, mutants)
SNIPPET = re.compile(r"[A-Za-z_][A-Za-z0-9_]*\s*\([^()]*\)")


def norm(text):
    return " ".join(text.split())


def load(path):
    if path not in [n for n, _ in MUTATED]:
        with open(path) as fh:
            text = fh.read()
        MUTATED.append((path, mutate.mutants_of(path, text, ROOT)))


def old_key(line):
    head, _, reason = line.partition("|")
    parts = [p.strip() for p in head.split(":")]
    if len(parts) != 3 or not parts[1].isdigit():
        return None, None
    return (parts[0], int(parts[1]), parts[2]), reason.strip()


def pair_of(reason):
    """The (A, B) of a reason that quotes both sides of the change, or (None, None)."""
    m = re.search(r"(\S[^\s]*\(.*?\))\s+against\s+(\S[^\s]*\(.*?\))", reason)
    if m:
        return norm(m.group(1)), norm(m.group(2))
    m = re.search(r"^(.*?)\s+as\s+(.*?):", reason)
    if m:
        return norm(m.group(1)), norm(m.group(2))
    return None, None


def snippets(reason):
    return [norm(s) for s in SNIPPET.findall(reason)]


def main(argv):
    for path in argv:
        with open(path) as fh:
            lines = [l.rstrip("\n") for l in fh]
        for raw in lines:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            key, reason = old_key(line)
            if key is None:
                print("SKIP %s" % line)
                continue
            file, number, kind = key
            a, b = pair_of(reason)
            out = []
            for name, muts in [(n, ms) for n, ms in MUTATED if n.startswith("src/")]:
                for m in muts:
                    hit = 0
                    if a and norm(m.old) == a and norm(m.new) == b:
                        hit = 3
                    elif a and a in norm(m.prefix) + norm(m.old) and \
                            b and b in norm(m.prefix) + norm(m.new):
                        hit = 2
                    for s in snippets(reason):
                        if s in norm(m.old) or s in norm(m.line_text):
                            hit = max(hit, 1)
                    if hit:
                        out.append((hit, m, name))
            out.sort(key=lambda t: (-t[0], t[2], t[1].start))
            print("\n%s:%d:%s\n    reason: %s" % (file, number, kind, reason))
            if not out:
                print("    NO CANDIDATE")
            for hit, m, name in out:
                print("    [%d] %s:%d %s  key=%s" % (hit, name, m.line, m.kind, m.key))
    return 0


if __name__ == "__main__":
    # every file of src/ is loaded once, so that a call that moved to another file is found
    for name in sorted(os.listdir(os.path.join(ROOT, "src"))):
        if name.endswith(".c"):
            load("src/" + name)
    sys.exit(main(sys.argv[1:]))
