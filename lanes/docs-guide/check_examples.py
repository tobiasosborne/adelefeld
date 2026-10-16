#!/usr/bin/env python3
"""lanes/docs-guide/check_examples.py: rerun every example of docs/GUIDE.md and diff it.

Run from the repository root (check_examples.sh does that). Standard library only.

What is an example (the markers are in the Markdown source and invisible on GitHub):

  ```adf            a driver session. A line that begins with "> " is an input line; every
  > show 22/7       other line is output. The input lines are run in one session of build/adf,
  22/7              in order, and the transcript is rebuilt: after each input line, the lines
  ```               that the driver printed for it. The rebuilt block must equal the block.

  <!-- check: file docs/examples/hello.c -->   the next fenced block must equal that file.
  <!-- check: hello -->                         the next fenced block must equal the output of
                                                build/hello, compiled from docs/examples/hello.c
                                                with the compile line of docs/examples/README.md.
  <!-- check: julia -->                         the next fenced block is a Julia program; it is run
  <!-- check: julia-output -->                  with the package's launcher, and its standard output
                                                must equal the fenced block after this marker.

How the transcript is rebuilt: the driver prints one line per command and none for a setting that
succeeds (tools/adf/README.md:381-384), but a setting that fails prints an error line, so the
script does not guess. For an input of n lines it runs the prefixes of 1, 2, ..., n lines and takes
the new output lines of each prefix as the output of its last line. Sessions are short (at most a
few dozen lines), so the n runs cost little.

Modes:
  (none)      check; print one line per example and the total "N sessions, M differ"; exit 1 if M > 0.
  --fill      rewrite the output lines of every adf block from fresh runs (used while writing).
  --record F  write F (numbers.md): every session as its command and verbatim output.
"""

import os
import re
import subprocess
import sys
import tempfile

ROOT = os.getcwd()
GUIDE = os.environ.get("CHECK_GUIDE", os.path.join(ROOT, "docs", "GUIDE.md"))
ADF = os.path.join(ROOT, "build", "adf")
HELLO_SRC = os.path.join(ROOT, "docs", "examples", "hello.c")
EXAMPLES_README = os.path.join(ROOT, "docs", "examples", "README.md")
HELLO_BIN = os.path.join(ROOT, "build", "hello")
COMPILE = ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-Iinclude", "docs/examples/hello.c",
           "build/libadelefeld.a", "-lflint", "-lgmp", "-lm", "-o", "build/hello"]
LANE = os.path.join(ROOT, "lanes", "docs-guide")
JULIA = ["julia/Adelefeld/bin/adelefeld-julia", "--startup-file=no", "--project=julia/Adelefeld"]


def run_adf(lines):
    data = "".join(line + "\n" for line in lines)
    p = subprocess.run([ADF], input=data.encode(), stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                       timeout=120)
    out = p.stdout.decode()
    return out.splitlines()


def transcript(inputs):
    """The rebuilt block for the input lines: '> ' + input, then its output lines."""
    result = []
    prev = []
    for i in range(1, len(inputs) + 1):
        out = run_adf(inputs[:i])
        if out[:len(prev)] != prev:
            raise RuntimeError("the output of a shorter prefix changed: %r" % inputs[:i])
        result.append("> " + inputs[i - 1])
        result.extend(out[len(prev):])
        prev = out
    return result


def fenced_blocks(lines):
    """Yield (start, end, info, body) for every fenced block; start is the index of the opening
    fence, end that of the closing fence."""
    i = 0
    while i < len(lines):
        m = re.match(r"^```(\S*)\s*$", lines[i])
        if m:
            j = i + 1
            while j < len(lines) and lines[j] != "```":
                j += 1
            yield i, j, m.group(1), lines[i + 1:j]
            i = j + 1
        else:
            i += 1


def marker_before(lines, start):
    """The check marker on the nearest non-blank line before a fence, or None."""
    k = start - 1
    while k >= 0 and lines[k].strip() == "":
        k -= 1
    if k >= 0:
        m = re.match(r"^<!-- check: (.+?) -->$", lines[k].strip())
        if m:
            return m.group(1)
    return None


def julia_env():
    env = dict(os.environ)
    env["JULIA_DEPOT_PATH"] = os.path.join(LANE, "depot") + ":" + os.path.expanduser("~/.julia") + ":"
    env["JULIA_PKG_OFFLINE"] = "true"
    env["JULIA_NUM_THREADS"] = "1"
    env["JULIA_NUM_PRECOMPILE_TASKS"] = "1"
    os.makedirs(os.path.join(LANE, "tmp"), exist_ok=True)
    env["TMPDIR"] = os.path.join(LANE, "tmp")
    return env


def main():
    mode = sys.argv[1] if len(sys.argv) > 1 else "check"
    with open(GUIDE) as fh:
        lines = fh.read().split("\n")

    blocks = list(fenced_blocks(lines))
    results = []          # (name, ok, detail)
    record = []           # (title, command, output lines)
    new_lines = list(lines)
    offset = 0
    julia_code = None
    session_no = 0
    hello_got = None

    for start, end, info, body in blocks:
        marker = marker_before(lines, start)
        if info == "adf":
            session_no += 1
            inputs = [b[2:] for b in body if b.startswith("> ")]
            got = transcript(inputs)
            name = "session %d (GUIDE.md:%d)" % (session_no, start + 1)
            ok = got == body
            detail = ""
            if not ok:
                for a, b in zip(got + [""] * len(body), body + [""] * len(got)):
                    if a != b:
                        detail = "first difference: guide %r, driver %r" % (b, a)
                        break
            results.append((name, ok, detail))
            record.append((name, "printf '%s\\n' " + " ".join(shellquote(x) for x in inputs)
                           + " | build/adf", got))
            if mode == "--fill":
                new_lines[start + 1 + offset:end + offset] = got
                offset += len(got) - len(body)
        elif marker is not None and marker.startswith("file "):
            path = os.path.join(ROOT, marker[5:])
            with open(path) as fh:
                want = fh.read().rstrip("\n").split("\n")
            ok = want == body
            results.append(("listing of %s (GUIDE.md:%d)" % (marker[5:], start + 1), ok,
                            "" if ok else "the listing differs from the file"))
            if mode == "--fill":
                new_lines[start + 1 + offset:end + offset] = want
                offset += len(want) - len(body)
        elif marker == "hello":
            c = subprocess.run(COMPILE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=300)
            if c.returncode != 0:
                results.append(("hello output (GUIDE.md:%d)" % (start + 1), False,
                                "compile failed: " + c.stdout.decode()))
                continue
            p = subprocess.run([HELLO_BIN], stdout=subprocess.PIPE, timeout=60)
            got = p.stdout.decode().rstrip("\n").split("\n")
            hello_got = got
            ok = got == body and p.returncode == 0
            results.append(("hello output (GUIDE.md:%d)" % (start + 1), ok,
                            "" if ok else "output or exit status differs"))
            record.append(("hello.c", " ".join(COMPILE) + " && ./build/hello", got))
            if mode == "--fill":
                new_lines[start + 1 + offset:end + offset] = got
                offset += len(got) - len(body)
        elif marker == "julia":
            julia_code = body
        elif marker == "julia-output":
            if julia_code is None:
                results.append(("julia output (GUIDE.md:%d)" % (start + 1), False, "no julia block"))
                continue
            with tempfile.NamedTemporaryFile("w", suffix=".jl", dir=os.path.join(LANE),
                                             delete=False) as tf:
                tf.write("\n".join(julia_code) + "\n")
                jl = tf.name
            try:
                p = subprocess.run(["timeout", "300"] + JULIA + [jl], stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, env=julia_env(), timeout=320)
            finally:
                os.unlink(jl)
            got = p.stdout.decode().rstrip("\n").split("\n")
            ok = got == body and p.returncode == 0
            results.append(("julia output (GUIDE.md:%d)" % (start + 1), ok,
                            "" if ok else "differs: %r (stderr %r)" % (got, p.stderr.decode()[-400:])))
            record.append(("julia", " ".join(JULIA) + " <the program of the guide, section 5>", got))
            if mode == "--fill":
                new_lines[start + 1 + offset:end + offset] = got
                offset += len(got) - len(body)
            julia_code = None

    # docs/examples/README.md shows the same output of hello under the same marker.
    with open(EXAMPLES_README) as fh:
        rlines = fh.read().split("\n")
    for start, end, info, body in fenced_blocks(rlines):
        if marker_before(rlines, start) == "hello" and hello_got is not None:
            ok = body == hello_got
            results.append(("hello output (docs/examples/README.md:%d)" % (start + 1), ok,
                            "" if ok else "differs from build/hello"))
            if mode == "--fill":
                rlines[start + 1:end] = hello_got
                with open(EXAMPLES_README, "w") as fh:
                    fh.write("\n".join(rlines))
            break

    if mode == "--fill":
        with open(GUIDE, "w") as fh:
            fh.write("\n".join(new_lines))
        print("filled %d sessions" % session_no)
        return 0
    if mode == "--record":
        out = sys.argv[2]
        with open(out, "w") as fh:
            fh.write(NUMBERS_HEAD)
            for title, cmd, got in record:
                fh.write("## %s\n\n    %s\n\n```text\n%s\n```\n\n" % (title, cmd, "\n".join(got)))
            a = subprocess.run([sys.executable, os.path.join(LANE, "arith_checks.py")],
                               stdout=subprocess.PIPE, timeout=60, check=True)
            fh.write("## The numbers derived in the prose\n\n    python3 lanes/docs-guide/arith_checks.py\n\n"
                     "```text\n%s```\n" % a.stdout.decode())
        print("recorded %d examples in %s" % (len(record), out))
        return 0

    bad = 0
    for name, ok, detail in results:
        print("%s %s%s" % ("same   " if ok else "DIFFERS", name, ("  " + detail) if detail else ""))
        bad += 0 if ok else 1
    n_sessions = sum(1 for r in results if r[0].startswith("session"))
    m_sessions = sum(1 for r in results if r[0].startswith("session") and not r[1])
    print("%d sessions, %d differ; %d other examples (listing, hello, julia), %d differ"
          % (n_sessions, m_sessions, len(results) - n_sessions, bad - m_sessions))
    return 1 if bad else 0


def shellquote(s):
    return "'" + s.replace("'", "'\\''") + "'"


NUMBERS_HEAD = """# numbers.md: every example of docs/GUIDE.md, as command and verbatim output

Written by `python3 lanes/docs-guide/check_examples.py --record lanes/docs-guide/numbers.md` from the
repository root of the worktree of lane docs-guide, on its own build (`make -j2 && make -C tools/adf`;
`sh tests/test_exports.sh` for `build/libadelefeld.so`, which the Julia example loads). Each session
below is the command that reproduces it and the transcript that the guide shows: a line `> ...` is an
input line, the lines after it are what `build/adf` printed for it. `sh lanes/docs-guide/check_examples.sh`
reruns all of them and diffs them against the guide.

"""

if __name__ == "__main__":
    sys.exit(main())
