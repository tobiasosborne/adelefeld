# Lane r-cost1: the remaining costs of adf_roots_real (bead adf-4dj)

`adf_roots_real` (`include/adelefeld/roots.h:345-398`, `src/roots_real.c`, design `docs/design/real-roots.md`,
proofs R1 to R9; Algorithm RR of `docs/proofs/solvers.md:1399-1417`) is correct on every known input and its
isolation is fast since lane r-slice2. Two costs remain (`docs/reviews/r1/bughunt-real-slice2.md` MINOR 1 and 2,
read them in full with their commands and numbers):

1. FLINT's trusted count `fmpz_poly_num_real_roots(g)` (S-D11) dominates: 6 to 8 s at degree 50 with 10^4-bit
   coefficients, when the isolation itself takes 0.2 s. The equivalent-input trick of R9 (a rational translate and
   a dyadic scale before the count) is applied for a negative scale only (roots `i 2^150`: 1.13 s against 0.04 s
   for `i 2^-150`). `adf_rootlist_verify_complete` recomputes the plain count from `f` (7.6 s of a 7.66 s driver
   line) because it is independent of the scaled count by design (S-D17).
2. High precision grows steeply with the degree: `X^50 - 2` at `prec = 2^21` (`ADF_ROOTS_REAL_PREC_MAX`) did not
   end in 100 s (0.63 s at 2^14, 3.5 s at 2^16, 23.1 s at 2^18), where `X^2 - 2` at 2^21 takes 0.37 s; Wilkinson
   50 at prec 20000 takes 38.8 s.

Your task, in thin slices, each measured before and after on the same machine with `bench/bench_roots_real.c`
(read its header; build as it says; `taskset -c 2`; the median of 5 runs; the eval2 reference where defined):

A. Measure first. Reproduce MINOR 1 and 2 with the bench (families `deg`, `small`, `sqrt2`, a new family for
   the dense random degree-50 input with a fixed seed, and `prod_(i=1..50) (3 2^190 X - (2^190 + i))`), and
   profile where the time goes in the count, in the refinement and in the certificate (`perf` is not installed;
   use clock sections in a scratch copy under your lane directory). Write the table first; decide from it.

B. The count. Apply R9 in both directions where it pays: a positive scale (`g(2^k X)` with the content removed)
   and the rational translate by the mean of the roots when it is a short rational, with the proof of R9 cited
   at the call; or, if the profile shows the count's cost is in FLINT's Sturm sequence on large coefficients
   regardless of scale, say so with numbers and stop here. Do NOT remove the recount in
   `adf_rootlist_verify_complete` and do not make it read the scaled count: its independence is S-D17. If a
   cheaper independent count exists (a Descartes bound equal to the number of isolated balls proves
   completeness for a squarefree polynomial once each ball is certified: check whether `docs/proofs/solvers.md`
   P3.13 allows it; if the proof is not on disk, it is a DESIGN-FINDING for the report, not code).

C. High precision. Find why `X^50 - 2` at 2^21 does not end: the refinement of R4 and R5 (galloping, QIR,
   bisection) evaluates the exact polynomial at points of `prec` bits, degree 50 times, at each of up to
   `8 (k_0 - k_end) + 4` steps (design section 6). The fix of lowest risk: a precision-doubling refinement
   (each QIR success doubles the accuracy, so the number of exact evaluations is `O(log prec)` instead of
   `O(prec)`: check what the code does now against R4 and the design's cost paragraph, and whether the
   doubling is already there and something else is quadratic: for example the end points growing to twice the
   precision, or a Taylor shift of degree 50 with `prec`-bit coefficients at every step). Implement the
   smallest change that gives `X^50 - 2` at 2^21 under 10 s and Wilkinson 50 at 20000 under 5 s, if one exists;
   otherwise lower `ADF_ROOTS_REAL_PREC_MAX` is NOT yours to change (header fixed: HEADER-FINDING with the
   measured growth). Every change keeps the certificate (the exact end point tests of P3.8) unchanged.

Rules: `lanes/COMMON.md`, `lanes/COMMON-C.md` (red-green: a test that shows the cost or the behaviour before
and after, for example a bound on the number of exact evaluations through a counting hook under
`ADF_CHECK_INVARIANTS`, or a time-independent count of refinement steps; mutation testing of
`src/roots_real.c` with `--limit 60`, 20 minutes, 2 jobs, `--san`; survivors listed). Every result is still
checked against the exact oracle of `tests/test_roots_real.c` and `test_roots_real_isolate.c` (run them whole:
plain, `SAN=1`, `INV=1`, `CC=clang`), and the driver fixtures that use `realroots` (`tests/driver/`: find them,
run `sh tests/test_driver.sh` or the targeted command the script documents). No change to `include/` (the
header is fixed; `HEADER-FINDING` where it does not fit), to `docs/proofs/`, or to any file not listed.

**You own:** `src/roots_real.c`, `bench/bench_roots_real.c` (new families and modes only), `tests/test_roots_real.c`
and `tests/test_roots_real_isolate.c` (append), `docs/design/real-roots.md` (a new section 10 appended: what
changed, the measurements before and after, the proof of any new equivalence used), `lanes/r-cost1/`.
Everything else is read-only. No git command that changes state, no `bd`. At most 2 cores; every program under
`timeout` (the slow inputs under `timeout 170`; a run that does not end is a measurement, not a failure of
the lane); build under `lanes/r-cost1/build*` (`make -j2 BUILD=...`); delete build trees at the end; no file
above 100 KB in the lane directory; lines at most 116 characters; files end with a newline.

Report `lanes/r-cost1/report.md`: the table before and after (input, prec, before s, after s, eval2 where
defined), what changed and why with the design citation, the tests added and seen red, the mutation result,
the HEADER-FINDINGs and DESIGN-FINDINGs, what is not done. Aim to finish within about two hours; A and B
first, C if time allows (then say exactly where C stopped).
