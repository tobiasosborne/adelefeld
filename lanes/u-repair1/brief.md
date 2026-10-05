# Lane u-repair1: repair of review u-review1 (the dump forms of the idele and the class: two BLOCKERS)

Review u-review1 (`docs/reviews/m2/review-dump-units.md`, the same text as `lanes/u-review1/result.md`; its
instruments are in `lanes/u-review1/w/` (reference `ref.py`, generator `gen.py`, harness `fz.c`) and
`lanes/u-review1/ft/` (`faults.py`: faults F01 to F17 and the two proposed repairs F18, F19 as patches)) found
two defects in `dp_arb_sign` (`src/dump.c:625-660`, lane u-dump1), which decides whether the real ball of a
dumped idele or class contains 0:
- F1 BLOCKER: for a negative midpoint the code skips the sign (`am.p++`) and does not shorten the token
  (`am.n--`), so the mantissa is read one byte too far. `adf1 Q idele 1 -1 0 1 0 1 1 1 0` (the ball `[-2, 0]`,
  which contains 0) passes stage 6, the constructor refuses, and the loader calls `flint_abort` (`src/dump.c`
  near line 2229, "cannot happen"). True status: `ADF_DOMAIN` (conventions 5.7, 10.2).
- F2 BLOCKER: when midpoint and radius lie in the same binade and their leading bits are equal, the code
  answers "contains 0", although an odd midpoint mantissa with more bits is strictly larger.
  `adf1 Q idele 1 3 0 1 1 1 1 1 0` (the ball `[1, 5]`) is refused with `DOMAIN`; it is the dump of a value the
  public constructor accepts, so the round trip is broken. The same for `adf1 Q idclass 3 0 1 1 1 0`.
Read the review completely first, then `src/dump.c` from `dp_arb_sign` to the idele and class loaders, and
conventions 5.7, 8.5, 10.1, 10.2.

Red first, then green, for each part; keep `lanes/u-repair1/redgreen.md` with the command and the failing
assertion (for F1 the red run is an abort of the test program: record the signal).

1. **Tests** in `tests/test_dump_units.c` (new test functions):
   - the review's texts: three for F1 (`DOMAIN`, the output untouched, no abort), two for F2 (`OK`, and the
     round trip `set_parts` then `dump_str` then `load_str` identical);
   - a FAMILY that does not use `dp_arb_sign`'s method: for the idele and for the class, all dumps with
     midpoint `m 2^e` and radius `r 2^f` for odd `m` with `|m| <= 127` (both signs), odd `r <= 127` and
     `r = 2^29 + 1`, `2^30 - 1`, `e`, `f` in `-9..9`, and twenty cases with exponents of 100 and 5000 bits:
     the expected status is computed in the test with exact arithmetic (`fmpq`: is `|m 2^e| > r 2^f`; for the
     class also `m > 0`), AND compared with what the public constructor says for the same `arb` ball
     (`adf_idele_set_parts`, `adf_idclass_set_parts` return `OK` exactly when the loader must): three verdicts
     that must agree on every case; on `OK` the dump of the loaded value equals the text;
   - review F5: the two faults of the radius mantissa that the two new test files do not notice: the texts
     `adf1 Q idele 1 1 40 40000001 0 1 1 1 0` (a 31-bit radius mantissa) and `adf1 Q idele 1 1 40 2 0 1 1 1 0`
     (an even radius mantissa) are `DOMAIN`; the same for the class and for a partial ball with tag `r` and
     `c` (in `tests/test_dump_local.c`).
2. **The repair** of `dp_arb_sign`: the review proposes two patches (F18, F19 in `ft/faults.py`). Check them
   before you take them: prove in a comment at the function, in five lines, why in the same binade with
   `bm > br` equality of the leading `br` bits means `|m| 2^me > rm 2^re` (and what holds for `bm == br`,
   `bm < br`). Then run the review's own differential check against the repaired library:
   `lanes/u-review1/w/gen.py idele 900 11` and `gen.py idclass 900 12` through `fz.c` as the review's
   `progress.md` shows: 0 mismatches (before the repair: 41 and 6).
3. **No abort reachable from a text.** A loader reads text from outside the program. In the twenty functions
   of lane u-dump1 find every `flint_abort` (or assertion in the release build) that a text could reach if a
   stage-6 check were wrong, as at line 2229: replace it by a returned `ADF_DOMAIN` with the output untouched,
   and keep the assertion under `ADF_CHECK_INVARIANTS` only (look at how the loaders of lane m1-dump for
   `rat`, `fball`, `adele` treat the same situation and say what you found; do not change those). Test: with
   fault F01, F15 and F17 of `ft/faults.py` planted (they made the test program abort), the test program now
   ends with failed checks and a count instead of a signal in the release build.
4. **Review F3** (the exponents of a dumped local ball): `include/adelefeld/lball.h` bounds the exponents of a
   local ball by `ADF_LBALL_EXP_MAX`, and the text reader returns `ADF_LIMIT` above it (`text.h`, the comment
   of `adf_lball_set_str`). What does the dump loader do with `v` or `N` between that bound and `LONG_MAX`
   when the caller raises `max_prec`? If it accepts a value that no public constructor can make, it must
   refuse it as the text reader does (`ADF_LIMIT`), for `lball` and inside `sball`; then `LONG_MIN` needs no
   special case. Test both sides of the bound. If the header does not make the bound part of the type, leave
   the code and report what the header says.
5. **Review F4**: the driver's `load` trims blanks around its operand although `tools/adf/README.md` says it
   does not. Read how the driver splits a line: if every command's operands are trimmed by the line reader,
   correct the README sentence; if `load` alone can pass its bytes through, do that. One fixture line either
   way (`tests/driver/u-dump-units.cmd` and `.out`, appended).
6. The fault table: all of F01 to F17 of `ft/faults.py` (copy the script into your lane directory) against the
   two test programs after the repair: F12 and F13 are detected now; none is lost.

**You own:** `src/dump.c` (the functions of lane u-dump1 and `dp_arb_sign`), the comment blocks of the twenty
declarations in `include/adelefeld/dump.h`, `tests/test_dump_units.c`, `tests/test_dump_local.c`,
`tests/driver/u-dump-units.cmd` and `.out`, `tools/adf/adf.c` (the `load` command only) and
`tools/adf/README.md` (its sentence on `load`), `lanes/u-repair1/`. Everything else is read-only. No git
command that changes state, no `bd`. Other lanes are writing `src/qclass.c`, `src/text.c`, `docs/api-3.md`,
`tools/adf/adf.c` (a new command `qreduce`) in other worktrees: keep your driver change to the `load` command.

**Checks at the end** (commands and numbers in the report; at most 2 jobs; every program under `timeout`, none
over 170 s): `timeout 1500 make -j2 check` (the count of test programs); `timeout 900 sh tests/test_driver.sh`;
the two test programs built with `CC=clang`, `INV=1` and `SAN=1` in build directories under your lane
directory, no warning, no sanitizer report with `ASAN_OPTIONS=detect_leaks=1`. Files end with a newline; lines
at most 116 characters. Remove your build directories at the end; leave no files in `/tmp`.

Report: `lanes/u-repair1/report.md` (rule 8 of `lanes/COMMON.md`): per part what is done; the red and green
runs; the differential counts; the fault table; what you decided in parts 3, 4, 5 and why; what is not done.
