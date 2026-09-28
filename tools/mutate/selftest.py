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

    python3 tools/mutate/selftest.py
    make mutate-selftest
"""

import os
import shutil
import signal
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EXAMPLE = os.path.join(ROOT, "tools", "mutate", "example")
SCRATCH = os.path.join(ROOT, "build", "mutate", "selftest")
MUTATE = os.path.join(ROOT, "tools", "mutate", "mutate.py")


# The one mutant of the example that no test can kill: the greatest common divisor is
# symmetric, so exchanging the two arguments of the call in adf_example_sum_radius computes the
# same value for every input. It is listed in an equivalent file of the run, so the self-test
# also shows that a listed survivor is excused and does not make the tool fail.
EQUIVALENT = """# the self-test of the mutation tool: the symmetric call
src/example.c:43:swap_args | adf_example_gcd is symmetric: gcd(n, m) = gcd(m, n) for every
n, m >= 0, so exchanging the arguments of the call computes the same value.
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


def main():
    os.makedirs(SCRATCH, exist_ok=True)

    weak = prepare("weak")
    survivors, output, code = run_mutations(weak, None, "weak")
    print("== the weak test")
    print(output.rstrip())
    ok = True
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
    if "1 excused" not in output:
        print("selftest: FAILED: the symmetric call of the example is not excused, so the "
              "equivalent file of the run was not read")
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
