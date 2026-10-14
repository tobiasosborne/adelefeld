# Lane r-cost2: the trusted count of adf_roots_real on an equivalent input (bead adf-z45q)

Lane r-cost1 (`lanes/r-cost1/report.md`, "Count experiments and profile"; `docs/design/real-roots.md` section 10)
measured that FLINT's trusted count `fmpz_poly_num_real_roots` dominates `adf_roots_real` on structured inputs
(cluster `prod (3 2^190 X - (2^190 + i))`: 11.9 s of 12; `scaled` `prod (X - i 2^150)`: 2 s) while the count of an
equivalent polynomial (Proposition R9, `docs/design/real-roots.md:366-384`: a rational translation and a dyadic
scaling preserve the number of real roots) takes 0.04 to 0.07 s. The helper `real_count` in `src/roots.c:1704`
applies R9 for a negative scale only and with a 16-bit cutoff on the mean of the roots, so neither case is
reached. The lane left the change as an unapplied proposal, `lanes/r-cost1/count-proposal.patch` (28 lines:
the mean cutoff 512 bits, a positive scale, and a fallback to the plain count when the transformed height does
not shrink). You apply it, correctly and measured.

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md`, `src/roots.c:1690-1780` (`real_count` and its
caller; the comment names the sources), `docs/design/real-roots.md` R9 (366-384) and section 10 (the
measurements and the proposal), `lanes/r-cost1/report.md`, `lanes/r-cost1/count-proposal.patch`,
`bench/bench_roots_real.c` (the modes `count`, `countscale`, `countmean` and the families added by r-cost1),
`refs/src/flint-3.0.1/fmpz_poly.rst` (the Taylor shift and `_fmpz_poly_scale_2exp`: the lines the comment
cites), `tests/test_roots_real.c` (the count tests: "no-root and minimum-scale tests" of R9).

Build, in this order, red-green:
1. Tests first, in `tests/test_roots_real.c` (append): for each of the three families of r-cost1 (`cluster`
   with `e = 190, d = 50`; `scaled` with `e = 150, d = 50`; `random` seed (1,2), degree 50, 10^4 bits: read the
   bench for the exact constructions and build the same polynomials in the test) the count equals the degree
   (50, 50) or the plain count (`random`: compute `fmpz_poly_num_real_roots` once in the test as the oracle; it
   is slow, about 20 s: keep it under `timeout 170`, or use a smaller random degree whose plain count is under
   2 s and say so), through `adf_roots_real` at `prec = 2` (the list's length and `complete = 1`); and a timing
   assertion that `adf_roots_real` on the cluster input takes under 2 s on this machine (the pattern of the
   existing 2 s check in `test_roots_real_isolate.c`: find it and use its clock). Seen red (the timing check
   fails on the unchanged code: record the time).
2. Apply the proposal to `real_count`: the mean cutoff 512 bits (justify the number from the measured 385-bit
   mean of the cluster input and the cost of a Taylor shift of degree 50 at that size: one sentence), both
   scaling directions with `|scale| <= 32` as the plain-count window, and the fallback to the plain count when
   the transformed height does not shrink (so the change can never cost more than one extra height
   comparison plus the transform). Cite R9 at the call for each direction. Check the sign conventions of
   `_fmpz_poly_scale_2exp` against `fmpz_poly.rst` and the source `scale_2exp.c` already cited at
   `src/roots_real.c:22` (the direction of the exponent): a wrong direction gives a wrong count, which the
   verifier would catch as NOT_DETERMINED, so the test of step 1 must also assert `OK`.
3. Measure with the bench (as r-cost1 did: `taskset -c 2`, the median of 5): `adf` and `count` on `cluster`,
   `scaled`, `tiny`, `random`, `deg`, `small`, before and after; the table in the report and appended to
   `docs/design/real-roots.md` section 10 (a new subsection "r-cost2").
4. Checks: `test_roots_real` and `test_roots_real_isolate` whole under plain, `SAN=1`, `INV=1`, `CC=clang`
   (in build directories under `lanes/r-cost2/`); the driver fixtures `s-realroots` and `s-hostile` through
   `sh tests/test_driver.sh` or its targeted form; mutation of `src/roots.c` restricted to the changed lines if
   the tool allows (read `tools/mutate/README.md`; otherwise `--limit 30` on the file, 15 minutes, 2 jobs,
   `--san`), survivors listed with one line each.

**You own:** `src/roots.c` (the function `real_count` only), `tests/test_roots_real.c` (append),
`bench/bench_roots_real.c` (new modes or families only, if needed), `docs/design/real-roots.md` (one appended
subsection), `lanes/r-cost2/`. Everything else is read-only (no header change; `adf_rootlist_verify_complete`
and its independent recount are NOT changed: S-D17). No git command that changes state, no `bd`. At most 2
cores; every program under `timeout` (the slow plain counts under `timeout 170`); builds under
`lanes/r-cost2/build*`, deleted at the end; no file above 100 KB in the lane directory; lines at most 116
characters; files end with a newline; plain sober prose; no LaTeX.

Report `lanes/r-cost2/report.md`: the red and green runs with their commands and numbers, the table before
and after, the mutation result, the HEADER-FINDINGs or DESIGN-FINDINGs, what is not done. Aim to finish within
about sixty minutes.
