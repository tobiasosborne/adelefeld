#!/usr/bin/env python3
"""check_uninit.py -- find FLINT / adelefeld locals that are used before their init.

The address and undefined-behaviour sanitizers do not see a read of an uninitialised
FLINT value: the type is an array of one struct, so a `fmpq_t q;` that is never passed
to `fmpq_init` is a valid pointer to uninitialised bytes, and the read only fails when
GMP follows a garbage limb pointer.  This tool reads the C source text instead and
reports three things per function:

  use-before-init    a local of a FLINT or adelefeld type is used (read or passed to a
                     function other than an init) before any init call with it
  clear-before-init  a clear call with the variable before any init
  init-without-clear an init with no matching clear before the variable leaves scope

The analysis is syntactic.  A call initialises a variable when the callee name contains
`_init` and the variable is the base identifier of the first argument, or when the
callee is a project helper whose corresponding parameter is itself initialised in its
own body (that relation is computed to a fixpoint, which is what recognises
`adele_init_fields`, `adele_build`, `adele_build_small`, `tx_buf_init` and the like).
A destructor is a call whose callee name contains `_clear`.

A declaration that is never used and never initialised is not a finding: the compiler
reports it as unused.  A pointer, a struct field or a function pointer is not a local
of a target type and is ignored.

Exit status is 0 when nothing was found and 1 when at least one finding was printed.
Pass `--category CAT` to print only one of the three categories (used by the red-green
test of the tool).

Usage:
    check_uninit.py FILE.c [FILE.c ...]
    check_uninit.py --category use-before-init tests/test_recon.c
"""

import argparse
import bisect
import re
import sys


# ---------------------------------------------------------------- types

FLINT_TYPES = {
    "fmpz_t",
    "fmpq_t",
    "arb_t",
    "acb_t",
    "arf_t",
    "mag_t",
}
# The adelefeld types that carry owned memory and so need a matching init and clear.
# adf_place_t and adf_text_limits_t are plain by-value structs with no init function
# (include/adelefeld/place.h:20-24, include/adelefeld/text.h:67-74) and are deliberately
# not listed: requiring an init for them would be a false positive.
ADF_TYPES = {
    "adf_adele_t",
    "adf_cadele_t",
    "adf_fball_t",
    "adf_rat_t",
    "adf_scaled_t",
    "adf_ctx_desc_t",
}
QUALIFIERS = {"const", "static", "volatile", "register", "auto", "extern", "_Thread_local"}
CONTROL_KEYWORDS = {
    "if",
    "for",
    "while",
    "switch",
    "return",
    "sizeof",
    "do",
    "else",
    "case",
    "defined",
    "__attribute__",
}

CATEGORIES = ("use-before-init", "clear-before-init", "init-without-clear")


def is_target_type(text):
    return text in FLINT_TYPES or text in ADF_TYPES


def is_identifier(text):
    return bool(text) and (text[0].isalpha() or text[0] == "_")


# ---------------------------------------------------------------- lexer

TOKEN_RE = re.compile(
    r"""
      [A-Za-z_][A-Za-z0-9_]*      # identifier or keyword
    | \.\.\.                       # ellipsis
    | ->                           # arrow
    | \+\+|--|<<|>>|<=|>=|==|!=|&&|\|\|
    | [0-9]+                       # integer literal
    | [-+*/%<>=!&|^~?:;,.()\[\]{}] # single character
    """,
    re.VERBOSE,
)


def strip_comments_and_strings(src):
    """Blank out comments, string and character literals and preprocessor lines.

    Newlines are kept so that a line number survives.  A blanked character is replaced
    by a space, so token boundaries and line offsets do not move.
    """
    out = list(src)
    i = 0
    n = len(src)
    line_start = True
    while i < n:
        c = src[i]
        if line_start:
            j = i
            while j < n and src[j] in " \t":
                j += 1
            if j < n and src[j] == "#":
                while i < n:
                    if src[i] == "\n":
                        if i > 0 and src[i - 1] == "\\":
                            out[i] = "\n"
                            i += 1
                            continue
                        line_start = True
                        break
                    out[i] = " "
                    i += 1
                continue
            line_start = False
        if c == "\n":
            line_start = True
            i += 1
            continue
        if c == "/" and i + 1 < n and src[i + 1] == "/":
            while i < n and src[i] != "\n":
                out[i] = " "
                i += 1
            continue
        if c == "/" and i + 1 < n and src[i + 1] == "*":
            out[i] = " "
            out[i + 1] = " "
            i += 2
            while i < n and not (src[i] == "*" and i + 1 < n and src[i + 1] == "/"):
                if src[i] != "\n":
                    out[i] = " "
                i += 1
            if i < n:
                out[i] = " "
                if i + 1 < n:
                    out[i + 1] = " "
                i += 2
            continue
        if c == '"' or c == "'":
            quote = c
            out[i] = " "
            i += 1
            while i < n and src[i] != quote:
                if src[i] == "\\" and i + 1 < n:
                    out[i] = " "
                    i += 1
                if src[i] != "\n":
                    out[i] = " "
                i += 1
            if i < n:
                out[i] = " "
                i += 1
            continue
        i += 1
    return "".join(out)


class Token:
    __slots__ = ("text", "line")

    def __init__(self, text, line):
        self.text = text
        self.line = line

    def __repr__(self):
        return "Token(%r,%d)" % (self.text, self.line)


def tokenize(src):
    stripped = strip_comments_and_strings(src)
    newline_positions = [i for i, c in enumerate(stripped) if c == "\n"]

    def line_of(pos):
        return bisect.bisect_right(newline_positions, pos - 1) + 1

    return [Token(m.group(0), line_of(m.start())) for m in TOKEN_RE.finditer(stripped)]


# ---------------------------------------------------------------- functions

def is_function_prelude(prelude):
    if not prelude:
        return False
    texts = [t.text for t in prelude]
    if "=" in texts:
        return False
    if ")" not in texts:
        return False
    if texts[0] in ("struct", "union", "enum"):
        return False
    return True


def find_functions(tokens):
    """Return [(prelude, body_tokens, start_line), ...] for top-level definitions."""
    funcs = []
    i = 0
    depth = 0
    stmt_start = 0
    n = len(tokens)
    while i < n:
        text = tokens[i].text
        if text == "{" and depth == 0 and is_function_prelude(tokens[stmt_start:i]):
            j = i + 1
            d = 1
            while j < n and d > 0:
                if tokens[j].text == "{":
                    d += 1
                elif tokens[j].text == "}":
                    d -= 1
                j += 1
            funcs.append((tokens[stmt_start:i], tokens[i + 1:j - 1], tokens[i].line))
            i = j
            stmt_start = i
            continue
        if text == "{":
            depth += 1
        elif text == "}":
            depth = max(0, depth - 1)
            stmt_start = i + 1
        elif text == ";" and depth == 0:
            stmt_start = i + 1
        i += 1
    return funcs


def matching_paren(tokens, open_index):
    depth = 0
    for j in range(open_index, len(tokens)):
        if tokens[j].text == "(":
            depth += 1
        elif tokens[j].text == ")":
            depth -= 1
            if depth == 0:
                return j
    return len(tokens) - 1


def split_top_level(tokens):
    """Split on commas that are not inside () or []."""
    parts = []
    current = []
    paren = 0
    bracket = 0
    for t in tokens:
        if t.text == "(":
            paren += 1
        elif t.text == ")":
            paren -= 1
        elif t.text == "[":
            bracket += 1
        elif t.text == "]":
            bracket -= 1
        elif t.text == "," and paren == 0 and bracket == 0:
            parts.append(current)
            current = []
            continue
        current.append(t)
    if current:
        parts.append(current)
    return parts


def iter_calls(tokens):
    """Yield (callee_name, [arg_tokens, ...], line) for every call in the token list."""
    i = 0
    n = len(tokens)
    while i < n:
        text = tokens[i].text
        if (
            is_identifier(text)
            and text not in CONTROL_KEYWORDS
            and i + 1 < n
            and tokens[i + 1].text == "("
        ):
            close = matching_paren(tokens, i + 1)
            args = split_top_level(tokens[i + 2:close])
            if len(args) == 1 and not args[0]:
                args = []
            yield text, args, tokens[i].line
            i += 1
            continue
        i += 1


def function_signature(prelude):
    """Return (name, [param_base_names]) for a function definition prelude."""
    open_index = None
    for i, t in enumerate(prelude):
        if t.text == "(":
            open_index = i
            break
    if open_index is None or open_index == 0:
        return None, []
    name = None
    for i in range(open_index - 1, -1, -1):
        if is_identifier(prelude[i].text):
            name = prelude[i].text
            break
    close = matching_paren(prelude, open_index)
    params = []
    for part in split_top_level(prelude[open_index + 1:close]):
        if not part:
            params.append(None)
            continue
        base = None
        for t in part:
            if t.text == "[":
                break
            if is_identifier(t.text):
                base = t.text
        params.append(base)
    return name, params


# ---------------------------------------------------------------- declarators

def base_identifier(segment):
    """The base name of a declarator segment, or None for a function pointer."""
    name = None
    saw_paren_after_name = False
    for t in segment:
        if t.text in ("->", "."):
            continue
        if is_identifier(t.text):
            if name is None:
                if t.text in QUALIFIERS or is_target_type(t.text):
                    continue
                name = t.text
            else:
                saw_paren_after_name = True
        elif t.text == "(" and name is not None:
            saw_paren_after_name = True
    return None if saw_paren_after_name else name


def parse_declaration(tokens, start):
    """Parse a declaration at tokens[start]; return ([(name, segment)], next_index)."""
    i = start
    n = len(tokens)
    while i < n and tokens[i].text in QUALIFIERS:
        i += 1
    if i >= n or not is_target_type(tokens[i].text):
        return [], start + 1
    i += 1
    decl_start = i
    paren = 0
    bracket = 0
    while i < n:
        t = tokens[i].text
        if t == "(":
            paren += 1
        elif t == ")":
            paren -= 1
        elif t == "[":
            bracket += 1
        elif t == "]":
            bracket -= 1
        elif t == ";" and paren == 0 and bracket == 0:
            break
        i += 1
    names = []
    for part in split_top_level(tokens[decl_start:i]):
        name = base_identifier(part)
        if name is not None:
            names.append((name, part))
    return names, i + 1


def argument_base(tokens):
    """The base identifier of an argument, e.g. x in `&x`, list in `list[k]`."""
    prev = None
    for t in tokens:
        if t.text in ("->", "."):
            prev = t.text
            continue
        if is_identifier(t.text) and prev not in ("->", "."):
            return t.text
        prev = t.text
    return None


# ---------------------------------------------------------------- inference

def infer_init_helpers(functions):
    """Map a function name to the set of parameter indices that it initialises.

    A parameter is initialised when the body calls an init function on it, directly or
    through another helper.  Computed to a fixpoint.
    """
    param_index = {}
    bodies = {}
    for prelude, body, _line in functions:
        name, params = function_signature(prelude)
        if name is None:
            continue
        param_index[name] = {p: k for k, p in enumerate(params) if p is not None}
        bodies[name] = body

    helpers = {}

    def add_helper(fname, index):
        indices = helpers.setdefault(fname, set())
        if index not in indices:
            indices.add(index)
            return True
        return False

    changed = True
    while changed:
        changed = False
        for fname, body in bodies.items():
            pa = param_index[fname]
            for callee, args, _line in iter_calls(body):
                is_init_named = "_init" in callee
                known = helpers.get(callee, set())
                if not is_init_named and not known:
                    continue
                for k, arg in enumerate(args):
                    if is_init_named and k != 0:
                        continue
                    if not is_init_named and k not in known:
                        continue
                    base = argument_base(arg)
                    if base is not None and base in pa:
                        if add_helper(fname, pa[base]):
                            changed = True
    return helpers


# ---------------------------------------------------------------- analysis

class Variable:
    __slots__ = ("name", "line", "init_line", "clear_line", "use_line", "use_text")

    def __init__(self, name, line):
        self.name = name
        self.line = line
        self.init_line = None
        self.clear_line = None
        self.use_line = None
        self.use_text = None


class Finding:
    __slots__ = ("file", "line", "name", "category", "detail")

    def __init__(self, file, line, name, category, detail):
        self.file = file
        self.line = line
        self.name = name
        self.category = category
        self.detail = detail

    def __str__(self):
        return "%s:%d: %s: %s: %s" % (
            self.file,
            self.line,
            self.name,
            self.category,
            self.detail,
        )


class Analyzer:
    def __init__(self, path, tokens, helpers):
        self.path = path
        self.tokens = tokens
        self.helpers = helpers
        self.scopes = [{}]
        self.findings = []

    def push(self):
        self.scopes.append({})

    def pop(self):
        scope = self.scopes.pop()
        for var in scope.values():
            if var.init_line is None and var.use_line is not None:
                self.findings.append(
                    Finding(
                        self.path,
                        var.use_line,
                        var.name,
                        "use-before-init",
                        "first use `%s` (declared line %d)" % (var.use_text, var.line),
                    )
                )
            if var.init_line is not None and var.clear_line is None:
                self.findings.append(
                    Finding(self.path, var.init_line, var.name, "init-without-clear", "precise")
                )
            if var.clear_line is not None and var.init_line is None:
                self.findings.append(
                    Finding(
                        self.path,
                        var.clear_line,
                        var.name,
                        "clear-before-init",
                        "no init before the clear",
                    )
                )

    def lookup(self, name):
        for scope in reversed(self.scopes):
            if name in scope:
                return scope[name]
        return None

    def record_use(self, name, line, text):
        var = self.lookup(name)
        if var is not None and var.use_line is None:
            var.use_line = line
            var.use_text = text

    def record_init(self, name, line):
        var = self.lookup(name)
        if var is not None and var.init_line is None:
            var.init_line = line

    def record_clear(self, name, line):
        var = self.lookup(name)
        if var is not None and var.clear_line is None:
            var.clear_line = line

    def handle_call(self, i):
        """Process a call at token i; return the index just past the closing paren."""
        name = self.tokens[i].text
        open_index = i + 1
        close = matching_paren(self.tokens, open_index)
        args = split_top_level(self.tokens[open_index + 1:close])
        if len(args) == 1 and not args[0]:
            args = []
        line = self.tokens[i].line

        is_init_named = "_init" in name
        is_clear_named = "_clear" in name
        helper = self.helpers.get(name, set())

        if is_clear_named:
            if args:
                base = argument_base(args[0])
                if base is not None:
                    self.record_clear(base, line)
            for k, arg in enumerate(args):
                if k == 0:
                    continue
                base = argument_base(arg)
                if base is not None:
                    self.record_use(base, line, base)
        elif is_init_named or helper:
            init_indices = set()
            if is_init_named:
                init_indices.add(0)
            init_indices |= helper
            for k, arg in enumerate(args):
                base = argument_base(arg)
                if base is None:
                    continue
                if k in init_indices:
                    self.record_init(base, line)
                else:
                    self.record_use(base, line, base)
        else:
            for arg in args:
                base = argument_base(arg)
                if base is not None:
                    self.record_use(base, line, base)
        return close + 1

    def run(self):
        tokens = self.tokens
        i = 0
        n = len(tokens)
        while i < n:
            text = tokens[i].text
            if text == "{":
                self.push()
                i += 1
                continue
            if text == "}":
                if len(self.scopes) > 1:
                    self.pop()
                i += 1
                continue
            j = i
            if tokens[j].text in QUALIFIERS or is_target_type(tokens[j].text):
                while j < n and tokens[j].text in QUALIFIERS:
                    j += 1
                if j < n and is_target_type(tokens[j].text):
                    names, nxt = parse_declaration(tokens, i)
                    for name, _part in names:
                        self.scopes[-1][name] = Variable(name, tokens[i].line)
                    i = nxt
                    continue
            if (
                is_identifier(text)
                and text not in CONTROL_KEYWORDS
                and i + 1 < n
                and tokens[i + 1].text == "("
            ):
                i = self.handle_call(i)
                continue
            if is_identifier(text):
                prev = tokens[i - 1].text if i > 0 else None
                if prev not in ("->", ".") and not is_target_type(text):
                    self.record_use(text, tokens[i].line, text)
            i += 1
        while len(self.scopes) > 1:
            self.pop()
        self.pop()
        return self.findings


def check_file(path):
    try:
        with open(path, "r", encoding="utf-8") as handle:
            src = handle.read()
    except OSError as exc:
        print("cannot read %s: %s" % (path, exc), file=sys.stderr)
        return []
    tokens = tokenize(src)
    functions = find_functions(tokens)
    helpers = infer_init_helpers(functions)
    findings = []
    for _prelude, body, _line in functions:
        findings.extend(Analyzer(path, body, helpers).run())
    return findings


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("files", nargs="+", help="C source files to check")
    parser.add_argument(
        "--category",
        choices=CATEGORIES,
        default=None,
        help="print only findings of this category",
    )
    args = parser.parse_args(argv)

    all_findings = []
    for path in args.files:
        all_findings.extend(check_file(path))

    shown = 0
    for finding in all_findings:
        if args.category is None or finding.category == args.category:
            print(finding)
            shown += 1

    if args.category is None:
        for category in CATEGORIES:
            count = sum(1 for f in all_findings if f.category == category)
            print("# %s: %d" % (category, count), file=sys.stderr)

    return 1 if shown else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
