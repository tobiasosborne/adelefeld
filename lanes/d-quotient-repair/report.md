# Lane d-quotient-repair: report

Date 2026-10-06. Repairs of the review `docs/reviews/m3-design/review.md` (lane q-review1) applied to
`docs/api-3.md`, the decisions D3-1 to D3-3 written into section 6 as taken, and
`proto/quotient3_checks.py` strengthened against the review's surviving mutants.

Files I own and changed: `docs/api-3.md`, `proto/quotient3_checks.py`, and this lane directory
(`report.md`, `check_repairs.py`, `mutate_oracle.py`, `logs/`, `mutants/`). Nothing else was written;
no git command that changes state and no `bd` was run.

## 1. Files written

- `docs/api-3.md`: repairs R1 to R10 and R12, the decisions as taken, a new section 9.
- `proto/quotient3_checks.py`: an independent Q1 radius kernel `q1_radius`, the rounding-direction
  cases, the glued-pair family and a `brute_sets` helper in `check_sets`, the section 1 invariant
  block and the docstring in `check_fault_witnesses`.
- `lanes/d-quotient-repair/check_repairs.py`: exact-rational checks of the repair texts R1 and R3.
- `lanes/d-quotient-repair/mutate_oracle.py`: copy of `lanes/q-review1/mutate_oracle.py` with the
  output directory changed to `lanes/d-quotient-repair/mutants/`. The mutants, the mutant list and
  the run order are unchanged.
- `lanes/d-quotient-repair/logs/`: `oracle_after.log`, `mutants_after.log`, `check_repairs.log`,
  `review_checks.log`.
- `lanes/d-quotient-repair/mutants/`: the 16 scratch copies written by the mutation run.

## 2. Repairs, one by one

R1 (Q1 step 3). APPLIED, with the text corrected. The repair text of the review ends case one with
`rho - d < 2s = 2^-28 u <= 2^-28 d`. That chain is false: `u >= d`, so `2^-28 u <= 2^-28 d` has the
wrong direction, and `2s = 2^(e-29)` is `2^-28 2^(e-1)`, not `2^-28 u` (they are equal only when
`u = 2^(e-1)`). The applied text is

    Let `s = 2^(e-30)` be the spacing of the binade `[2^(e-1), 2^e)` holding `d`; then
    `2s = 2^-28 2^(e-1)`. Since `u` is the least 30-bit number at least `d`, we have `2^(e-1) <= d`
    and `u - d < s`. If `u < 2^e` then `rho = u + s`, so `rho - d < 2s = 2^-28 2^(e-1) <= 2^-28 d`.
    If `u = 2^e` then `d > u - s` and `rho = u + 2s`, so `rho - d < 3s`, while
    `2^-28 d > 2^-28 (2^e - s) = 4s - 2^-28 s > 3s`. So `rho - d <= 2^-28 d` in both cases.

The review's own proof of the two cases is unchanged; only case one was re-derived. Step numbering and
the conclusion are as before. Checked numerically, see section 5.

R2 (Q1 hypothesis, and section 3.3). APPLIED as given. Q1 now says that the enclosure and the excess
bound use only `l <= m <= h` and hold unchanged for `-1 <= l <= h <= 1`, while only the midpoint claim
needs `h <= 1`; section 3.3 now names the second form instead of saying "Use sign symmetry for
negative midpoints", and adds that a hull coordinate is not a stored piece, so the invariant of
section 1 does not apply to it.

R3 (Q4 step 2). APPLIED with the case split written out, because the review's one-line derivation of
`dist(t-1/2,Z) = 1/2 - min(u,1-u)` needs the two cases of `u` to be a proof rather than an assertion.
Both cases are in the applied text, together with `cos(2 pi (1/2-x)) = -cos(2 pi x)` and the sentence
about the largest circle distance.

R4 (F1). APPLIED as given: `SPEC:402-403` is cited for the "exactly when" sentence, `conventions:745`
for the sufficient direction only.

R5 (section 2.3). APPLIED as given, inserted before the budget sentence, with one added sentence: the
cost of the normalization does not depend on the numeric value of `denominator(N)`, and a LIFT whose
finite radius is `1/10^100` is refused before the loop. Algorithm R step 4 now refers to this preflight
by name, since D3-2 requires it wherever a count can be huge.

R6 (section 2.3). APPLIED as given.

R7 (section 3.2 `_at`). APPLIED, with the premise corrected. The repair text says "`conventions 5.5`
allows `arch = 0`, so an adele can have no archimedean place". That is false for the argument type:
`adf_adele_struct` is `{ arb_t inf; adf_fball_struct fin; }` (`docs/conventions.md:569`), it has no
`arch` field, and `include/adelefeld/adele.h:119-128` gives the real coordinate unconditionally.
`arch = ADF_ARCH_NONE = 0` belongs to `adf_sball` (`conventions:697-710`, places at :708), which is a
partial ball over a set of places and can have none. The applied text keeps the `DOMAIN` rule with
`where = v`, states that for a canonical `v` the case is empty for the adele signature, and says which
type the `arch = 0` sentence applies to. Both declarations take `adf_place_t *where` after the value
output, per `conventions:133-138`; `where` may be NULL and is untouched on OK, per
`include/adelefeld/rfunc.h:160`.

R8 (section 3.2). APPLIED as given: the four calls returning `NOT_DETERMINED` belong to the row
"Characters, Gauss sums, local factors" (`conventions:223`), not to the row "Quotient by `Q`"
(`conventions:222`). The second half of R8, the `void` status of `adf_qclass_add_rat`, was not a
repair text; it is now recorded as an open status-row question in the comment of section 2.4 with the
citation `conventions:198` (the review says 199; line 198 is the row of the void ring arithmetic, 199
is the next row, see section 7 below).

R9 (F4). APPLIED as given: `SPEC:413-414`.

R10 (D3-1). APPLIED: the amendment of the row "Set predicates" is written into D3-1 in section 6 as
part of the decision. `docs/conventions.md` itself is not changed by this lane; the sentence to paste
is in section 8 below.

R11. No text change needed; the review lists it as reproduced findings, not as a repair.

R12 (section 3.1 and the class call). APPLIED as given: the descent to `A/Q` is step 4:92-94. The
applied text also says what step 2:87-88 proves (`psi(q) = 1` for rational `q`), so the two are not
confused again.

## 3. The decisions as taken

Section 6 is retitled "Decisions D3-1 to D3-3, taken 2026-10-05"; the table column "Recommendation" is
now "Decision"; the rejected alternatives are kept as such. Section 1, the resource policy, now reads
as a decision taken. The comment blocks of sections 2.2, 2.3 and 3.2 no longer say "proposal" or "if
D3-x is taken": D3-1 is stated as taken in the preamble of 2.3, D3-2 in section 1 and in algorithm R
step 4, D3-3 in the comment of `adf_qclass_psi_tate_strict`. The "Decision first:" lines of section 7
now say "Decision: D3-x, taken". The strict-class comment carries the sentence the review asked for:
the status may change under an exact reduction that preserves the image set (E3 lift NOT_DETERMINED,
its two-piece exact reduction OK, both images `{+1,-1}`), and that sentence is part of the contract.

Declarations in `docs/api-3.md`: 40 before, 40 after. `where` arguments added: 0 before, 2 after
(`adf_adele_psi_tate_at`, `adf_adele_psi_tate_strict_at`).

## 4. The oracle

### 4.1 Counts before and after

`timeout 120 python3 proto/quotient3_checks.py`, exit 0 both times, 15 checks both times.

| group | before | after |
|---|---|---|
| reduction | 288 cases, 19470 comparisons | 288 cases, 19470 comparisons |
| count_limit | 18 | 18 |
| sets | 120 cases, 37720 pairs | 188 cases, 37720 pairs, 68 glued-endpoint pairs |
| arithmetic | 600 | 600 |
| rounding | 363 | 735, of which 171 required radii pinned against `q1_radius` |
| phases | 335 | 335 |
| hulls | 98 | 98 |
| local_images | 128 | 128 |
| ball_additivity | 180 cases, 1354 witnesses | 180 cases, 1354 witnesses |
| golden_phases | 15 | 15 |
| golden_qclass | 29 (17 valid, 12 status rows) | 29 (17 valid, 12 status rows) |
| full_and_width | 66 | 66 |
| gauss_boundary | 17 | 17 |
| examples_findings | 11 | 11 |
| fault_witnesses | 12 | 24, of which 12 midpoint and enclosure checks |

What was added, and why.

- `q1_radius(d)`: Q1's radius stated a second time, in exact rationals with integer arithmetic,
  independently of `round_binary`. `check_rounding` now asserts
  `(q.lo, q.hi) == (m - q1_radius(need), m + q1_radius(need))` for 372 new pieces with denominators
  7, 9, 11, 13, 17, 19, 23, 29, 31, 37 at precisions 20 and 53, and reports that 171 of the 732
  required radii have a nearest 30-bit value strictly below them. Those are the cases that pin the
  rounding direction; the successor alone would enclose in the other cases.
- The boundary glue: `check_sets` now also runs, for every modulus from 2 to 8 and every centre `c`,
  the pair `[1/2,1] x (c mod M)` against `[0,1/2] x (c-1 mod M)`, which meet only in the glued class
  `(0 ; c-1)`, and compares `compare` with the same three relations decided by `direct_member` on the
  grid (68 pairs, modulus 3 with centre 0 is the witness of the review). For moduli 3 to 8 the pair
  against `[0,1/2] x (c+1 mod M)` is also run and must be disjoint. A new helper `brute_sets` does the
  grid decision and is used by the random part of the group as before.
- `check_fault_witnesses` now says in its docstring what the group proves (one concrete input on which
  each of the twelve named alternatives gives a different answer) and what it does not prove (that no
  other alternative passes; that is the mutation run's business). It also repeats the storage rule of
  section 1 on four pieces with non-dyadic end points, at three precisions: the stored midpoint lies in
  `[0,1]` and the stored ball encloses the exact one.

### 4.2 Mutants before and after

`timeout 170 python3 lanes/d-quotient-repair/mutate_oracle.py`, 8.4 s, exit 0 (the script reports the
mutants it ran). 15 mutants plus the unmutated copy; the mutant list is the review's, unchanged.

| Mutant | first group before | first group after |
|---|---|---|
| M1 sign of the character | phases | phases |
| M2 piece count off by one | reduction | reduction |
| M3 closed upper end made open | reduction | reduction |
| M4 radius rounded to nearest instead of up | none (survived) | rounding |
| M5 midpoint invariant assertion deleted | none (survived) | none (survived), see 6.3 |
| M5b midpoint moved by 1/4 | rounding | rounding |
| M6 finite radius ignored in the phase | phases | phases |
| M4b inward rounding without successor | rounding | rounding |
| M8 refine a modulus to one residue | sets | sets |
| M9 containment direction reversed | sets | sets |
| M10 equality in one direction only | sets | sets |
| M11 overlap ignores exact points | examples_findings | examples_findings |
| M12 hull ignores the radius | hulls | hulls |
| M13 radius zero treated as modulus one | sets | sets |
| M7 the boundary glue deleted | none (survived) | sets |

14 of the 15 are killed; M5 is not, and cannot be, see 6.3. The brief asks for 15; that part of the
brief is not satisfiable and I did not weaken anything to pretend otherwise.

## 5. Checks run, with results

1. `timeout 100 python3 lanes/d-quotient-repair/check_repairs.py` (new, exit 0, 0.3 s, 1574 checks).
   Exact rationals, the kernel of the oracle imported, not re-implemented:
   - 374 radii `d = 2^e + offset` for `e` from -40 to 0 and seven offsets, including offsets of
     denominator `2^40` so that `d` is not a 30-bit number; 354 with `u < 2^e`, 20 with `u = 2^e`.
   - `rho - d <= 2^-28 d` in every case. Worst `(rho-d)/d = 4095/1099511627777 = 3.7244e-09`, bound
     `2^-28 = 3.7253e-09`, fraction of the bound 0.999756. This is the same worst value the review
     measured in its section `C_q1`.
   - Case `u < 2^e`: worst `rho-d = 4095/1099511627776` at `d = 1/2 + 549755813889/1099511627776`,
     fraction of the bound 0.999756.
   - Case `u = 2^e` (the successor is two spacings): worst `rho-d = 11/1099511627776` at
     `d = 2^-8 - 3/2^40`, fraction of the bound 0.687500; at `e = 0`, `d = 1 - 3/2^40` gives
     `rho-d = 2051/2^40` and fraction 0.500732, `d = 1 - 1/2^40` gives 0.500244.
   - `2s = 2^-28 2^(e-1) <= 2^-28 d` and `4s - 2^-28 s > 3s` verified in the two cases separately.
   - R3: 1200 rational `t` with `dist(t,Z) = 1/2 - dist(t-1/2,Z)` exactly, and
     `cos(2 pi (1/2-x)) + cos(2 pi x)` below 1e-55 at 60 digits for four values of `x` each.
2. `timeout 120 python3 proto/quotient3_checks.py`, exit 0, 0.8 s, 15 checks, counts in 4.1.
3. `timeout 170 python3 lanes/d-quotient-repair/mutate_oracle.py`, exit 0, 8.4 s, table in 4.2.
4. `timeout 170 python3 lanes/q-review1/review_checks.py`, unchanged, exit 0, 57 s, 15 sections, counts
   identical to the review's own run: A_reduction_image 720 / 208200, A2_rounded_superset 360 / 414222,
   B_piece_count 293, C_q1 3246 with worst `4095/1099511627777`, C2 374, D 27 pairs 0 differences,
   D2 300 pairs 0 differences, E 2048 / 301500, F 360, F2 7, F3 64 / 576, G 138 / 76 missing points,
   G2 5, H 6, I 318.
5. Length and end-of-file: no line of `docs/api-3.md`, `proto/quotient3_checks.py` or
   `lanes/d-quotient-repair/*.py` is longer than 116 characters; all three end with a newline.

## 6. Findings

### 6.1 The repair text R1 is false as written

`2s = 2^-28 u <= 2^-28 d` (review, R1, case one) cannot hold: `u` is the least 30-bit number at least
`d`, so `u >= d`, and `2^-28 u <= 2^-28 d` is the wrong direction. The correct comparison is
`2s = 2^-28 2^(e-1) <= 2^-28 d`, which uses `2^(e-1) <= d`. I applied the corrected text; the
statement Q1 proves is unchanged and still true. Anyone who pastes the review's text verbatim writes
a false inequality into the proof.

### 6.2 The premise of R7 about `arch = 0` is false for an adele

`adf_adele_struct` has no `arch` field (`docs/conventions.md:569`); the archimedean coordinate is
always there (`include/adelefeld/adele.h:121-128`). `arch = ADF_ARCH_NONE = 0` is a field of
`adf_sball_struct` (`conventions:700`), whose place set contains the archimedean place only if
`arch != 0` (`conventions:708`). So the sentence "an adele with `arch = 0` has no archimedean place"
belongs to `adf_sball`. The conclusion of R7 (a `where` argument, `DOMAIN` with `where = v`) stands;
its reason does not, and the applied text says so.

### 6.3 The mutant M5 cannot be killed

M5 deletes the line `assert 0 <= (q.lo+q.hi)/2 <= 1` from `check_rounding`. The program computes
exactly the same values before and after; only an assertion of a fact that still holds is removed.
No oracle, however large, can distinguish the two texts from the outside, so "all 15 mutants must be
killed" is not satisfiable for this mutant. What can be done, and is done: the invariant is now
asserted in a second group as well, so that an implementation that violates it is caught in two
places, and `check_fault_witnesses` says in its docstring that it is not evidence against a mutation
that only deletes an assertion. The other 14 mutants are killed.

### 6.4 Two smaller things in the design, corrected

- Section 5, fault 3 said the modulus-3 boundary witness in `check_fault_witnesses` kills the wrong
  glue. The review is right that this witness only distinguishes `m -> m+1` from `m -> m-1`; a
  deleted glue is a different fault. The fault list now says so and points at the glued-pair family
  of the `sets` group for the deleted glue.
- The review cites the void ring-arithmetic row as `conventions:199`. The row is `conventions:198`;
  199 is the next row. The design's own citation of `conventions:200` (set predicates) is correct.

### 6.5 Status rows that conventions 3.2 does not cover

Three, not one: the set predicates (D3-1, decided, amendment requested), the class character calls
(return `NOT_DETERMINED`, so they belong to the character row, R8), and `adf_qclass_add_rat`, which
is `void` while the row of `conventions:198` lists no `adf_qclass` type and `adf_qclass_add` returns a
status. The design records the third as open. Sentences to paste are in section 8.

Sources pending: none new. The two `[source pending: ...]` markers already in `docs/api-3.md` (the
Tate thesis section number, the FLINT Conrey labelling) are unchanged and still pending.

## 7. For the orchestrator

Ready-to-paste sentences. I did not change these files.

1. `docs/SPEC.md:402-403`, the full-image sentence.
   Present: "The image is all of `A/Q` exactly when the real width is at least `N`. **[proved]**
   (`proofs/quotient.md` Propositions 5, 6, 7) **[design]** (M0-D4)".
   Replacement: "For `N > 0` the image is all of `A/Q` exactly when the real width is at least `N`; for
   `N = 0` the image is never all of `A/Q`, whatever the width. **[proved]** (`proofs/quotient.md`
   Propositions 5, 6, 7) **[design]** (M0-D4)".
   Reason: F1, reproduced; `(1/2 ; 0)` is missing from the zero class. `docs/conventions.md:745` needs
   no change; it states only "When `hi - lo >= N` the image is all of `A/Q`" and should not be quoted
   as an "exactly when" sentence.

2. `docs/conventions.md:200`, the row "Set predicates" (decision D3-1, taken 2026-10-05).
   Present: "| Set predicates (`equal_set`, `overlaps`, `contains`) | no status: they return `int` 0 or 1
   (2.3; the other predicates likewise) |".
   Replacement: "| Set predicates (`equal_set`, `overlaps`, `contains`) | no status: they return `int` 0
   or 1 (2.3; the other predicates likewise); exception: the three quotient set queries
   `adf_qclass_equal_set`, `adf_qclass_contains`, `adf_qclass_overlaps` return `OK` with the truth
   value written, or `LIMIT` with `truth` untouched (N-D21); the point comparisons keep their `CMP`
   codes |".

3. `docs/conventions.md:198`, the void ring-arithmetic row (status row of `adf_qclass_add_rat`).
   Present: "| Ring arithmetic of `adf_rat`, `adf_fball`, `adf_adele`, `adf_cadele` (add, sub, neg,
   mul, scale by exact rational) | none: `void` |".
   Replacement: "| Ring arithmetic of `adf_rat`, `adf_fball`, `adf_adele`, `adf_cadele` (add, sub, neg,
   mul, scale by exact rational), and `adf_qclass_add_rat` (translation by a rational, the identity on
   every class) | none: `void` |".
   Without this, `adf_qclass_add_rat` has no row at all, while `adf_qclass_add` and `adf_qclass_neg`
   need one; the natural row for them is "Quotient by `Q`" as it stands.

4. `docs/conventions.md:222`, the row "Quotient by `Q`" (R8).
   Present: "| Quotient by `Q` | `OK`, `NEEDS_SPLIT`, `LIMIT` |".
   Replacement: "| Quotient by `Q` | `OK`, `NEEDS_SPLIT`, `LIMIT`; the class character calls
   `adf_qclass_psi_tate`, `adf_qclass_psi_tate_strict` and `adf_qclass_psi_tate_phase` belong to the
   row below, not to this one, because they return `NOT_DETERMINED` (`conventions.md` 6.1, N-D21) |".
   Alternative, if a pointer is not wanted there: add that sentence to 6.1 next to
   `conventions.md:886-887`, which says the class character functions are the same functions.

5. `docs/PLAN.md:320`, the translation test of slice 3.1 (F4).
   Present: "... fractional radii; equality of the sets after translation by a rational; ...".
   Replacement: "... fractional radii; equality of the sets after an exact translation by a rational
   (translate exact rational intervals, a dyadic `q` exactly representable in the test, or call
   `adf_qclass_add_rat`; never assert equality after an inexact adele translation, which encloses
   rather than equals); ...".
   Reason: P10.3:240 gives containment after outward rounding; the oracle checks proper inclusion,
   `compare([{0} x {0}], translated) = (False, True, True)`.

6. `docs/SPEC.md` 15.4, the new row N-D21 (you write it). Text I would use, for you to shorten or
   replace, in one fenced block so that the pipes do not read as a table:

```
| N-D21 | Quotient sets and the Tate additive character (`docs/api-3.md`, milestone 3):
   status rows, construction and rounding policy, meaning of class `_strict` | `include/adelefeld/
   qclass.h`, `psi.h`: the three set queries `adf_qclass_equal_set`, `_contains`, `_overlaps` write a
   truth value on `OK` and leave it untouched on `LIMIT` (row "Set predicates" amended); `LIMIT` for a
   raw piece count above the argument, the split count `B` above `work_limit`, an absolute binary
   exponent above `2^20`, a projected numerator or denominator above `2^21` bits, and no full-image
   shortcut; rounding kernel Q1 (30-bit radius rounded up, then its successor), the excess bound
   `2 eta + 2^-28 d`; `_strict` on a class certifies each stored representative and its status may
   change under an exact reduction that preserves the image; `adf_adele_psi_tate_at` and its strict
   variant take `adf_place_t *where` and return `DOMAIN` when `v` is not a place of `x`; the class
   character calls belong to the row "Characters, Gauss sums, local factors". Designed by lane
   d-quotient (codex), repaired by lane d-quotient-repair after the review q-review1, taken by the
   orchestrator on 2026-10-05 before the code. Open to reversal: the exact bit caps; the conservative
   budget of section 2.3. |
```
   The full text of D3-1, D3-2 and D3-3 is section 6 of `docs/api-3.md` as it now stands.

7. One item for the plan, not a repair: lane q-slice2 is writing `src/qclass.c` and
   `tests/` from `docs/api-3.md` 2.1 and 2.4. `adf_qclass_add_rat` is `void` there and stays `void`
   here; only item 3 above is open, and it does not change the signature.
