# Lane m1-repair-dump: findings R1 to R4 of reviewer `dump`

Read `lanes/COMMON.md` and `lanes/COMMON-C.md`. Then `docs/reviews/m1/dump/review.md` with its reproducers
(`docs/reviews/m1/dump/checks/`: `verify.py`, `cost.py`, `bridge.c`, `status_findings.py`, `fuzz_driver.c`);
`docs/SPEC.md` section 15 rows M1-D5 and M1-D9; `docs/conventions.md` 8.2, 8.4, 8.5, 10.1, 10.2;
`include/adelefeld/dump.h`, `include/adelefeld/modctx.h`, `include/adelefeld/text.h`.

**You own:** `src/dump.c`; in `src/modctx.c` the function `adf_modctx_new_from_dump` only;
`tests/test_dump.c`, `tests/test_dump_ctx.c`, `tests/test_dump_golden.c`, a new `tests/test_dump_limits.c`;
`tests/fuzz/fuzz_dump.c`; `tests/golden/dump.tsv` (added rows only); in `proto/text_grammar.py` and
`proto/test_text_grammar.py` the dump reader only; `lanes/m1-repair-dump/`.
The headers and `docs/` are not yours: what must change there goes into the report, with the exact text.

Red first for each finding: the test with the expected status worked out from the contract (the reason in
a comment), seen failing, then the code. Keep `lanes/m1-repair-dump/red-green.log`.

1. **R1 (M1-D5).** A context occurrence of a dump with more than `ADF_MODCTX_MAX_BLOCKS` blocks is
   `ADF_UNSUPPORTED`: in `adf_modctx_new_from_dump`, in every typed loader and in every inspector, decided
   from the block count as it stands in the text, before the blocks are read into integers, before any
   validation of coprimality and before any allocation in proportion to the count. Place it in the order
   of conventions 8.5 and say in the report which stage you chose and why (the count limit `max_items`
   is stage 4 and gives `ADF_LIMIT`; the cap of M1-D5 is a restriction of the implementation, as the word
   restrictions of stage 5 are). Tests: 65536 blocks load; 65537 are refused within one second with the
   outputs untouched; a count of `2^40` and of `2^64` in the text with a short body; the same with
   `max_items` set below and above the cap, so that the order of the two statuses is pinned.
2. **R2 (M1-D9).** The bound on the binary exponents of a piece of a `qclass` becomes a limit of stage 4:
   applied to every piece before any semantic check, so that a text with a zero denominator and an
   exponent above the bound gives `ADF_LIMIT`, and so does a negative midpoint with such an exponent.
   The constant is `ADF_DUMP_QCLASS_EXP_MAX`; the header does not define it yet, so define it in
   `src/dump.c` under `#ifndef`, mark the place `HEADER-FINDING`, and put the text for the header and for
   conventions 8.4 into the report. The exponent is compared as a number of any length (M1-D7 is the
   pattern: no hidden bound of a machine word). Tests at the bound, one above, and with 40 hexadecimal
   digits. Look for every other bound in `src/dump.c` that no document states, and list each with its
   line, whether you found one or not.
3. **R3.** `tests/fuzz/fuzz_dump.c`: the length is read after the call that writes it, at all six places
   (a call to a printer, then `same_bytes` in a second statement). Initialise `tl`. Then
   `make fuzz FUZZ_TARGET=dump FUZZ_SECONDS=120`, and the seed `adf1 Q rat 1 1` under valgrind built with
   gcc at `-O0`. Every crash found is a finding: report it with the input, do not hide it.
4. **R4.** A TAB, LF or CR is a byte of the alphabet (8.2), so stage 2 lets it pass; the header is judged
   next, and another version or field is `ADF_UNSUPPORTED` whatever whitespace the body holds; with the
   header `adf1 Q ` such a byte in the body is `ADF_PARSE` by the grammar (10.1). Tests: the reviewer's
   two inputs; each of the three bytes at the first, a middle and the last position of the body and
   inside the header; NUL and `0x80` at the same positions stay `ADF_PARSE`. Repair the reference
   `proto/text_grammar.py` in the same way and run its tests.
5. The reviewer's observation on cost: a valid dump of 1 MB loads in 60 to 69 CPU seconds. Find where the
   time goes (`perf` or timers around the stages) and report it with numbers. Do not optimise.
6. `make -j2 check`, `make clean && make -j2 check SAN=1`, `make clean && make -j2 check CC=clang`;
   `python3 -B docs/reviews/m1/dump/checks/verify.py unit` and `cost` against the repaired library, with
   their new output in your lane directory (the review's files stay unchanged; if the script writes into
   its own directory, copy it first); `make mutate FILES=src/dump.c JOBS=2 LIMIT=300`, once, in the
   foreground, with nothing else running; survivors killed by tests or listed in the report with the
   reason (do not edit `tools/mutate/equivalent.txt`).

Memory: never two builds or two mutation runs at the same time; if `free -g` shows less than 6 GB
available, wait. Finish with `report.md`.

## Resume note of the orchestrator (2026-09-28, 20:20)

Your first session ended at its time limit of 90 minutes. Nothing is lost: your files are in the worktree
as you left them. The orchestrator ran `make clean && make -j2 check` on them at 20:18: all 39 test
programs pass. Do not start again and do not rewrite what works. Run `git status`, read
`lanes/m1-repair-dump/report.md` (sections 1 to 3 are written; section 4 is an empty heading; the first
paragraph promises checks that the file does not contain), `red-green.log` and `green.log`.

What is left, in this order; write each result into `report.md` as soon as you have it:
1. Section 4 of the report (the tests, with the red runs from your logs).
2. Item 6 of the brief without the mutation run: the sanitizer build, the clang build, the review's
   `verify.py unit` and `cost` (copies in your lane directory), and item 3's fuzz run and valgrind run.
3. Item 5 (where the time of a 1 MB dump goes): you have `bigctx`, `precomp` and a `gmon.out` in
   `checks/`; report the numbers you measured.
4. The mutation run, last, in the foreground, with a limit that ends within 30 minutes:
   `make mutate FILES=src/dump.c JOBS=2 LIMIT=60`. If it does not end in 30 minutes, stop it and say so.
5. The sections of `lanes/COMMON.md` rule 8 that are missing: what is not done, sources pending, findings
   against the specification (your section 3.2 belongs there too), and the text for the header
   `dump.h` and for conventions 8.4 (the constant of M1-D9).
Remove the first paragraph's promise or make it true. Delete the binaries in `checks/` at the end.
