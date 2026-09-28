#!/usr/bin/env python3
"""tools/mutate/mutate.py: mutation testing for the C code of adelefeld, without any tool
outside the standard library.

A mutant is one source file with one token changed. The mutations are:

    op            an arithmetic operator replaced by another of the same class. `&`, `*` and
                  `-` are mutated only where they stand for a binary operator: the address-of,
                  dereference and unary-minus occurrences of the same characters are left alone
                  (adf-obp), and `&&`/`||` are never touched here (they are their own token, see
                  `logic`, and `->` is always one token, never split).
    cmp           a comparison operator replaced by another, including moving its boundary
                  (`<` to `<=`, `<=` to `<`, and so on)
    logic         `&&` and `||` exchanged
    gcd_lcm       `gcd` replaced by `lcm` inside an identifier, and the other way round, kept
                  only when the renamed identifier is declared in a header this file includes
                  (directly or through its own local headers), so that a lone `gcd` is never
                  renamed to an undeclared `lcm` (adf-obp)
    swap_args     two arguments of a call to a commutative-looking function exchanged. For a
                  call of 3 or more arguments the first argument is never one of the two: it is
                  treated as the FLINT-style output slot, so the mutant never tries to pass a
                  `const` input where the output belongs (adf-obp)
    status        a returned status constant: `ADF_OK` becomes `ADF_DOMAIN`, and any other
                  status constant (docs/conventions.md 3.1, `include/adelefeld/status.h`)
                  becomes `ADF_OK` — only in a bare `return ADF_X;`
    drop_call     a statement that is a single call, `name(...);`, removed. A name ending in
                  `_init` or `_clear` is excluded (rationale in the module docstring below)
    call_swap     a call to `X_add` becomes `X_sub` and the reverse; `X_mul` becomes `X_add`;
                  kept only when the replacement name is declared in an included header
    prec          an argument that is the bare identifier `prec` replaced by `2`
    zero_one      the number 0 replaced by 1, and 1 by 0
    drop_assign   a statement of the form `x = ...;` removed
    negate_if     the condition of an `if` or a `while` negated

drop_call and the sanitizer (adf-obp, decided here): dropping an `_init` call leaves a FLINT
struct read before it is written, and dropping a `_clear` call only leaks it; neither is
guaranteed to make `make -s -j2 check` (the tool's own default, no sanitizer) fail, so such a
mutant would mostly report "survived" without saying anything about test quality. The tool has
no sanitizer run of its own -- `--make` can be pointed at `make -s -j2 check SAN=1` by a caller
who wants one, but that is not the default, because it roughly doubles every build in a budget
of at most 2 cores and about 3 minutes per mutant. So drop_call does not generate the `_init`/
`_clear` mutants at all, rather than generate them and let them survive by default.

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
import signal
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

# `&`, `*` and `-` also spell the address-of, dereference and unary-minus operators; a mutant
# of those is not a mutant of an arithmetic operator, it is text a C compiler does not accept
# (adf-obp: `&x->fin` mutated by `&` -> `|` is `|x->fin`, "expected expression before '|'").
# They are mutated by `op` only where the token before them ends an expression (see
# `ends_expression` and `is_unary_position`).
UNARY_CANDIDATES = ("&", "*", "-")

# `X_add` <-> `X_sub`, and `X_mul` -> `X_add` (one direction only: docs/PLAN.md gives no claim
# that a product may stand in for a sum). Kept only when the replacement name is declared
# (`is_declared`), so a call whose library has no `_sub`/`_add` twin is left alone.
CALL_SWAP_SUFFIX = {"_add": "_sub", "_sub": "_add", "_mul": "_add"}

# The longest first, so that `<<=` is not read as `<<`.
OPERATORS = ["<<=", ">>=", "...", "->", "++", "--", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||",
             "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "+", "-", "*", "/", "%", "<", ">",
             "=", "!", "~", "&", "|", "^", "?", ":", ";", ",", ".", "(", ")", "[", "]", "{", "}"]

IDENT_START = re.compile(r"[A-Za-z_]")
IDENT_CHAR = re.compile(r"[A-Za-z0-9_]")
NUMBER = re.compile(r"(0[xX][0-9a-fA-F]+|[0-9]+)")

# A `#include "..."` or `#include <...>` line, so that a renamed identifier can be checked
# against what the file actually declares (gcd_lcm, call_swap).
INCLUDE_RE = re.compile(r'^[ \t]*#[ \t]*include[ \t]*(["<])([^">]+)[">]', re.MULTILINE)

CONTROL_KEYWORDS = frozenset(["if", "for", "while", "switch", "do", "else", "return", "sizeof",
                              "case", "default", "goto", "break", "continue", "typedef",
                              "struct", "union", "enum"])

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


def next_token_is(text, pos, what):
    """1 if the first byte after pos that is not white space starts the token `what`."""
    i = pos
    n = len(text)
    while i < n and text[i] in " \t\r\n":
        i += 1
    return i < n and text[i] == what


def is_commutative(name):
    return name in COMMUTATIVE_NAMES or any(name.endswith(s) for s in COMMUTATIVE_SUFFIXES)


def ends_expression(tok):
    """1 if a token of this kind or text can be the last token of a value: `&x` is unary
    (the token before `&` does not end an expression), `a & b` is binary (`a` does). A keyword
    of CONTROL_KEYWORDS is lexically an "ident" (scan() does not know C's keywords from its
    names) but never ends an expression: `return -x;` is `return` then unary `-x`, not a value
    called `return` minus `x` (adf-obp)."""
    if tok is None:
        return False
    if tok.kind == "ident":
        return tok.text not in CONTROL_KEYWORDS
    if tok.kind == "number":
        return True
    return tok.text in (")", "]")


def is_unary_position(tokens, index):
    """1 if tokens[index] (one of UNARY_CANDIDATES) stands where a prefix operator stands:
    at the start of the file, or after a token that does not end an expression. Used only for
    `&`, `*` and `-`, the three characters that are also prefix operators of C (adf-obp)."""
    return not ends_expression(tokens[index - 1] if index > 0 else None)


# The tokens that can introduce a parameter's or a declaration's type, so that `TYPE * name`
# is read as the one token before `TYPE` that a value-identifier before a real multiplication
# is never preceded by (see is_pointer_declarator_star).
DECL_INTRODUCERS = frozenset(["(", ",", "const", "static", "volatile", "struct", "union",
                              "enum", "unsigned", "signed"])


def is_pointer_declarator_star(tokens, index):
    """1 if tokens[index] (a `*`) sits where a pointer declarator's `*` sits: `TYPE * name`,
    where index - 1 and index + 1 are identifiers and index - 2 is one of DECL_INTRODUCERS --
    e.g. `slong * e` (src/fball.c adf_fball_prec_at) or `adf_example_box * out`. `TYPE * name`
    and a multiplication `a * b` are the same three tokens (ident, `*`, ident); telling them
    apart in general needs a symbol table, which this tool does not build. This instead checks
    the one shape adelefeld's declarations use: `grep` over src/*.c shows every `IDENT * IDENT`
    followed by `,` or `)` is a pointer parameter, immediately preceded by `(`, `,` or a
    qualifier, and no multiplication in src/*.c is written so that it is (adf-obp: without this,
    `adf_example_box * out` in a parameter list was mutated to `adf_example_box / out`, "expected
    ';', ',' or ')' before '/' token")."""
    if index < 2 or index + 1 >= len(tokens):
        return False
    if tokens[index - 1].kind != "ident" or tokens[index + 1].kind != "ident":
        return False
    return tokens[index - 2].text in DECL_INTRODUCERS


def _resolve_include(inc, quoted, from_dir, root):
    """The path of an #include, or None. Quoted looks next to the including file first; both
    forms fall back to <root>/include (the project's own -Iinclude) and, for <...>, the system
    include directories that hold FLINT (conventions: FLINT 3.0.1, /usr/include/flint)."""
    candidates = []
    if quoted:
        candidates.append(os.path.join(from_dir, inc))
    candidates.append(os.path.join(root, "include", inc))
    if not quoted:
        candidates.append(os.path.join("/usr/include", inc))
        candidates.append(os.path.join("/usr/local/include", inc))
    for c in candidates:
        if os.path.isfile(c):
            return os.path.normpath(c)
    return None


def _collect_header_text(root, path, depth, seen, chunks):
    """Read path and, up to depth levels, every header it includes that resolves under the
    project's include/ or a system/FLINT include directory; append each text to chunks. seen
    guards against reading the same header twice (a diamond of includes) or looping forever."""
    real = os.path.realpath(path)
    if real in seen or depth < 0:
        return
    seen.add(real)
    try:
        with open(path, "r", errors="replace") as fh:
            text = fh.read()
    except OSError:
        return
    chunks.append(text)
    for m in INCLUDE_RE.finditer(text):
        resolved = _resolve_include(m.group(2), m.group(1) == '"', os.path.dirname(path), root)
        if resolved:
            _collect_header_text(root, resolved, depth - 1, seen, chunks)


def declared_text_of(root, path, text):
    """The text of this source file plus every header it includes, directly or (for the
    project's own headers) through those headers' own includes -- e.g. src/adele.c includes
    only <adelefeld/adele.h>, and adele.h includes "adelefeld/fball.h", which is where
    adf_fball_sub is declared. Depth 6 is more than the project's own header nesting needs.
    Used by gcd_lcm and call_swap so that a rename is only offered when the renamed name is
    something this file could actually call (adf-obp: `fmpq_gcd` renamed to a bare `lcm` that
    nothing declares)."""
    seen = set()
    chunks = [text]
    for m in INCLUDE_RE.finditer(text):
        resolved = _resolve_include(m.group(2), m.group(1) == '"',
                                    os.path.dirname(os.path.join(root, path)), root)
        if resolved:
            _collect_header_text(root, resolved, 6, seen, chunks)
    return "\n".join(chunks)


def is_declared(name, declared_text):
    return re.search(r"(?<![A-Za-z0-9_])" + re.escape(name) + r"\s*\(", declared_text) is not None


def gcd_lcm_rename(name):
    """name with its one occurrence of `gcd` turned into `lcm`, or the other way round; None
    if name has neither, or has both (ambiguous, so left alone)."""
    has_gcd = "gcd" in name
    has_lcm = "lcm" in name
    if has_gcd == has_lcm:
        return None
    old, new = ("gcd", "lcm") if has_gcd else ("lcm", "gcd")
    return name.replace(old, new)


_STATUS_CACHE = {}


def status_names(root):
    """The status constants of include/adelefeld/status.h (conventions 3.1, "The list"):
    ADF_OK and the rest, read from the header itself and not copied by hand so that a status
    added there is picked up here too. ADF_STATUS_COUNT is a count, not a status; the ADF_CMP_
    constants are a different thing the same header defines (its own comment: "Not a status")
    and are excluded by name."""
    if root not in _STATUS_CACHE:
        names = []
        path = os.path.join(root, "include", "adelefeld", "status.h")
        try:
            with open(path, "r") as fh:
                text = fh.read()
        except OSError:
            text = ""
        for m in re.finditer(r"#define\s+(ADF_[A-Za-z0-9_]+)\s+\d+", text):
            name = m.group(1)
            if name == "ADF_STATUS_COUNT" or name.startswith("ADF_CMP_"):
                continue
            names.append(name)
        _STATUS_CACHE[root] = names
    return _STATUS_CACHE[root]


def split_top_level_args(text, start, stop):
    """The (start, end) span of each argument of a call whose argument list runs from start to
    stop (just inside the parentheses), split on the commas at bracket depth 0. A quoted string
    or character literal is skipped whole, so a comma or bracket inside one is not counted."""
    spans = []
    depth = 0
    i = start
    arg_start = start
    while i < stop:
        c = text[i]
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        elif c in "\"'":
            quote = c
            i += 1
            while i < stop and text[i] != quote:
                i += 2 if text[i] == "\\" else 1
            i += 1
            continue
        elif c == "," and depth == 0:
            spans.append((arg_start, i))
            arg_start = i + 1
        i += 1
    spans.append((arg_start, stop))
    return spans


def statement_start(text, tokens, index):
    """The index of the first token of the statement that ends at tokens[index] (';')."""
    i = index - 1
    while i >= 0 and tokens[i].text not in (";", "{", "}", ":"):
        i -= 1
    return i + 1


def mutants_of(path, text, root="."):
    """Every mutant of one file, in the order the tokens stand in the file. root is the tree
    the file is read from, needed only to resolve the headers gcd_lcm, call_swap and status
    check against (declared_text_of, status_names); every other kind needs only the text."""
    out = []
    tokens = scan(text)
    declared_text = None  # computed once, only if gcd_lcm or call_swap actually needs it

    for index, tok in enumerate(tokens):
        line, col = line_of(text, tok.start), col_of(text, tok.start)
        if tok.kind == "op":
            if tok.text in UNARY_CANDIDATES and (is_unary_position(tokens, index) or
                    (tok.text == "*" and is_pointer_declarator_star(tokens, index))):
                # &x, *x, -x: the address-of, dereference and unary-minus operators, and a
                # pointer declarator's `*` (`TYPE * name`) -- none of them the binary operators
                # OP_MAP knows about (adf-obp; see UNARY_CANDIDATES and
                # is_pointer_declarator_star above).
                pass
            else:
                for kind, table in (("op", OP_MAP), ("cmp", CMP_MAP), ("logic", LOGIC_MAP)):
                    for new in table.get(tok.text, []):
                        out.append(Mutant(path, line, col, kind, tok.text, new, tok.start,
                                          tok.end, new))
        elif tok.kind == "number" and tok.text in ("0", "1"):
            new = "1" if tok.text == "0" else "0"
            out.append(Mutant(path, line, col, "zero_one", tok.text, new, tok.start, tok.end, new))
        elif tok.kind == "ident":
            if tok.text == "prec" and index > 0 and index + 1 < len(tokens) and \
                    tokens[index - 1].text in ("(", ",") and tokens[index + 1].text in (")", ","):
                out.append(Mutant(path, line, col, "prec", "prec", "2", tok.start, tok.end, "2"))
            if tok.text in status_names(root) and index > 0 and index + 1 < len(tokens) and \
                    tokens[index - 1].text == "return" and tokens[index + 1].text == ";":
                new = "ADF_DOMAIN" if tok.text == "ADF_OK" else "ADF_OK"
                out.append(Mutant(path, line, col, "status", tok.text, new, tok.start, tok.end, new))

            open_pos = tokens[index + 1].start if index + 1 < len(tokens) else -1
            after = match_paren(text, open_pos) if open_pos >= 0 and \
                tokens[index + 1].text == "(" else -1
            # A definition is not a call: its name and its parameter list are not mutants of
            # the arithmetic, and renaming it only shows that the file does not build.
            is_definition = after > 0 and next_token_is(text, after, "{")
            if after > 0 and not is_definition:
                renamed = gcd_lcm_rename(tok.text)
                if renamed and renamed != tok.text:
                    if declared_text is None:
                        declared_text = declared_text_of(root, path, text)
                    if is_declared(renamed, declared_text):
                        out.append(Mutant(path, line, col, "gcd_lcm", tok.text, renamed,
                                          tok.start, tok.end, renamed))

                for suffix, replacement in CALL_SWAP_SUFFIX.items():
                    if tok.text.endswith(suffix):
                        new_name = tok.text[: -len(suffix)] + replacement
                        if declared_text is None:
                            declared_text = declared_text_of(root, path, text)
                        if is_declared(new_name, declared_text):
                            out.append(Mutant(path, line, col, "call_swap", tok.text, new_name,
                                              tok.start, tok.end, new_name))
                        break

            if after > 0 and not is_definition and is_commutative(tok.text):
                open_pos = tokens[index + 1].start
                args = split_top_level_args(text, open_pos + 1, after - 1)
                # A call of 3 or more arguments is read as FLINT-style, `f(out, a, b, ...)`:
                # arg 0 is never one of the two that are exchanged, or the mutant would try to
                # write the result into what may be a `const` input (adf-obp). A call of
                # exactly 2 arguments has no separate output slot (e.g. adf_example_gcd(n, m),
                # which returns its result), so both are exchanged.
                if len(args) == 2:
                    i, j = 0, 1
                elif len(args) >= 3:
                    i, j = 1, 2
                else:
                    i = j = -1
                if i >= 0:
                    a_s, a_e = args[i]
                    b_s, b_e = args[j]
                    a_text, b_text = text[a_s:a_e], text[b_s:b_e]
                    if a_text.strip() and b_text.strip() and "(" not in a_text and \
                            "(" not in b_text:
                        stripped = [text[s:e].strip() for s, e in args]
                        old_call = "%s(%s)" % (tok.text, ", ".join(stripped))
                        swapped = list(stripped)
                        swapped[i], swapped[j] = swapped[j], swapped[i]
                        new_call = "%s(%s)" % (tok.text, ", ".join(swapped))
                        # the text between the two arguments is kept as it stands, so that the
                        # diff shows only the exchange
                        replacement = b_text + text[a_e:b_s] + a_text
                        out.append(Mutant(path, line, col, "swap_args", old_call, new_call,
                                          a_s, b_e, replacement))

    # A statement `x = ...;` that can be removed, a statement that is a single call and can be
    # removed too (drop_call), and a condition that can be negated.
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
            line, col = line_of(text, stmt_start), col_of(text, stmt_start)
            if eq > 0 and IDENT_START.match(body[0]):
                lhs = body[:eq].strip()
                if re.match(r"^[A-Za-z_][A-Za-z0-9_]*(\s*\[[^\[\]]*\])?$", lhs):
                    out.append(Mutant(path, line, col, "drop_assign", " ".join(body.split()), "",
                                      stmt_start, stmt_end, "", note="the statement is removed"))
                    continue
            if eq <= 0 and IDENT_START.match(body[0]) and start_index + 1 < index and \
                    tokens[start_index + 1].text == "(":
                name = first.text
                open_pos = tokens[start_index + 1].start
                after = match_paren(text, open_pos)
                if after > 0 and text[after:tok.start].strip() == "" and \
                        name not in CONTROL_KEYWORDS and not name.endswith(("_init", "_clear")):
                    out.append(Mutant(path, line, col, "drop_call", " ".join(body.split()), "",
                                      stmt_start, stmt_end, "", note="the statement is removed"))
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
    """Run the command in the directory; return (exit code, output), the code None on timeout.

    The command runs in a session of its own, and on timeout the whole process group is
    killed: the shell, make, and the test program that make started. Killing the shell alone
    leaves a mutant with an infinite loop running for ever (issue adf-98j)."""
    proc = subprocess.Popen(command, cwd=directory, shell=True, start_new_session=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    try:
        output, _ = proc.communicate(timeout=timeout)
        code = proc.returncode
    except subprocess.TimeoutExpired:
        code = None
    finally:
        # also when the run is interrupted, and for what a finished command left behind
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except OSError:
            pass
    if code is None:
        output, _ = proc.communicate()
    return code, (output or b"").decode("utf-8", "replace")


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
    scratch_root = os.path.abspath(args.scratch)
    if scratch_root.startswith(root + os.sep) and \
            os.path.relpath(scratch_root, root).split(os.sep)[0] == "src":
        print("mutate: the scratch directory must not be inside src/", file=sys.stderr)
        return 2
    os.makedirs(scratch_root, exist_ok=True)
    # Every run gets its own subdirectory under --scratch, named for this process (adf-obp).
    # --scratch defaults to the same build/mutate path for every invocation, and two runs that
    # share it -- two `make mutate` started close together, or one begun before an earlier run
    # finished cleaning up -- would otherwise pick the same numbered workdir (w00042, say) for
    # two different mutants. copy_tree's exists-then-rmtree-then-makedirs is not atomic, so two
    # runs racing on that path can interleave one mutant's files into another's build; the
    # tests then judge something that is neither mutant, and which one depends on scheduling.
    # Reported effect: `make mutate FILES=src/recon.c JOBS=2`, run five times with the same
    # seed, reported between two and five different survivors. try/finally below removes this
    # run's own subdirectory again when it is done, --keep aside.
    scratch = os.path.join(scratch_root, "run-%d" % os.getpid())
    if os.path.exists(scratch):
        shutil.rmtree(scratch)
    os.makedirs(scratch)

    try:
        return run_mutate(args, root, scratch)
    finally:
        if not args.keep:
            shutil.rmtree(scratch, ignore_errors=True)


def run_mutate(args, root, scratch):
    mutants = []
    for name in args.files:
        path = os.path.join(root, name)
        if not os.path.isfile(path):
            print("mutate: no such file: %s" % name, file=sys.stderr)
            return 2
        with open(path, "r") as fh:
            text = fh.read()
        found = mutants_of(name, text, root)
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
