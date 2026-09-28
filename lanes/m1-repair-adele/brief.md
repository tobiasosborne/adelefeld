# Lane m1-repair-adele: findings R3, R4 and part of R6 of reviewer `arith`; issue adf-eal

Read `lanes/COMMON-C.md`. Then `docs/reviews/m1/arith/review.md` completely, its reproducers
`docs/reviews/m1/arith/checks/adele_prec1.c`, `swap_equiv.c`, `citations.py` with their outputs,
`include/adelefeld/adele.h` (changed today: decision M1-D4: a `prec` below 2 is taken as 2; products and
quotients with an exact rational `n/d` use the integers `n` and `d`), `docs/SPEC.md` section 15 row M1-D4,
`src/adele.c`, `tests/test_adele.c`, `tests/test_cadele.c`, `tests/test_adele_prec.c`,
`refs/src/flint-3.0.1/arb.rst` and `acb.rst` for `arb_mul_fmpz`, `arb_div_fmpz`, `arb_add_fmpz`,
`acb_mul_fmpz`, `acb_div_fmpz` (cite file and line).

**You own:** `src/adele.c`, `tests/test_adele_lowprec.c` (new), `src/rat.c` and `src/fball.c` for comments
only (no statement of those two files changes), `lanes/m1-repair-adele/`.

1. Red first: `tests/test_adele_lowprec.c`: for `prec` in 1, 0, -5 and for 2, 3, 53: `div_rat`, `mul_rat`,
   `add_rat`, `set_rat`, `add`, `sub`, `mul` of `adf_adele` and `adf_cadele`, with `q` in `1/3`, `-1/3`,
   `3`, `7/1024`, a rational of 4096 bits, and with the output aliasing the input: the result satisfies
   `is_canonical` (its real part is finite), contains the exact value (`arb_contains_fmpq` on the exact
   rational computed with `fmpq`), and the result at `prec` below 2 is identical to the result at
   `prec = 2`. With the code as it stands `div_rat` at `prec = 1` fails (a non-finite real part).
2. Then the code: a static helper clamps `prec`; `mul_rat` is `arb_mul_fmpz` by the numerator and
   `arb_div_fmpz` by the denominator; `div_rat` by `q = n/d` is `arb_mul_fmpz` by `d` and `arb_div_fmpz` by
   `n` (the sign of `n` is carried by the integer; `n = 0` is `ADF_NOT_UNIT` before); `add_rat` adds a ball
   of `q` at the clamped `prec` (an exact rational has no exact sum with a ball in FLINT 3.0.1; say so).
   Show in the report, for 1000 random inputs at `prec = 53`, how the radius of the new `mul_rat` and
   `div_rat` compares with the old one (smaller, equal, larger: counts). The HEADER-FINDING comment of the
   file on `arb_add_fmpq` is replaced by what the header now says.
3. Comments, finding R6 of the review: the citations in `src/rat.c:15`, `:225` (`fmpq_sub` is
   `fmpq.h:212`), `src/adele.c:30-32` (`arb.rst:9-11`), `src/adele.c:110-112` (`arb.rst:25-26`), and the
   stale text and the dead `#pragma weak` with its two guards in `src/fball.c` near lines 29 to 53 and in
   `adf_fball_prec_at` (the place functions are always linked; removing the two guards is the one change of
   a statement of `src/fball.c` that you may make; all tests must pass unchanged). Check every line number
   you write against the file on disk.
4. `make -j2 check`, `make clean && make -j2 check SAN=1`, `make clean && make -j2 check CC=clang`; the
   reproducer `adele_prec1.c` of the review built against the repaired library, with its new output.
5. `make mutate FILES=src/adele.c JOBS=2 LIMIT=300`: survivors killed by tests or listed in the report with
   the reason (do not edit `tools/mutate/equivalent.txt`). On finding R4: a mutant that exchanges the
   operands of `arb_mul` gives another ball that is also an enclosure; the library promises an enclosure,
   not a particular ball; list such mutants under that reason, which is the true one.

## Resume note of the orchestrator (2026-09-28, 15:50)

This lane was stopped from outside when the machine ran low on memory. It was not your fault and nothing is
lost: your files are in the worktree as you left them, uncommitted. Before anything else run `git status`
and `git diff --stat`, read the logs in your lane directory, and find the first item of the brief that is
not finished. Do not start again from the beginning and do not rewrite what works. Memory: never run two
builds or two mutation runs at the same time; use `make -j2`; run a mutation run in the foreground, not
with `nohup`; if `free -g` shows less than 6 GB available, wait. Finish with `report.md`.
You changed one record of `tests/ref/vectors/m1-adele/set_rat.jsonl` (3/4 at `prec = 1`, `exact` false to
true). The orchestrator has seen it and accepts it as a consequence of decision M1-D4; say in the report
why it is right (the odd mantissa 3 has 2 bits and `prec = 1` is taken as 2), and change no other record.
