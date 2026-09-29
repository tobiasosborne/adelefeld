# Lane f-repair2: repairs of partial balls and real functions after the review f-review2

The review `docs/reviews/f1/review-sball-rfunc.md` (codex gpt-6.1-sol) closed F1 to F6 of the first
review, found no wrong enclosure, and has the findings R1 to R8. Its programs are in `lanes/f-review2/`
(`edge.c` with the modes `limits`, `precedence`, `s5`; `edge_runs.py`; `julia_leak.jl`; `text_checks.py`).
Build them as the report says and see each finding of yours reproduce BEFORE you change anything (the
red run; log it in `lanes/f-repair2/redgreen.log`).

Decisions of the orchestrator:
- R1, R2 (`sub` and `div` of `adf_lball` return `LIMIT` where a power of `p` of more than
  `ADF_LBALL_BITS_MAX` bits would be formed to align or to reduce, although the stored result is small):
  NOT yours. The rule of the header is reworded by the orchestrator after another lane has landed
  (`lball.h` and `src/lball.c` belong to lane f-slice3 at this moment). Do not touch these files.
- R3 (a complex component masks the `LIMIT` of a prime): repair. The combined status of a partial-ball
  operation is the one of `docs/conventions.md` 3.3 (read it; the reviewer cites line 211): every
  component is inspected, `where` is the place of the status returned. Correct the sentence of
  `sball.h` that says the statuses cannot mix.
- R4 (six entry paths of `adf_sball` without the check of `ADF_CHECK_INVARIANTS`): repair as
  conventions 4.4 says, as lane f-repair1 did for `adf_lball` (`lanes/f-repair1/result.md`; the test
  with one forked child for each case in `tests/test_lball.c` is the pattern). The self branch of
  `set` checks too.
- R5 (`prec = LONG_MAX` aborts in `arb`): a limit. `ADF_REAL_PREC_MAX 2097152` in `rfunc.h` (the value of
  `ADF_ROOTS_REAL_PREC_MAX` and `ADF_IDELE_PREC_MAX`); every function of `rfunc.h` and every function
  of `sball.h` that takes a `prec` returns `ADF_LIMIT` above it, decided from `prec` alone before any
  allocation and before every other status, outputs untouched, `where` the archimedean place where
  the function has a `where`. The sentence "the caller's error" goes.
- R6 (`tests/julia/sball.jl` leaks its temporary `arb`, and does not clear its adeles and `fmpz`):
  repair; the Julia test clears what it makes.
- R7: the paragraph on the stored centre of L4a in `docs/api-1f.md` section 1 gets its exception for
  the centre 0 (one sentence; the quotient formula is not changed).
- R8: S5 of `docs/api-1f.md` gets the exception of a result that is not finite (as S7 says).

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold for `sball.h` and
`rfunc.h`; rule 5 is replaced by item 3 below), the review, `docs/conventions.md` 3.3 and 4.4,
`lanes/f-slice2/result.md`.

**You own:** `src/sball.c`, `src/rfunc.c`, `include/adelefeld/sball.h`, `include/adelefeld/rfunc.h`,
`tests/test_sball.c`, `tests/test_rfunc.c`, `tests/julia/sball.jl`, `docs/api-1f.md` (ONLY the paragraph
of L4a named in R7 and the section of slice 2; another lane adds a section at the end of the file),
`lanes/f-repair2/`. Everything else is read-only.

1. Tests first, red then green: R3 (the reviewer's input and the mirrored ones: `DOMAIN` against
   `UNSUPPORTED`, two primes with different statuses), R4 (INV build only), R5 (each function at
   `ADF_REAL_PREC_MAX`, which must work on a small input, and at `ADF_REAL_PREC_MAX + 1` and `LONG_MAX`).
2. The repairs. No change of a result on inputs that gave `OK` before: the vectors of
   `tests/ref/vectors/f-slice2/` pass unchanged.
3. No mutation run. For R3 and R5 the old code is the fault that the new tests catch (the red run).
4. `make clean && make -j2 check-all`, `make clean && make -j2 check SAN=1`,
   `make clean && make -j2 check CC=clang`, `make clean && make -j2 check INV=1`,
   `sh lanes/m1-headers/check_headers.sh` pass, each under `timeout 900`; give the last line of each.
   Then the reviewer's programs again, with what each prints.

Result: `lanes/f-repair2/result.md` and the same text as your final message: each finding with what
was changed and the test that shows it; what is not done. Leave no compiled binary in your lane
directory.
