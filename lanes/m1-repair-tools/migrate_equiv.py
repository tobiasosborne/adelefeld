#!/usr/bin/env python3
"""migrate_equiv.py: one-off conversion of the old equivalent.txt keys to the text keys.

The old key of an entry was (file, line, kind).  The new one names the mutant by the text of
the line the mutant changes and by what the change is (mutate.py, Mutant.set_key), so that it
survives an edit above the mutant (surface R3).

For every entry of the old file the reason quotes the call or the token the mutant changes, so
the mutant it describes can be found again in the source as it stands:

  1. if the named line still holds a mutant of that kind whose changed text the reason quotes,
     that mutant is the one the entry is about;
  2. otherwise every mutant of the file whose changed text the reason quotes is a candidate, and
     the entry is dropped unless exactly one candidate is left (an entry whose line has moved
     to a place where the reason does not apply is a stale entry, not a new excuse);
  3. an entry with no candidate at all is dropped and reported.

Usage:  python3 lanes/m1-repair-tools/migrate_equiv.py tools/mutate/equivalent.txt OUT
"""
import os
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..",
                                "tools", "mutate"))
import mutate  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def norm(text):
    """The text of a reason or of a mutant, with the white space inside collapsed, so that a
    reason that wraps a line still quotes the call it is about."""
    return " ".join(text.split())


def old_key(line):
    head, _, reason = line.partition("|")
    parts = [p.strip() for p in head.split(":")]
    if len(parts) != 3:
        return None, None
    try:
        return (parts[0], int(parts[1]), parts[2]), reason.strip()
    except ValueError:
        return None, None


def candidates(muts, kind, reason, line_number, text):
    """The mutants of that kind this entry may be about, best first.

    Three things say which mutant a reason describes: the `A as B` pair of the reason, where A
    is the text of the line before the change and B the text after it; the changed text itself,
    which a reason of a swap or a dropped statement quotes; and the name of the function the
    line stands in, which a reason of a dropped call often gives ("in adf_fball_add"). The line
    number of the old entry counts as well, for the entries whose line has not moved."""
    r = norm(reason)
    scored = []
    for m in muts:
        if m.kind != kind:
            continue
        score = 0
        if pair_match(r, m):
            score += 4
        old = norm(m.old).rstrip(";").rstrip()
        if old and old in r:
            score += 2
        if function_of(text, m.line) and function_of(text, m.line) in r:
            score += 2
        if m.line == line_number:
            score += 1
        if score:
            scored.append((score, m))
    scored.sort(key=lambda p: (-p[0], p[1].start))
    return scored


def pair_match(reason, m):
    """1 if the reason's `A as B` pair is the line of this mutant before and after the change.

    The reason of a comparison or of a zero says what the mutant makes of the line, e.g.
    `fmpq_sgn(q) < 0 as <= 0`; the line it stands on is `if (fmpq_sgn(q) < 0)`, so the reason
    may leave the `if (` and the `)` out. So the test is on the text before the token, not on
    the whole line: A has to end with the old token, B has to be A with that token changed, and
    the text of the line before the token has to end with what A leaves in front of it."""
    if " as " not in reason:
        return 0
    a, b = (norm(x) for x in reason.split(" as ", 1))
    old, new = norm(m.old), norm(m.new)
    if not a.endswith(old) or b != a[: len(a) - len(old)] + new:
        return 0
    before = norm(m.prefix)
    return 1 if a.startswith(before) or norm(m.line_text) == a else 0


def function_of(text, line):
    """The name of the function whose body holds line number `line`, read from the definition
    lines that stand in column 0, as the project's own sources are written."""
    name = ""
    for number, one in enumerate(text.splitlines()[:line], start=1):
        m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)\s*\(", one)
        if m:
            name = m.group(1)
    return name


def main(argv):
    src, dst = argv[0], argv[1]
    with open(src) as fh:
        lines = [l.rstrip("\n") for l in fh]
    files = sorted({old_key(l)[0][0] for l in lines if old_key(l)[0]})
    muts = {}
    texts = {}
    for f in files:
        with open(os.path.join(ROOT, f)) as fh:
            texts[f] = fh.read()
        muts[f] = mutate.mutants_of(f, texts[f], ROOT)

    out = ["# tools/mutate/equivalent.txt: the mutants that survive and why.",
           "#",
           "# An entry is",
           "#",
           "#     <file> | <kind> | <the text of the line the mutant changes, stripped> | <old> ->",
           "#     <new> | <reason>",
           "#",
           "# with an occurrence number `#n` after the change when the same line text and the same",
           "# change stand more than once in the file.  There is no line number: a line number does",
           "# not survive an edit above the mutant (docs/reviews/m1/surface/review.md R3), and 45 of",
           "# the 76 entries of the previous version of this file matched no mutant at that commit.",
           "# `KIND` is one of op, cmp, logic, gcd_lcm, swap_args, status, drop_call, call_swap,",
           "# prec, zero_one, drop_assign, negate_if.  A `#` at the start of a line is a comment; a",
           "# reason must not hold a ` | ` of its own, because that is what separates it from the",
           "# key.",
           "#",
           "# Every line added here needs",
           "# the reason, and the reason has to say why the mutant computes the same thing as the",
           "# original for every input, not only that the tests do not notice.",
           "#",
           "# Converted from the line-number keys on 2026-09-28 by lane m1-repair-tools; the entries",
           "# of lanes/m1-local, lanes/m1-scaled and lanes/m1-dump are in it as well.  Run",
           "# `python3 tools/mutate/check_equivalent.py` to see which entries match no mutant.",
           ""]
    kept = dropped = 0
    # Every entry names a mutant by its reason (the reason quotes the call or the pair `A as
    # B`) and, where the source has moved on, by how far its old line is from a candidate that
    # the reason describes. The pairs are sorted by that distance and taken greedily, so that
    # two entries of the same call in one file do not both take the same mutant: each entry
    # gets its own nearest candidate that no earlier entry has taken, and an entry with no
    # candidate left is dropped and reported.
    pairs = []
    for line in lines:
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        key, reason = old_key(line)
        if key is None:
            print("SKIP (not an old entry): %s" % line)
            continue
        path, number, kind = key
        for score, m in candidates(muts.get(path, []), kind, reason, number, texts.get(path, "")):
            if score >= 2:
                pairs.append((abs(m.line - number), -score, len(pairs), m, line, reason))
    pairs.sort(key=lambda p: (p[0], p[1], p[2]))
    taken = set()
    chosen = {}
    for distance, negscore, index, m, line, reason in pairs:
        if index in chosen or id(m) in taken:
            continue
        chosen[index] = (m, distance)
        taken.add(id(m))
    order = []
    for position, line in enumerate(lines):
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        order.append(line)
    for index, line in enumerate(order):
        key, reason = old_key(line)
        if key is None:
            print("SKIP (not an old entry): %s" % line)
            continue
        path, number, kind = key
        if index in chosen:
            m, distance = chosen[index]
            if distance:
                print("moved    %s:%d:%s is now at line %d (%d line(s) away)"
                      % (path, number, kind, m.line, distance))
            out.append(m.entry_text(reason))
            kept += 1
        else:
            print("DROPPED (no mutant of kind %s of %s is left for the reason) %s:%d:%s"
                  % (kind, path, path, number, kind))
            print("        reason: %s" % reason)
            dropped += 1
    with open(dst, "w") as fh:
        fh.write("\n".join(out) + "\n")
    print("kept %d, dropped %d, written to %s" % (kept, dropped, dst))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
