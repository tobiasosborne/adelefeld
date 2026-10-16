#!/usr/bin/env python3
"""lanes/docs-julia/make_numbers.py: assembles lanes/docs-julia/numbers.md from the recorded output files of the
lane (build.log, exports.log, tutorial.out, pkgtest.log, noload.out, exports.out, check.out), verbatim.
Run from the worktree root after the commands that numbers.md lists."""
L = 'lanes/docs-julia/'
ENV = ('env JULIA_NUM_THREADS=1 JULIA_PKG_OFFLINE=true JULIA_DEPOT_PATH="$PWD/lanes/docs-julia/depot:$HOME/.julia:" \\\n'
       '    TMPDIR="$PWD/lanes/docs-julia/tmp"')
def f(n): return open(L + n).read().rstrip("\n")
build_tail = "\n".join(f('build.log').split("\n")[-2:])
exp_tail = f('exports.log').split("\n")[-1]
pk = f('pkgtest.log').split("\n")
pk_from = next(i for i, l in enumerate(pk) if l.startswith("     Testing Running tests"))
out = f"""# lanes/docs-julia/numbers.md: the commands and outputs behind every number of the two pages

Every command is run from the worktree root `/home/tobias/Projects/adelefeld-wt/docs-julia` on 2026-10-16,
Julia 1.12.5, one thread, after `mkdir -p lanes/docs-julia/tmp lanes/docs-julia/depot`. Each output below is
copied verbatim from the file named beside its command by `python3 lanes/docs-julia/make_numbers.py`; the
files are in this directory.

## 1. Build

    timeout 600 make -j2                  # output: lanes/docs-julia/build.log (last two lines below)

```text
{build_tail}
```

`make -j2` builds `build/libadelefeld.a` only; the shared object comes from the export check:

    timeout 600 sh tests/test_exports.sh  # output: lanes/docs-julia/exports.log (last line below)

```text
{exp_tail}
```

## 2. The tutorial (julia/Adelefeld/README.md, steps 1 to 8)

    timeout 300 {ENV} \\
    julia/Adelefeld/bin/adelefeld-julia --startup-file=no --project=julia/Adelefeld lanes/docs-julia/tutorial.jl \\
    > lanes/docs-julia/tutorial.out 2>&1

Exit status 0. A second run of the same command into `lanes/docs-julia/tutorial.rerun.out` is byte-identical
(`cmp` prints nothing). The README's REPL blocks are written from `tutorial.out` by
`python3 lanes/docs-julia/fill_readme.py` and compared with the second run by `python3 lanes/docs-julia/check.py`
(section 6). The whole output:

```text
{f('tutorial.out')}
```

## 3. Pkg.test

    (time timeout 900 env JULIA_NUM_THREADS=1 JULIA_NUM_PRECOMPILE_TASKS=1 JULIA_PKG_OFFLINE=true \\
        JULIA_DEPOT_PATH="$PWD/lanes/docs-julia/depot:$HOME/.julia:" TMPDIR="$PWD/lanes/docs-julia/tmp" \\
        julia/Adelefeld/bin/adelefeld-julia --startup-file=no --project=julia/Adelefeld \\
        -e 'using Pkg; Pkg.test()') > lanes/docs-julia/pkgtest.log 2>&1

Exit status 0. From `Testing Running tests` to the end:

```text
{chr(10).join(pk[pk_from:])}
```

The five step testsets pass 904 + 1015 + 1649 + 1313 + 488 = 5369 tests (the count of HANDOFF.md). The testset
`C call counters` (43) is a code string run by a child `julia` process against the fixture library
`count-ja4.so` (`julia/Adelefeld/test/runtests.jl:1732-1742`), and prints its own summary. In all, 5412 passing
tests are printed, 0 failed, 0 errored. Pkg.test writes `julia/Adelefeld/Manifest.toml` (gitignored,
`.gitignore:36`); `git status` shows no tracked file changed.

## 4. Julia without the launcher

    timeout 120 env JULIA_NUM_THREADS=1 JULIA_PKG_OFFLINE=true JULIA_DEPOT_PATH=... TMPDIR=... \\
        julia --startup-file=no --project=julia/Adelefeld \\
        -e 'try; @eval using Adelefeld; catch e; println(typeof(e)); showerror(stdout, e.error); println(); end' \\
        > lanes/docs-julia/noload.out 2>&1

```text
{f('noload.out')}
```

## 5. The exported names and their docstrings

    timeout 300 {ENV} \\
    julia/Adelefeld/bin/adelefeld-julia --startup-file=no --project=julia/Adelefeld lanes/docs-julia/exports.jl \\
    > lanes/docs-julia/exports.out 2>&1

```text
{f('exports.out')}
```

## 6. The checks of the two pages

    python3 lanes/docs-julia/fill_readme.py
    python3 lanes/docs-julia/check.py lanes/docs-julia/tutorial.rerun.out > lanes/docs-julia/check.out 2>&1

```text
{f('check.out')}
```
"""
open(L + 'numbers.md', 'w').write(out)
print("make_numbers: wrote", L + "numbers.md")
