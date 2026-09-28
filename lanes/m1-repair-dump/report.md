# Lane m1-repair-dump: findings R1 to R4 of the review `dump`

Status: the code and the tests of R1 to R4 were written by the lane (pi, `space-bunny-alpha`), which ended
two sessions without finishing this file. Sections 1 to 3 are the lane's. **Sections 4 to 9 were written by
the orchestrator (Claude Fable) on 2026-09-28 from the lane's logs and from its own runs**; each number
names the log or the run it was read off. Line numbers of `src/dump.c` in sections 1 to 3 are those of the
lane's commit `b942ac8`; the orchestrator's edit of section 7 moved the lines after line 68 up by 8.

## 1. What was done

| Finding | Change | Where |
|---|---|---|
| R1 (M1-D5) | A context occurrence of more than `ADF_MODCTX_MAX_BLOCKS = 65536` blocks is `ADF_UNSUPPORTED` at stage 5, decided from the count in the text | `src/dump.c`, `dp_w_ctx` (line 616) |
| R2 (M1-D9) | The bound `ADF_DUMP_QCLASS_EXP_MAX = 2^20` on the binary exponents of the real ball of a piece of a `qclass` is a limit of stage 4, applied to every piece on the digit strings before any semantic check | `src/dump.c`, `dp_w_arb` (line 567), set in `dp_w_qclass` (line 805) |
| R3 | The length of a printed text is read in a statement of its own, after the call that writes it; `tl` is initialised | `tests/fuzz/fuzz_dump.c`, six places |
| R4 | TAB, LF and CR are bytes of the alphabet (8.2), so stage 2 lets them past; the header is judged next | `src/dump.c`, `dp_header` (line 1063) |

The same three repairs were made in the reference `proto/text_grammar.py` (the dump reader only),
and `proto/test_text_grammar.py` has a new test class `TestDumpReviewFindings` for them.

No other source file of the library was changed. `src/modctx.c` needed no change: the cap is
enforced inside the shared validation `adf_dump_ctx_occurrence` of `src/dump.c`, which
`adf_modctx_new_from_dump` calls before it checks the occurrence index and before
`adf_modctx_alloc`.

## 2. Files written

* `src/dump.c`: the three repairs (the only library file changed).
* `tests/test_dump_limits.c`: new, nine tests for R1 and R2.
* `tests/test_dump.c`: new test `header_before_the_whitespace_of_the_body` (R4).
* `tests/test_dump_golden.c`: the row counts of the golden file, which grew by ten rows.
* `tests/golden/dump.tsv`: ten added rows at the end (six for R4, four for R2). No other row touched.
* `tests/fuzz/fuzz_dump.c`: the six places of R3 and a note in the header comment.
* `proto/text_grammar.py`: the dump reader only: the alphabet of stage 2, the block cap of M1-D5, the
  exponent bound of M1-D9 as a limit of stage 4, and the module docstring.
* `proto/test_text_grammar.py`: the new class `TestDumpReviewFindings`, and the "adele" line of the
  new R2 test.
* `lanes/m1-repair-dump/red-green.log`, `lanes/m1-repair-dump/green.log`, and the programs under
  `lanes/m1-repair-dump/checks/` (see section 5).

## 3. Decisions that a reader of the contract has to make

### 3.1 The stage of the cap of M1-D5

The cap is placed in stage 5 of `conventions.md` 8.5, the stage of the "word restrictions".  A
count limit (`max_items`) is stage 4 and gives `ADF_LIMIT`; the cap of M1-D5 is a restriction of the
implementation, not a limit of the text, and the only statuses of 8.5 that are not `ADF_PARSE`,
`ADF_LIMIT` and `ADF_DOMAIN` are the `ADF_UNSUPPORTED` of stage 3 (another version or field) and of
stage 5 (`p` or `q` at least `2^64`).  The cap is of the second kind: it says that this
implementation does not build a context of more than 65536 blocks.  Consequences, all pinned by
`tests/test_dump_limits.c`:

* a text with 65537 blocks is `ADF_UNSUPPORTED` even when its context also fails the predicate of
  conventions 5.14 (the count is decided before the blocks are read into integers);
* a text with 65537 blocks and `max_items = 65536` is `ADF_LIMIT` (stage 4 comes first);
* a text with 65537 blocks and `max_items = 65537` is `ADF_UNSUPPORTED`;
* a text whose count is `2^40` or `2^64` and whose body is short is `ADF_PARSE` whatever
  `max_items`: a count fixes the number of repetitions that follow it (conventions 10.1) and a
  mismatch is a grammar failure of stage 3, so the cap is never reached.

This is the order of `include/adelefeld/modctx.h` as well: "The order of the checks is: ADF_DOMAIN
first, then ADF_UNSUPPORTED", with the size decided "before any table is built" and before the
coprimality.

### 3.2 What "a piece of a qclass" means for the bound of M1-D9

M1-D9 bounds "the real ball of a `qclass` piece".  The bound is applied to every piece of the form
`pieces` and not to the form `lift`.  Reasons, in order of weight:

1. the grammar of conventions 10.1 gives a "piece" only in the repetitions of the form `pieces`; a
   `lift` is a single `arch fb`, one adele, and has no piece;
2. the reason M1-D9 gives for the bound is the cost: "The check of the range of a piece forms the
   exact end points of its real ball, so its cost grows with the exponents."  A lift forms no
   range; only `dp_q_piece` forms `lo` and `hi` with `ARF_PREC_EXACT`;
3. `tests/ref/vectors/m1-dump/dump_ref.jsonl` (a file of another lane, read by
   `tests/test_dump_ctx.c`) contains `qclass lift` texts whose real balls have exponents above
   `2^20` and records them as valid with a round trip.  Bounding the lift form would make the C
   contradict those vectors, and the file is not this lane's to regenerate.

If TJO reads "piece" as including a lift, then the change is one line in `src/dump.c`
(`dp_w_qclass`: drop `&& pieces`) and one in `proto/text_grammar.py`, and the vectors named above
have to be regenerated.  This is listed under "Findings against the specification".

### 3.3 The bytes of a field token

A TAB, LF or CR after the field letter stays inside the field token: `field = upper, {nonspace}`
(conventions 10.1) and "nonspace" is every byte but `0x20`, so `adf1 Q\trat 1 1` writes the field
`Q\trat`, which is not `Q`, and 10.2 ("A reader of version 1 rejects any other version and any
other field with `ADF_UNSUPPORTED`") gives `ADF_UNSUPPORTED`.  A TAB where the grammar wants the one
space after the version, or instead of the field letter (the field must begin with an upper-case
letter), is `ADF_PARSE`.  The reference already behaved this way; both are pinned by
`tests/test_dump.c` and by the reference test.

## 4. The tests

Red runs of the lane (`red-green.log`; the counts are the summary lines of the test programs built
against the library before the repair, `checks/red/dump_before.c`):

| Program | Red (before the repair) | Green |
|---|---|---|
| `tests/test_dump.c` (R4, test `header_before_the_whitespace_of_the_body`) | 16 tests, 12 to 16 failed checks, 1 failed test (lines 45, 89, 188 of the log) | 16 tests, 0 failed |
| `tests/test_dump_limits.c` (R1, R2) | 9 tests, 281 checks, 45 failed checks, 6 failed tests (line 165); 40 failed checks, 4 failed tests (line 251, after R1 alone) | 9 tests, 243 checks, 0 failed |
| `tests/test_dump_golden.c` (the ten added rows) | 1 test, 7 failed checks (line 56), 4 failed checks (line 199) | 1 test, 0 failed |

The nine tests of `tests/test_dump_limits.c`: `block_cap_refuses_65537_blocks_everywhere`,
`block_cap_accepts_65536_blocks`, `block_count_of_a_short_text_is_a_grammar_failure`,
`block_cap_and_max_items_in_that_order`, `block_cap_before_the_predicate_of_the_context`,
`qclass_exponent_bound_at_and_above_the_bound`, `qclass_exponent_bound_is_a_limit_of_stage_four`,
`no_exponent_bound_in_the_other_bodies`, `block_cap_accepts_65536_blocks_in_full` (the loaders and
`adf_modctx_new_from_dump` at the cap only with `ADF_DUMP_LIMITS_FULL=1`). The program takes 22 s
(orchestrator, 21:20), of the 46 s of the whole `make -j2 check`.

Added by the orchestrator: `tests/test_dump.c`, test `grammar_before_limits_in_sball_and_rfun` (section 6).

## 5. Checks

| Check | Result | Where |
|---|---|---|
| `make -j2 check`, gcc, lane's state | 39 test programs pass | `check-gcc.log`; repeated by the orchestrator at 20:18 |
| `make -j2 check SAN=1`, lane's state | 39 pass | `check-san.log` |
| `make -j2 check CC=clang`, lane's state | 39 pass | `check-clang.log` |
| the same three after the orchestrator's edits (sections 6, 7), 21:35 to 21:39 | 39 pass with `SAN=1`, with `CC=clang`, with gcc; `proto/test_text_grammar.py`: 34 passed | `land-checks.summary`, `land-check-*.log` |
| `make fuzz FUZZ_TARGET=dump FUZZ_SECONDS=120`, twice | 822202 and 748916 runs in 121 s each, no crash, no sanitizer report | `fuzz.log`, `fuzz2.log` |
| seed `adf1 Q rat 1 1` under valgrind, gcc `-O0`, before the `memset` of the harness | 120 errors from 36 contexts (padding bytes of the structs compared by the harness; no error in the library) | `valgrind-seed.log` |
| the same after it | 0 errors from 0 contexts | `valgrind-seed2.log` |
| review's `verify.py unit` against the repaired library (copy in `verify/`) | `test_dump`, `test_dump_ctx`, `test_dump_golden`, `bridge`, `roundtrip`: exit 0 | `verify-unit.log` |
| review's `status_findings.py` | R4: both inputs `UNSUPPORTED`, as the review expects. R2: the three texts with an exponent above the bound give `LIMIT`; the script expects `OK`, `DOMAIN`, `DOMAIN`, which is the contract before M1-D9 | `verify/verified-status.log` |
| review's `cost.py 65537` | `UNSUPPORTED`, inspector 0.011 s, constructor 0.011 s (before the repair: `OK` after 65 CPU seconds) | `verify/verified-cost.log` |
| `make mutate FILES=src/dump.c JOBS=2 LIMIT=60` | 1012 mutants, 60 run in 1435 s: 47 killed, 3 survived, 6 not compiled, 4 timed out | `mutate.log`; judged in section 6 |

Only 60 of the 1012 mutants of `src/dump.c` were run. The sweep over the whole file belongs to lane
m1-repair-tools (adf-xf4).

## 6. The survivors and the timeouts of the mutation run (orchestrator)

**The four timeouts are not loops without end.** Each mutant was built and every test program run alone
with a limit of 150 s (scratch copy of the lane's tree, 21:15 to 21:22):

| Mutant | Test programs that fail at once | `test_dump_limits` |
|---|---|---|
| `:1026:20` `return 1` to `return 0` (modctx walk) | `test_dump_ctx` (segmentation fault), `test_dump_golden`, `test_modctx` | not finished in 150 s, failed checks printed before |
| `:656:12` `return 1` to `return 0` (end of `dp_w_fb`) | `test_dump`, `test_dump_ctx` (segmentation fault), `test_dump_golden` | not finished in 150 s, 10 failed checks printed before |
| `:358:16` `==` to `!=` in `dp_fail` | `test_dump` (abort), `test_dump_ctx` (abort), `test_dump_golden`, `test_modctx` | not finished in 150 s, 8 failed checks printed before |
| `:1008:13` negated condition (adele walk) | `test_dump` (segmentation fault), `test_dump_ctx` (abort), `test_dump_golden` | finished after 147.9 s with 14 failed checks |

All four make a failed walk return with the status `ADF_OK`, so the text is taken as validated. In
`test_dump_limits` the texts with 65537 blocks are then accepted and a context is built from each of
them, which costs about 64 s each (section 8). That is the cost the cap of M1-D5 exists to refuse; the
unmutated reader never reaches it. The four count as killed. The tool reports a timeout because
`make check` runs all programs and looks at the result only at the end.

**The three survivors.**

| Mutant | Verdict | Reason |
|---|---|---|
| `:938:30` `st->mode == DP_LIMITS` to `!=` (the count `L` of a term of an `rfun`) | killed by the new test | The mutant compares `L` with `max_items` in the grammar pass, so a text with `L` above `max_items` and a grammar failure after that count gives `ADF_LIMIT`. By conventions 8.5 the grammar is stage 3 and the limits stage 4: `ADF_PARSE`. No test had an `rfun` with a limit and a grammar failure |
| `:883:28` `return 0` to `return 1` (complex ball of an `sball` with `c`) | killed by the new test | With the mutant a text that fails inside the complex ball and has no token after the failure passes every stage: `adf1 Q sball c zz` was accepted. No test had an `sball` that ends inside its ball |
| `:503:30` `r = 0` to `r = 1` (`dp_v_fb`) | equivalent | `r` is read only in `!dp_word(tr, &r) \|\| r >= q` and after it. `dp_word` writes `*v` whenever it returns 1; when it returns 0 the `\|\|` does not evaluate `r >= q` and `ok = 0` skips every later use. The initial value is never read. Line for `tools/mutate/equivalent.txt` (to be added by lane m1-repair-tools in its new key format): `src/dump.c` `ulong q = 0, r = 0;` zero_one on `r` |

Red and green of the new test `grammar_before_limits_in_sball_and_rfun` (`survivors-red-green.log`):
with mutant 938, 1 failed check (the `rfun` with `zz` and `max_items = 2`); with mutant 883, 5 failed
checks (the five `sball c` texts); with the original, 17 tests, 12259 checks, 0 failed.

## 7. Edits outside the lane's files (orchestrator)

The lane could not edit headers and documents and defined the constant under `#ifndef` in `src/dump.c`.
Done when landing:

* `include/adelefeld/dump.h`: `#define ADF_DUMP_QCLASS_EXP_MAX 1048576` with its comment, and a rule in
  the header comment (the bound of M1-D9, form `pieces` only; the cap of M1-D5 at stage 5).
* `docs/conventions.md` 8.4: a paragraph after the reasons of CV-27 that states the constant, its stage
  and that a caller cannot change it.
* `src/dump.c`: the `#ifndef` block and its HEADER-FINDING removed.
* `lanes/m1-repair-dump/checks/red/libadelefeld.a` and `dump.o` deleted.

## 8. Where the time of a large valid dump goes (item 5 of the brief)

Measured by the orchestrator at 21:25 (load average 1.9; programs `checks/bigctx.c`, `checks/precomp.c`,
gcc `-O2`, CPU seconds of `clock()`), contexts of the first `k` primes:

| `k` | bytes of the dump | inspector (validation only) | `adf_modctx_new_from_dump` | `fmpz_multi_mod_precompute` | `fmpz_multi_CRT_precompute` |
|---|---|---|---|---|---|
| 16384 | 156104 | 0.073 | 3.888 | 1.927 | 2.021 |
| 32768 | 328437 | 0.351 | 16.090 | 7.901 | 7.889 |
| 65536 | 682056 | 1.342 | 64.146 | 30.775 | 31.494 |

At the cap 62.3 of the 64.1 s (97%) are the two precomputations of FLINT that `adf_modctx_alloc` calls;
the reader of the dump takes 1.3 s. Both parts grow by a factor of 4 when `k` doubles. The cost is that of
every constructor of a context of this size and not of the dump form. Nothing was optimised. Not
measured: the review's case with 32768 squared primes (1 MB); where the time goes inside the two FLINT
functions.

## 9. Not done, sources pending, findings against the specification

Not done:
* Bounds of `src/dump.c` that no document states (item 2 of the brief): the lane left no list. The
  orchestrator searched the file for numeric constants and found the word bound `2^64` (8.5 item 5,
  `dp_word`, `dp_over_word`), the bound `2^30` on a radius mantissa (10.2, `dp_v_arb`), the overflow
  guard of a count (`dp_count`: a count beyond `size_t` exceeds the tokens left, so `ADF_PARSE` by 10.1)
  and the bound of M1-D9. No other was found. This was a search, not a reading of every line.
* The mutation run covers 60 of 1012 mutants.
* The review's `status_findings.py` still expects the contract before M1-D9 for its R2 inputs; the file
  is the reviewer's and was not changed. The closure check (adf-igt) must judge R2 against M1-D9.

Sources pending: none new.

Findings against the specification:
* M1-D9 (`docs/SPEC.md` 15) is still PROPOSED. Its row says "a `qclass` piece"; the code applies the
  bound to the form `pieces` only (section 3.2). The row should say so.
* `include/adelefeld/dump.h` does not say which status a typed loader gives for the dump of another
  type; the code gives `ADF_PARSE` (HEADER-FINDING in `src/dump.c`, `dp_validate`).
