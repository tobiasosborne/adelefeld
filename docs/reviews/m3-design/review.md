<!-- ROLE: record of a review. The text below the rule is lanes/q-review1/review.md as written (pi agent with
     stealth/space-bunny-alpha, 2026-10-05), not edited. Under review: docs/api-3.md and proto/quotient3_checks.py
     (lane d-quotient, codex gpt-6-astra). Its own checks: lanes/q-review1/review_checks.py. The design is
     repaired by lane d-quotient-repair. -->

# Adversarial review of `docs/api-3.md` (milestone 3, first half)

VALID 14 / MINOR 9 / INVALID 0. MAY BE IMPLEMENTED AFTER THE REPAIRS

Reviewed in lane `q-review1`, 2026-10-06, against `docs/api-3.md` (877 lines), the oracle
`proto/quotient3_checks.py` (700 lines) and the author's report `lanes/d-quotient/report.md`.
No design, specification, plan, convention or author file was changed. Nothing outside
`lanes/q-review1/` was written.

The units counted are the five statements Q1 to Q5, the ten subsections of sections 1 to 3, the five
findings F1 to F5 and the three decisions D3-1 to D3-3. The weaknesses of the author's oracle are listed
afterwards; they are not counted, because they concern the evidence and not the design.

No statement Q1 to Q5 is false. The two statements with a missing proof step (Q1 step 3, Q4 step 2) are
true, and the two repairs are one or two lines each. The interface has two unreported status-row
questions and one missing `where`. Nothing blocks the first slice, which is LIFT only.

## Summary table

| Unit | Verdict | Evidence or finding |
|---|---|---|
| Q1 rounding kernel | MINOR | Step 3 wrong for a power of two; hypothesis `[0,1]` too narrow for 3.3. R1, R2 |
| Q2 set relations | VALID | 27 named and 300 random pairs, 0 differences against brute force |
| Q3 operations | VALID | 1024 sums and 32 negations, 301500 membership comparisons |
| Q4 phase extrema | MINOR | 360 hulls against enumeration; step 2 needs one identity. R3 |
| Q5 full image | VALID | True as stated; it is the statement that repairs SPEC:402 (R4) |
| 1 type and contracts | VALID | ABI claims verified by compiling against the headers |
| 2.1 lifecycle, access | VALID | Compare with conventions 2 and 5.10 |
| 2.2 construction, R | VALID | 720 reductions, 208200 membership comparisons, spill and points |
| 2.3 comparison of sets | MINOR | LIMIT is not a preflight; cost line disagrees with the budget. R5, R6 |
| 2.4 translation, arithmetic | VALID | With conventions:756-757 and P10; void `add_rat` is sound |
| 2.5 text and dump | VALID | Grammar at conventions:1411 and M1-D6 agree with the design |
| 3.1 character definition | MINOR | Sign and sources are right; one step cited is the wrong one. R12 |
| 3.2 declarations | MINOR | `_at` has no `where` and no `DOMAIN`; psi status row unnamed. R7, R8 |
| 3.3 numerical hull | MINOR | Applies Q1 outside its hypothesis (same defect as R2) |
| 3.4 what this fixes | VALID | CV-60 names and `acb_dirichlet.rst:358-362` checked |
| F1 full image at N = 0 | MINOR | Real; one misquote of conventions:745. R4 |
| F2 stored set equality | VALID | conventions:753-755 against :200 is a real conflict |
| F3 class strict | VALID | Witness reproduced: lift fails, reduced pieces pass, image `{+1,-1}` |
| F4 translation equality | MINOR | Real; SPEC line number off by one. R9 |
| F5 CV-45 explanation | VALID | `[15/16 +/- 1/16] = [7/8,1]` encloses `[9/10,1]` and stays inside |
| D3-1 bounded set query | VALID | Recommendation sound; one repair, R10 |
| D3-2 construction policy | MINOR | Sound, but 2.3 does not follow it. R5 |
| D3-3 class strict meaning | VALID | Sound and representation-dependent success is the only option |

## Details

### R1. Q1 step 3 is wrong when the rounded radius is a power of two

What the design says (section 4, Q1, step 3): "In a binade, the spacing of 30-bit positive numbers is at
most `2^-29` times the smaller endpoint's value. RU contributes less than one such spacing. Its successor
contributes at most another spacing, including when RU lands on a power of two. Their sum is at most
`2^-28 d`."

What is true: if the rounded radius `u` is a power of two, then `u` is the first 30-bit number of its binade,
so one more spacing of that binade reaches only `u + 2^(e-1-29)`, half the way to the next 30-bit number.
The successor is `u + 2^(e-29)`, two spacings, not one. The conclusion `rho - d <= 2^-28 d` nevertheless
holds, because in that case `d > 2^e - 2s` with `s` the spacing, so `2^-28 d > 2^e/2^28 - 2^-27 s` while
`rho - d < 3s`, and `2^e/2^28 = 4s > 3s`. So the statement is true and its proof has one wrong sentence.

Reproduction: my check `C_q1` walks 606 dyadic radii in the neighbourhood of every binade edge from `2^-60`
to `2^0` and measures `(rho-d)/d`. The worst value is `4095/1099511627777 = 3.7243e-9`, against the bound
`2^-28 = 3.7253e-9`. The margin is about `1e-12`, so the bound is nearly tight and the argument matters.

    timeout 170 python3 lanes/q-review1/review_checks.py   # section C_q1

### R2. Q1 is stated for `[0,1]`, but section 3.3 applies it to `[-1,1]`

Q1 assumes `0 <= l <= h <= 1`. The phase hulls of section 3.3 have coordinates in `[-1,1]`, and a hull with
a negative midpoint is a normal case, for instance the image of a ball around phase angle `3/4`. The
design says "Use sign symmetry for negative midpoints", which is not a proof and is not needed: the excess
argument of Q1 uses only `d = (h-l)/2 + eta` and `rho >= d`, which hold for any `l <= m <= h`. What is
missing is a statement that covers the case.

My check `C2_q1_negative_midpoint` runs Q1 on 374 intervals inside `[-1,1]` with negative midpoints at
precisions 20 and 53: the enclosure holds and the excess bound `2 eta + 2^-28 d` holds in every case.

    timeout 60 python3 -c "import sys; sys.path.insert(0,'lanes/q-review1');\
        import review_checks as r; r.section_C2()"

### R3. Q4 step 2 omits the identity that links the two extrema

Step 2 asserts that the minimum of the real coordinate is "minus the cosine of the distance to `1/2`".
That needs `dist(t, Z) = 1/2 - dist(t - 1/2, Z)` for every real `t`, which is a one-line fact and is not
written. Without it the sentence is an assertion. My check `F_q4_hull` compares the formula with the
extrema of every arc endpoint and every interior critical point for 360 images with `B` from 1 to 12 and
`r` from 0 to 1, at 90 digits with margin `1e-60`: no difference.

### R4. F1: the finding is real, one citation is not

SPEC:402-403 says "The image is all of `A/Q` exactly when the real width is at least `N`." At `N = 0` and
width 0 the image is the zero class, and `(1/2 ; 0)` is missing; my `G2_q5_zero_radius` finds a missing point
for each of the widths `0, 1/20, 1, 3/2, 100`. Q5 excludes `N = 0` and the proof is correct, so F1 stands.
But F1 writes that "SPEC:402-403 and conventions:745 say the image is all of `A/Q` exactly when real width
is at least `N`". conventions:745 says only "When `hi - lo >= N` the image is all of `A/Q`", one direction,
without "exactly when". Only SPEC:402 needs the repair. The alternative that the sentence inherits the
positive-radius hypothesis of the surrounding paragraph is fairly stated.

### R5. The LIMIT of section 2.3 is not a preflight

Section 2.3 says: "Use exact R without real rounding as an internal normalization... Budget: require raw
`K <= work_limit`, `L <= work_limit`, and `(2E+1) K L <= work_limit`... Compare these integer expressions
before allocation." The `K` there is the count after normalization. Normalization applies P8, which splits
a finite radius `N = A/B` into `B` balls. A LIFT whose finite radius is `1/10^100` therefore runs `10^100`
splits before any comparison with `work_limit`. The design knows this cost elsewhere: section 3.2 says the
character cost is "independent of the numeric value of denominator(N)". The same sentence belongs here.
D3-2 recommends exactly the policy that section 2.3 does not yet apply.

### R6. The cost line of section 2.3 disagrees with its own budget

The declaration says "Cost: `K` exact pieces; `O(K log K)` endpoint sorting; `O(K^2 L)` simple fiber work".
The budget in the text bounds the fiber work by `(2E+1) K L`, and with `E <= 2K` both are `O(K^2 L)`, so
the two agree up to a constant. It is worth saying so, because a reader who takes `K^2 L` literally will
size `work_limit` differently from one who takes the budget expression.

### R7. `_at` without `where` and without `DOMAIN`

`adf_adele_psi_tate_at` and its strict variant take a place `v`. conventions 5.5 allows `arch = 0`, so an
adele can have no archimedean place (conventions:708 lists the places of an adele). conventions:220 makes
`DOMAIN` with a place the status of a function at a place when `v` is not a place of the argument, and
conventions:1030-1037 makes the place an opaque handle that is reported as `where`. The design's
declaration has no `where` argument and lists only `OK`, `NOT_DETERMINED`, `LIMIT`. Then
`adf psi_at '(0 ; 0)' with inf` has no specified behaviour.

### R8. The status row of the four class character calls is not named

`adf_qclass_psi_tate` and `adf_qclass_psi_tate_strict` return `NOT_DETERMINED`. The row "Quotient by `Q`"
of conventions 3.2 reads `OK`, `NEEDS_SPLIT`, `LIMIT` and does not contain it; the row "Characters, Gauss
sums, local factors" does. conventions:886-887 says the class character functions "are the same functions",
which points to the character row, but the design never says so. This is a third status-row question
besides D3-1 and D3-2, and it is not reported as one. The same gap exists for `adf_qclass_add_rat`, which
is `void` while `adf_qclass_add` returns a status: conventions:199 lists the types whose arithmetic is
`void` and `adf_qclass` is not among them.

### R9. F4: the finding is real, the line number is not

"The oracle checks proper inclusion" is right: `compare([{0} x {0}], translated) = (False, True, True)`
in my own implementation, with the translated pieces
`[0, 4294967297/3298534883328] x {0}` and `[3298534883327/3298534883328, 1] x {1}`. P10 step 3 gives
containment, not equality, so the PLAN test must not assert equality after an inexact translation. But the
sentence "translation by a rational does not change the pieces" is on SPEC:413-414, not 414-415.
conventions:756-757 is cited correctly.

### R10. D3-1 is sound; it needs one extra sentence

The recommendation, a status plus a truth value for the three set queries, is the only one that keeps the
commitments of conventions 3.1 and the piece limits. Two additions: the exception must name the three
functions `equal_set`, `contains`, `overlaps` and say that the point comparisons keep their `CMP` codes, and
it must say that `LIMIT` is reported for the budget of section 2.3 with the preflight of R5.

### R12. The descent to `A/Q` is cited to the wrong step of analysis Lemma 2

Section 3.1 says "In particular step 2:87-88 proves that psi descends to `A/Q`", and
`adf_qclass_psi_tate` gives "Source: analysis L2:87-88 and Q4". Lines 87-88 are step 2, which proves
`psi(q) = 1` for rational `q`, that is triviality on `Q`. The descent is step 4, lines 92-94: subtract
`sum_p fp_p(x_p)`, then `floor(x_inf - q)`, and use the uniqueness of the representative in
`[0,1) x Zhat`. The conclusion the design needs is right; the reference is to a step that proves
something else.

### R11. F2, F3, F5 reproduced

- F2: `[0,1] x (0 + 2 Zhat)` equals itself under Q2, and the conventions rows conflict, 753-755 against 200.
- F3: the lift `(0 ; 0 mod 1/2)` has radius `1/2` and fails the integral-radius test; its reduction is
  `[{0},{0}] x Zhat` and `[{1/2},{1/2}] x Zhat`, both of radius 1, and both images together are `{+1,-1}`.
  The status changes with the representation, the image does not.
- F5: `[15/16 +/- 1/16] = [7/8,1]` contains `[9/10,1]` and lies in `[0,1]`, so the sentence "cannot keep
  inside `[0,1]`" at conventions:737 is false as a general claim.

## What was tried and found nothing

- Reduction: 720 combinations of centre, radius (including `1/2, 1/3, 2/3, 3/2`), interval end points
  crossing 0, 1, 2 and 3 integers, widths 0, 1/10, 1, 5/2, 1/2, and translation by `-7/11`. The reduced
  list and the original ball gave the same membership on 208200 rational-grid comparisons. No difference.
- Piece count: for 153 integer-radius intervals the raw construction count is `k+1` (`k` integers strictly
  inside) or 1 for a point, in every case. For 140 fractional radii the raw count is the sum of the
  per-ball counts. The stored count is smaller in 33 of 153 integer cases and 2 of 140 fractional cases,
  always by deduplication only, for instance `[0,2] x (0 mod 1)` builds two pieces that are one set. That
  is what M0-D4 and quotient P6 say: the count is of the construction.
- Q2: 27 hand-made pairs, including zero-radius fibres, pieces equal after a shift of the finite centre
  by 1, a shift of the real part by 1, spill above and below, glued end points, `mod 2` against `mod 3`,
  `mod 4` against `mod 6`, a coset against two of its integer points. Then 300 random families. The
  design's algorithm, implemented by me from the text, agreed with brute-force membership in every case.
- Q3: 1024 sums and 32 negations of balls with fractional radii, compared with the pairwise construction.
- Q4: 360 hulls, 7 diameters, 64 pairs of balls for additivity of psi as a set of angles.
- Q5: 138 pairs `(N,width)`; every one of the tested members is present when `width >= N`, and 76 explicit
  missing points were found when `width < N`.
- Character: `(0 ; 1/3)` has phase `E(1/3)`, non-trivial, which is what PLAN 3.2 asks for; `psi_inf` is
  `E(-x)`, so `(1/3 ; 0)` has phase `E(2/3)`; `fp_p(1/p^n) = 1/p^n`, matching `tate-poonen:notes.txt:700`,
  and the sum of the twelve local fractional parts of `a` is `a` mod 1.
- ABI: `sizeof(adf_adele_struct) = 96`, `sizeof(adf_fball_struct) = 48`, the qclass struct is 24 bytes with
  offsets 0, 8, 16 and alignment 8, all as section 1 says.
- Q1: the excess bound `2 eta + 2^-28 d` held for 2640 pieces at five precisions from 2 to 128.

## Weaknesses of the author's oracle

Fifteen groups, mutation run in `lanes/q-review1/mutate_oracle.py` on scratch copies.

| Mutant | First group that notices | Result |
|---|---|---|
| sign of the character | `phases` | killed |
| piece count off by one | `reduction` | killed |
| closed upper end made open | `reduction` | killed |
| radius rounded to nearest instead of up | none | survives |
| true inward rounding (down, no successor) | `rounding` | killed |
| midpoint invariant assertion deleted | none | survives by construction |
| midpoint moved by `1/4` | `rounding` | killed |
| finite radius ignored in the phase | `phases` | killed |
| the boundary glue deleted | none | survives |
| refine a modulus to one residue | `sets` | killed |
| containment direction reversed | `sets` | killed |
| equality tested in one direction only | `sets` | killed |
| overlap ignores exact points | `examples_findings` | killed |
| hull ignores the radius | `hulls` | killed |
| radius zero treated as modulus one | `sets` | killed |

Three of these matter.

**The boundary glue is untested.** Deleting the two lines that add `p.m - 1` at `s = 0` leaves all fifteen
groups green. The witness: `[1/2,1] x (0 mod 3)` and `[0,1/2] x (2 mod 3)` overlap at the glued point;
`compare` returns `(False, False, True)` with the glue and `(False, False, False)` without it. The design's
fault 3 says the wrong glue is killed by "the modulus-3 boundary witness in `check_fault_witnesses`"; that
assertion only distinguishes `m -> m+1` from `m -> m-1`. It does not notice a missing glue. My check
`D_q2_named` covers this pair and my brute force agrees with the design.

**The rounding direction is not determined.** Rounding the radius to nearest and then taking the successor
passes every group, because the successor alone gives the enclosure. So "round up" is not pinned by the
oracle, and the bound of Q1 is a one-sided test. This is not a defect of the kernel, only a gap in the
evidence; the C test in section 5 should compare against the exact kernel of Q1, not against the bound.

**The weakest group.** `fault_witnesses` is the weakest. It is the first line of defence in the design's
fault list and it is the first to notice no mutant at all: all nine killed mutants are caught earlier by
`reduction`, `sets`, `arithmetic`, `rounding`, `phases` or `hulls`. `examples_findings` is next: it
notices only the mutant that ignores exact points in the overlap, and several of its assertions are the
F1 to F5 witnesses that this review reproduces independently.

## Verdicts on F1 to F5 and D3-1 to D3-3

- F1: real, MINOR (R4). SPEC:402 needs `N > 0`; conventions:745 needs no repair, but must not be quoted
  as an "exactly when" sentence.
- F2: real and correctly diagnosed. No repair beyond D3-1.
- F3: real and correctly diagnosed. No repair beyond D3-3.
- F4: real, MINOR (R9). The PLAN test must translate exactly or call `adf_qclass_add_rat`.
- F5: real. CV-45 and M0-D4 stand; only the reason sentence overstates.
- D3-1: recommendation sound. Alternative "unbounded bool" is fairly stated and unsafe; "bounded undecided"
  is the same function under another name. Repair R10.
- D3-2: recommendation sound, MINOR (R5). The alternative "count merged output" needs a merging algorithm
  that does not exist; "tighter kernel" is fine; "unbounded bit work" contradicts D3-1.
- D3-3: recommendation sound. The alternative "singleton image" would reject the real uncertainty that
  adele strict accepts; "new provenance fields" conflicts with 5.10. One sentence is missing: the status
  may change under an exact reduction that preserves the image, and that must be said in the header.

## Repairs

R1 (Q1 step 3), replace the sentence:

    Let `s = 2^(e-30)` be the spacing of the binade `[2^(e-1), 2^e)` holding `u`; then `2s = 2^-28 u`.
    Since `u` is the least 30-bit number at least `d`, we have `2^(e-1) < d` and `u - d < s`.
    If `u < 2^e` then `rho = u + s`, so `rho - d < 2s = 2^-28 u <= 2^-28 d`.
    If `u = 2^e` then `d > u - s` and `rho = u + 2s`, so `rho - d < 3s`, while
    `2^-28 d > 2^-28 (2^e - s) = 4s - 2^-28 s > 3s`.

R2 (Q1), replace the hypothesis line and add a sentence:

    Given exact rational `l <= h` with `l >= 0` and `h <= 1`, ... (as now). The statement and its excess
    bound use only `l <= m <= h`, so they hold unchanged for `-1 <= l <= h <= 1`; only the claim about the
    midpoint needs `h <= 1`. Section 3.3 uses the second form, for which the midpoint is unrestricted.

R3 (Q4 step 2), insert before the sentence about the minimum:

    For every real `t`, `dist(t, Z) = 1/2 - dist(t - 1/2, Z)`: write `u = t mod 1`, then
    `dist(t,Z) = min(u, 1-u)` and `dist(t-1/2, Z) = 1/2 - min(u,1-u)`. Hence the largest circle distance
    from `0` over the image is `1/2 - d(1/2)`, and `cos(2 pi (1/2 - x)) = -cos(2 pi x)`. The same identity
    with `1/4` gives the imaginary coordinate.

R4 (F1), replace the first sentence of F1:

    SPEC:402-403 says the image is all of `A/Q` exactly when the real width is at least `N`.
    conventions:745 states only the sufficient direction.

R5 (section 2.3), insert before the budget sentence:

    The normalization is itself bounded: before the loop of R step 2, reject `B > work_limit`, and
    accumulate the integer-range lengths of R step 4 with saturation at `work_limit + 1`. Only a value
    that passes this preflight is normalized.

R6 (section 2.3), replace the cost line:

    Cost: `K` exact pieces; `O(K log K)` endpoint sorting; `(2E+1) K L` fiber work, which is
    `O(K^2 L)` because `E <= 2K`; memory `O(K+L)` plus exact integers.

R7 (section 3.2), replace the `_at` declaration comment:

    `psi_v` on the projection of `x` at `v`. At infinity use `E(-I)`; at a prime use `a + p^v_p(N) Z_p`
    with `N = 0` exact. `DOMAIN` if `v` is not a place of `x` (an adele with `arch = 0` has no
    archimedean place), with `where` set to `v`. Both variants permit real uncertainty. No factorization.
    LIMIT precedence and costs as above; no member aliasing.

and add the `where` argument to both declarations.

R8 (section 3.2), add one sentence after the class calls:

    These four class calls belong to the row "Characters, Gauss sums, local factors" of conventions 3.2,
    not to the row "Quotient by `Q`", because they return `NOT_DETERMINED`.

R9 (F4), replace the line reference:

    SPEC:413-414 and conventions:756-757 say translation does not change the pieces.

R10 (D3-1), add to the recommendation:

    Amend conventions 3.2 so that the row "Set predicates" reads `OK`, `LIMIT` for the three quotient set
    queries `adf_qclass_equal_set`, `adf_qclass_contains`, `adf_qclass_overlaps`, with `truth` untouched
    on `LIMIT`; the point comparisons keep their `CMP` codes. The budget is the one of R5.

R12 (section 3.1 and the class call), replace the two references:

    Step 4:92-94 of analysis Lemma 2 proves that psi descends to A/Q.

## Checks

    timeout 170 python3 lanes/q-review1/review_checks.py
    15 sections, exit 0, 72.8 s. Counts printed per section:
      A_reduction_image 720 cases, 208200 membership comparisons
      A2_rounded_superset 360 cases, 414222 comparisons
      B_piece_count 293 cases, 153 integer-radius intervals, 0 k+1 mismatches, 33 deduplications
      C_q1 3246 cases, 0 excess-bound violations, worst (rho-d)/d = 4095/1099511627777
      C2_q1_negative_midpoint 374 cases
      D_q2_named 27 pairs, 0 differences
      D2_q2_random 300 pairs, 0 differences
      E_q3_arithmetic 2048 cases, 301500 comparisons
      F_q4_hull 360 images, 0 differences at 1e-60
      F2_q4_width 7 arcs
      F3_q4_additivity 64 pairs, 576 witnesses
      G_q5_positive_radius 138 pairs, 76 missing points
      G2_q5_zero_radius 5 widths
      H_examples_findings 6 witnesses
      I_character_sign 318 cases

    timeout 175 python3 lanes/q-review1/mutate_oracle.py
    15 mutants plus the unmutated copy; results in the table above.

    gcc -I include -o lanes/q-review1/abi/sizes lanes/q-review1/abi/sizes.c && ./lanes/q-review1/abi/sizes
    adf_adele_struct 96/8, adf_fball_struct 48/8, qclass 24/8 with offsets 0, 8, 16.

What would have made a case fail: a reduction that dropped the endpoint piece, a Q2 comparison that missed
the glued class, a hull computed from arc end points only, a Q1 radius rounded inward, a piece count of
`k` instead of `k+1`, a phase that ignored `B`. Each of these fails an assertion in the section named.

## What I did not examine

- The C implementation: none exists yet. No allocation, aliasing, ABI call, driver grammar or Julia
  binding was exercised. The section 5 acceptance list was read but not tested.
- The dump grammar beyond the one-line body of section 2.5 and the M1-D9 reference; no dump byte was
  parsed or produced.
- `tests/golden/qclass.tsv` row by row. The author's checker was read; I did not re-derive its 17
  enclosure rows and 12 malformed rows.
- The 3.3 and 3.4 material (Gauss sums, root numbers, gamma and epsilon factors) beyond the naming and
  the sign of `tau`, which I checked against `acb_dirichlet.rst:358-362`.
- Section 3.4's claim about the FLINT Conrey label; the source is not on disk, as the design says.
- The cost estimates, beyond the one inconsistency in R6.
- Local-backend CRT and cancellation: no local value was constructed. The design's requirement of one
  reducible raw triple with `d` becoming 1 is untested here.