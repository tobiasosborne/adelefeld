# Milestone 1 contexts review closure

4 CLOSED; 1 CLOSED WITH EDIT; 1 OPEN; 0 SETTLED BY DECISION. NO BLOCKER OPEN.

| Finding | Review severity | Verdict | Evidence |
|---|---|---|---|
| R1 | BLOCKER | CLOSED | Original 7,200 aliases: 0 wrong `lost`; 148,800 oracle records: 0 failures. |
| R2 | MAJOR | CLOSED WITH EDIT | Eight hostile calls refuse in 0.00 s; 6 boundary calls agree with M1-D5. |
| R3 | MAJOR | CLOSED | Original 20,086 inputs match the reference; 95 grammar cases have 0 failures. |
| R4 | MINOR | CLOSED | Five unknown versions with four bad separators each give PARSE; 95 cases pass. |
| R5 | MINOR | OPEN | 27 of 28 scaled mutation excuses still name stale lines; repair-tools has not landed. |
| R6 | MINOR | CLOSED | Citation claims corrected; 8,800 reduction/recombination calls have 0 errors. |

## R2: remaining documentation edit

M1-D5 (`docs/SPEC.md:864`) limits a context to 65,536 blocks. The cap is in
`include/adelefeld/modctx.h:23-32` and `src/modctx.c:202,239,371-374`. The status row of
`docs/conventions.md:188` still names only an oversized prime power as a reason for `UNSUPPORTED`.
In that row, replace the exact substring

    `UNSUPPORTED` (a prime power above one word)

with

    `UNSUPPORTED` (a block above one word or more than 65536 blocks)

The original `primorial_time.sh` ran unchanged from a copy beside its binary. All eight inputs returned
`UNSUPPORTED`, with exit 0, 0.00 s reported by the program, and 2-3 MB maximum resident size.
For `e = 4`, `n = 65535` and `65536` succeeded with 6,542 blocks in 0.63 and 0.61 s;
`n = 65537` returned `UNSUPPORTED` in 0.00 s. For `e = 0`, `n = 821646` and `821647`
returned a zero-block context; `e = 1, n = 821647` returned `UNSUPPORTED`.
The unchanged constructor oracle checked 8,115 calls with 0 mismatches. The new cap test ran
7 tests and 239 checks with 0 failures. Removing both `k` guards in a scratch object made that test
exit on signal 11; the repaired object passes. Its expected large-n statuses follow M1-D5, though
its overflow threshold helper calls the same FLINT `n_root` as the implementation.

## R5: open mutation excuses

The unlanded m1-repair-tools lane owns this repair. The unchanged `excuse_lines.py` finds 28
`src/scaled.c` excuses: 1 names its stated code and 27 do not. The entry
`tools/mutate/equivalent.txt:110`, keyed as `src/scaled.c:466:drop_call`, describes an optional
`fmpq_zero(t.s)`. Line 466 now calls `fmpq_mul(t.s, aq, w->s)`. The mutation tool keys an excuse
only by path, line, and kind (`tools/mutate/mutate.py:170,638-656`). Removing that multiply in a
scratch object made the exact-arithmetic scaled oracle fail, including 23 `mul_tight` enclosure
cases in 100 rounds. The excuse must be relocated or deleted after the tool lane lands.
No production library defect was found from this stale entry.

## Evidence for the closed findings

R1: `src/scaled.c:270-279` computes `lost` from the input before writing the output. The
unchanged `set_context_alias.c` found 0 wrong flags in 7,200 cases. Three unchanged
`scaled_ops.c` runs, 3,000 rounds each, produced 49,600 records per seed. The independent
fraction oracle found 0 failures, including alias, exact, sign, and local-input cases.
Moving the `lost` computation after the write in a scratch object reproduced 4,865 wrong
aliased flags. `test_scaled_alias` ran 9 tests, 142,938 checks, 0 failures. Its expected
loss follows integrality of `u K'/K` from `docs/proofs/policies.md` Proposition 11.

R3: `src/modctx.c:573-586` delegates validation and occurrence selection to `src/dump.c`.
The unchanged `dump_diff.py` ran 20,086 inputs through the unchanged guard-page runner:
0 C-side failures, 0 differences from its reference, and 0 noncanonical context dumps.
Its 49 differences from its own old reading are expected because that reading accepts only
`modctx` bodies and predates the repair. The new 95-case check uses explicit token positions
from `docs/conventions.md:1334-1366`. It covers local fball, scaled, adele, cadele, qclass lift,
and two qclass pieces with separate contexts; it found 0 errors. It also passed under ASan and
UBSan with 0 reports. Restoring the old modctx-only restriction in a scratch object made
61 of the 95 cases fail. `test_dump_ctx` ran 14 tests, 28,587 checks, 0 failures.

R4: `src/dump.c:1063-1071` checks the separator before rejecting an unknown version.
The original differential run has 0 cases of the old version/separator discrepancy.
The 95-case check includes 20 unknown-version bad-separator cases, all PARSE, and five
space-separated unknown versions, all UNSUPPORTED. Restoring the old check order in a
scratch object made 20 cases fail. These expected statuses follow the separator in
`docs/conventions.md:1334` and the version rule at `:1438`.

R6: `src/modctx.c:10-31` now distinguishes FLINT declarations from the documentation under
`refs/`. The documentation of `fmpz_multi_CRT_precomp` promises a congruent result of smallest
absolute value (`refs/src/flint-3.0.1/fmpz.rst:1362-1365`); it does not define `sign`.
`src/modctx.c:543-548` reduces the result modulo `K`, which proves the promised range `[0,K)`.
The unchanged integer oracle checked 4,400 reductions and 4,400 recombinations with 0 errors.
Eleven contexts shared by four threads for 3,000 round trips each gave 0 wrong values.
The new range pin in `test_modctx_limits` passes, but it also passes without the reduction on
this FLINT build; it is not a red test for that line. The unchanged `ground_truth.sh` ran, but
its fixed line labels are stale after the repair, so its output was checked against the current
source and the cited reference lines by hand. The FLINT-only thread baseline completed.

## New findings

None.

## Not examined

- R5 remains OPEN while m1-repair-tools has not landed. The memory checker in that lane and
  the unlanded m1-invariants (`ADF_CHECK_INVARIANTS`) were not examined.
- The full admitted 65,536-block construction was not rerun; the repair lane measured over
  five minutes for that case. No full mutation campaign or coverage-guided fuzzing was run.
- The unchanged `mutants_698_710.sh` ran, but its line numbers are stale. It made no mutation;
  four compilations and runs passed 11 tests and 10,197 checks each. It supplies no current
  evidence about the parser.
- The FLINT 3.0.1 documentation under `refs/` lacks `n_root` and the multi-mod precomp functions.
  [source pending: FLINT 3.0.1 documentation of `n_root` and `fmpz_multi_mod_precomp`]
  [source pending: FLINT 3.0.1 range and alias rule for `fmpz_fdiv_r`]
