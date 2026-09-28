#!/usr/bin/env python3
"""tools/memcheck/selftest.py: the self-test of tools/memcheck/check_uninit.py and run.sh.

    python3 tools/memcheck/selftest.py [--no-valgrind]

The review of the surface (docs/reviews/m1/surface/review.md) found that the static checker missed a
use that stands before the init in straight-line code (R1), and that the README did not say that the
checker is blind to branches, array elements and `goto` (R4). The snippets in tools/memcheck/snippets/
are the reproducers of the review, each one a small program with the defect in it:

  s0_control.c              no init at all: the one shape the checker always caught
  s1_use_then_init.c        fmpz_add_ui(a, a, k); fmpz_init(a);            R1, must be reported
  s4_adf_use_then_init.c    adf_fball_add(x, x, y); adf_fball_init(x);     R1, must be reported
  s2_init_on_one_branch.c   if (flag) fmpq_init(q); fmpq_set_si(q, 7, 3);  R4, a limit of the tool
  s3_array_element.c        fmpz_init(v[0]); fmpz_add(v[0], v[0], v[1]);   R4, a limit of the tool
  s5_goto_skips_init.c      goto over arb_init(x), then arb_clear(x)       R4, a limit of the tool

The test checks, without a build:

  1. s0, s1 and s4 give the findings they must (a use before the init, at the line of the use);
  2. a correct init-then-use-then-clear function gives none (no false positive);
  3. s2, s3 and s5 give none: this pins what the tool does, so that the limits named in the README
     are the limits of the code, and a change that makes the tool see one of them has to change the
     README and this test together;
  4. the tree, `src/*.c tests/*.c tools/adf/adf.c`, gives no finding (the reviewer's one-method variant
     found none either: the tree has no defect of the shape of s1 today).

and with valgrind (the one at ~/.local/bin/valgrind or on PATH; the test says so and skips this part
when there is none):

  5. run.sh finds that valgrind (`run.sh --valgrind-path`), which the README used to deny;
  6. every snippet that builds against FLINT alone, s0, s1, s2, s3 and s5, is a real defect: valgrind
     reports an error for it. That is what makes s2, s3 and s5 limits of the checker and not clean code.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
CHECK = os.path.join(HERE, "check_uninit.py")
SNIPPETS = os.path.join(HERE, "snippets")
RUN_SH = os.path.join(HERE, "run.sh")

failed = []


def check(name, cond, detail=""):
    print(("ok   " if cond else "FAIL ") + name + ("" if cond else ": %s" % (detail,)))
    if not cond:
        failed.append(name)


def checker(*paths, category=None):
    cmd = [sys.executable, CHECK] + list(paths)
    if category:
        cmd[2:2] = ["--category", category]
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          universal_newlines=True, cwd=ROOT)
    lines = [l for l in proc.stdout.splitlines() if l.strip()]
    return proc.returncode, lines


def line_of(path, needle):
    with open(path) as fh:
        for number, text in enumerate(fh, start=1):
            if needle in text and not text.lstrip().startswith(("/*", "*")):
                return number
    raise SystemExit("%s does not hold %r" % (path, needle))


def find_valgrind():
    home = os.path.expanduser("~/.local/bin/valgrind")
    return shutil.which("valgrind") or (home if os.access(home, os.X_OK) else None)


def main():
    s = lambda name: os.path.join(SNIPPETS, name)          # noqa: E731

    print("== a use before the first init in text order is reported (surface R1)")
    code, lines = checker(s("s0_control.c"), category="use-before-init")
    check("s0 (no init at all): one use-before-init", len(lines) == 1, lines)
    code, lines = checker(s("s1_use_then_init.c"), category="use-before-init")
    want = line_of(s("s1_use_then_init.c"), "fmpz_add_ui(a, a, (ulong) k);")
    check("s1 (fmpz_add_ui before fmpz_init): one use-before-init, at the line of the use",
          len(lines) == 1 and (":%d:" % want) in lines[0] and " a: " in lines[0], (want, lines))
    check("s1: the exit status is 1", checker(s("s1_use_then_init.c"))[0] == 1)
    code, lines = checker(s("s4_adf_use_then_init.c"), category="use-before-init")
    want = line_of(s("s4_adf_use_then_init.c"), "adf_fball_add(x, x, y);")
    check("s4 (adf_fball_add before adf_fball_init): one use-before-init, at the line of the use",
          len(lines) == 1 and (":%d:" % want) in lines[0] and " x: " in lines[0], (want, lines))

    print("== no false positive on correct code")
    with tempfile.TemporaryDirectory() as tmp:
        ok_c = os.path.join(tmp, "ok.c")
        with open(ok_c, "w") as fh:
            fh.write("""#include <flint/fmpz.h>
static int f(int k)
{
    fmpz_t a, b;
    int r;

    fmpz_init(a);
    fmpz_init(b);
    fmpz_set_ui(a, (ulong) k);
    fmpz_add(b, a, a);
    r = (int) fmpz_get_si(b);
    fmpz_clear(a);
    fmpz_clear(b);
    return r;
}
static int g(int k)
{
    fmpz_t a;

    fmpz_init(a);
    if (k > 0)
    {
        fmpz_t t;
        fmpz_init(t);
        fmpz_add(a, a, t);
        fmpz_clear(t);
    }
    fmpz_clear(a);
    return k;
}
static int h(void)
{
    fmpz_t a;

    memset(a, 0, sizeof(a));       /* bytes defined for memcmp, the tests do this before the init */
    fmpz_init(a);
    fmpz_clear(a);
    return 0;
}
""")
        code, lines = checker(ok_c)
        check("init, use, clear in order (also memset before the init): no finding, exit 0",
              code == 0 and not lines, (code, lines))
        bad_c = os.path.join(tmp, "bad.c")
        with open(bad_c, "w") as fh:
            fh.write("""#include <flint/fmpz.h>
static int h(void)
{
    fmpz_t a;

    memset(a, 0, sizeof(a));       /* a memset is not an init */
    fmpz_add_ui(a, a, 1);
    fmpz_clear(a);
    return 0;
}
""")
        code, lines = checker(bad_c, category="use-before-init")
        check("a memset is not an init: the use after it is reported", code == 1 and len(lines) == 1,
              (code, lines))

    print("== the limits named in tools/memcheck/README.md are the limits of the code (surface R4)")
    for name, what in (("s2_init_on_one_branch.c", "an init on one branch only"),
                       ("s3_array_element.c", "an array element"),
                       ("s5_goto_skips_init.c", "a goto over the init")):
        code, lines = checker(s(name))
        check("%s (%s): no finding, as the README says" % (name, what), code == 0 and not lines,
              (code, lines))

    print("== the tree has no finding")
    trees = ["src/" + n for n in sorted(os.listdir(os.path.join(ROOT, "src"))) if n.endswith(".c")]
    trees += ["tests/" + n for n in sorted(os.listdir(os.path.join(ROOT, "tests"))) if n.endswith(".c")]
    if os.path.exists(os.path.join(ROOT, "tools", "adf", "adf.c")):
        trees.append("tools/adf/adf.c")
    code, lines = checker(*trees)
    check("src/*.c tests/*.c tools/adf/adf.c (%d files): no finding" % len(trees),
          code == 0 and not lines, lines[:5])

    print("== valgrind")
    valgrind = None if "--no-valgrind" in sys.argv[1:] else find_valgrind()
    if valgrind is None:
        print("skip no valgrind (not installed, or --no-valgrind): run.sh and the snippets are not "
              "run under it")
    else:
        home = os.path.expanduser("~/.local/bin/valgrind")
        expected = home if os.access(home, os.X_OK) else valgrind
        proc = subprocess.run(["bash", RUN_SH, "--valgrind-path"], stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, universal_newlines=True, cwd=ROOT,
                              env=dict(os.environ, PATH="/usr/bin:/bin"))
        check("run.sh finds valgrind although it is not on PATH (`run.sh --valgrind-path`)",
              proc.returncode == 0 and proc.stdout.strip() == expected,
              (proc.returncode, proc.stdout, expected))
        with tempfile.TemporaryDirectory() as tmp:
            for name in ("s0_control.c", "s1_use_then_init.c", "s2_init_on_one_branch.c",
                         "s3_array_element.c", "s5_goto_skips_init.c"):
                exe = os.path.join(tmp, name[:-2])
                cc = subprocess.run(["cc", "-std=c11", "-O0", "-g", s(name), "-o", exe, "-lflint",
                                     "-lgmp", "-lm"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    universal_newlines=True)
                if cc.returncode != 0:
                    check("%s builds against FLINT" % name, False, cc.stdout[-300:])
                    continue
                vg = subprocess.run([valgrind, "-q", "--error-exitcode=77", exe],
                                    stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
                                    universal_newlines=True, timeout=120)
                check("%s: valgrind reports the defect (exit 77)" % name, vg.returncode == 77,
                      (vg.returncode, vg.stderr[-200:]))

    if failed:
        print("selftest: FAILED: %d check(s): %s" % (len(failed), "; ".join(failed)))
        return 1
    print("selftest: passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
