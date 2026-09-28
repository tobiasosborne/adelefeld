# Lane m1-repair-tools: mutation tool and memory checker (review surface R1 to R6; arith R4, R5;
# contexts R5; issues adf-tz1, adf-pf5)

Read `lanes/COMMON.md`. Then `docs/reviews/m1/surface/review.md` findings R1 to R6 with their reproducers
under `docs/reviews/m1/surface/checks/` (`memcheck/`, `mini/`, `equiv_keys.py`, `stale_excuse.py`,
`mutate_sigterm.py`); `docs/reviews/m1/arith/review.md` R4, R5; `docs/reviews/m1/contexts/review.md` R5;
`lanes/tools-mutate/report.md`; `tools/mutate/`, `tools/memcheck/`, `tests/README.md`, the `mutate` targets
of the `Makefile` (read-only). The files `lanes/m1-local/equivalent-added.txt`,
`lanes/m1-scaled/equivalent-added.txt`, `lanes/m1-dump/equivalent-added.txt` and the ten entries that lane
m1-modctx-b proposed (`git -C ../m1-modctx-b diff` is not available to you; they are quoted in
`lanes/m1-modctx-b/report.md` finding 6 and section "Checks run", and two of them are not equivalent).

**You own:** `tools/mutate/`, `tools/memcheck/`, `tests/README.md` (the parts on mutation testing and the
memory check), `lanes/m1-repair-tools/`. Python standard library only. Red-green for every change, in
`tools/mutate/selftest.py` and in a new `tools/memcheck/selftest.py`.

1. **Keys that survive edits (surface R3).** A key of `equivalent.txt` is
   `<file> | <kind> | <the text of the line the mutant changes, stripped> | <old> -> <new> | <reason>`,
   with an optional occurrence number when the same line text and mutation occur more than once in the
   file. No line number. Convert the file: for every present entry find, in the source as it stands, the
   mutant that its reason describes (the reason quotes the call); an entry whose mutant no longer exists
   is dropped and listed in the report; an entry that now matches another statement (the review names
   `src/fball.c:142:drop_call`) is dropped. Add the entries of the three `equivalent-added.txt` files in
   the same way. Entries whose stated reason is "the same value is written" for a swap of the operands of
   `arb_mul` or `acb_mul` get the true reason: both orders give an enclosure, and the library promises an
   enclosure, not a particular ball (arith R4).
2. **`--keep` (surface R2)** judges as the normal run does. **Test names (surface R5):** a compile error
   is recognised by the compiler's own format at the start of a diagnostic line (`<file>:<line>:<col>:
   error:` or `fatal error:`, and the linker's `undefined reference`), not by `error:` anywhere.
   **SIGTERM and SIGINT (surface R6):** the tool kills the process groups of its running mutants and
   removes its scratch directory.
3. **A sanitizer mode (issue adf-pf5):** `--san` builds and runs the mutants with `SAN=1`, so that a mutant
   that only reads or writes out of bounds, or leaks, is killed. The `Makefile` target cannot be changed by
   you; document the direct call in `tests/README.md`.
4. **Memory checker (surface R1, R4):** a use before the first init in text order is reported; an init on
   one branch only, an array element, and a `goto` over the init are reported or named as limits in the
   README with a test that pins what the tool does. The README's statement on valgrind is out of date
   (valgrind is installed in `~/.local/bin`).
5. Then run, one after the other, never two at once, for every file of `src/`:
   `python3 tools/mutate/mutate.py --root . --files src/<f>.c --jobs 2 --seed 20260928 --limit 300 --san`
   (if one run exceeds 40 minutes stop it and lower the limit for that file; say so). Report the table of
   counts per file and every survivor with the missing test. Do not change `src/` or `tests/`.

## Resume note of the orchestrator (2026-09-28, 15:50)

This lane was stopped from outside when the machine ran low on memory. It was not your fault and nothing is
lost: your files are in the worktree as you left them, uncommitted. Before anything else run `git status`
and `git diff --stat`, read the logs in your lane directory, and find the first item of the brief that is
not finished. Do not start again from the beginning and do not rewrite what works. Memory: never run two
builds or two mutation runs at the same time; use `make -j2`; run a mutation run in the foreground, not
with `nohup`; if `free -g` shows less than 6 GB available, wait. Finish with `report.md`.

## Second resume note of the orchestrator (2026-09-28, 23:20)

Master was merged into your worktree: the sources under `src/` and the tests are now the repaired ones
(40 test programs). Your own files are as you left them. Convert `equivalent.txt` against the sources as
they stand NOW; `equivalent.new.txt` of your first session was made against older sources, check every
entry of it again. Further sources of entries and of stale lines, all to be read:
`lanes/m1-repair-dump/report.md` (one equivalent mutant of `src/dump.c`, the declaration
`ulong q = 0, r = 0;` in `dp_v_fb`), the sections on mutation of `lanes/m1-repair-recon/report.md`,
`lanes/m1-repair-adele/report.md`, `lanes/m1-repair-ctx/report.md`, `lanes/m1-repair-text/report.md`.
An entry is accepted only if you built the mutant and the reason is true of the code; do not copy a
reason you did not check.

Two additions (issue adf-4lj and an observation of the evening):
- 6. `mutate.py` prints every survivor at the moment it is found (flushed), not only at the end: a run
  that is stopped by a timeout must leave its survivors in the log.
- 7. An option `--make "<command>"` that replaces `make check` as the judge of a mutant, and a mode in
  which the mutated file is not under `src/`, so that `tools/adf/adf.c` can be mutated with
  `sh tests/test_driver.sh` as the judge. Selftest first (red, then green). Document it in
  `tests/README.md`. You do not own the `Makefile`.

Item 5 (the sweep) is run by you in this session, after items 1 to 4, 6 and 7 are green. Another lane
builds on this machine at the same time: `--jobs 2`, one run at a time, in the foreground; before each
run `free -g` must show 6 GB or more available. Write the counts of each file into
`lanes/m1-repair-tools/sweep.md` when its run ends, before you start the next file, so that an
interruption loses one file at most. Order: the small files first (`status.c`, `place.c`, `rat.c`,
`inlines.c`), `dump.c` and `text.c` last. Finish with `report.md`.

## Third resume note of the orchestrator (2026-09-28, 23:30)

Your provider ends a session every few minutes; nothing is wrong with your work. The orchestrator ran
`python3 tools/mutate/selftest.py` at 23:28: it passes. Items 2, 3, 6 and 7 are taken as done.

- Your notes of progress are now in `lanes/m1-repair-tools/progress.md` (the file you wrote as
  `report.md`). Keep it up to date after every finished step. Do NOT create `report.md` before
  everything below is done: the runner stops the lane as soon as that file exists.
- Still to do, in this order: item 1 (the conversion of `tools/mutate/equivalent.txt`; read the second
  resume note again for the sources of entries), item 4 (the memory checker and
  `tools/memcheck/selftest.py`), the parts of `tests/README.md`.
- Item 5 (the sweep) is NOT yours any more: the orchestrator runs it. Do not start a mutation run over
  a file of `src/` except a run with `--list`, or a run of single mutants that you need to judge an
  entry of `equivalent.txt` (`--limit` 5 or less).
- Then write `report.md` as `lanes/COMMON.md` rule 8 says.
