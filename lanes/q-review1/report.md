# q-review1 report

Adversarial review of `docs/api-3.md` (milestone 3, first half, lane d-quotient), its oracle
`proto/quotient3_checks.py` and its findings F1 to F5 and decisions D3-1 to D3-3.

**Result: VALID 14 / MINOR 9 / INVALID 0. MAY BE IMPLEMENTED AFTER THE REPAIRS.**
No statement Q1 to Q5 is false. Eleven repair texts (R1 to R10 and R12) are ready to paste in `review.md`.

## Files written

- `lanes/q-review1/review.md`: the review, verdicts, details, repairs, checks, what was not examined.
- `lanes/q-review1/review_checks.py`: my own oracle, 15 sections, no import of the author's code.
- `lanes/q-review1/mutate_oracle.py`: 15 mutants of the author's oracle on scratch copies.
- `lanes/q-review1/mutants/`: the 15 mutated copies and the unmutated control.
- `lanes/q-review1/abi/sizes.c`: layout probe compiled against `include/adelefeld/adele.h`.
- `lanes/q-review1/checks-final.txt`, `mutants-final.txt`, `checks-time.txt`: outputs.
- `lanes/q-review1/progress.md`: notes.

No file outside `lanes/q-review1/` was written. No git command and no `bd` was run. At most two cores;
every program ran under `timeout`; the longest took 96 s.

## Every check that was run

1. `timeout 170 python3 lanes/q-review1/review_checks.py`: exit 0, 15 sections, 72.8 s.
   Counts: A_reduction_image 720 cases and 208200 membership comparisons; A2_rounded_superset 360 and
   414222; B_piece_count 293 (153 integer-radius intervals, 0 mismatches of `k+1`, 33 deduplications);
   C_q1 3246 (0 excess-bound violations, worst `(rho-d)/d = 4095/1099511627777 = 3.7243e-9` against
   `2^-28 = 3.7253e-9`); C2 374; D_q2_named 27 pairs, 0 differences; D2_q2_random 300 pairs,
   0 differences; E_q3 2048 and 301500 comparisons; F_q4_hull 360 images, 0 differences at 1e-60;
   F2 7 arcs; F3 64 pairs and 576 witnesses; G_q5 138 pairs and 76 explicit missing points; G2 5 widths;
   H 6 witnesses; I 318 sign cases. Output in `checks-final.txt`.
2. `timeout 175 python3 lanes/q-review1/mutate_oracle.py`: exit 0; the unmutated copy passed 15 groups.
   Fifteen mutants: 13 killed, 2 survived. The first group to notice: `reduction` (M2, M3), `sets`
   (M8, M9, M10, M13), `rounding` (M4b, M5b), `phases` (M1, M6), `hulls` (M12), `examples_findings`
   (M11). Survivors: the radius rounded to nearest instead of up (M4) and the boundary glue deleted (M7).
   Output in `mutants-final.txt`.
3. `gcc -I include -o lanes/q-review1/abi/sizes lanes/q-review1/abi/sizes.c && ./lanes/q-review1/abi/sizes`:
   `sizeof(adf_adele_struct) = 96`, `sizeof(adf_fball_struct) = 48`, qclass struct 24 bytes with offsets
   0, 8, 16 and alignment 8. The layout claims of section 1 are correct.

## Findings, one line each

- R1 MINOR: Q1 step 3 says the successor of a power-of-two radius contributes one spacing; it contributes
  two. The conclusion `rho - d <= 2^-28 d` still holds and is now proved; worst measured ratio 3.7243e-9.
- R2 MINOR: Q1 assumes `0 <= l <= h <= 1`, but section 3.3 applies it to hull coordinates in `[-1,1]`;
  the extension is true on 374 negative-midpoint intervals but is not stated.
- R3 MINOR: Q4 step 2 asserts the real minimum from the distance to `1/2` without the identity
  `dist(t,Z) = 1/2 - dist(t-1/2,Z)`; one line is missing.
- R4 MINOR: F1 is real (SPEC:402 is false at `N = 0`, five widths give a missing point) but it quotes
  conventions:745 as an "exactly when" sentence; that line gives one direction only.
- R5 MINOR: the budget of section 2.3 is compared after normalization, so a LIFT with radius `1/10^100`
  runs `10^100` splits before the LIMIT; the preflight is missing. D3-2 recommends exactly this policy.
- R6 MINOR: the cost line `O(K^2 L)` and the budget `(2E+1) K L` are not reconciled in the text.
- R7 MINOR: `adf_adele_psi_tate_at` has no `where` and no `DOMAIN`, yet an adele may have `arch = 0`, so
  the archimedean place may not be a place of the argument (conventions:220).
- R8 MINOR: the status row of the four class character calls is never named; `NOT_DETERMINED` is not in
  the "Quotient by `Q`" row. Same for the `void` `adf_qclass_add_rat` against conventions:199. The design
  reports two status-row exceptions and there are four.
- R9 MINOR: F4 is real; the sentence it cites is on SPEC:413-414, not 414-415.
- R10 MINOR: D3-1 is sound; the amendment must name the three functions, keep `CMP` for points, and use
  the preflight of R5.
- R11 VALID: F2, F3, F5 reproduced; the conventions conflict and both witnesses hold.
- R12 MINOR: the descent of psi to `A/Q` is cited to analysis Lemma 2 step 2 (lines 87-88), which proves
  triviality on `Q`; it is step 4 (lines 92-94). Two citations in section 3.1 and 3.2.
- D3-3 VALID: per-entry certification is the only workable rule; the missing sentence is that the status
  may change under an exact reduction.
- VALID: Q2, Q3, Q5 and the ten subsections 1, 2.1, 2.2, 2.4, 2.5, 3.4, checked as tabulated.
- Weakness of the author's checks: deleting the boundary glue leaves all 15 groups green; the witness is
  `[1/2,1] x (0 mod 3)` against `[0,1/2] x (2 mod 3)`, which overlap only at the glued class.
- Weakness of the author's checks: rounding the Q1 radius to nearest and then taking the successor passes
  all 15 groups, so the rounding direction is not pinned.
- Weakness of the author's checks: `fault_witnesses` is the first group to notice none of the 15 mutants,
  although the design's fault 3 relies on it.

## Not done

- No C code, header, build, driver, Julia binding or allocation test; the design's section 5 acceptance
  list was read, not executed. No production mutation run.
- No dump byte parsed or produced; only the grammar line of conventions:1411 was compared.
- `tests/golden/qclass.tsv` and `psi_phases.tsv` were not re-derived row by row.
- Sections 3.3 and 3.4 beyond the names of CV-60 and the sign of `tau`, checked at
  `acb_dirichlet.rst:358-362`.
- No local-backend value was constructed; the CRT and cancellation requirement is untested.
- Cost estimates beyond the one inconsistency in R6.

## Sources pending

None new. The design's two pending sources (a readable Tate thesis for the section number, the FLINT
Conrey labelling) are not needed for the five statements and were not used. The character signs used here
come from conventions:844 and from `refs/src/tate-poonen/notes.txt:693-700` and 733-740, which are on disk
and were opened.

## Findings against the specification

- SPEC:402-403 ("The image is all of `A/Q` exactly when the real width is at least `N`") is false for the
  radius `N = 0` that the types admit: the image is then a bounded arc of real parts with one finite
  value. Q5 of the design is the correct statement. This is the content of the design's own F1.
- conventions:735-739 justifies CV-45 with "an `arb` enclosure of an interval with a non-dyadic end point
  cannot keep inside `[0,1]`". That is false as a general claim: the dyadic ball `[15/16 +/- 1/16] =
  [7/8,1]` encloses `[9/10,1]` and lies inside `[0,1]`. This is the content of the design's own F5.
- conventions:753-755 and conventions:200 give incompatible answers for the same predicate. This is the
  content of the design's own F2.
- Observed, pre-existing, not counted: a finite ball with `d > 1`, such as the golden `(* ; 0 mod 1/2)`,
  denotes a coset in the full product of `Q_p` and is not inside `A_f = Q + Zhat`
  (`docs/proofs/quotient.md` Lemma 1). SPEC 4.1 calls `adf_adele` an adele ball and SPEC 6 gives it the
  `B`-th-roots-of-unity behaviour, which needs the ball to leave `A_f`. The design repeats SPEC 6 and does
  not add a new dependence, but `pi` is only defined on `A` if the standing convention is read that way.
  The orchestrator should settle which reading the library implements.