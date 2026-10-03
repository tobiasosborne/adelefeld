# Lane f-slice11: `Log` on ideles (milestone 1F, WP 1F.8, third part), from the design of lane d-idlog

The design is done and proved: `docs/design/idele-log.md` (statements IL1 to IL8; section 2: six declarations
with their comment blocks and the common contract; section 4: the test plan; section 6: five open points) with
its oracle in exact integers `proto/idlog_checks.py` (`expected_ball`, `expected_refinement`, the fixture
writer). Read the design in full before anything else: it is your specification. The orchestrator took the five
recommendations of its section 6 (to be recorded as N-D17): the results are an `adf_adele` for the all-places
forms and an `adf_sball` for a single place; `Log` on a positive real ball and a separately named `log_abs`;
the conservative finite ball `0 + 4 Zhat` also for exact inputs, the exact-zero shortcut only in the `_at` form;
no public local-component function now, an internal descriptor; compact centres at `min(N, E)` and the two
explicit bounds.

You build it as thin working slices that a user can call (library, driver, Julia), ONE AFTER THE OTHER, each
complete end to end before the next starts:

- **Slice A:** `adf_idele_Log`, `adf_idele_log_abs` (the conservative all-places forms) and `adf_idele_Log_at`,
  `adf_idele_log_abs_at` (one named place: the exact local image by IL2 to IL4, `K = min(N, E)`). Driver
  commands in the pattern of the existing ones (`Log X`, `logabs X`, `Log_at X with PLACE`; the names `Log`,
  `log` are free; `tools/adf/adf.c` has `root`, `exp` from lane f-slice10 as the newest pattern), README, a
  Julia example (an idele with modulus 6: `Log` at all places; refined at 2, 3 and at 5, a prime not dividing
  the modulus, where the image is `5 Z_5` plus the contribution of the content).
- **Slice B:** `adf_idele_Log_refine`, `adf_idele_log_abs_refine` (the all-places enclosure refined at a list of
  named primes by integer CRT, IL5; the bounds of the design). Driver `Log_refine X with P1,P2,...` or the form
  you justify; Julia.

Another lane (f-review9) is reviewing `gfunc.h` and `src/gfunc.c` as lane f-slice10 left them, and a repair lane
may follow: so put your code in a NEW source file `src/gfunc_log.c`, your declarations in a NEW block at the END
of `include/adelefeld/gfunc.h`, your tests in a NEW file `tests/test_gfunc_log.c`, and touch nothing else of
`gfunc.h` and nothing of `src/gfunc.c` (if you need one of its static helpers, copy at most 30 lines with a
comment naming the origin, and say so in the report).

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold for
your block of `gfunc.h`; rule 5 as written, with the note in item 4 below), `docs/design/idele-log.md`,
`lanes/d-idlog/report.md` (its two findings against the draft), `proto/idlog_checks.py`, `docs/SPEC.md` 9.3.2
(lines 574-602), 15.4 (N-D8, N-D10, N-D14, N-D16), `docs/proofs/functions.md` Lemma 9, Propositions 10 to 12,
`docs/proofs/ideles.md` Definition 4, Lemmas 5 to 7, `docs/conventions.md` 2.1, 2.2, 3.1 to 3.3, 5.6 to 5.9,
`include/adelefeld/gfunc.h` and `src/gfunc.c` (the pattern of an all-places function: the real part through
the real-place function, the combination of statuses, `where`), `idele.h`, `ucoset.h`, `lball.h`, `lfunc.h`
(`adf_lball_Log`, F6), `sball.h`, `rfunc.h` (`Log_at`, `log_abs_at`), `adele.h`, `fball.h`,
`tests/test_gfunc.c`, `lanes/f-slice10/result.md` ("For the next lane"), `tools/adf/adf.c` and
`tools/adf/README.md` (the commands `root`, `exp`, `exp_at`, `Log`-free names), `tests/julia/gfunc.jl`,
`tests/test_julia.sh`. `refs/src/` is on disk (FLINT 3.0.1 under `refs/src/flint-3.0.1/`); cite by file and line.

**You own:** `src/gfunc_log.c` (new), `include/adelefeld/gfunc.h` (a new block at the end only),
`tests/test_gfunc_log.c` (new), `tests/julia/gfunc_log.jl` (new), `tests/ref/vectors/f-slice11/` (new, below
1 MB: a SELECTION of the oracle's rows; the full fixture file of 3 MB is not committed), `proto/idlog_checks.py`
(only to add a writer of your selection or to repair a defect you prove; say what you changed),
`docs/api-1f8.md` (a new section at the end: statements G7 ... that tie each function to IL1 to IL8, with the
proof of everything the design left to the implementation: the descriptor, the working precisions, the limits),
`tools/adf/adf.c`, `tools/adf/README.md`, `tests/driver/gfunc-log*.cmd` and `.out` (new), `lanes/f-slice11/`.
In `tests/test_julia.sh` you may add the lines your file needs. Everything else is read-only. No git command
that changes state, no `bd`. At most 2 cores (other lanes and another session share this laptop); every program
under `timeout`; build into `BUILD=lanes/f-slice11/build` while you work and into `build/` only for the final
checks. Julia is on the PATH.

Order of work, for slice A and then again for slice B:
1. Header first: the comment blocks of the design, corrected where the design's contract cannot be implemented
   as written (say where and why; a contradiction between the design and the library's existing functions is a
   finding with its input).
2. Tests first, red then green (`lanes/f-slice11/redgreen.log`; a link error counts only for the first test).
   The oracle is `proto/idlog_checks.py` (exact integers; its stated precision `H`): the selection of fixture
   rows must cover every case of IL2, IL3, IL4 (restricted odd primes, `p = 2` with `k >= 2` and `k = 0, 1`,
   unrestricted primes, exact units with both signs, contents of positive and negative valuation, `N < E`,
   `N = E`, `N > E`, the stored non-normal input `(c, M) = (1, 2)` of the design's finding); the real part is
   checked against the real functions of `rfunc.h` (identical ball) and against `mpmath` at a higher precision
   (containment); every status of the design with `where` and the outputs untouched; every aliasing the
   contract permits; primes 2, 3, 5, 7, 65537 and `2^64 - 59` (at the two large primes the oracle cannot
   enumerate: use the identity `Log(x y) = Log x + Log y`, the comparison with `adf_lball_Log` on the restricted
   ball, and witnesses, and say what precision each comparison has); the limits (`LIMIT`, outputs untouched).
3. The code; the driver commands with golden files (expected lines written BY HAND from the oracle before the
   run); the Julia example.
4. The eight faults of the design's test plan, and four of your own, planted in a scratch copy under your build
   directory, each with the test that fails. Mutation testing of `src/gfunc_log.c` (at most 60 mutants,
   `--seed 1`, 2 jobs, `--san`, `timeout 1300`); the tool was repaired yesterday (`lanes/m-tool2/report.md`):
   read its last line, and say how many mutants compiled.
5. Final checks, once, at the end: `timeout 900 make -j2 check-all` in `build/` (this one run may take 10
   minutes); one `ASAN_OPTIONS=detect_leaks=1` sanitizer build of `test_gfunc_log` in `BUILD=build/san` and its
   run. Give the last line of each.

If you run short of time, finish slice A completely rather than leaving both half done, and say what of slice B
is missing. Decisions where the design is silent are yours, listed with the alternatives in the report.

Report: `lanes/f-slice11/report.md`, written ONCE, AT THE END; running notes in `lanes/f-slice11/progress.md` as
you go (slice A recorded as finished before slice B starts). In it: functions built; decisions and
alternatives; what is proved and what is not; the numbers of the tests and what would have made a case fail;
the fault table; mutation survivors one line each; findings against the design, the specification or this
brief; what is not done.
