# Report lane m0-repair-functions

Date: 2026-09-27. Scope: the brief in `lanes/m0-repair-functions/brief.md` (apply the review of the
functions proofs). Everything below was run on this working tree; all numbers come from the recorded
outputs in this lane directory.

## What was done

1. Repairs R-1 and R-2 applied to `docs/proofs/functions.md`.
   - R-1: Proposition 8, proof step 2, replaced by the review's text. It now proves that the division
     by p^e is an exact integer operation: v(y) >= v gives v(y^k) >= k v > v_p(k!) >= e (k v > v_p(k)
     for log), a nonconstant term is retained only when n >= 1 (K >= 2 forces n > v; J >= 2 forces
     n >= v), hence W >= n + D > e and the residue y^k mod p^W is divisible by p^e. One clause was
     added beyond the review's text: "for k >= 1 ... The constant term needs no division", because at
     k = 0 the review's chain reads 0 > 0. Nothing else was changed. Re-read after the edit: the step
     is a proof (exactness, the e <= D error bound, the unit inverse, and the side result that
     max(v, .) in W never binds all follow from the displayed estimates).
   - R-2: Proposition 12, proof step 4, first sentence replaced by the review's text. The enclosure
     now cites Proposition 4 (u in 1+p^c Z_p) and Lemma 9 (log maps 1+p^c Z_p onto p^c Z_p), not
     Proposition 11. Re-read after the edit: the step is a proof and the rest of the step still flows.
2. Optional items applied.
   - O-1: new "Proposition 7b (tight count for log)" after Proposition 7, with statement and stepwise
     proof: T_tight = J-1 with J the least k >= 1 with k v - e(k) >= n, e(k) the largest e with
     p^e <= k. Proved: safety (v_p(k) <= e(k), monotone bound) and T_tight <= T_log (via
     e(k) <= k/2 and Proposition 7's J). The safe count of Proposition 7 is kept unchanged. The text
     states that the C code of PLAN 1F.7 uses the tight count by default (its proof is complete) and
     the safe count is the fallback. Both counts are checked by enumeration.
   - O-2: new "Remark 15r (odd degree roots at 2 need only r >= 1)" after Proposition 15, with a
     stepwise proof (ball decomposition, x -> x^n bijective on Z_2^x for odd n, torsion of Q_2^x is
     exactly {1,-1} with proof, unique root in Q_2, image b+2^(j+1) Z_2 with the exponent identity
     N-(n-1)j = j+1). It is marked "Not part of SPEC 9.3.3" and notes that SPEC 9.3.3 and
     Proposition 15 keep the stronger guard. The main statement was not changed.
3. Weak checks strengthened, red-green. `check_regression_mutations` (13 fixed values) is replaced by
   `check_mutation_testing`: 28 wrong rules planted as textual mutants in a copy of the file under
   test, each demanded rejected by the named check, with the failing assertion reported. The planted
   rules include `W = n + D` without the maximum (both `max(1, n+D)` and plain `n+D`). The other
   named weaknesses were strengthened and each is exercised by at least one mutant (see the red-green
   record below). The eleven literal anchors of `docs/reviews/m0-proofs/functions_review_checks.py`
   section M are kept unique (verified: 15 of 15 anchors unique in the file, 11 of them the review's).
4. "Review record" added at the end of `docs/proofs/functions.md`: date 2026-09-27, reviewer Claude
   opus, review file `docs/reviews/m0-proofs/functions-review.md`, verdict counts 20 VALID,
   2 MINOR, 0 INVALID, and one line per change.

Statement index updated with rows 7b and 15r. The formulas of Proposition 7 stay on lines 170-181 of
`docs/proofs/functions.md` (the line reference in the reviewer's `counts()` docstring remains true;
all insertions are after that block).

## Files written

- `docs/proofs/functions.md` (owned; edits: header, Proposition 7b, R-1, R-2, Remark 15r, index rows,
  Review record).
- `proto/functions_checks.py` (owned; edits: imports, `ilog`, `tight_log_count`, strengthened
  `check_truncation`, `check_working_precision`, `check_power_precision`, `check_root_precision` with
  `poly_at` and `branch_image_check`, `check_typed_and_projection`, `check_mutation_testing` with
  `run_mutant`, `run_mutation_tests`, the anchor table and the 28-mutant table, `main`).
- `lanes/m0-repair-functions/run_red.py` (runner for the red half).
- `lanes/m0-repair-functions/functions_checks_pre.py` (snapshot of `proto/functions_checks.py` taken
  before this lane's strengthening; the red target).
- `lanes/m0-repair-functions/red_output.txt`, `red_coverage.txt`, `green_output.txt`,
  `review_checks_output.txt` (recorded outputs).

## Checks that were run, with results

1. `python3 proto/functions_checks.py` -- exit 0, 4.5 s wall (limit 180 s). Full output in
   `green_output.txt`. Key numbers:
   - check_truncation: scenarios=4928 omitted_terms=14377224 max_degree=4096 max_n=40
     tight_log_terms=2876715 (both the safe and the tight log count, every omitted degree checked
     two ways: k v - e(k) >= n and k v - v_p(k) >= n; also 0 <= T_tight <= T_log asserted).
   - check_working_precision: rounded_partial_sums=3942 exact_divisions=24759 inputs=3 lifts=3
     W-1_wrong=852 W_without_D_wrong=1155 (of 3942 evaluations; the precondition v(y) >= v and the
     exact division are asserted in every case).
   - check_power_precision: independent_uncertain_images=900 one_exact_input_images=480
     uncapped_radii=635 by_p={2: 202, 3: 141, 5: 146, 13: 146} (alpha now takes c and c+1 finitely
     and infinity; p = 13 runs at modulus p^3 with 225 uncertain images, 146 of them uncapped).
   - check_root_precision: branch_images=418 branch_preimages=418 scaled_images_p3=1254
     guard_r1_at_2=245 unique_roots_at_2=112 (five roots b, degrees 1,2,3,4,5,6,8,9,10,12,25, both
     directions at p^3 sample points via the exact binomial coefficients, plus the Remark 15r block).
   - check_typed_and_projection: valuation_balls=120 (every NOT_DETERMINED ball exhibits at least two
     valuations on the grid).
   - check_mutation_testing: planted_wrong_rules=28 rejected=28 survivors=0 review_anchors_unique=11.
2. `python3 docs/reviews/m0-proofs/functions_review_checks.py` -- exit 0, 9.2 s wall. Full output in
   `review_checks_output.txt`. All sections ok; failures=[]. Section M: "22 of 22 killed; survivors:
   []" (before this lane the same run reported 21 of 22, with `W = n+D (drop v)` surviving).
3. Anchor uniqueness script (inline python over `proto/functions_checks.py`): 15 anchors, each
   occurring exactly once ("anchor counts not equal to 1: none (all 15 unique)").
4. Design probes before writing the checks (inline python): tight count safe and T_tight <= T_log on
   p in 2,3,5,7,13, v = 1..10, n = -10..2000 (0 violations, min(T_log - T_tight) = 0);
   `check_typed_and_projection` grid has at least two valuations in every NOT_DETERMINED case and one
   valuation in every determined case over p in 2,3,5,13 (0 violations); the mutant of the
   determination digit is witnessed by 6 (p,a,n) cases.

## Red-green record (brief item 3)

Red command: `python3 -B lanes/m0-repair-functions/run_red.py`. It runs the same 28-mutant harness on
`functions_checks_pre.py` (the file before this lane's strengthening). Result: planted=28,
rejected=24, survived=2, skipped=2 (0.8 s). Full output in `red_output.txt`. Survivors in red:

- `W = n+D (drop v, clamped at 1)`: SURVIVED (this is the review's known survivor; the old check
  cannot reject it).
- `root out exponent caps the p-power loss at 1 digit at odd p`: SURVIVED (the old check had no
  degree with v_p(n) >= 2 at an odd p; degrees 9 at 3 and 25 at 5 now kill it).

Skipped in red (rule not yet present): the two tight-count mutants. In red the plain `W = n+D (no
maximum at all)` was reported killed, but only by the helper precondition `assert n >= 1` of the
residue function (invalid modulus exponent), not by any claim of the check.

Green command: `python3 proto/functions_checks.py` tail (check_mutation_testing): planted=28,
rejected=28, survivors=0 (4.5 s for the whole file). Full output in `green_output.txt`. Both W
variants are now rejected at the same assertion, `assert vp(x+F(p)**w*t, p) >= v` (the certified-domain
precondition W >= v that repair R-1 uses). Honest note: on output values alone these two variants are
indistinguishable from the correct rule when a nonconstant term is kept, because then n + D >= v and
the maximum never binds (this is the review's own observation under statement 8). The rejection is at
the level of the rule's stated precondition, which the proof of R-1 depends on ("Since W >= v, the
representative y satisfies v(y) >= v"). The value-level probes still separate W-1 and W-without-D
(852 and 1155 wrong results of 3942).

Coverage before/after (red_coverage.txt and green_output.txt):

- check_truncation: omitted_terms 11500509 -> 14377224 (the tight count adds 2876715 checked terms).
- check_working_precision: rounded_partial_sums 876 (1 input, 2 lifts) -> 3942 (3 inputs, 3 lifts),
  plus 24759 asserted exact divisions and 3942 asserted domain preconditions.
- check_power_precision: uncertain images 372 -> 900; at p = 13 the old grid had 48 uncertain cases
  of which 22 were capped at the modulus (radius k = 2, trivial full-ring prediction) and 26 had
  radius 1; the new grid has 225 cases at p = 13 with 146 uncapped radii and radii up to 3.
- check_root_precision: branch_images 96 (2 roots b, degrees 1..6) -> 418 (4-5 roots b, degrees up
  to 25, including 9 and 25); scaled part 288 one-direction mod p^2 checks -> 1254 both-direction
  bijections at p^3 sample points; plus 245 Remark 15r images and 112 unique-root witnesses.

## What is not done

- `check_real_and_character` still tests real roots on integer points only. The review accepts this as
  declared ("this is declared"); the brief does not name it. Not changed.
- O-2 is a remark only. SPEC 9.3.3 and Proposition 15 keep the stronger guard, as the brief requires.
  If 1F wants fewer NOT_DETERMINED results, the remark is proved and checked (245 + 112 cases).
- The W = n + D variants cannot be separated from W = max(v, n+D) by output values; the mutation test
  rejects them on the precondition W >= v. This is stated above rather than papered over.
- No change to `docs/SPEC.md` or to any file outside `docs/proofs/functions.md`,
  `proto/functions_checks.py` and this lane directory.

## Sources pending

Standing `[source pending]` items of `docs/proofs/functions.md` are unchanged and none was added by
this lane: the real intermediate value theorem and real/complex exponential normalisations (Lemma 2),
the name and convention of the Teichmueller representative (Proposition 4), the name and
normalisation of the Iwasawa logarithm (Proposition 11), and an on-disk Tate source for SPEC 6's
sign convention (Proposition 20). Everything added here (R-1, R-2, Proposition 7b, Remark 15r) is
p-adic algebra proved stepwise in the file and needs no external source.

## Findings against the specification

None. SPEC 9.3.1 to 9.3.6 was not changed and nothing in it was found wrong. Two informational
discrepancies in the review file (not in SPEC), recorded for the orchestrator:

- The review says of `check_power_precision`: "alpha is only c or infinity, never a finite value
  above c". At p = 2 the old grid's u0 = 1+2p^c = 9 has log(u0) of valuation 3 = c+1, so alpha above
  c was tested at p = 2 already; the red run kills the "alpha capped at c" mutant even before the
  strengthening. What was missing is alpha above c at odd p (now added via u0 = 1+p^(c+1) and
  1+3p^(c+1)) and depth at p = 13.
- The review says "At p = 13, k = 2 almost every radius is capped at k, so that prime tests nothing."
  Measured on the old grid: 48 uncertain cases at p = 13, 26 of them with radius below the cap and 22
  capped. The criticism is directionally right (radii only 1 or 2 at modulus p^2) but "almost every"
  overstates it.
