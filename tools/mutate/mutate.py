#!/usr/bin/env python3
"""tools/mutate/mutate.py: mutation testing for the C code of adelefeld, without any tool
outside the standard library.

A mutant is one source file with one token changed. The mutations are:

    op            an arithmetic operator replaced by another of the same class
    cmp           a comparison operator replaced by another, including moving its boundary
                  (`<` to `<=`, `<=` to `<`, and so on)
    logic         `&&` and `||` exchanged
    gcd_lcm       the name `gcd` replaced by `lcm`, and the other way round
    swap_args     the two arguments of a call to a commutative-looking function exchanged
    zero_one      the number 0 replaced by 1, and 1 by 0
    drop_assign   a statement of the form `x = ...;` removed
    negate_if     the condition of an `if` or a `while` negated

One mutant at a time. For each mutant the tool copies the sources into a scratch directory
under build/mutate/, writes the mutant there, builds and runs the tests with a timeout, and
reports the mutant as

    killed        the tests fail, so a wrong implementation of the claim would be caught
    survived      the tests pass: the test suite does not check this line
    not compiled  the mutant does not build
    timed out     the tests do not finish in the time given

The source tree is never written to: the copy is the only place a mutant exists. The tool
fails if a mutant survives, unless tools/mutate/equivalent.txt lists it with a reason.

    python3 tools/mutate/mutate.py --files src/fball.c
    python3 tools/mutate/mutate.py --files src/a.c src/b.c --limit 50 --seed 7 --jobs 2
    python3 tools/mutate/mutate.py --files src/fball.c --timeout 120 --make 'make -s check'

Options are listed by `python3 tools/mutate/mutate.py --help`. The self-test of the tool is
`make mutate-selftest`, which runs it over tools/mutate/example/ with a deliberately weak test
and then with a strong one.
"""

import argparse
import concurrent.futures
import difflib
import os
import random
import re
import shutil
import subprocess
import sys
import time

# The entries of the repository that a mutant is built from. The scratch copy needs the
# Makefile, the public header, the sources and the tests (with tests/golden and
# tests/ref/vectors, which the tests read); it needs nothing else, and copying refs/ and
# lanes/ would cost 200 MB per mutant.
COPY_ENTRIES = ["Makefile", "include", "src", "tests"]

# A call to a function whose name ends with one of these looks commutative: the two arguments
# may be exchanged without changing what the call computes, so exchanging them is a mutant
# that a test may or may not notice. Nothing is assumed about a function whose name does not
# end with one of them.
COMMUTATIVE_SUFFIXES = ["_add", "_mul", "_gcd", "_lcm", "_min", "_max", "_minmax"]
COMMUTATIVE_NAMES = ["gcd", "lcm", "min", "max", "fmpz_gcd", "fmpz_lcm", "nmod_gcd", "adf_qlcm",
                     "adf_qgcd"]

# The arithmetic operators and what each becomes. A mutant never becomes the token itself.
OP_MAP = {
    "+": ["-"],
    "-": ["+"],
    "*": ["/", "+"],
    "/": ["*"],
    "%": ["/"],
    "&": ["|"],
    "|": ["&"],
    "^": ["&"],
    "<<": [">>"],
    ">>": ["<<"],
}

# The comparison operators and their boundaries.
CMP_MAP = {
    "<": ["<="],
    "<=": ["<"],
    ">": [">="],
    ">=": [">"],
    "==": ["!="],
    "!=": ["=="],
}

LOGIC_MAP = {"&&": ["||"], "||": ["&&"]}

# The longest first, so that `<<=` is not read as `<<`.
OPERATORS = ["<<=", ">>=", "...", "->", "++", "--", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||",
             "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "+", "-", "*", "/", "%", "<", ">",
             "=", "!", "~", "&", "|", "^", "?", ":", ";", ",", ".", "(", ")", "[", "]", "{", "}"]

IDENT_START = re.compile(r"[A-Za-z_]")
IDENT_CHAR = re.compile(r"[A-Za-z0-9_]")
NUMBER = re.compile(r"(0[xX][0-9a-fA-F]+|[0-9]+)")

# What a build failure looks like, as opposed to a test that fails. "make: *** Error 1" is
# printed for both, so it is not in this list: a test that fails is a killed mutant, and a
# mutant that does not compile is a separate report.
COMPILE_ERROR = re.compile(r"(error:|undefined reference|collect2:|ld returned)")


class Mutant(object):
    """One changed file: the text from `start` to `end` is replaced by `replacement`."""

    def __init__(self, path, line, col, kind, old, new, start, end, replacement, note=""):
        self.path = path
        self.line = line
        self.col = col
        self.kind = kind
        self.old = old
        self.new = new
        self.start = start
        self.end = end
        self.replacement = replacement
        self.note = note
        self.status = None
        self.detail = ""

    @property
    def key(self):
        return (self.path, self.line, self.kind)

    def __str__(self):
        return "%s:%d:%d %s: %r -> %r%s" % (self.path, self.line, self.col, self.kind, self.old,
                                            self.new, (" (%s)" % self.note) if self.note else "")

    def diff(self, old_text, new_text):
        return "".join(difflib.unified_diff(
            old_text.splitlines(True), new_text.splitlines(True),
            fromfile="a/" + self.path, tofile="b/" + self.path, n=2))

    def apply(self, text):
        return text[: self.start] + self.replacement + text[self.end:]


class Token(object):
    __slots__ = ("kind", "start", "end", "text")

    def __init__(self, kind, start, end, text):
        self.kind = kind
        self.start = start
        self.end = end
        self.text = text

    def __repr__(self):
        return "Token(%s, %d, %r)" % (self.kind, self.start, self.text)


def scan(text):
    """The tokens of a C file that a mutation may touch: identifiers, numbers and operators.

    Comments, string and character literals, and every preprocessor line, are skipped: a
    mutation inside them changes nothing that is compiled, and a mutation of a macro name would
    be a different kind of mutant."""
    tokens = []
    i = 0
    n = len(text)
    at_line_start = True
    while i < n:
        c = text[i]
        if c == "\n":
            i += 1
            at_line_start = True
            continue
        if c in " \t\r":
            i += 1
            continue
        if c == "\\" and i + 1 < n and text[i + 1] == "\n":  # a line continuation
            i += 2
            continue
        if at_line_start and c == "#":
            j = text.find("\n", i)
            i = n if j < 0 else j
            continue
        at_line_start = False
        if text.startswith("//", i):
            j = text.find("\n", i)
            i = n if j < 0 else j
            continue
        if text.startswith("/*", i):
            j = text.find("*/", i + 2)
            i = n if j < 0 else j + 2
            continue
        if c == '"' or c == "'":
            quote = c
            i += 1
            while i < n:
                if text[i] == "\\":
                    i += 2
                    continue
                if text[i] == quote:
                    i += 1
                    break
                if text[i] == "\n":
                    break
                i += 1
            continue
        if IDENT_START.match(c):
            j = i
            while j < n and IDENT_CHAR.match(text[j]):
                j += 1
            tokens.append(Token("ident", i, j, text[i:j]))
            i = j
            continue
        m = NUMBER.match(text, i)
        if m and (c.isdigit() or (c == "." and i + 1 < n and text[i + 1].isdigit())):
            tokens.append(Token("number", i, m.end(), m.group(0)))
            i = m.end()
            continue
        for op in OPERATORS:
            if text.startswith(op, i):
                tokens.append(Token("op", i, i + len(op), op))
                i += len(op)
                break
        else:
            i += 1
    return tokens


def line_of(text, pos):
    return text.count("\n", 0, pos) + 1


def col_of(text, pos):
    start = text.rfind("\n", 0, pos)
    return pos - start


def match_paren(text, open_pos):
    """The position just after the parenthesis that matches the one at open_pos, or -1."""
    depth = 0
    i = open_pos
    n = len(text)
    while i < n:
        c = text[i]
        if c == '"' or c == "'":
            quote = c
            i += 1
            while i < n and text[i] != quote:
                i += 2 if text[i] == "\\" else 1
            i += 1
            continue
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                return i + 1
        elif c == ";" and depth == 0:
            return -1
        i += 1
    return -1


def top_level_comma(text, start, stop):
    """The position of the one comma at bracket depth 0 between start and stop, or -1."""
    depth = 0
    i = start
    while i < stop:
        c = text[i]
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        elif c == "," and depth == 0:
            return i
        elif c in "\"'":
            quote = c
            i += 1
            while i < stop and text[i] != quote:
                i += 2 if text[i] == "\\" else 1
        i += 1
    return -1


def next_token_is(text, pos, what):
    """1 if the first byte after pos that is not white space starts the token `what`."""
    i = pos
    n = len(text)
    while i < n and text[i] in " \t\r\n":
        i += 1
    return i < n and text[i] == what


def is_commutative(name):
    return name in COMMUTATIVE_NAMES or any(name.endswith(s) for s in COMMUTATIVE_SUFFIXES)


def statement_start(text, tokens, index):
    """The index of the first token of the statement that ends at tokens[index] (';')."""
    i = index - 1
    while i >= 0 and tokens[i].text not in (";", "{", "}", ":"):
        i -= 1
    return i + 1


def mutants_of(path, text):
    """Every mutant of one file, in the order the tokens stand in the file."""
    out = []
    tokens = scan(text)

    for index, tok in enumerate(tokens):
        line, col = line_of(text, tok.start), col_of(text, tok.start)
        if tok.kind == "op":
            for kind, table in (("op", OP_MAP), ("cmp", CMP_MAP), ("logic", LOGIC_MAP)):
                for new in table.get(tok.text, []):
                    out.append(Mutant(path, line, col, kind, tok.text, new, tok.start, tok.end, new))
        elif tok.kind == "number" and tok.text in ("0", "1"):
            new = "1" if tok.text == "0" else "0"
            out.append(Mutant(path, line, col, "zero_one", tok.text, new, tok.start, tok.end, new))
        elif tok.kind == "ident":
            open_pos = tokens[index + 1].start if index + 1 < len(tokens) else -1
            after = match_paren(text, open_pos) if open_pos >= 0 and \
                tokens[index + 1].text == "(" else -1
            # A definition is not a call: its name and its parameter list are not mutants of
            # the arithmetic, and renaming it only shows that the file does not build.
            is_definition = after > 0 and next_token_is(text, after, "{")
            if not is_definition:
                if tok.text in ("gcd", "lcm") or tok.text.endswith(("_gcd", "_qlcm")):
                    name = "lcm" if "gcd" in tok.text else "gcd"
                    if name != tok.text:
                        out.append(Mutant(path, line, col, "gcd_lcm", tok.text, name, tok.start,
                                          tok.end, name))
            if after > 0 and not is_definition and is_commutative(tok.text):
                open_pos = tokens[index + 1].start
                if True:
                    comma = top_level_comma(text, open_pos + 1, after - 1)
                    if comma > 0:
                        left = text[open_pos + 1: comma]
                        right = text[comma + 1: after - 1]
                        if left.strip() and right.strip() and "(" not in left and "(" not in right:
                            # the text between the arguments is kept as it stands, so that the
                            # diff shows the exchange and nothing else
                            middle = text[comma: comma + 1]
                            out.append(Mutant(path, line, col, "swap_args",
                                              "%s(%s, %s)" % (tok.text, left.strip(), right.strip()),
                                              "%s(%s, %s)" % (tok.text, right.strip(), left.strip()),
                                              open_pos + 1, after - 1,
                                              right.strip() + middle + " " + left.strip()))

    # A statement `x = ...;` that can be removed, and a condition that can be negated.
    for index, tok in enumerate(tokens):
        if tok.text == ";":
            start_index = statement_start(text, tokens, index)
            if start_index >= index:
                continue
            first = tokens[start_index]
            stmt_start, stmt_end = first.start, tok.end
            body = text[stmt_start:stmt_end]
            depth = 0
            eq = -1
            for k, ch in enumerate(body):
                if ch in "([{":
                    depth += 1
                elif ch in ")]}":
                    depth -= 1
                elif ch == "=" and depth == 0:
                    eq = k
                    break
            if eq <= 0 or not IDENT_START.match(body[0]):
                continue
            lhs = body[:eq].strip()
            if not re.match(r"^[A-Za-z_][A-Za-z0-9_]*(\s*\[[^\[\]]*\])?$", lhs):
                continue
            line, col = line_of(text, stmt_start), col_of(text, stmt_start)
            out.append(Mutant(path, line, col, "drop_assign", " ".join(body.split()), "",
                              stmt_start, stmt_end, "",
                              note="the statement is removed"))
        if tok.text in ("if", "while") and index + 1 < len(tokens) and tokens[index + 1].text == "(":
            open_pos = tokens[index + 1].start
            after = match_paren(text, open_pos)
            if after > 0:
                cond = text[open_pos + 1: after - 1]
                if cond.strip() and "\n" not in cond and len(cond) < 200:
                    line, col = line_of(text, tok.start), col_of(text, tok.start)
                    out.append(Mutant(path, line, col, "negate_if", tok.text + "(%s)" % cond.strip(),
                                      tok.text + "(!(%s))" % cond.strip(), open_pos + 1, after - 1,
                                      "!(%s)" % cond.strip()))
    return out


def read_equivalent(path):
    """The excused survivors: `FILE:LINE:KIND | reason`, one per line, `#` for a comment."""
    out = {}
    if not os.path.isfile(path):
        return out
    with open(path, "r") as fh:
        for raw in fh:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            head, _, reason = line.partition("|")
            parts = [p.strip() for p in head.split(":")]
            if len(parts) != 3:
                continue
            try:
                key = (parts[0], int(parts[1]), parts[2])
            except ValueError:
                continue
            out[key] = reason.strip()
    return out


def copy_tree(root, entries, dest):
    if os.path.exists(dest):
        shutil.rmtree(dest)
    os.makedirs(dest)
    for entry in entries:
        source = os.path.join(root, entry)
        if not os.path.exists(source):
            continue
        target = os.path.join(dest, entry)
        if os.path.isdir(source):
            shutil.copytree(source, target, ignore=shutil.ignore_patterns("__pycache__"))
        else:
            shutil.copy2(source, target)


def run_make(directory, command, timeout):
    try:
        proc = subprocess.run(command, cwd=directory, shell=True, timeout=timeout,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except subprocess.TimeoutExpired as exc:
        return None, (exc.output or b"").decode("utf-8", "replace")
    return proc.returncode, proc.stdout.decode("utf-8", "replace")


def check_mutant(mutant, root, scratch, entries, command, timeout, number):
    """Build and run the tests with one mutant; return its status and a one-line reason.

    Every mutant has its own directory, so that two runs at the same time never share one."""
    workdir = os.path.join(scratch, "w%05d" % number)
    copy_tree(root, entries, workdir)
    path = os.path.join(workdir, mutant.path)
    with open(path, "r") as fh:
        text = fh.read()
    with open(path, "w") as fh:
        fh.write(mutant.apply(text))
    mutant.diff_text = mutant.diff(text, mutant.apply(text))
    del text
    code, output = run_make(workdir, command, timeout)
    if code is None:
        mutant.status = "timed out"
        mutant.detail = "the tests did not finish in %d s" % timeout
    elif code == 0:
        mutant.status = "survived"
        mutant.detail = "the tests pass with the mutant"
    elif COMPILE_ERROR.search(output):
        mutant.status = "not compiled"
        first = [l for l in output.splitlines() if "error:" in l or "undefined reference" in l]
        mutant.detail = first[0].strip() if first else "the build failed"
    else:
        mutant.status = "killed"
        lines = [l for l in output.splitlines() if l.startswith("FAIL")]
        mutant.detail = lines[0].strip() if lines else "a test failed"
    shutil.rmtree(workdir, ignore_errors=True)
    return mutant


def main(argv):
    here = os.path.dirname(os.path.abspath(__file__))
    parser = argparse.ArgumentParser(description="mutation testing for the C code of adelefeld",
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--root", default=".", help="the root of the tree to copy (default .)")
    parser.add_argument("--scratch", default=os.path.join("build", "mutate"),
                        help="the scratch directory; never inside the source tree (default "
                             "build/mutate)")
    parser.add_argument("--files", nargs="+", required=True, help="the files to mutate")
    parser.add_argument("--limit", type=int, default=200, help="the most mutants to run")
    parser.add_argument("--seed", type=int, default=20260928,
                        help="the seed of the choice of mutants, so that a run repeats")
    parser.add_argument("--jobs", type=int, default=2, help="the runs at the same time")
    parser.add_argument("--timeout", type=int, default=120, help="the seconds of one run")
    parser.add_argument("--make", dest="command", default="make -s -j2 check",
                        help="the build and test command of a mutant")
    parser.add_argument("--copy", nargs="*", default=COPY_ENTRIES,
                        help="the entries of the root that the scratch copy holds")
    parser.add_argument("--equivalent", default=os.path.join(here, "equivalent.txt"),
                        help="the file of the excused survivors")
    parser.add_argument("--list", action="store_true", help="only list the mutants, run none")
    parser.add_argument("--keep", action="store_true",
                        help="keep the scratch copy of every mutant under the scratch directory")
    args = parser.parse_args(argv)

    root = os.path.abspath(args.root)
    scratch = os.path.abspath(args.scratch)
    if scratch.startswith(root + os.sep) and os.path.relpath(scratch, root).split(os.sep)[0] == "src":
        print("mutate: the scratch directory must not be inside src/", file=sys.stderr)
        return 2
    os.makedirs(scratch, exist_ok=True)

    mutants = []
    for name in args.files:
        path = os.path.join(root, name)
        if not os.path.isfile(path):
            print("mutate: no such file: %s" % name, file=sys.stderr)
            return 2
        with open(path, "r") as fh:
            text = fh.read()
        found = mutants_of(name, text)
        print("mutate: %s: %d mutants" % (name, len(found)))
        mutants.extend(found)

    rng = random.Random(args.seed)
    rng.shuffle(mutants)
    if args.limit > 0 and len(mutants) > args.limit:
        print("mutate: %d mutants, %d of them run (--limit %d, --seed %d)"
              % (len(mutants), args.limit, args.limit, args.seed))
        mutants = mutants[: args.limit]

    if args.list:
        for m in mutants:
            print(m)
        return 0

    originals = {}
    for name in args.files:
        with open(os.path.join(root, name), "r") as fh:
            originals[name] = fh.read()

    start = time.time()
    base = os.path.join(scratch, "baseline")
    copy_tree(root, args.copy, base)
    code, output = run_make(base, args.command, args.timeout)
    if code != 0:
        print("mutate: the unmutated tree does not pass, so no mutant can be judged:", file=sys.stderr)
        print(output[-4000:], file=sys.stderr)
        shutil.rmtree(base, ignore_errors=True)
        return 2
    shutil.rmtree(base, ignore_errors=True)
    print("mutate: the baseline passes (%.1f s)" % (time.time() - start))

    def run(number):
        m = mutants[number]
        if args.keep:
            workdir = os.path.join(scratch, "keep", "%05d" % number)
            copy_tree(root, args.copy, workdir)
            with open(os.path.join(workdir, m.path), "w") as fh:
                fh.write(m.apply(originals[m.path]))
            code, _ = run_make(workdir, args.command, args.timeout)
            m.diff_text = m.diff(originals[m.path], m.apply(originals[m.path]))
            m.status = "survived" if code == 0 else "killed"
            m.detail = "the tests pass with the mutant" if code == 0 else "a test failed"
            return m
        return check_mutant(m, root, scratch, args.copy, args.command, args.timeout, number)

    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        for _ in pool.map(run, range(len(mutants))):
            pass

    equivalent = read_equivalent(args.equivalent)
    counts = {"killed": 0, "survived": 0, "not compiled": 0, "timed out": 0, "excused": 0}
    survivors = []
    for m in mutants:
        if m.status == "survived" and m.key in equivalent:
            m.status = "excused"
            m.detail = equivalent[m.key]
        counts[m.status] = counts.get(m.status, 0) + 1
        if m.status == "survived":
            survivors.append(m)

    for m in mutants:
        if m.status == "survived":
            print("\nSURVIVED %s\n  %s" % (m, m.detail))
            print("".join("  " + l for l in m.diff_text.splitlines(True)))
    for status in ("not compiled", "timed out"):
        for m in mutants:
            if m.status == status:
                print("\n%s %s\n  %s" % (status.upper(), m, m.detail))

    total = len(mutants)
    print("\nmutate: %d mutants in %.1f s: %d killed, %d survived, %d not compiled, %d timed out, "
          "%d excused" % (total, time.time() - start, counts["killed"], counts["survived"],
                          counts["not compiled"], counts["timed out"], counts["excused"]))
    if survivors:
        print("mutate: FAILED: %d mutant(s) survived; each one is a claim the tests do not check"
              % len(survivors))
        return 1
    print("mutate: passed: every mutant was killed, or is excused in %s"
          % os.path.relpath(args.equivalent, root))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
