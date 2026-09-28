# Closure of milestone 1 arithmetic review
CLOSED 2; CLOSED WITH EDIT 0; OPEN 2; SETTLED BY DECISION 2. NO BLOCKER OPEN.

The library was rebuilt with `make clean && make -j2`. The review's C probes were compiled
unchanged against it. Its Python oracles and source inspection scripts were rerun. The three
new probes are in `closure-checks/`. M1-D3 and M1-D4 are the accepted rules in
`docs/SPEC.md` section 15.

| Finding | Severity | Verdict | Evidence |
|---|---|---|---|
| R1 | BLOCKER | SETTLED BY DECISION | M1-D3 requires LIMIT; 8 old inputs and 24 boundary cases pass. |
| R2 | MAJOR | SETTLED BY DECISION | M1-D3 requires LIMIT; all 3 allocation probes exit 0 under 2 GB. |
| R3 | MAJOR | CLOSED | 24 new low-precision cases pass; repair test has 1426 checks, 0 failures. |
| R4 | MINOR | OPEN | 5113 real and 7222 complex public products differ by operand order. |
| R5 | MINOR | OPEN | All 18 original `fball.c` excuses still point at moved lines. |
| R6 | MINOR | CLOSED | Citation script rerun; cited lines and the repaired comments were read. |

## R1: SETTLED BY DECISION

M1-D3 changes the expected answer on the huge-exponent balls from the set answer to
`ADF_LIMIT`. `include/adelefeld/recon.h:16-24`, `src/recon.c:155-166` and
`src/recon.c:265-271` state and implement the exponent test. The original
`checks/recon_huge_exp.c` reports LIMIT and leaves `q = -99` on all 8 inputs. Its printed
"expected: NO_SOLUTION" lines state the old requirement; they are superseded by M1-D3.

`closure-checks/boundary.c` tests both signs, midpoint and radius exponents at the bound
minus one, the bound, and the bound plus one, on both sides of zero. It reads the stored
exponent that M1-D3 names. All 24 cases pass. At an admitted midpoint, the real point is
not the finite point `+1` or `-1`, so the set answer is `ADF_NO_SOLUTION`. At an admitted
radius, the real ball is centred on that finite point, so the set answer is `ADF_OK` with
that point. Outside the bound, the expected answer is LIMIT and the marker is untouched.
ASan and UBSan report 0 bytes of diagnostics for the 24 cases and for all 8 old inputs.

The new `tests/test_recon_limit.c` has 778 checks and 0 failures on the repaired tree.
In a scratch build of the pre-repair `recon.c`, four of the old balls fail its LIMIT
assertion before the fifth aborts. The first three return `ADF_OK` and write `q`.
Thus the new test detects the old defect. Its expected boundary statuses follow the
candidate progression and the exponent rule; they are not copied from the program.

## R2: SETTLED BY DECISION

The three unchanged `checks/recon_big_alloc.c` inputs now return LIMIT with exit 0 under
`ulimit -v 2000000`. The guard in `src/recon.c:265-271` precedes
`arb_get_interval_fmpz_2exp`. The FLINT warning at
`refs/src/flint-3.0.1/arb.rst:468-477` covers the allocation risk. The old scratch
build aborts at the `2^(2^36)` input, exit 134. The new test checks this input and the
small-radius input as LIMIT and checks that `q` is untouched. `recon.h:16-24` and M1-D3
agree. There is no document disagreement found for R1 or R2.

## R3: CLOSED

M1-D4 takes every `prec` below 2 as 2 and applies the exact numerator and denominator
separately in rational products and quotients. `docs/conventions.md:131`,
`include/adelefeld/adele.h:21-23`, and `src/adele.c:51-55` agree. The unchanged
`checks/adele_prec1.c` now reports canonical real and complex outputs at `prec = 1`.
`closure-checks/lowprec_neighbor.c` checks both signs of `1/3`, real and complex
division, separate and aliased output, and `prec` in 1, 0, -5: 24 cases, 0 failures.
Each result contains the exact quotient and is identical to the result at `prec = 2`.
`tests/test_adele_lowprec.c` has 1426 checks and 0 failures. Reversing its precision
clamp and rational-division blocks in a scratch copy reproduces a non-finite result at
`prec = 1`. The expected exact values are `+3` and `-3`, by division of `+1` by
`+1/3` and `-1/3`; they are independent of the library's output.

## R4: OPEN

The old `checks/swap_equiv.c` still finds 3856 `arb_mul` and 5536 `acb_mul` operand-order
differences in 20000 pairs. `closure-checks/swap_public.c` repeats the comparison through
`adf_adele_mul`, `adf_cadele_mul` and their public `identical` predicates. It finds 5113
real and 7222 complex differences in 20000 pairs. The outputs remain valid enclosures;
the false claim is that the stored balls are identical. The four original excuses in
`tools/mutate/equivalent.txt:54-65` remain and point to moved lines. All 14 `adele.c`
entries in that block now point to moved lines. The m1-repair-tools lane has not landed.

## R5: OPEN

`checks/stale_excuses.py` reports 36 stale entries of 39 across the four reviewed files:
18 in `src/fball.c`, 14 in `src/adele.c`, and 4 in `src/recon.c`. In particular,
`tools/mutate/equivalent.txt` still calls `src/fball.c:142` an `fmpz_zero(x->H)` call;
that line is now the signature of `fb_is_integer`. Line 468 now tests a negative radius.
No repair of the 18 original excuses has landed. The m1-repair-tools lane and its mutation
checker were not available, so the effect on a current mutation run was not examined.

## R6: CLOSED

`checks/citations.py` was rerun. `src/rat.c:15,225` now names `fmpq.h:212` for
`fmpq_sub`; `src/adele.c:29-31,126-127` points to the enclosure and exact-input
sentences at `refs/src/flint-3.0.1/arb.rst:9-11,25-26`. The reconstruction comment now
includes the allocation warning at `arb.rst:472-477`. The local-backend comment describes
the landed backend, and `src/fball.c` has no weak place references. The old stale phrases
and citations are absent from the four source files. R6 changed comments and citations;
its check is source inspection rather than an arithmetic regression.

## New findings

None. The additional stale `adele.c` and `recon.c` entries are the same line-drift issue
as R4 and R5, recorded above.

## Not examined

- The unlanded m1-invariants lane (`ADF_CHECK_INVARIANTS`) and m1-repair-tools lane.
  R4 and R5 stay OPEN; no mutation run was made.
- LeakSanitizer: it failed under ptrace before either instrumented probe started.
  ASan and UBSan were rerun with leak detection disabled, with 0 diagnostics.
- The local `fball.c` backend, a broad complex-coordinate enclosure fuzz, and exponents
  beyond the M1-D3 limit after refusal. The code of `arb_get_interval_fmpz_2exp` is not
  present under `refs/`; no general bound on its endpoint size is asserted here.
