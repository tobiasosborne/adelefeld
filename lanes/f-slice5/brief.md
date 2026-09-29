# Lane f-slice5: the cost of the series at a prime (after review f-review3, finding R1)

The review `docs/reviews/f1/review-lfunc.md` found no wrong enclosure in `src/lfunc.c` and one MAJOR
finding on cost: at the prime `p = 2^64 - 59` and `N = 10000`, `log(1 + p)` takes 59 to 65 s and
`Log(2)` does not end in 120 s. The reviewer's programs: `lanes/f-review3/` (`timing.c`, `probe.c`, its
oracle and `oracle-proof.md`). The author of `src/lfunc.c` named the avoidable costs himself
(`lanes/f-slice4/result.md`, "Not done"): one `fmpz_invmod` modulo the full working power for every
term of `log`; a second reduction in every step; every term computed modulo the full `p^W` although
the term `z^k/k` with `v(z) = v` is divisible by `p^(k v - v_p(k))` and needs only the remaining digits;
no splitting; and `Log` at odd `p` forms `a^(p-1)` with an exponent of 64 bits at full precision.

This is performance work on numerical code whose correctness is proved and reviewed: the result of
every call must stay IDENTICAL (same centre, same exponent, same exact flag, same status), which the
existing vectors and tests check. Read `docs/PERF.md` first (a lower bound before a measurement), then
`docs/api-1f4.md` (F1 to F7), `docs/proofs/functions.md` Propositions 7, 7b, 8, `src/lfunc.c`.

Decisions of the orchestrator:
- The target: `exp`, `log`, `Log` at `N = 10000` at a prime of one word in under 2 s on this laptop,
  and no slower than now at small `N`. Measure first where the time goes (the reviewer's two inputs,
  and `p` = 2, 3, 5 at `N` = 2000, 10000, 100000), state the lower bound in the sense of `docs/PERF.md`
  (the size of the output, one multiplication modulo `p^N`), and FLINT's `padic_exp`, `padic_log` as
  the comparison row.
- Methods, in the order of their gain for their risk; stop when the target is met: (1) a decreasing
  modulus for the terms (the term of degree `k` is computed modulo the power of `p` it needs; state and
  prove the working precision of each term as a statement F8 in `docs/api-1f4.md`); (2) the inverses
  of the small integers `k / p^e` by a division of word size or a batch, not by `fmpz_invmod` modulo
  the full power; (3) for `Log` at odd `p`: the power `a^(p-1)` formed once, modulo the power it needs;
  or the Teichmueller route through `adf_lball_teichmuller` (`lball.h`, lane f-slice3, on master) if
  it is cheaper: measure; (4) binary splitting or a rectangular scheme only if (1) to (3) miss the
  target; then read FLINT's documentation of its own method (`refs/src/flint-3.0.1/padic.rst`) and cite.
- FLINT's `padic` stays out of the library (N-D9).

Read `lanes/COMMON.md` and `lanes/COMMON-C.md` first (rule 6 does not hold for `lfunc.h`: you may change
its sentences on cost; rule 5 is replaced by item 3 below; rule 7 "do not optimise" does not hold
here).

**You own:** `src/lfunc.c`, `include/adelefeld/lfunc.h` (sentences on cost only), `docs/api-1f4.md` (new
statements at its end and the sentences on cost), `tests/test_lfunc.c` (additions), `bench/bench_lfunc.c`
(new), `docs/PERF.md` (a row for the series at a prime), `lanes/f-slice5/`. Everything else is
read-only. No git command that changes state, no `bd`. At most 2 cores; every program under
`timeout`; build into `BUILD=lanes/f-slice5/build` while you work and into `build/` only for the final
checks.

1. Tests first: a test with a bound on the time (the reviewer's two inputs, 5 s each as the constant
   of the test, with a comment), red on the present code; a test that compares the results of 2000
   random calls (all three functions, `p` = 2, 3, 5, 7, 11, `2^64 - 59`, `N` up to 3000, balls and exact
   inputs, negative valuations for `Log`) with results STORED from the present code (generate the
   stored results first, with the code as it is on master, into `tests/ref/vectors/f-slice5/`, below
   1 MB): identical fields.
2. The changes, one method at a time, each measured.
3. Show that the new comparison test bites: three planted faults in a scratch copy (a working
   precision of a term one digit too small; an inverse of the wrong integer; the decreasing modulus
   applied to the sum and not to the term). No mutation run.
4. `make clean && make -j2 check-all`, `make clean && make -j2 check SAN=1`,
   `make clean && make -j2 check CC=clang`, `make clean && make -j2 check INV=1`,
   `sh lanes/m1-headers/check_headers.sh` pass, each under `timeout 900`; give the last line of each.
   The reviewer's probe suite again (`lanes/f-review3/`, as its report says how): 0 differences.

Report: `lanes/f-slice5/report.md`, written ONCE, AT THE END; running notes in
`lanes/f-slice5/progress.md`. In it: the table before and after, the distance from the lower bound and
from FLINT, what is proved (F8), what is left.
