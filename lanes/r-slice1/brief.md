# Lane r-slice1: real roots by exact isolation (issue adf-8di), first slice, in C

`adf_roots_real` (`src/roots.c`, Algorithm RR of `docs/proofs/solvers.md`, Propositions 3.8 to 3.10) is
correct and its cost has no bound: the candidates come from `arb_fmpz_poly_complex_roots`, which needs a
working precision of about `2^(8 + e/100)` bits for two roots of relative distance `2^-e` (cause and
measurements: `lanes/d-realroots/progress.md`, `lanes/d-realroots/runs/`). The quadratic with the roots
`2^1500` and `2^1500 + 1` takes 97 s at `prec = 2`.

This lane replaces the source of the candidates by an isolation of the real roots in exact integer
arithmetic: bisection with Descartes' rule of signs on the normalised polynomial `g`, then refinement of
each isolating interval by bisection to the accuracy of S-D19. The prototype exists and passes its tests:
`proto/real_isolation.py`, `proto/test_real_isolation.py` (lane d-realroots, unreviewed). The design file
that the prototype cites (`docs/design/real-roots.md`) was never written: you write the part of it that
this slice needs.

Decisions taken by the orchestrator for this slice (TJO: decide and go on; `docs/SPEC.md` 15 will record
them): (1) Descartes bisection, not a Sturm chain (the text of Sturm's theorem is not on disk, Descartes'
rule is: `refs/src/sagraloff-mehlhorn/tex/arxivfinal.tex:547` to `569`). (2) Refinement by plain bisection
in this slice; quadratic interval refinement is a later slice if the benchmark asks for it. (3) The
certificate does not change: exact signs of `g` at the end points (Proposition 3.8) and the count of
`fmpz_poly_num_real_roots` (S-D11); the verifiers are not touched. (4) A root that is a dyadic rational
met at a bisection point is returned as an exact ball. (5) The interface and the statuses of `roots.h` do
not change; only sentences about cost and method.

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold
for `roots.h`; rule 5 is replaced by item 6 below), `docs/PERF.md`, the sources above with file and line,
`docs/proofs/solvers.md` 3.8 to 3.10, `include/adelefeld/roots.h`, the real part of `src/roots.c`,
`docs/reviews/s2/review-real.md`.

**You own:** `src/roots_real.c` (new; the isolation and refinement; keep `src/roots.c` changes to the call
site inside `adf_roots_real` and its static helpers for the real place), `include/adelefeld/roots.h` (the
sentences about the real roots only), `tests/test_roots_real_isolate.c` (new), `tests/test_roots_real.c`
(additions), `bench/bench_roots_real.c`, `docs/design/real-roots.md` (new), `docs/PERF.md` (one row and its
lower bound), `proto/real_isolation.py` and its test, `tests/fuzz/diff_roots_real.py`, `lanes/r-slice1/`.
Everything else is read-only. Another lane reviews the p-adic part of `src/roots.c`: do not touch it.

1. `docs/design/real-roots.md`, short: the algorithm, the propositions it needs with proofs (isolation is
   correct and ends; the refinement keeps exactly one root in the interval; cost in the degree, the bit
   size and the separation), the lower bound in the sense of `docs/PERF.md`. No more than the slice uses.
2. Tests first, red then green (`lanes/r-slice1/redgreen.log`): the 18 `REAL_CASES` of
   `proto/solvers_checks.py`; the slow family (`2^e` and `2^e + 1`, `e` = 600, 1500, 3000, 10000) with a
   bound on the time in the test; close small roots (`1` and `1 + 2^-e`); Wilkinson 20; Mignotte
   `X^n - 2 (a X - 1)^2`; roots at dyadic points and at 0; no real root; degree 1; multiple roots in `f`;
   negative leading coefficient; every result passes both verifiers and has the accuracy of S-D19.
3. The code, in FLINT idiom (`fmpz_poly` Taylor shift and scaling: read FLINT's documentation and source
   for each routine you call and cite file and line). Checked `slong` arithmetic for exponents; `LIMIT`
   as the header says.
4. The old route: remove it, unless you find a class of inputs where it is needed; then say which.
5. Benchmark: `bench/bench_roots_real.c` before and after, the table in the result file, the row in
   `docs/PERF.md` with the distance from the lower bound.
6. Show that the tests bite, in a scratch copy under `build/`: three faults of your choice in the
   isolation (for example the sign-variation test accepting 2, a wrong shift, a dropped exact root).
   No mutation run.
7. `tests/fuzz/diff_roots_real.py` runs against the new code for 120 s under `timeout`: a smoke test, say
   so. `make clean && make check-all`, `make clean && make -j2 check SAN=1`,
   `make clean && make -j2 check CC=clang`, `sh lanes/m1-headers/check_headers.sh` pass; give the last
   line of each.

Result: `lanes/r-slice1/result.md` and the same text as your final message: what was built, the numbers,
what is proved and what is not, findings, what is left.
