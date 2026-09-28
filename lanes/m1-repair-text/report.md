# Lane m1-repair-text: report

Findings R2, R3, R4, R9, R10 of reviewer `text`; issues adf-b8l, adf-5qq. The work of the first
attempt was saved by the orchestrator in commit `79cf338` ("WIP m1-repair-text"). At the resume the
worktree was clean against that commit, so the source edits were already in place. This report
verifies them and finishes the checks. All numbers below were measured in this worktree.

## What was done

The first attempt had already written the code and the tests. This resume did the following.

- Read the brief, `lanes/COMMON-C.md`, the review, the header, SPEC 15 rows M1-D6/M1-D7, and the
  changed sources and tests. Confirmed the edits are the ones the brief asks for.
- Ran every check of item 6, the resource reproducer, valgrind, the fuzzer, and the mutation run.
- Wrote this report. Added a corpus-scanning helper and check logs under `lanes/m1-repair-text/`.

The code changes (all in the WIP commit, unmodified here) are these.

- `src/text.c`, `tx_dec_over`: the exponent digit string is compared with `max_exp10` as a number
  of any length; a negative `max_exp10` admits no written exponent; leading zeros are skipped. The
  hidden 18-digit refusal is gone (finding R4, decision M1-D7).
- `src/text.c`, `tx_dec_value`: a zero coefficient is the exact zero and forms no power of ten.
- `src/text.c`, `tx_bin_exp_over` and `tx_arb_printable`: the M1-D6 bound is tested on `ARF_EXP` /
  `MAG_EXP` before any conversion. An exponent with more than 17 bits is over; otherwise it is
  compared with `ADF_PRINT_EXP_MAX` in absolute value.
- `src/text.c`, `adf_adele_get_str` and `adf_cadele_get_str`: return NULL with `*len = 0` for an
  over-bound value. The `flint_abort` near old line 1078 was removed (finding R2, issue adf-b8l).
- `tests/test_text_limits.c` (new): the printer-bound table, the M1-D7 limit cases, and the R3
  round-trip case; 11 tests, 105 checks.
- `tests/test_text_adele.c`: field-by-field sentinel comparison (R10); M1-D6 handling in
  `check_adele_value` / `check_cadele_value`; the extreme-exponent and `max_exp10` tests updated.
- `tests/fuzz/fuzz_text.c`: `prec` and `digits` from two control bytes (R9); every accepted text
  is also printed and read back at `prec = 2`, `digits = 1`; a NULL printer is accepted only when
  `tx_arb_printable` is false; sentinel checks field by field (R10).
- `proto/text_grammar.py`: the exponent compared as a number of any length; zero short-circuit.
- `proto/test_text_grammar.py`: `TestRepairFindings` for that rule.
- `lanes/m1-repair-text/prefix_corpus.py`: prefixed the committed text-fuzz seeds with two control
  bytes (already applied; the corpus files are in the WIP commit).

## Files written by this resume

- `lanes/m1-repair-text/report.md` (this file).
- `lanes/m1-repair-text/logs/` (raw command output; see the checks below).
- `lanes/m1-repair-text/checks/precdigits.c` and `precdigits`: a helper that decodes the control
  bytes of a corpus as the fuzzer does and reports the accepted `prec`/`digits` sets.
- `lanes/m1-repair-text/checks/resources.py`, `checks/probe`: a copy of the reviewer's reproducer
  and its probe, built here so that the read-only review directory is not written to.
- `lanes/m1-repair-text/run_limits.sh` and `prefix_corpus.py` were written by the first attempt.

## Checks that were run

1. `make -j2 check` (normal, gcc): exit 0, all 34 test programs. In particular
   `build/test_text_limits` reports 11 tests, 105 checks, 0 failed. Log:
   `logs/make_check.log`, `logs/make_check_normal_after_clang.log`.

2. R2 under the resource limit: `sh lanes/m1-repair-text/run_limits.sh` (it sets
   `ulimit -v 2000000`, `ulimit -t 20`). 11 tests, 105 checks, 0 failed; whole run real 0.145 s,
   user 0.142 s. The printer returns fast; it does not expand an over-bound value. Log:
   `logs/run_limits.log`.

3. `python3 proto/test_text_grammar.py`: `Ran 32 tests`, `OK`, real 36.6 s. Log:
   `logs/test_text_grammar.log`.

4. R10 valgrind: `valgrind -q --error-exitcode=9 --leak-check=full
   --errors-for-leak-kinds=definite,indirect ./build/test_text_adele`: exit 0. The stderr has 22
   "possibly lost" records (FLINT-internal allocations under `tx_arb_set_ball`), 0 definite, 0
   indirect, 0 invalid read/write, 0 uninitialised-use. Under the required leak-kind setting these
   are not errors. Log: `logs/valgrind_adele.err`, `logs/valgrind_adele.out`.

5. Reviewer's `resources.py` against the repaired library (copy in this lane): exit 0, 0.92 s for
   all 17 children. The new table (`logs/resources_new.log`) is:

   | input e | returncode | seconds | length/text |
   |---|---:|---:|---|
   | 100000 | 0 | 0.007 | 0 / NULL (refused, M1-D6) |
   | 1000000 | 0 | 0.006 | 0 / NULL |
   | 10000000 | 0 | 0.006 | 0 / NULL |
   | 100000000 | 0 | 0.005 | 0 / NULL |
   | 1073741824 | 0 | 0.005 | 0 / NULL |
   | -100000 | 0 | 0.024 | 49 bytes, text printed |
   | -1000000 | 0 | 0.006 | 0 / NULL |
   | -10000000 | 0 | 0.005 | 0 / NULL |
   | -100000000 | 0 | 0.006 | 0 / NULL |
   | -1073741824 | 0 | 0.006 | 0 / NULL |
   | 18446744073709551616 | 0 | 0.008 | 0 / NULL (was the flint_abort) |

   Here e is the exponent of the exact input `2^e`; `ARF_EXP = e + 1`, so e = 100000 is above the
   bound and e = -100000 is at it. The old table had 20 s CPU aborts and one `flint_abort`; the new
   one has no abort and every over-bound print is a fast refusal. The `predicate` modes still
   return -11 (finding R1, another lane; out of scope here). The `nested` mode now returns 0.

6. `make clean && make -j2 check SAN=1`: exit 0, all 34 test programs. Log: `logs/check_san.log`.

7. `make clean && make -j2 check CC=clang`: exit 0, all 34 test programs. Log:
   `logs/check_clang.log`.

8. `sh tests/test_driver.sh`: exit 0, `23 cases, 100320 expected lines, all equal (SAN=0)`,
   real 0.87 s. The driver's own guard is kept (it is another lane's file). Log:
   `logs/test_driver.log`.

9. `make fuzz FUZZ_TARGET=text FUZZ_SECONDS=120`: exit 0, no crash.
   `stat::number_of_executed_units: 2121442`, 121 s, `new_units_added: 1848`,
   `peak_rss_mb: 485`. Coverage (llvm-cov): `src/text.c` 833 regions, 18 missed, 97.84 %;
   1053 lines, 19 missed, 98.20 %; 70 functions, 0 missed. `tests/fuzz/fuzz_text.c`
   289 regions, 51 missed, 82.35 %. Log: `logs/fuzz_text_120.log`.

   Accepted-input sets, measured by `checks/precdigits` over the 869 corpus files that libFuzzer
   left after the run (229 accepted as adele, 79 as cadele): `prec` = {3, 4, 7, 8, 9, 11, 12, 14,
   18, 20, 22, 23, 26, 28, 31, 35, 39, 44, 45, 50, 51, 52, 53, 55, 58, 62, 63, 64, 65, 66, 69,
   71, 72, 73, 76, 82, 84, 87, 89, 93, 95, 106, 112, 117, 118, 120, 122, 123, 124, 129, 130,
   137, 138, 143, 150, 151, 156, 157, 159, 166, 167, 177, 179, 181, 185, 189}; `digits` = {1,
   2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
   28, 29, 30}. The two values the old scheme could not reach are exercised for every accepted
   text by the explicit read-back at `prec = 2`, `digits = 1`. Log: `logs/precdigits.log`.

## What is not done

- The mutation run of item 7 did not finish. `make mutate FILES=src/text.c JOBS=2 LIMIT=300` ran
  for 40 minutes and was killed by the tool's own timeout (mutate.py prints the survivors only at
  the very end, so the buffered output yielded no survivor list). A second run with an incremental
  scratch copy (`python3 -u tools/mutate/mutate.py --root . --scratch
  lanes/m1-repair-text/mutscratch --files src/text.c --limit 300 --jobs 2 --copy Makefile include
  src tests build`) had a baseline of 2.6 s and reached mutant 266 of 300 (the two worker
  directories `w00265` and `w00267` were in flight) when the 40-minute tool timeout killed it. No
  survivor list was recovered, so the survivors are unknown. This is the only brief item not
  finished. The source file has 911 mutants; 300 were selected by the given seed.
  Log: `logs/mutate_text_incremental.log` (header only). `tools/mutate/equivalent.txt` was not
  edited.
- The literal R3 case of the brief, `(9.99e100000 ; 0)` read at the default limits, printed at
  `digits = 1`, and read back at the default limits, cannot happen after M1-D6: the printer refuses
  the value (its binary exponent is about 332193 > 100000), so there is no text to read back. See
  the findings below. The test in `tests/test_text_limits.c` pins the M1-D6 refusal for that exact
  input and pins the documented "reader not closed" behaviour at `max_exp10 = 21` (a value read at
  21 prints with decimal exponent 22, so the reader at 21 gives ADF_LIMIT and at 22 gives ADF_OK
  and an enclosure).

## Sources pending

- `[source pending: FLINT 3.0.1 exactness of mag_set_ui_2exp_si for every representable 30-bit
  mantissa]` (carried over from the reviewer; not needed by the changes of this lane).
- `[source pending: FLINT 3.0.1 behaviour of flint_malloc on allocation failure]` (carried over).

## Findings against the specification

- The brief's R3 case conflicts with SPEC 15 row M1-D6. With M1-D6 the printer of
  `(9.99e100000 ; 0)` returns NULL at the default limits, so the brief's read-back at the default
  limits gives no text and not `ADF_LIMIT`; only the raised-`max_exp10` arms of the brief are
  directly testable, at a smaller limit where the binary bound is not reached. This is a conflict
  between the brief and a decision of the same day, not a fault of the code or of the specification.
- M1-D7 on reading. A non-zero coefficient with an exponent within a raised `max_exp10` is formed
  as an integer: `tx_dec_value` calls `tx_pow10_fmpz`, so `10^max_exp10` is built. At the default
  `max_exp10 = 100000` that is about 332193 bits, which is fine. A caller who raises `max_exp10`
  to `WORD_MAX` asks for about `10^18` bits (about `4 * 10^17` bytes) and the allocation will
  fail; the code does not add a second, hidden bound. The default is the protection, as the brief
  says. The tests pin the boundary and the exact-zero short circuit, not a large non-zero value.
- R4 in the reference. `proto/text_grammar.py` now compares the exponent as a number of any length
  and short-circuits a zero coefficient; 32 grammar tests pass.
