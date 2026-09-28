#!/usr/bin/env python3
"""tools/mutate/selftest.py: the self-test of tools/mutate/mutate.py.

The tool is run twice over tools/mutate/example/:

  1. with the deliberately weak test tests/test_weak.c. The tool must find at least one
     surviving mutant, and it must say so: a test that never checks the radius of a ball lets a
     mutant of the radius through.
  2. with the strong test tests/test_strong.c, which enumerates every small case of the claim.
     The tool must find no surviving mutant.

Both runs are made on a copy of the example under build/mutate/, so the tree is not written to.
The run fails if the weak test leaves no survivor, or if the strong test leaves one, or if the
tool itself crashes.

  3. after both runs no process of a mutant may be alive. A mutant that loops forever is
     stopped by the timeout of the tool, and the timeout must stop the test program and not
     only the shell or the make that started it (issue adf-98j: nine such programs ran for
     eight hours). Processes that are found are killed by the self-test, so a failing run
     leaves nothing behind either.

  4. test_generation_rules() below calls mutate.mutants_of() directly, on small snippets built
     for the purpose, and checks the exact mutants it returns -- no build, no run. This is
     where adf-obp's generation bugs are checked: a unary &, * or - must not be offered as the
     binary operator of the same character; && and || are single tokens, so a `logic` mutant is
     the only kind that ever touches them, and never as a lone `&` or `|`; -> is never split;
     gcd_lcm offers a rename only when the renamed name is declared somewhere the file could
     have found it; swap_args never exchanges the first argument of a 3-or-more-argument call
     (the FLINT-style output slot). It also has one case each of the four kinds adf-obp adds:
     status, drop_call, call_swap, prec.

  5. test_equivalent_key_format() checks the key of tools/mutate/equivalent.txt: it names the
     mutant by the text of the line the mutant changes and by what the change is, with an
     occurrence number when the same line text and the same change stand more than once in the
     file, and no line number (surface R3).  A key must therefore still match after a line has
     been added above the mutant, which is what a line number cannot do.

  6. test_report_of_compile_errors() runs the tool over a tree of its own (write_tiny_tree) in
     which one mutant does not compile, one loops for ever and one is killed by a test whose
     name ends in `error`.  A compile error is a diagnostic the compiler itself wrote, and only
     such a mutant is reported as not compiled (surface R5).

  7. test_keep_judges_as_the_normal_run() runs the same tree twice, with and without --keep, and
     requires the same counts and the same exit status from both: --keep keeps the scratch copies
     and changes nothing about the judgement (surface R2).  It also requires the kept copies to
     be there, which is what --keep is for.

  8. test_sigterm_leaves_nothing() stops a run with SIGTERM while a mutant's test program loops,
     and requires that the tool's scratch directory is gone and that no program of a mutant is
     left running (surface R6).

  9. test_san_mode() runs the tool with --san over the same tree and requires that the mutants
     were built and run with SAN=1, read from a file the tree's Makefile writes (issue adf-pf5):
     a mutant that only reads out of bounds, or only leaks, is then killed.

  10. test_concurrent_runs_share_scratch_safely() runs two instances of the tool at the same
     time, sharing one --scratch path (the ordinary case: --scratch defaults to the same
     build/mutate for every invocation). Neither may crash, and both must report the same
     counts and the same survivors, same seed and files (adf-obp: `make mutate FILES=src/
     recon.c JOBS=2`, run five times with the same seed, reported between two and five
     different survivors -- two runs racing on one scratch path via the not-atomic
     exists-then-rmtree-then-makedirs of copy_tree).

    python3 tools/mutate/selftest.py
    make mutate-selftest
"""

import importlib.util
import os
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EXAMPLE = os.path.join(ROOT, "tools", "mutate", "example")
SCRATCH = os.path.join(ROOT, "build", "mutate", "selftest")
MUTATE = os.path.join(ROOT, "tools", "mutate", "mutate.py")


# Mutants of the example that no test can kill because they compute the same thing as the
# original for every input, listed here so the self-test also shows that a listed survivor is
# excused and does not make the tool fail (the same mechanism as tools/mutate/equivalent.txt of
# the real tree).  The key is the text of the line the mutant changes, the kind, the change and
# the reason; no line number, because a line number does not survive an edit (surface R3).
EQUIVALENT = """# the self-test of the mutation tool: two symmetric calls
# gcd(n, m) is symmetric, so exchanging the arguments of a call to adf_example_gcd computes the
# same value for every n, m >= 0. Two call sites, two lines.
src/example.c | swap_args | return adf_example_gcd(n, m); | 'adf_example_gcd(n, m)' -> 'adf_example_gcd(m, n)' | adf_example_gcd is symmetric: gcd(n, m) = gcd(m, n) for every n, m >= 0, so the exchange computes the same value.
src/example.c | swap_args | long g = adf_example_gcd(n, m); | 'adf_example_gcd(n, m)' -> 'adf_example_gcd(m, n)' | adf_example_gcd is symmetric: gcd(n, m) = gcd(m, n) for every n, m >= 0, so the exchange computes the same value.

# adf_example_box_add(out, a, b) computes a + b, the output argument (never one of the two
# exchanged, adf-obp) untouched; a + b = b + a for every long, so exchanging the two others
# computes the same value.
src/example.c | swap_args | adf_example_box_add(&out, &a, &b); | 'adf_example_box_add(&out, &a, &b)' -> 'adf_example_box_add(&out, &b, &a)' | adf_example_box_add is a commutative sum: exchanging its two non-output arguments computes the same value.
"""


def prepare(name):
    """A copy of the example with tests/test_runner.h in it, ready to be mutated."""
    work = os.path.join(SCRATCH, name)
    if os.path.exists(work):
        shutil.rmtree(work)
    shutil.copytree(EXAMPLE, work, ignore=shutil.ignore_patterns("__pycache__", "build"))
    shutil.copy2(os.path.join(ROOT, "tests", "test_runner.h"),
                 os.path.join(work, "tests", "test_runner.h"))
    return work


def run_mutations(work, test, label):
    """Run the tool over the copy; return (survivors, killed, returncode, output)."""
    if test is not None:
        shutil.copy2(os.path.join(work, "tests", test), os.path.join(work, "tests", "test_weak.c"))
    command = [sys.executable, MUTATE,
               "--root", work,
               "--scratch", os.path.join(SCRATCH, "scratch-" + label),
               "--files", "src/example.c",
               "--limit", "200",
               "--timeout", "5",
               "--equivalent", os.path.join(SCRATCH, "equivalent-" + label + ".txt")]
    with open(os.path.join(SCRATCH, "equivalent-" + label + ".txt"), "w") as fh:
        fh.write(EQUIVALENT)
    proc = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output = proc.stdout.decode("utf-8", "replace")
    survivors = output.count("\nSURVIVED ")
    return survivors, output, proc.returncode


# ---------------------------------------------------------------- the tiny tree of items 6 to 9
#
# A tree of the self-test's own, written here rather than in tools/mutate/example/, because the
# example is a tree with one strong test and no mutant that fails to build on purpose.  What the
# four items need is a tree with a mutant that does not compile, one that loops for ever, one
# that a test whose name ends in `error` kills, and a Makefile that writes down the value of SAN
# so that the sanitizer mode can be read off a run.

TINY_C = """/* tiny.c: the tree of the self-tests of the mutation tool.
   `tiny_dist` gives a mutant that does not compile (`q - p` on two pointers becomes `q + p`),
   `tiny_count` gives a mutant that loops for ever (`n = n - 1` becomes `n = n + 1`), and
   `tiny_digit` is killed by a test whose name ends in `error` (surface R5). */
#include <stddef.h>

#include "tiny.h"

ptrdiff_t
tiny_dist(const char *p, const char *q)
{
    return q - p;
}

unsigned long
tiny_count(unsigned long n)
{
    while (n != 0)
        n = n - 1;
    return n;
}

int
tiny_digit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    return -1;
}
"""

TINY_H = """/* tiny.h: the three functions of src/tiny.c. */
#ifndef TINY_H
#define TINY_H

#include <stddef.h>

ptrdiff_t tiny_dist(const char *p, const char *q);
unsigned long tiny_count(unsigned long n);
int tiny_digit(char c);

#endif
"""

TINY_TEST = """/* tiny.c's test.  The name of the first test ends in `error` on purpose: a failing test is
   printed by tests/test_runner.h as `FAIL <file>:<line>: <name>: check failed: ...`, and the
   old compile-error test of the tool read the `error:` of that name as a build failure
   (surface R5).  The check of the digit '0' kills the mutant `c >= '0'` to `c > '0'`, which
   must be reported as killed and not as a mutant that does not build. */
#include <stdio.h>

#include "tiny.h"
#include "test_runner.h"

ADF_TEST(a_non_digit_is_a_parse_error)
{
    ADF_CHECK(tiny_digit('x') == -1);
    ADF_CHECK(tiny_digit('0') == 0);
    ADF_CHECK(tiny_digit('7') == 7);
}

ADF_TEST(the_distance_of_a_string)
{
    char text[8] = "abc";

    ADF_CHECK(tiny_dist(text, text + 2) == 2);
    ADF_CHECK(tiny_dist(text + 2, text) == -2);
    ADF_CHECK(tiny_count(3) == 0);
}
"""

TINY_MAKEFILE = """# tiny tree Makefile: the same interface as the repository's (a `check` target), plus one
# line that writes down the value of SAN so that the self-test can read the sanitizer mode of a
# run off the work directory.
CC     ?= cc
CFLAGS  = -std=c11 -O1 -g -Wall -Wextra -Werror
SAN    ?= 0
CFLAGS  += -DGOT_SAN=$(SAN)

check: build/test_tiny
\t@echo "SAN=$(SAN)" > san.txt
\t./build/test_tiny

build/test_tiny: src/tiny.c src/tiny.h tests/test_tiny.c
\tmkdir -p build
\t$(CC) -Isrc -Itests $(CFLAGS) src/tiny.c tests/test_tiny.c -o $@

clean:
\trm -rf build san.txt
"""


def write_tiny_tree(name):
    """Write the tree of the items 6 to 9 under build/mutate/selftest/<name> and return it."""
    work = os.path.join(SCRATCH, name)
    if os.path.exists(work):
        shutil.rmtree(work)
    os.makedirs(work)
    _write(work, "Makefile", TINY_MAKEFILE)
    _write(work, "src/tiny.c", TINY_C)
    _write(work, "src/tiny.h", TINY_H)
    _write(work, "tests/test_tiny.c", TINY_TEST)
    shutil.copy2(os.path.join(ROOT, "tests", "test_runner.h"),
                 os.path.join(work, "tests", "test_runner.h"))
    return work


def _run_tool(args, timeout=None):
    """Run mutate.py with the given arguments; return (exit code, output)."""
    command = [sys.executable, MUTATE] + args
    proc = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
    return proc.returncode, proc.stdout.decode("utf-8", "replace")


def _counts(output):
    """The five counts of the summary line of a run, as a dict."""
    m = re.search(r"mutate: (\d+) mutants in [\d.]+ s: (\d+) killed, (\d+) survived, "
                  r"(\d+) not compiled, (\d+) timed out, (\d+) excused", output)
    if not m:
        return None
    return {"total": int(m.group(1)), "killed": int(m.group(2)), "survived": int(m.group(3)),
            "not compiled": int(m.group(4)), "timed out": int(m.group(5)),
            "excused": int(m.group(6))}


def test_equivalent_key_format():
    """The key of an equivalent entry names the mutant by text, not by line number (surface R3).

    A key of tools/mutate/equivalent.txt is

        <file> | <kind> | <the text of the line the mutant changes, stripped> | <old> -> <new>
        | <reason>

    and it carries an occurrence number when the same line text and the same change stand more
    than once in the file.  The old key was (file, line, kind), which 45 of the 76 entries of
    equivalent.txt no longer matched at the commit the review read, and which excused a different
    statement in one case (src/fball.c:142:drop_call, which the review names).  Returns
    (ok, lines)."""
    mutate = _load_mutate()
    ok = [True]
    lines = []

    def check(name, cond, detail=""):
        lines.append(("ok   " if cond else "FAIL ") + name + ("" if cond else ": " + str(detail)))
        if not cond:
            ok[0] = False

    with open(os.path.join(EXAMPLE, "src", "example.c")) as fh:
        text = fh.read()
    ms = mutate.mutants_of("src/example.c", text, EXAMPLE)
    m = [x for x in ms if x.kind == "swap_args" and x.old == "adf_example_gcd(n, m)"]
    check("the example has the two adf_example_gcd swaps", len(m) == 2, [str(x) for x in m])

    # every mutant of a file carries the text of its line, and its key is built from it
    def key_of(mutant, body):
        return "%s | %s | %s | %s -> %s" % (mutant.path, mutant.kind, body, mutant.old,
                                            mutant.new)

    body = "return adf_example_gcd(n, m);"
    entry = ("src/example.c | swap_args | %s | 'adf_example_gcd(n, m)' -> "
             "'adf_example_gcd(m, n)' | the gcd is symmetric\n" % body)
    with tempfile.TemporaryDirectory() as tmp:
        path = os.path.join(tmp, "equivalent.txt")
        with open(path, "w") as fh:
            fh.write(entry)
        parsed = mutate.read_equivalent(path)
        check("an entry of the new form is read to the key of the mutant",
              parsed.get(key_of(m[0], body), "").startswith("the gcd is symmetric"), parsed)
        check("the entry excuses that mutant and not the other one",
              len(parsed) == 1 and key_of(m[1], "long g = adf_example_gcd(n, m);") not in parsed,
              parsed)

        # the old form is not read: FILE:LINE:KIND cannot be a key that names a mutant
        with open(path, "w") as fh:
            fh.write("src/example.c:43:swap_args | the old form\n")
        check("an entry of the old line-number form excuses nothing",
              mutate.read_equivalent(path) == {}, mutate.read_equivalent(path))

    # a line number must not be part of a key: the same entry still matches after a line has
    # been added above the mutant (this is the whole of R3)
    shifted = "# one more line\n" + text
    ms2 = mutate.mutants_of("src/example.c", shifted, EXAMPLE)
    m2 = [x for x in ms2 if x.kind == "swap_args" and x.old == "adf_example_gcd(n, m)"
          and x.line == m[0].line + 1]
    check("the same mutant is one line further down after an edit above it", len(m2) == 1,
          [str(x) for x in ms2 if x.old == "adf_example_gcd(n, m)"])
    if m2:
        check("its key does not mention the line number",
              key_of(m2[0], body) == key_of(m[0], body),
              (key_of(m2[0], body), key_of(m[0], body)))

    # the same line text and the same change twice in one file: the occurrence number tells them
    # apart, and an entry without it is ambiguous and matches neither
    twice = ("long g(long a, long b)\n"
             "{\n"
             "    return my_mul(a, b) + my_mul(a, b);\n"
             "}\n")
    ms3 = mutate.mutants_of("t.c", twice, ".")
    swaps = [x for x in ms3 if x.kind == "swap_args"]
    check("two identical statements give two mutants with the same text", len(swaps) == 2,
          [str(x) for x in ms3])
    if len(swaps) == 2:
        numbered = sorted(x.key for x in swaps)
        check("the two of them are told apart by an occurrence number",
              len(set(numbered)) == 2 and all(re.search(r"#[12]$", k) for k in numbered),
              numbered)
        body = "return my_mul(a, b) + my_mul(a, b);"
        one = ("t.c | swap_args | %s | 'my_mul(a, b)' -> 'my_mul(b, a)' #2 | the second one\n"
               % body)
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "equivalent.txt")
            with open(path, "w") as fh:
                fh.write(one)
            parsed = mutate.read_equivalent(path)
            second = [x for x in swaps if x.key.endswith("#2")]
            check("an entry with an occurrence number names that one mutant",
                  len(second) == 1 and parsed.get(second[0].key, "").startswith("the second"),
                  (sorted(parsed), [x.key for x in swaps]))
    return ok[0], lines


def test_report_of_compile_errors():
    """Only a diagnostic the compiler wrote is a compile error (surface R5).

    The tree of write_tiny_tree() has one mutant that does not compile (`q - p` to `q + p`) and
    one that is killed by a test whose name ends in `error`, printed by tests/test_runner.h as
    `FAIL <file>:<line>: a_non_digit_is_a_parse_error: check failed: ...`.  The tool must report
    one mutant as not compiled and the other as killed.  Returns (ok, lines)."""
    work = write_tiny_tree("compile-errors")
    scratch = os.path.join(SCRATCH, "scratch-compile-errors")
    shutil.rmtree(scratch, ignore_errors=True)
    code, output = _run_tool(["--root", work, "--scratch", scratch, "--files", "src/tiny.c",
                              "--limit", "200", "--timeout", "3", "--equivalent", "/dev/null"])
    ok = True
    lines = []
    counts = _counts(output)
    lines.append(("ok   " if counts is not None else "FAIL ") +
                 "the run reports its counts" + ("" if counts else ": " + output[-500:]))
    if counts is None:
        return False, lines
    ok = counts["not compiled"] == 1
    lines.append(("ok   " if ok else "FAIL ") +
                 "exactly one mutant is reported as not compiled (the one that does not build)"
                 + ("" if ok else ": %d" % counts["not compiled"]))
    detail = re.search(r"NOT COMPILED [^\n]*\n  (.*)", output)
    ok2 = detail is not None and "invalid operands" in detail.group(1)
    lines.append(("ok   " if ok2 else "FAIL ") +
                 "the report of it is the compiler's own diagnostic"
                 + ("" if ok2 else ": %s" % (detail.group(1) if detail else "none")))
    ok3 = counts["killed"] >= 1 and counts["timed out"] >= 1
    lines.append(("ok   " if ok3 else "FAIL ") +
                 "the mutant that loops is timed out and the killed ones are killed"
                 + ("" if ok3 else ": %s" % counts))
    ok = ok and ok2 and ok3
    lines.append(("ok   " if code in (0, 1) else "FAIL ") + "the run does not crash"
                 + ("" if code in (0, 1) else ": exit %d" % code))
    ok = ok and code in (0, 1)
    shutil.rmtree(work, ignore_errors=True)
    shutil.rmtree(scratch, ignore_errors=True)
    return ok, lines


def test_keep_judges_as_the_normal_run():
    """--keep keeps the scratch copies; it does not change the judgement (surface R2).

    With --keep the tool used to report every mutant that does not build or does not finish in
    the time given as killed, and the review's reproducer showed 12 killed where the ordinary
    run reported 6 killed, 4 not compiled and 2 timed out.  Both runs here must give the same
    counts and the same exit status, and --keep must leave one copy per mutant behind.
    Returns (ok, lines)."""
    work = write_tiny_tree("keep")
    ok = True
    lines = []
    results = {}
    for label, extra in (("plain", []), ("keep", ["--keep"])):
        scratch = os.path.join(SCRATCH, "scratch-keep-" + label)
        shutil.rmtree(scratch, ignore_errors=True)
        code, output = _run_tool(["--root", work, "--scratch", scratch, "--files", "src/tiny.c",
                                  "--limit", "200", "--timeout", "3", "--equivalent", "/dev/null"]
                                 + extra)
        results[label] = (code, _counts(output), output, scratch)
    plain, keep = results["plain"], results["keep"]
    same = plain[1] is not None and plain[1] == keep[1] and plain[0] == keep[0]
    lines.append(("ok   " if same else "FAIL ") + "--keep gives the same counts and the same "
                 "exit status as the ordinary run"
                 + ("" if same else ": %s vs %s, exit %d vs %d" %
                    (plain[1], keep[1], plain[0], keep[0])))
    ok = ok and same
    if plain[1] is not None and keep[1] is not None:
        judged = plain[1]["not compiled"] >= 1 and plain[1]["timed out"] >= 1
        lines.append(("ok   " if judged else "FAIL ") + "the ordinary run judges a mutant that "
                     "does not build and one that loops" + ("" if judged else ": %s" % (plain[1],)))
        ok = ok and judged
    survivors_plain = set(re.findall(r"\nSURVIVED ([^\n]+)", plain[2]))
    survivors_keep = set(re.findall(r"\nSURVIVED ([^\n]+)", keep[2]))
    same_s = survivors_plain == survivors_keep
    lines.append(("ok   " if same_s else "FAIL ") + "--keep reports the same survivors"
                 + ("" if same_s else ": %s vs %s" % (sorted(survivors_plain),
                                                      sorted(survivors_keep))))
    ok = ok and same_s
    kept = []
    for root_dir, dirs, _files in os.walk(keep[3]):
        if os.path.basename(root_dir) == "keep":
            kept.extend(dirs)
    there = len(kept) == (plain[1]["total"] if plain[1] else -1)
    lines.append(("ok   " if there else "FAIL ") + "--keep leaves one copy per mutant behind (%d)"
                 % len(kept) + ("" if there else ", expected %s" %
                                (plain[1]["total"] if plain[1] else "?")))
    ok = ok and there
    shutil.rmtree(work, ignore_errors=True)
    shutil.rmtree(plain[3], ignore_errors=True)
    shutil.rmtree(keep[3], ignore_errors=True)
    return ok, lines


def test_sigterm_leaves_nothing():
    """SIGTERM (what `timeout`, a CI runner and `kill` send) stops the tool cleanly (surface R6).

    The tool removes its scratch directory in a try/finally that a signal does not run, and the
    mutants' test programs are in a session of their own, so they were left running.  After the
    SIGTERM the scratch directory must be gone and no program of a mutant may be left.
    Returns (ok, lines)."""
    work = write_tiny_tree("sigterm")
    scratch = os.path.join(SCRATCH, "scratch-sigterm")
    shutil.rmtree(scratch, ignore_errors=True)
    command = [sys.executable, MUTATE, "--root", work, "--scratch", scratch,
               "--files", "src/tiny.c", "--limit", "200", "--timeout", "600", "--jobs", "2",
               "--equivalent", "/dev/null"]
    proc = subprocess.Popen(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    # wait until a mutant is really being built, so that the SIGTERM arrives during a run and
    # not before it: otherwise the test would pass on a tool that never started
    started = False
    for _ in range(120):
        if proc.poll() is not None:
            break
        for d, _dirs, _files in os.walk(scratch):
            if os.path.basename(os.path.dirname(d)).startswith("w"):
                started = True
                break
        if started:
            break
        time.sleep(0.25)
    time.sleep(2.0)
    proc.send_signal(signal.SIGTERM)
    proc.wait(timeout=30)
    time.sleep(1.0)
    ok = True
    lines = []
    lines.append(("ok   " if started else "FAIL ") + "a mutant was being built when the signal "
                 "was sent" + ("" if started else ""))
    ok = ok and started
    stopped = proc.returncode != 0
    lines.append(("ok   " if stopped else "FAIL ") + "the tool stops with a non-zero status"
                 + ("" if stopped else ": exit %d" % proc.returncode))
    ok = ok and stopped
    left = [os.path.join(d, f) for d, _dirs, files in os.walk(scratch) for f in files]
    gone = not left
    lines.append(("ok   " if gone else "FAIL ") + "the scratch directory is removed after SIGTERM"
                 + ("" if gone else ": %d file(s) left" % len(left)))
    ok = ok and gone
    alive = processes_under(scratch)
    for pid, path in alive:
        try:
            os.kill(pid, signal.SIGKILL)
        except OSError:
            pass
    clean = not alive
    lines.append(("ok   " if clean else "FAIL ") + "no program of a mutant is left running"
                 + ("" if clean else ": %s" % alive))
    ok = ok and clean
    shutil.rmtree(work, ignore_errors=True)
    shutil.rmtree(scratch, ignore_errors=True)
    return ok, lines


def test_san_mode():
    """--san builds and runs the mutants with SAN=1 (issue adf-pf5).

    The Makefile of the tiny tree writes `SAN=$(SAN)` into san.txt of the work directory, so a
    kept run shows what the mutants were built with.  A run without --san must write SAN=0 and a
    run with --san must write SAN=1.  Returns (ok, lines)."""
    work = write_tiny_tree("san")
    ok = True
    lines = []
    for label, extra, want in (("plain", [], "0"), ("san", ["--san"], "1")):
        scratch = os.path.join(SCRATCH, "scratch-san-" + label)
        shutil.rmtree(scratch, ignore_errors=True)
        code, output = _run_tool(["--root", work, "--scratch", scratch, "--files", "src/tiny.c",
                                  "--limit", "2", "--timeout", "30", "--keep",
                                  "--equivalent", "/dev/null"] + extra)
        got = []
        for d, _dirs, files in os.walk(scratch):
            if "san.txt" in files:
                with open(os.path.join(d, "san.txt")) as fh:
                    got.append(fh.read().strip())
        good = bool(got) and all(g == "SAN=" + want for g in got)
        lines.append(("ok   " if good else "FAIL ") +
                     ("a run with --san builds with SAN=1" if want == "1"
                      else "a run without --san builds with SAN=0")
                     + ("" if good else ": ran %d, saw %s, exit %d" % (len(got), sorted(set(got)),
                                                                       code)))
        ok = ok and good
        shutil.rmtree(scratch, ignore_errors=True)
    shutil.rmtree(work, ignore_errors=True)
    return ok, lines


def test_concurrent_runs_share_scratch_safely():
    """Two invocations of mutate.py, the same seed and files, started at the same time and
    sharing one --scratch path -- the ordinary case, since --scratch defaults to the same
    build/mutate for every invocation of the tool, in or out of make. Before the fix (adf-obp),
    copy_tree's exists-then-rmtree-then-makedirs was not atomic, so two runs racing on the same
    numbered workdir could crash outright or interleave one mutant's files into the other's
    build; reported effect: `make mutate FILES=src/recon.c JOBS=2`, run five times with the
    same seed, reported between two and five different survivors. Neither run here may crash,
    and the two must report the same counts and the same set of survivors: the seed fixes which
    mutants run, and nothing about running next to another instance may change how any one of
    them is judged. Returns (ok, lines)."""
    work = prepare("concurrent")
    shared = os.path.join(SCRATCH, "concurrent-shared")
    if os.path.exists(shared):
        shutil.rmtree(shared)
    equiv = os.path.join(SCRATCH, "concurrent-equivalent.txt")
    with open(equiv, "w") as fh:
        fh.write(EQUIVALENT)
    command = [sys.executable, MUTATE, "--root", work, "--scratch", shared,
              "--files", "src/example.c", "--limit", "16", "--seed", "7", "--jobs", "2",
              "--timeout", "10", "--equivalent", equiv]
    procs = [subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            for _ in range(2)]
    outputs = [p.communicate()[0].decode("utf-8", "replace") for p in procs]
    codes = [p.returncode for p in procs]

    ok = True
    lines = []

    def check(name, cond, detail=""):
        nonlocal ok
        lines.append(("ok   " if cond else "FAIL ") + name + ("" if cond else ": " + str(detail)))
        if not cond:
            ok = False

    check("neither concurrent run crashes",
          all(c in (0, 1) for c in codes),
          list(zip(codes, [o[-300:] for o in outputs])))
    summaries = [re.search(r"mutate: \d+ mutants in [\d.]+ s: .*excused", o) for o in outputs]
    summaries = [m.group(0) if m else None for m in summaries]
    # the elapsed seconds in the summary line differ run to run; compare with that stripped out
    stripped = [re.sub(r"in [\d.]+ s", "in N s", s) if s else s for s in summaries]
    check("both runs report the same counts", None not in stripped and stripped[0] == stripped[1],
          stripped)
    survived = [set(re.findall(r"\nSURVIVED ([^\n]+)", o)) for o in outputs]
    check("both runs agree on which mutants survive", survived[0] == survived[1], survived)

    time.sleep(0.2)
    left = processes_under(shared)
    for pid, path in left:
        lines.append("FAIL   process %d of a mutant is still running: %s" % (pid, path))
        try:
            os.kill(pid, signal.SIGKILL)
        except OSError:
            pass
        ok = False

    shutil.rmtree(shared, ignore_errors=True)
    return ok, lines


def processes_under(directory):
    """The processes whose program or working directory lies under the directory: (pid, path).

    Read from /proc. The program of a mutant whose directory is already removed is still
    found: its link reads '<path> (deleted)'."""
    found = []
    prefix = os.path.realpath(directory) + os.sep
    for name in os.listdir("/proc"):
        if not name.isdigit() or int(name) == os.getpid():
            continue
        for link in ("exe", "cwd"):
            try:
                path = os.readlink(os.path.join("/proc", name, link))
            except OSError:
                continue
            if path.startswith(prefix):
                found.append((int(name), path))
                break
    return found


def _load_mutate():
    """tools/mutate/mutate.py as a module, so its mutants_of() can be called directly."""
    spec = importlib.util.spec_from_file_location("mutate", MUTATE)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _write(root, rel, text):
    path = os.path.join(root, rel)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as fh:
        fh.write(text)
    return path


def test_generation_rules():
    """Direct checks of mutate.mutants_of() against snippets built for the purpose (module
    docstring, item 4): no build, no run, so this is fast and exact about which mutant is or is
    not offered. Returns (ok, lines) -- lines is one "ok"/"FAIL" line per check, printed by
    main() the same way the weak/strong runs are."""
    mutate = _load_mutate()
    ok = [True]
    lines = []

    def check(name, cond, detail=""):
        lines.append(("ok   " if cond else "FAIL ") + name + ("" if cond else ": " + str(detail)))
        if not cond:
            ok[0] = False

    def mutants(text, root="."):
        return mutate.mutants_of("t.c", text, root)

    def has(muts, kind, old=None, new=None):
        return any(m.kind == kind and (old is None or m.old == old) and
                  (new is None or m.new == new) for m in muts)

    # A unary & is not offered as the binary & of `op` (adf-obp: &x->fin -> |x->fin).
    m = mutants("void f(void) { g(&x->fin); }\n")
    check("a unary & is not an op mutant", not has(m, "op", old="&"), [str(x) for x in m])
    m = mutants("long f(long a, long b) { return a & b; }\n")
    check("a binary & is still an op mutant", has(m, "op", old="&", new="|"))

    # A unary * is not offered as the binary * of `op`, whether a dereference or a pointer
    # declarator's * (adf-obp: `adf_example_box * out` -> `adf_example_box / out`).
    m = mutants("void f(long *p) { *p = 0; }\n")
    check("a dereference/declarator * is not an op mutant", not has(m, "op", old="*"),
          [str(x) for x in m])
    m = mutants("long f(long a, long b) { return a * b; }\n")
    check("a binary * is still an op mutant", has(m, "op", old="*"))

    # A unary - is not offered as the binary - of `op`, including right after `return` (adf-obp:
    # ends_expression must not read the keyword `return` as a value that a `-` subtracts from).
    m = mutants("long f(long x) { return -x; }\n")
    check("a unary - after return is not an op mutant", not has(m, "op", old="-"),
          [str(x) for x in m])
    m = mutants("long f(long a, long b) { return a - b; }\n")
    check("a binary - is still an op mutant", has(m, "op", old="-", new="+"))

    # && and || are one token each: `logic` is the only kind that touches them, and never as a
    # lone & or | (the tracker's claim, not reproduced, but locked in here).
    m = mutants("int f(int a, int b) { return a && b; }\n")
    check("&& is mutated only by logic, to ||",
          has(m, "logic", old="&&", new="||") and not any(mm.old == "&" for mm in m))
    m = mutants("int f(int a, int b) { return a || b; }\n")
    check("|| is mutated only by logic, to &&",
          has(m, "logic", old="||", new="&&") and not any(mm.old == "|" for mm in m))

    # -> is never split into a mutable -.
    m = mutants("void f(struct s *x) { g(x->fin); }\n")
    check("-> is never split into a - mutant", not any(mm.old == "-" for mm in m),
          [str(x) for x in m])

    # gcd_lcm renames only when the renamed name is declared where this file could find it.
    with tempfile.TemporaryDirectory() as tmp:
        _write(tmp, "src/foo.h", "void my_lcm(int a, int b);\n")
        text = '#include "foo.h"\nvoid f(void) { my_gcd(1, 2); }\n'
        _write(tmp, "src/t.c", text)
        m = mutate.mutants_of("src/t.c", text, tmp)
        check("gcd_lcm renames my_gcd to my_lcm when my_lcm is declared",
              has(m, "gcd_lcm", old="my_gcd", new="my_lcm"), [str(x) for x in m])
    with tempfile.TemporaryDirectory() as tmp:
        text = "void f(void) { my_gcd(1, 2); }\n"
        _write(tmp, "src/t.c", text)
        m = mutate.mutants_of("src/t.c", text, tmp)
        check("gcd_lcm drops the rename when nothing declares my_lcm",
              not has(m, "gcd_lcm"), [str(x) for x in m])

    # swap_args never exchanges the first argument of a 3-or-more-argument call (the
    # FLINT-style output slot; adf-obp: adf_fball_add(z, x, y) -> adf_fball_add(x, y, z)
    # discarded z's const-ness by writing into what may be a const input).
    m = mutants("void f(void) { my_add(z, x, y); }\n")
    check("swap_args of a 3-argument call keeps the first argument fixed",
          has(m, "swap_args", old="my_add(z, x, y)", new="my_add(z, y, x)"),
          [str(x) for x in m if x.kind == "swap_args"])
    m = mutants("long f(long a, long b) { return my_add(a, b); }\n")
    check("swap_args of a 2-argument call exchanges both",
          has(m, "swap_args", old="my_add(a, b)", new="my_add(b, a)"),
          [str(x) for x in m if x.kind == "swap_args"])

    # status: a bare `return ADF_X;` of a name status.h declares.
    with tempfile.TemporaryDirectory() as tmp:
        _write(tmp, "include/adelefeld/status.h",
              "#define ADF_OK 0\n#define ADF_DOMAIN 1\n#define ADF_UNSUPPORTED 2\n")
        text = "int f(long n) { if (n == 0) return ADF_UNSUPPORTED; return ADF_OK; }\n"
        _write(tmp, "src/t.c", text)
        m = mutate.mutants_of("src/t.c", text, tmp)
        check("status turns ADF_UNSUPPORTED into ADF_OK",
              has(m, "status", old="ADF_UNSUPPORTED", new="ADF_OK"), [str(x) for x in m])
        check("status turns ADF_OK into ADF_DOMAIN",
              has(m, "status", old="ADF_OK", new="ADF_DOMAIN"), [str(x) for x in m])

    # drop_call: a bare `name(...);` statement, except a name ending in _init or _clear.
    m = mutants("void f(void) { helper(); helper_init(); helper_clear(); }\n")
    check("drop_call removes a bare call statement",
          has(m, "drop_call", old="helper();"), [str(x) for x in m])
    check("drop_call excludes _init and _clear",
          not any(mm.kind == "drop_call" and mm.old in ("helper_init();", "helper_clear();")
                  for mm in m), [str(x) for x in m])

    # call_swap: X_add -> X_sub (and the reverse), X_mul -> X_add, kept only when declared.
    with tempfile.TemporaryDirectory() as tmp:
        _write(tmp, "src/foo.h", "void my_sub(int a, int b);\n")
        text = '#include "foo.h"\nvoid f(void) { my_add(1, 2); }\n'
        _write(tmp, "src/t.c", text)
        m = mutate.mutants_of("src/t.c", text, tmp)
        check("call_swap renames my_add to my_sub when my_sub is declared",
              has(m, "call_swap", old="my_add", new="my_sub"), [str(x) for x in m])
    with tempfile.TemporaryDirectory() as tmp:
        text = "void f(void) { my_add(1, 2); }\n"
        _write(tmp, "src/t.c", text)
        m = mutate.mutants_of("src/t.c", text, tmp)
        check("call_swap drops the rename when nothing declares my_sub",
              not has(m, "call_swap"), [str(x) for x in m])

    # prec: a bare `prec` argument becomes 2; the parameter declaration itself is untouched.
    m = mutants("void f(long prec) { g(x, prec); }\n")
    check("prec replaces the argument with 2",
          has(m, "prec", old="prec", new="2"), [str(x) for x in m])
    check("prec leaves the parameter declaration alone",
          len([mm for mm in m if mm.kind == "prec"]) == 1, [str(x) for x in m])

    return ok[0], lines


def main():
    os.makedirs(SCRATCH, exist_ok=True)

    print("== the generation rules (mutants_of() directly, no build)")
    gen_ok, gen_lines = test_generation_rules()
    for line in gen_lines:
        print(line)
    ok = gen_ok
    if not gen_ok:
        print("selftest: FAILED: at least one generation rule of item 4 does not hold")

    for title, fn in (("the key of an equivalent entry (item 5)", test_equivalent_key_format),
                      ("the report of a compile error (item 6)", test_report_of_compile_errors),
                      ("--keep judges as the ordinary run (item 7)", test_keep_judges_as_the_normal_run),
                      ("SIGTERM leaves nothing behind (item 8)", test_sigterm_leaves_nothing),
                      ("--san builds with SAN=1 (item 9)", test_san_mode)):
        print("== " + title)
        try:
            sub_ok, sub_lines = fn()
        except Exception as exc:  # a crash in a self-test is a failure of that self-test
            sub_ok, sub_lines = False, ["FAIL   the self-test raised %s: %s"
                                        % (type(exc).__name__, exc)]
        for line in sub_lines:
            print(line)
        if not sub_ok:
            print("selftest: FAILED: " + title)
            ok = False

    print("== two concurrent runs sharing one --scratch path")

    conc_ok, conc_lines = test_concurrent_runs_share_scratch_safely()
    for line in conc_lines:
        print(line)
    if not conc_ok:
        print("selftest: FAILED: two concurrent runs of the tool do not agree, or one of them "
              "crashed")
        ok = False

    weak = prepare("weak")
    survivors, output, code = run_mutations(weak, None, "weak")
    print("== the weak test")
    print(output.rstrip())
    if code == 0:
        print("selftest: FAILED: the tool passed the example with the weak test, so it does not "
              "find a surviving mutant")
        ok = False
    if survivors == 0:
        print("selftest: FAILED: the weak test left no surviving mutant, so the example does not "
              "show what the tool is for")
        ok = False

    strong = prepare("strong")
    survivors, output, code = run_mutations(strong, "test_strong.c", "strong")
    print("== the strong test")
    print(output.rstrip())
    if code != 0:
        print("selftest: FAILED: the tool reports a surviving mutant for the strong test")
        ok = False
    if survivors != 0:
        print("selftest: FAILED: %d mutant(s) survive the strong test" % survivors)
        ok = False
    if not re.search(r"\b[1-9]\d* excused\b", output):
        print("selftest: FAILED: none of the equivalent mutants of EQUIVALENT is excused, so "
              "the equivalent file of the run was not read")
        ok = False
    if "TIMED OUT" not in output:
        print("selftest: FAILED: no mutant timed out, so the timeout of the tool is not exercised")
        ok = False

    time.sleep(0.5)
    left = processes_under(SCRATCH)
    for pid, path in left:
        print("selftest: FAILED: process %d of a mutant is still running: %s" % (pid, path))
        try:
            os.kill(pid, signal.SIGKILL)
        except OSError:
            pass
        ok = False

    if ok:
        print("selftest: passed: the weak test leaves a survivor, the strong test leaves none, "
              "and no process of a mutant is left")
        return 0
    return 1


if __name__ == "__main__":
    sys.exit(main())
