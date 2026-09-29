# r-slice1 progress

- 2026-09-29 22:41: worktree at f0a23fc; refs/src linked. Read the briefs, sources (SM:547-575, KS:278-342),
  FLINT taylor_shift.c, scale_2exp.c, div_root.c, roots.h, roots.c real part, test_roots_real.c.
- Baseline: test_roots_real 24.34 s; bench before in lanes/r-slice1/runs/bench_before.txt (pair 1500: 111.5 s).
- Existing test test_roots_real.c asks X^2 - 2 at ADF_ROOTS_REAL_PREC_MAX = 2^21: plain bisection is
  Theta(prec^2) bit operations there (estimate: minutes). Plan: bisection first, measure, then QIR (KS
  Algorithm Eqir) if the measurement says so; the sequence of cells must not depend on need (nesting).
- Design decisions made: interface roots_real.c -> roots.c: hidden int adf_roots_real_isolate(cand, &m, g, prec);
  roots.c keeps count + real_finish (certificate unchanged). "apart" test uses the right end of the previous
  item's ISOLATING cell (static), so the refinement of each root does not depend on prec of its neighbour:
  lists are nested when prec grows. LIMIT when a cell of exponent k < 2 - M would be formed (M = BITS_MAX).
- 23:05 tests/test_roots_real_isolate.c written; red runs logged in redgreen.log.

- 23:20 src/roots_real.c written: isolation (Descartes), refinement = step G (gallop for cells touching 0), step
  Q (Eqir), bisection. roots.c: call site only (plus one comment line in the slice-3 block). Both real tests
  green (redgreen.log). bench/bench_roots_real.c: families thirds, sqrt2, x5, mignotte; modes eval2; --run.

- 23:25 step G generalised: the anchor is 0 or a root at an end (pair e = 10000: 0.10 s -> 0.0006 s).
  bench after: lanes/r-slice1/runs/bench_after.txt (load 3.5). docs/design/real-roots.md written.

- 23:30-00:00 header sentences, PERF row, proto sync (+ nesting test), fuzz script (same-as-proto), bite 6/6,
  fuzz smoke 120 s (1229 calls, 0 disagreements), check-all, SAN, clang, headers: all pass. result.md written.

## Done. See result.md.
