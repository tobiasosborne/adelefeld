# Lane r-slice2: real roots, second slice: the cost of the isolation (after review r-review1)

The review `docs/reviews/r1/review-real-isolation.md` found no wrong list and one MAJOR finding: inputs
of modest size (degree 50, coefficients of at most 10^4 bits, `prec` 2) take 11 to 45 s in the
isolation of `src/roots_real.c`, two of them with NO real root. Three MINOR findings concern the design
text and the differential script. The reviewer's programs are in `lanes/r-review1/` (`probe.c` with the
families `mignotte`, `positive`, `complex`, `linear`; `design_probe.c`; `mutate_diff.py`). This is
performance work on intricate code: read `docs/PERF.md` first (a lower bound before a measurement), then
`docs/design/real-roots.md`, `src/roots_real.c`, `lanes/r-slice1/result.md`.

Decisions of the orchestrator:
- D1. The count of FLINT (`fmpz_poly_num_real_roots`, trusted by S-D11) is computed BEFORE the isolation
  and steers it: with count 0 no isolation is run; the descent stops as soon as `count` isolating cells
  are found (cells certified by their sign change or exact root), and a subtree is not descended
  further once the roots found elsewhere account for all of them. Soundness is unchanged because the
  certificate of `real_finish` (exact signs at the end points of every ball, strict order, number of
  balls equal to the count) does not depend on how the candidates were found: write this argument as a
  proposition in the design file, with its proof, and say exactly what is trusted (the count).
- D2. The descent toward a cluster far from its own scale (one level for each bit: the family
  `2^e + 1/3, 2^e + 2/3`, ratio 747 to the floor in `lanes/r-slice1/result.md`, and the Mignotte input
  of the review, 45 s) gets a faster step. Choose between a Newton-like or quadratic step on the cell
  (Sagraloff and Mehlhorn, `refs/src/sagraloff-mehlhorn/tex/arxivfinal.tex`, the NewDsc step; read it and
  cite file and line) and a simpler device you can prove (for example a count of the sign variations on
  a geometrically shrinking cell around the cluster, with fallback to bisection), implement the one
  whose correctness you can prove in the design file, and measure. A failed fast step must fall back
  to bisection: the fast step may only save work, never decide a status.
- D3. The regression at high precision for well-separated roots (about 2 times slower than FLINT's
  route) is NOT repaired by a return to `arb_fmpz_poly_complex_roots`. If time remains after D1 and D2:
  measure where the refinement spends its time at `prec = 2^21` and report; change it only if the gain
  is clear.
- D4. Minor findings 2 and 3: correct the statements R4(3) (condition: the refinement returns without
  `LIMIT`) and R4(4) (the same floor for both precisions) in the design file. Minor finding 4: the
  differential script checks the place of the list.

Read `lanes/COMMON.md` and `lanes/COMMON-C.md` first (rule 6 does not hold for `roots.h`: you may change
its sentences on cost; rule 5 is replaced by item 4 below).

**You own:** `src/roots_real.c`, in `src/roots.c` ONLY `adf_roots_real` and its static helpers for the
real place, `include/adelefeld/roots.h` (sentences on the real roots only), `docs/design/real-roots.md`,
`docs/PERF.md` (the row of the real roots), `tests/test_roots_real_isolate.c`, `tests/test_roots_real.c`
(additions), `tests/fuzz/diff_roots_real.py`, `proto/real_isolation.py`, `proto/test_real_isolation.py`,
`bench/bench_roots_real.c`, `lanes/r-slice2/`. Everything else is read-only. No git command that changes
state, no `bd`. At most 2 cores; every program under `timeout`; build into
`BUILD=lanes/r-slice2/build` while you work and into `build/` only for the final checks.

1. Tests first, red then green (`lanes/r-slice2/redgreen.log`): the three inputs of finding 1 and the
   cluster family at `e` = 30000, each with a bound on the time in the test (2 s on this laptop, stated
   as a constant with a comment); the results unchanged (same balls as before for every input of the
   existing tests: the balls are part of the contract through nesting). Planted roots near the
   smallest admitted scale, several of them.
2. The prototype `proto/real_isolation.py` follows the C code (the differential script compares balls
   exactly).
3. Benchmark before and after with `bench/bench_roots_real.c`, the reviewer's three inputs added as
   families; the table in the report; the row of `docs/PERF.md`.
4. Show that the new tests bite with three planted faults in a scratch copy (for example: the early
   stop one cell too early; the fast step accepted without its check; the count taken as 0 when it is
   1). No mutation run.
5. `make clean && make -j2 check-all`, `make clean && make -j2 check SAN=1`,
   `make clean && make -j2 check CC=clang`, `sh lanes/m1-headers/check_headers.sh` pass, each under
   `timeout 900`; give the last line of each. `timeout 180 python3 tests/fuzz/diff_roots_real.py
   --seconds 120 --seed 20260930`: a smoke test, say so.

Report: `lanes/r-slice2/report.md`, written ONCE, AT THE END; running notes in
`lanes/r-slice2/progress.md`. In it: what was built, the numbers, what is proved and what is not, what is
left.
