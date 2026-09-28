#!/usr/bin/env python3
"""One-shot helper of lane m0-closure-apply: fold tests/golden/*.tsv into lanes/m0-conventions/write_golden.py.

Reads the authoritative vectors from lanes/m0-closure-apply/golden_before/ (a copy of tests/golden/ made
before regeneration) and replaces the FILES block of write_golden.py for each file whose regenerated content
differed, so that write_golden.py reproduces tests/golden/ byte for byte. Not part of the build; kept in the
lane directory as evidence of how the script was updated.
"""
import re
import sys

GOLDEN = "lanes/m0-closure-apply/golden_before"
WRITER = "lanes/m0-conventions/write_golden.py"
REPLACE = ["dump", "realball_print", "realball_read", "rfun"]


def tsv_to_block(path):
    with open(path, encoding="ascii") as f:
        raw = f.read()
    assert raw.endswith("\n")
    out = []
    for line in raw.split("\n")[:-1]:
        if line == "" or line.startswith("#"):
            out.append(line)
            continue
        assert line.count("\t") == 1, (path, line)
        inp, exp = line.split("\t")
        assert " ==> " not in inp and " ==> " not in exp, (path, line)
        out.append(inp + " ==> " + exp)
    return "\n".join(out)


def main():
    with open(WRITER, encoding="utf-8") as f:
        text = f.read()
    for name in REPLACE:
        block = tsv_to_block("%s/%s.tsv" % (GOLDEN, name))
        pattern = re.compile(r'FILES\["%s"\] = r"""\n.*?\n"""' % re.escape(name), re.DOTALL)
        assert len(pattern.findall(text)) == 1, name
        text = pattern.sub(lambda m: 'FILES["%s"] = r"""\n%s\n"""' % (name, block), text, count=1)
    with open(WRITER, "w", encoding="utf-8") as f:
        f.write(text)
    print("rewrote blocks:", ", ".join(REPLACE))


if __name__ == "__main__":
    sys.exit(main())