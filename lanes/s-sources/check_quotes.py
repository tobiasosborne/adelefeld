#!/usr/bin/env python3
"""Check every quote of table 3 of docs/sources.md against the files under refs/src/.

A row of the table is a line of the form

    | statement | key | file:line, line, ... | "quote" |

The quote is checked part by part: the parts are separated by " / ", part `i` of a row whose third cell names
`n` lines must be a substring of the `i`-th of those lines, after the documented rendering is undone
(a pipe written \\| is a pipe, the leading indentation of an extraction line is dropped). The first and the
last line of a part may be a fragment of the extraction line; that is accepted, and the number of characters
matched is reported so that a silently shortened quote is visible.

Exit status 0 when every part matches, 1 otherwise.

Usage:  python3 lanes/s-sources/check_quotes.py [--verbose]
"""
import os
import re
import sys

LANE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(LANE, "..", "..")
DOC = os.path.join(ROOT, "docs", "sources.md")
SRC = os.path.join(ROOT, "refs", "src")
TABLE = "## Table 3: milestone S"


def rows_of(text):
    """The quote rows of table 3, as (line number of the row, key, file, [lines], quote)."""
    start = text.index(TABLE)
    end = text.index("\n## Sources pending", start)
    out = []
    for no, line in enumerate(text[start:end].split("\n"), 1):
        if not line.startswith("| S."):
            continue
        cells = [c.strip() for c in line.strip().strip("|").split(" | ")]
        if len(cells) != 4:
            raise SystemExit(f"row at table-3 line {no}: {len(cells)} cells, expected 4")
        key, where, quote = cells[1], cells[2], cells[3]
        if not (quote.startswith('"') and quote.endswith('"')):
            raise SystemExit(f"row at table-3 line {no}: quote is not quoted")
        m = re.match(r"^([^:]+):((?:\s*\d+\s*,?)+)$", where)
        if not m:
            raise SystemExit(f"row at table-3 line {no}: cannot read {where!r}")
        out.append((no, key, m.group(1), [int(x) for x in re.findall(r"\d+", m.group(2))], quote[1:-1]))
    return out


def unescape(text):
    return text.replace("\\|", "|")


def main():
    verbose = "--verbose" in sys.argv[1:]
    text = open(DOC, encoding="utf-8").read()
    rows = rows_of(text)
    cache = {}
    bad = 0
    chars = 0
    for no, key, rel, lines, quote in rows:
        path = os.path.join(SRC, key, rel)
        if path not in cache:
            if not os.path.exists(path):
                raise SystemExit(f"missing file: {path}")
            cache[path] = open(path, encoding="utf-8").read().split("\n")
        content = cache[path]
        parts = unescape(quote).split(" / ")
        if len(parts) != len(lines):
            print(f"FAIL table-3 line {no}: {len(parts)} quote parts, {len(lines)} line numbers")
            bad += 1
            continue
        for (part, ln) in zip(parts, lines):
            chars += len(part)
            if ln > len(content):
                print(f"FAIL {key}:{rel}:{ln}: line {ln} does not exist (file has {len(content)})")
                bad += 1
                continue
            if part not in content[ln - 1]:
                print(f"FAIL {key}:{rel}:{ln}: not in the line:")
                print(f"  quote: {part!r}")
                print(f"  line : {content[ln - 1].strip()!r}")
                bad += 1
            elif verbose:
                print(f"ok   {key}:{rel}:{ln}: {len(part)} characters")
    print(f"rows: {len(rows)}, quoted characters: {chars}, failures: {bad}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
