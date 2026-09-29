#!/usr/bin/env python3
"""Check every quote of the rows this lane added to docs/sources.md against refs/src/.

Usage: check_quotes.py docs/sources.md <line> [<line> ...]

Convention of docs/sources.md: the cell "File and line" gives file:line[, line ...] against
the file refs/src/<key>/<file>; the quote is the concatenation of those lines, leading
indentation dropped, a break between lines written as " / ", a pipe inside a cell as \\|.
"""
import re
import sys

SRC = "refs/src/"


def check(row):
    cells = row.rstrip("\n").split(" | ")
    key = cells[1].strip()
    spec = cells[2].strip()
    quote = cells[3].rstrip()
    if quote.endswith("|"):
        quote = quote[:-1].rstrip()
    m = re.fullmatch(r"([\w.\-]+):(.+)", spec)
    if not m:
        return ("BAD-SPEC", spec)
    path = SRC + key + "/" + m.group(1)
    nums = [int(x) for x in m.group(2).replace(",", " ").split()]
    try:
        with open(path, encoding="utf-8") as fh:
            content = fh.readlines()
    except OSError as err:
        return ("NOFILE", str(err))
    for n in nums:
        if n < 1 or n > len(content):
            return ("LINE-OUT", "%s has %d lines" % (path, len(content)))
    got = " / ".join(content[n - 1].rstrip("\n").lstrip() for n in nums)
    want = quote
    if want.startswith('"') and want.endswith('"'):
        want = want[1:-1]
    return ("OK", got) if got == want.replace("\\|", "|") else ("MISMATCH", (want, got))


def main(argv):
    path, nums = argv[0], [int(x) for x in argv[1:]]
    lines = open(path, encoding="utf-8").readlines()
    bad = 0
    for n in nums:
        res = check(lines[n - 1])
        if res[0] != "OK":
            bad += 1
            print("line %d: %s" % (n, res[0]))
            if res[0] == "MISMATCH":
                print("  want: %r" % res[1][0][:500])
                print("  got : %r" % res[1][1][:500])
            else:
                print("  %s" % (res[1],))
        else:
            print("line %d: OK   %s..." % (n, res[1][:70]))
    print("rows: %d, mismatches: %d" % (len(nums), bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
