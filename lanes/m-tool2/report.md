# Lane m-tool2: three defects of the mutation tool

Files written: `tools/mutate/mutate.py`, `tools/mutate/selftest.py`, `lanes/m-tool2/redgreen.log`,
`lanes/m-tool2/selftest.log`, `lanes/m-tool2/mutate-lpow.log`, this report. Nothing else was
changed; `tools/mutate/example/` was not touched (the judge of item 13 is written into a copy of
it by the self-test).

## What was changed

### Defect 1: `--san` defeated by `ASAN_OPTIONS`

New function `tools/mutate/mutate.py:937`:

```python
def command_sets_san(command):
    return re.search(r"(?:^|\s)SAN\s*=", command) is not None
```

Used at `tools/mutate/mutate.py:1107`, in place of `if args.san and "SAN" not in args.command`.
The question is whether the command sets the make variable `SAN`: a word `SAN=...` at the start of
the command or after white space, which is how a shell hands a variable to `make`. The letters in
`ASAN_OPTIONS=`, `UBSAN_OPTIONS=`, `SANITIZER=` are not that word.

The line the tool prints under `--san` (`tools/mutate/mutate.py:1120`) now says where `SAN=1`
comes from: with `SAN=1` when the tool put it into the environment, and "by the --make command,
which sets SAN itself" when the tool did not. Before, it claimed `SAN=1` in the second case too,
which is false for a command that sets `SAN=0`.

Docstring: the `--san` paragraph (near line 70) now states the decision and why the letters are not
enough.

### Defect 2: a run in which mutants did not build was reported as passed

Summary and exit code, `tools/mutate/mutate.py:1160` to 1186:

* survivors: unchanged, exit 1, `mutate: FAILED: %d mutant(s) survived; ...`;
* more than half of the mutants did not build (`not_compiled * 2 > total`): exit 2 and

  ```
  mutate: FAILED: %d of %d mutants did not build, so they were not tested; a build that fails for
  this many of them is the build command and not the mutants: --make '%s'
  ```
* a few did not build: exit 0 and
  `mutate: passed: every mutant that built was killed, or is excused in <file>; %d of %d did not
  build`;
* none did not build: the old line, `mutate: passed: every mutant was killed, or is excused in
  <file>`, which is now only printed when it is true.

Exit code 2 is the code the tool already used for "this run cannot be judged" (unmutated tree does
not pass, bad scratch directory). The docstring states the rule and the three exit codes.

### Defect 3: `swap_args` offered a mutant identical to the original

`tools/mutate/mutate.py:685`: the guard in `mutants_of` now also requires
`a_text.strip() != b_text.strip()`, so no `swap_args` mutant is generated whose two exchanged
arguments are the same text. On the tree as it stands this removes 4 mutants of 10829:

```
src/idpow.c:228:13 swap_args: 'arf_mul(bl, bl, bl, p, ARF_RND_FLOOR)' -> the same
src/idpow.c:229:13 swap_args: 'arf_mul(bh, bh, bh, p, ARF_RND_CEIL)' -> the same
src/lfunc.c:723:5  swap_args: 'fmpz_mul(x2, x, x)' -> the same   (lanes/f-repair3 line 121)
src/roots.c:449:5  swap_args: 'fmpz_mul(q, pk, pk)' -> the same
```

(measured with `timeout 300 python3 /tmp/count.py <mutate.py>`, one run over `src/*.c` for the
repair and one for a copy of the pre-repair file: 10829 mutants, 4 identical, against 10825
mutants, 0 identical.)

## Tests, red before the repair

All three were written in `tools/mutate/selftest.py` first and seen to fail; the runs are in
`lanes/m-tool2/redgreen.log`.

* defect 3, inside `test_generation_rules()` (item 4), `tools/mutate/selftest.py:1000`:
  `fmpz_mul(x2, x, x)` offers no `swap_args` mutant, `fmpz_mul(x2, x, y)` still offers one.
  Red: `FAIL swap_args of fmpz_mul(x2, x, x) is not offered at all: ["t.c:1:16 swap_args:
  'fmpz_mul(x2, x, x)' -> 'fmpz_mul(x2, x, x)'"]`, the second check already `ok`.
* defect 1, new item 12 `test_san_decision()` (`tools/mutate/selftest.py:714`), which calls
  `mutate.command_sets_san` directly on 11 commands: `make check`, `make -s -j2 check`,
  `ASAN_OPTIONS=detect_leaks=0 make check`, `ASAN_OPTIONS=detect_leaks=0 make -s -j2 check`,
  `make check UBSAN_OPTIONS=x`, `make check SANITIZER=1`, `make -s -j2 check INV=1` (none set
  SAN), `SAN=1 make check`, `make check SAN=1`, `make -s -j2 SAN=1 check`, `SAN=0 make check`
  (all set it). Red: every line `got None, want ...`, the function did not exist.
* defect 2, new item 13 `test_a_build_that_does_not_work_fails()` (`tools/mutate/selftest.py:752`),
  a run over a copy of `tools/mutate/example/` with the judge of `prepare_stale_judge()`
  (`tools/mutate/selftest.py:293`, `JUDGE_MAKEFILE` at line 258): `judge.mk` compiles
  `src/example.c` with `-fsanitize=undefined` and links the test program without it, so the link of
  a fresh object fails; the copy carries `lib/example.o` and `build/example`, both built with the
  sanitizer (`make -f judge.mk stale` before the run), and `copy_tree` keeps the times of the
  files, so the baseline finds everything up to date and passes, while a mutant, written into the
  copy after the copy was made, is newer than `lib/example.o`, is rebuilt, and its link fails.
  This is the shape of the run of `lanes/f-slice9/mutate-run1-notcompiled.log`, where the tree
  carried a sanitized archive and the command did not build the mutants against it. Red (6 mutants,
  `--limit 6`):
  ```
  ok   the baseline of the judge builds
  ok   every mutant is reported as not compiled
  FAIL the run does not print 'every mutant was killed': ['mutate: passed: every mutant was killed,
       or is excused in ../../../../../../../../../dev/null']
  FAIL the run ends with a non-zero exit code: exit 0
  FAIL the failure names the build command, not the mutants
  ```

  Note: the brief suggested a `--make` with a compiler flag that does not exist. Such a command
  fails the baseline too, so the tool has always stopped there with exit 2 and never reached the
  summary; that would not have been a red test. The judge above keeps the baseline building.

## Checks

| Command | Result |
|---|---|
| `timeout 900 make mutate-selftest` | exit 0, 84 `ok` lines, no `FAIL` (65 s) |
| `timeout 60 python3 tools/mutate/mutate.py --help` | exit 0, option list, unchanged names |
| `timeout 60 python3 tools/mutate/mutate.py --files src/lfunc.c --list --limit 3` | exit 0, 544 mutants, 3 listed |
| `timeout 60 python3 tools/mutate/mutate.py --files src/lfunc.c --keys --limit 0` | exit 0, 544 entry lines |
| `timeout 300 python3 tools/mutate/check_equivalent.py --files src/*.c` | exit 1, 3 stale entries, below |
| the `lpow` run of the brief, under `timeout 900` | exit 2 at the baseline, below |
| the same run with a `test_lpow`-only `--make` | exit 1, 6 of 6 compiled, below |

The whole self-test is in `lanes/m-tool2/selftest.log`, the `lpow` run in
`lanes/m-tool2/mutate-lpow.log`.

The `lpow` runs use
`python3 tools/mutate/mutate.py --files src/lpow.c --limit 6 --seed 1 --jobs 2 --san --timeout 150`
and, for the second, add `--make 'make -s -j2 SAN=1 build/test_lpow && ./build/test_lpow'`.

The last lines of `make mutate-selftest`:

```
mutate: passed: every mutant that built was killed, or is excused in ../equivalent-strong.txt; 7 of 54 did not build
selftest: passed: the weak test leaves a survivor, the strong test leaves none, and no process of a mutant is left
```

`check_equivalent.py`: the 3 entries of `tools/mutate/equivalent.txt` that match 0 mutants are the
lines 88, 112 and 157. The same 3 entries, and no others, fail with a copy of the pre-repair
`mutate.py`, so the `swap_args` repair removes no entry of `equivalent.txt`.
`check_equivalent.py` is not part of `make check-all`.

The `lpow` run of the brief, as it stands: exit 2, `mutate: the unmutated tree does not pass, so no
mutant can be judged:`, and the log ends in the middle of `== build/test_lpow`. The whole `make
check` of this tree does not finish in 150 s, so the baseline is stopped by `--timeout` and no
mutant is run at all.

The same run with the judge `lanes/f-slice9` used (a subset of the tests), in
`lanes/m-tool2/mutate-lpow.log`: exit 1. Last three lines:

```
mutate: 6 mutants in 120.4 s: 5 killed, 1 survived, 0 not compiled, 0 timed out, 0 excused
mutate: FAILED: 1 mutant(s) survived; each one is a claim the tests do not check
(empty)
```

**6 of the 6 mutants compiled** (0 not compiled, 0 timed out). The survivor is `src/lpow.c:220:30
cmp: '<' -> '<='`, a claim `test_lpow` does not check; `src/lpow.c` and `tests/test_lpow.c` are not
in this lane. The line under `--san` reads `mutate: every mutant is built and run by the --make
command, which sets SAN itself`, because the command sets `SAN=1` as a word and the tool therefore
adds nothing.

Time: the whole self-test 65 s, the `lpow` run 2 min. Two cores at most, every command under
`timeout`.

## What is not done

* `tools/mutate/equivalent.txt` is not mine; the three stale entries that `check_equivalent.py`
  reports (lines 88, 112, 157) were already stale before this repair and are left alone.
* No change to how many `not compiled` mutants is the limit: the rule is more than half, decided
  here as the brief leaves it. A tree where more than half of the mutants are type errors would
  fail with exit 2 and the message naming `--make`; that is the intended reading of the rule.
* The survivor `src/lpow.c:220:30` of the run above is reported, not repaired: `src/lpow.c` and
  `tests/test_lpow.c` are not in this lane.
* Nothing was committed; no git command that changes state was run.

## Sources pending

None. The three defects and the two references to them (`lanes/f-slice9/result.md` line 155-158,
`lanes/f-repair3/result.md` line 121, `lanes/f-slice9/mutate-run1-notcompiled.log`) are files in
the tree and were read. No formula of another author is used anywhere in this lane.

## Findings against the specification

`docs/SPEC.md` says nothing about the mutation tool, so there is nothing to contradict there. Two
things are worth the orchestrator's attention:

1. **The three stale entries of `tools/mutate/equivalent.txt`** (lines 88, 112, 157: `src/fball.c`
   drop_call #4, `src/recon.c` drop_call, `src/text.c` swap_args `fmpq_add(t, t, rad)`) match no
   mutant of the tree as it stands, so the reason they were written excuses nothing. This is true
   before and after this lane. `make check-all` does not run `check_equivalent.py`, so nothing
   notices.
2. **The `--timeout` of the tool bounds the baseline too.** `--timeout 150` against a tree whose
   `make check` needs about 6 minutes ends the run with "the unmutated tree does not pass", which
   names the mutants' judge and not the time budget. A caller who wants a subset of the tests must
   pass it as `--make` (as lanes/f-slice9 did). Not repaired here: changing the meaning of
   `--timeout` is a decision about the tool's interface, not one of the three defects.