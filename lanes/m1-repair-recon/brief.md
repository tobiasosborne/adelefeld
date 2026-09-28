# Lane m1-repair-recon: findings R1, R2 and part of R6 of reviewer `arith`

Read `lanes/COMMON-C.md`. Then `docs/reviews/m1/arith/review.md` completely, its reproducers
`docs/reviews/m1/arith/checks/recon_huge_exp.c`, `recon_big_alloc.c`, `recon_mut111.c` with their outputs,
`include/adelefeld/recon.h` (changed today: decision M1-D3, the status `ADF_LIMIT` and the constant
`ADF_RECON_EXP_MAX`), `docs/SPEC.md` section 15 row M1-D3, `src/recon.c`, `tests/test_recon.c`,
`refs/src/flint-3.0.1/arb.rst` lines 455 to 480, `arf.rst` and `mag.rst` on the exponent of an `arf` and of
a `mag` (and `/usr/include/flint/arf.h`, `mag.h`: `ARF_EXPREF`, `MAG_EXPREF`, `arf_is_zero`, `mag_is_zero`).

**You own:** `src/recon.c`, `tests/test_recon.c` (append tests; change no existing test),
`tests/test_recon_limit.c` (new), `lanes/m1-repair-recon/`.

1. Red first: `tests/test_recon_limit.c` with the inputs of the review (the real ball `3 * 2^(2^64+1)` with
   finite part 6; `2^-(2^64+3)` with finite part 1/8; `2^(2^64)` with 1; exponent `-2^63`; `2^(2^36)` with
   finite part 1; `1 +/- 2^(-2^36)` with finite part 1), built with `arb_mul_2exp_fmpz` and
   `mag_set_fmpz_2exp_fmpz` or `mag_mul_2exp_fmpz`. Each must return `ADF_LIMIT` with the output untouched,
   within a second, and under `ulimit -v 2000000` (run the test program that way from a script in your lane
   directory and report). Also the boundary: exponents exactly `ADF_RECON_EXP_MAX` and one above, for the
   midpoint and for the radius, both signs; at the bound the function must return the right status of
   OK, NO_SOLUTION, NOT_UNIQUE (work out each expected answer by hand and say how); a midpoint of zero with
   a radius, a radius of zero with a midpoint, both zero. See the tests fail (wrong status, or abort).
2. Then the code: test the exponents before `arb_get_interval_fmpz_2exp` is called; no `fmpz_get_ui` or
   `fmpz_get_si` on an exponent that was not first shown to fit; no negation of a `slong` that may be
   `WORD_MIN`. Remove the comment that says such exponents are not reachable.
3. `fmpq_canonicalise` in `fmpq_set_dyadic` stays (FLINT's `fmpq` functions require canonical input,
   `fmpq.rst:21-26`); say so in a comment at the call. Bring the comment at `src/recon.c:50-61` on the local
   backend up to date (the local backend exists; `adf_fball_get_fmpz3` gives the canonical triple) and add
   the warning of `arb.rst:468-474` to the citation at lines 40 to 43.
4. `make -j2 check`, `make clean && make -j2 check SAN=1`, `make clean && make -j2 check CC=clang`; the
   three reproducers of the review built against the repaired library, with their new output.
5. `make mutate FILES=src/recon.c JOBS=2 LIMIT=300`: survivors killed by tests or listed in the report with
   the reason (do not edit `tools/mutate/equivalent.txt`).

## Resume note of the orchestrator (2026-09-28, 15:50)

This lane was stopped from outside when the machine ran low on memory. It was not your fault and nothing is
lost: your files are in the worktree as you left them, uncommitted. Before anything else run `git status`
and `git diff --stat`, read the logs in your lane directory, and find the first item of the brief that is
not finished. Do not start again from the beginning and do not rewrite what works. Memory: never run two
builds or two mutation runs at the same time; use `make -j2`; run a mutation run in the foreground, not
with `nohup`; if `free -g` shows less than 6 GB available, wait. Finish with `report.md`.
