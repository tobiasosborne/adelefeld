# Lane n-review2: adversarial review of the repairs of n-repair1 (the printer bound N-D11 in particular)

Your task is to REFUTE, not to confirm. Lane n-repair1 (`lanes/n-repair1/result.md`, Claude Sonnet, commit
`0f023a9`, merged `b5ad7f3`; the finding list is `docs/reviews/f1/review-slices-3-6-text.md`) repaired one
BLOCKER and four findings in one hour and has not been reviewed. Under review:
- D2, decision N-D11 (`docs/SPEC.md` 15.4 row N-D11; `docs/api-2.md` 4.2 "The bound on the work" and
  Statement Q; `include/adelefeld/text.h` the paragraph "(2) Decision N-D11"): the constrained printer of
  ideles and classes (`src/text.c`, `TX_COND_WORK_MAX = 2^25`, `tx_put_real_cond`, `adf_tx_write_unit_form`)
  counts bit-levels and returns NULL with `*len = 0` past the bound; `adf_idele_get_str`,
  `adf_idclass_get_str` (`src/text_idele.c`), the driver prints `error: LIMIT`. The test is
  `constrained_printer_ends_in_bounded_time` in `tests/test_text_idele.c` (its wall-clock guard was 2 s
  and is 30 s since `995960d`: 2.75 s on battery).
- D1 (BLOCKER, driver): `adf_drv_places` in `tools/adf/adf.c` refuses every operand type other than
  rational, finite ball, adele for `project`, `exp_at`, `log_at`; golden file `tests/driver/f-cross.cmd`.
- C1: `adf_lball_decompose_teich` (`src/lball_decomp.c`) tests the exponent bounds before `N - v`; the
  test `inputs_beyond_the_exponent_limit_are_limit_not_overflow` in `tests/test_lball.c`.
- R5: `prec > ADF_REAL_PREC_MAX` tested before the entry checks in `src/sball.c` (`add`, `sub`, `mul`) and
  `src/rfunc.c` (`at_place`).
- C2: `fball_limit_certain` in `src/lball.c`: the second upper bound of `v_p(H)` from
  `bits(H) / log2(p) + 1` in double arithmetic.
Contract: the headers named; `docs/conventions.md` 9.5 (the constrained form), 3.1, 4.3; `docs/api-2.md`
4.2 to 4.4; `docs/SPEC.md` 15.4 (M1-D6, N-D7, N-D8, N-D11).

**You own:** `lanes/n-review2/` only. Everything else is read-only. No git command that changes state, no
`bd`. At most 2 cores. Build with `make -j2 BUILD=lanes/n-review2/build` ONCE and link your programs
against that archive; do not run the test suites of the repository (the orchestrator has run them).

**Battery.** The machine runs on battery today: no program over 60 s, every program under `timeout`, a
few thousand inputs per attack, not millions; no sanitizer build of the whole library (compile only your
reproducers with `-fsanitize=address,undefined` against the normal archive if you need it; the codex
sandbox cannot run LeakSanitizer: `ASAN_OPTIONS=detect_leaks=0`).

## What counts as a finding

- N-D11, the bound: a ball admitted by M1-D6 (both exponents at most `ADF_PRINT_EXP_MAX`) on which the
  printer does not end within a few seconds although the counter should have stopped it (a level whose
  cost is not counted; a pass outside the counted region; the counter reset between passes; a loop
  before the first level); a counter that overflows or is compared wrongly; a text of fewer than the
  bound's levels that is refused (NULL where the text is small: find the smallest refused ball of a
  family and compare with `2^25 / S`); a text where the search passed the bound (the counter is
  checked after the work, so the bound may be exceeded by one level: say by how much at most);
  `*len` not 0 on NULL, or a leak on the NULL path (the buffer freed, the levels' integers cleared);
  NULL from `adf_idele_get_str` where `adf_idclass_get_str` prints the same real part, or the reverse;
  the real part exactly representable (radius 0, midpoint an integer or a dyadic) with huge exponents:
  does the counter engage although no search is needed? A statement of Statement Q, of the header
  paragraph, or of row N-D11 that the code does not keep (the "about 300 levels at 10^5 bits";
  "every other ball is printed by the same algorithm as before": show one ball below the bound whose
  text changed, if there is one, by comparing with the printer BEFORE the repair, `src/text.c` and
  `src/text_idele.c` at commit `674c9db` obtained with `git show` and built under renamed symbols, on a
  few hundred random balls below the bound).
- The monotonicity claim in Statement Q: the repair says "level k satisfies" is NOT known to be monotone
  and the levels are not nested. Either prove monotonicity from some level on (then a logarithmic
  search is possible and N-D11 could be replaced; write the proof stepwise) or give a ball and two
  levels `k1 < k2` where `k1` satisfies the condition and `k2` does not (a counterexample that settles
  the question). This is the one item of theorem grade in this review: do it first.
- D1: a driver command that still reads a field of the wrong type for some operand kind (unit coset,
  idele, class, complex adele in the operand position of every command of `tools/adf/README.md`), seen
  as a printed value that is not `error: UNSUPPORTED`, `error: DOMAIN` or `error: PARSE`. Use the driver
  binary from your build (`make -j2 BUILD=lanes/n-review2/build` builds `tools/adf`? read the Makefile
  and `tools/adf/Makefile`; build the driver into your lane directory).
- C1: another `N - v`, `N + M`, `N - 2v`, `k * v` in `src/lball.c`, `src/lball_decomp.c`, `src/lfunc.c`,
  `src/sball.c`, `src/rfunc.c` reached with operands near `LONG_MAX` or `LONG_MIN` before a bounds test
  (read, then run with `-fsanitize=undefined` on your reproducers; the balls around 0 with huge `N`
  and pairs of one huge and one small ball were NOT tested by the lane: start there).
- R5: a function of `sball.h`, `rfunc.h`, `lfunc.h` that allocates or reads its inputs before
  `LIMIT` at `prec > ADF_REAL_PREC_MAX` (the allocator counter of `INV=1`: `lanes/i-repair1` says how).
- C2: an `H`, `d`, `q` for which the double bound `bits(H)/log2(p) + 1` is BELOW `v_p(H)` (then
  "certain" would be wrong, not just slow): prove it cannot happen for `p < 2^64` and `bits(H)` up to
  `BITS_MAX`, with the rounding of `log2` and of the division made explicit, or find the input.
- A test of the lane that cannot fail; a sentence of a header that the code does not keep.

## Report

`lanes/n-review2/report.md`, written once, at the end; running notes in `lanes/n-review2/progress.md` as
you go (if the session is cut off, they are the report). For each finding: severity (BLOCKER: a wrong
printed value, a wrong enclosure, a memory fault, undefined behaviour; MAJOR; MINOR), the input, what
the code returns, what is true and why, and the command that reproduces it with a program in your lane
directory. The monotonicity item: the proof or the counterexample, with its numbers. Then what you
attacked without result, with counts and what would have made a case fail. No praise, no summary.
