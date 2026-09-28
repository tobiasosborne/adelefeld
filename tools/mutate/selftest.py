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

  5. test_concurrent_runs_share_scratch_safely() runs two instances of the tool at the same
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
# the real tree).
EQUIVALENT = """# the self-test of the mutation tool: two symmetric calls
# gcd(n, m) is symmetric, so exchanging the arguments of a call to adf_example_gcd computes the
# same value for every n, m >= 0. Two call sites, two lines.
src/example.c:43:swap_args | adf_example_gcd is symmetric: gcd(n, m) = gcd(m, n) for every
n, m >= 0, so exchanging the arguments of the call computes the same value.
src/example.c:55:swap_args | adf_example_gcd is symmetric: gcd(n, m) = gcd(m, n) for every
n, m >= 0, so exchanging the arguments of the call computes the same value.

# adf_example_box_add(out, a, b) computes a + b, the output argument (never one of the two
# exchanged, adf-obp) untouched; a + b = b + a for every long, so exchanging the two others
# computes the same value.
src/example.c:118:swap_args | adf_example_box_add is a commutative sum: exchanging its two
non-output arguments computes the same value.
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
