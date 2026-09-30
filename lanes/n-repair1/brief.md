# Lane n-repair1: repairs after review n-review1

The review `docs/reviews/f1/review-slices-3-6-text.md` (assembled from the evidence in `lanes/n-review1/`)
has one BLOCKER and three MAJOR findings, and one OPEN item of an earlier review. See each reproduce
BEFORE you change anything (`lanes/n-repair1/redgreen.log`); the programs and inputs are in
`lanes/n-review1/` (`driver-cross.cmd` with `.expected`; `edges.c`; `partial_alloc.c`; `printer-worst.log`
names the ball).

Decisions of the orchestrator:
- D1 (BLOCKER, driver): `project`, `exp_at`, `log_at` (and every other command of `tools/adf/adf.c` that
  takes an adele-like operand) check the kind of the operand and answer `error: UNSUPPORTED` for a unit
  coset, an idele or a class where the README does not admit them. Add the 21 lines of
  `driver-cross.cmd` as a case `tests/driver/f-cross.cmd` with the expected output of the review.
- C1 (MAJOR): every function of `lball.h` that forms `N - v`, `N + M` or any sum of two exponents tests
  the limits of N-D4 (`|v|, |N| <= ADF_LBALL_EXP_MAX`) on its inputs FIRST, and returns `LIMIT` (the rule
  of the header). Audit `src/lball.c` and `src/lball_decomp.c` for every such expression; a test with a
  canonical ball `v = -1`, `N = LONG_MAX` (and `v = LONG_MIN + 1`) through every public function, built
  with UBSan (`make check SAN=1` runs UBSan: the test is in the suite).
- D2 (MAJOR): the constrained printer of conventions 9.5 in `src/text.c` (lane t-slice1) must end in
  bounded time on every ball that M1-D6 admits. Decide the method: a proved lower bound on the level
  `k` from the exponents of midpoint and radius (write the statement and its proof in `docs/api-2.md`
  section 4), or a search that is logarithmic in the level, or a bound on the levels tried after which
  the printer returns NULL (then the driver prints `error: LIMIT`, M1-D6: say so in the header and in
  N-D11 in your result). The worst admitted input (`2^99999 + 1/2 +/- 2^99999`) must print or refuse in
  under 2 s; the golden and the reference tests must pass unchanged.
- R5 OPEN (MAJOR): in `src/sball.c` and `src/rfunc.c`, the test of `prec` against `ADF_REAL_PREC_MAX`
  comes first, before the entry check of `ADF_CHECK_INVARIANTS` and before reading any input (as lane
  i-repair1 did for ideles: `lanes/i-repair1/result.md` F1; its allocator-counter test is the pattern).
- C2 (MINOR): `adf_lball_set_fball`: extend the early `LIMIT` of L13 to the branch where `bits(A)` alone
  is above the bound, if it is sound; else say why not.

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold for the headers
named; no mutation run), the review, `docs/SPEC.md` 15.4.

**You own:** `tools/adf/adf.c`, `tools/adf/README.md`, `tests/driver/f-cross.cmd` and `.out` (new),
`src/lball.c`, `src/lball_decomp.c`, `include/adelefeld/lball.h` (sentences), `tests/test_lball.c`,
`tests/test_lball_decomp.c` (additions), `src/text.c` (the constrained printer only), `src/text_idele.c`,
`include/adelefeld/text.h` (sentences), `tests/test_text_idele.c` (additions), `docs/api-2.md` (section 4
only), `src/sball.c`, `src/rfunc.c`, `tests/test_sball.c`, `tests/test_rfunc.c` (additions),
`lanes/n-repair1/`. Everything else is read-only.

Checks: `make clean && make -j2 check-all`, `make clean && make -j2 check SAN=1`,
`make clean && make -j2 check CC=clang`, `make clean && make -j2 check INV=1`,
`sh lanes/m1-headers/check_headers.sh`, `SAN=1 sh tests/test_driver.sh` pass, each under `timeout 900`, in
the foreground; the last line of each. The review's inputs again, with what they print now.

Result: `lanes/n-repair1/result.md` and the same text as your final message: finding by finding, what
was changed and the test that shows it; the decision on D2 with the alternatives; what is not done.
Leave no compiled binary in your lane directory.
