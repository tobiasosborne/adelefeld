# Report: lane o-decisions1

Ten decisions of the orchestrator (TJO 2026-10-15, 00:00) recorded. Text only; no code, build, test, git or bd.

## Files written

`docs/SPEC.md` (N-D21 last sentence, N-D23 one sentence, new row N-D25 at line 948), `docs/api-4.md`,
`docs/api-5.md`, `docs/api-3.md`, `lanes/o-decisions1/report.md`. Checks: `awk 'length>116'` on the three api files
gives 0 lines; all files end with a newline; `git diff --numstat` shows SPEC 3/2, api-3 3/1, api-4 9/3, api-5 2/1.

## Per item (before -> after)

1. `docs/api-4.md` F4 step 3 (now lines 180-184). Before: "R_k <= (...)/M + delta_div. / Each delta is bounded by
   directed arf rounding, or already included by the acb operation." After: "R_k <= (...)/M + delta_div + rho_k.
   The term rho_k, of the order L 2^-30 (sum_j r_j)/M, is the upward 30-bit rounding of the radii in acb_mul,
   acb_add, acb_div_ui (m4-review1, finding 1); tests/test_ffun.c:216-220 allow it inside
   4 sum_j(rad_re+rad_im)/M. Each delta is ..." The test allowance (tests/test_ffun.c:216-220) is
   B = 4 sum_j(rad_re+rad_im)/M + 128 L^2 max(1,...) 2^-p/M; its factor 4 covers rectangle propagation and the
   radius
   rounding together; I state it so, I did not derive the constant (the review gives "about L 2^-30 sum r_j / M").
2. `docs/api-4.md` section 2. Before (line 106): "Dumping then loading is identical, including retained zero terms."
   After (lines 107-108): "Loading the dump is identical, including retained zero terms, when the load limits and
   D1 admit the dumped representation (else LIMIT; include/adelefeld/dump.h:288-303)." Load comment (line 100)
   before:
   "/* Same load; require nbinds=0 after full validation. binds is then ignored. */" After: same plus "Load cost is
   the
   input size plus clearing the old x, which is the size of the old representation (dump.h:288-303, f4-slice7
   HEADER-FINDING 2)." Recorded in `lanes/f4-slice7/report.md:242-252` and `docs/api-4a.md:321,334`,
   `docs/api-4b.md:273,280`. NOTE: the brief names the headers ffun.h and rfun.h; the text lives in dump.h:288-303
   (ffun.h and rfun.h have none), so I cited dump.h.
3. `docs/SPEC.md` N-D23 (line 946): after "(the theta witness)." added "The stall rule of `adf_tensor_poisson` (lane
   f4-slice6, G4) is taken: a precision doubling from 64 bits that does not halve the largest radius stops with
   `NOT_DETERMINED`." (matches src/poisson.c:509-512, PN_STALL_PREC 64.)
4. `docs/api-4.md` adf_tensor_poisson comment. Before: "Cost L^2 plus real transforms and L(2NL+1)+2NR+1 real
   evaluations; charge polynomial sizes, tail searches and precision retries." After: "... L(2NL+1)+2NR+1
   evaluations at
   the working precision, charged with polynomial sizes, tail searches, retries. The work cap bounds these units,
   not a
   clock: time <= cap * (cost of one evaluation at that precision, which grows with the bits)."
5. Recorded in the row only (lanes/f4-slice3/result.md:235-241). No text amendment.
6. `docs/api-5.md` line 33. Before: "Working precision starts at max(2,prec); guards count against the precision
cap."
   After: "Working precision starts at max(2,prec); guards count against the cap, except prec+32 inside the
   delegated /
   local factor of the trivial calls, which is not itself capped." (The old "line 80" is now line 81.) The clause
   "LIMIT first for prec above the cap" is already the first sentence of the paragraph (line 32), so I added only
   the
   exception, one clause.
7. `docs/SPEC.md` N-D21 (line 944). The sentence "Waits for TJO (not decided here): ... (review q-review1, last
   finding)." is replaced by "Decided (N-D25, item 7): the library implements the restricted-product reading, in
   which a
   finite ball with a fractional radius (`0 mod 1/2`) is a subset of `A_f` (every component of `(1/2) Zhat` is in
   `Z_p` except at 2) and its class in the quotient is the one SPEC 6 gives (lane q-slice3, bead adf-s1v)."
8. `docs/api-3.md` Q1 (lines 523-527). Before (523-525): "The statement and its excess bound use only `l <= m <=
h`, so
   they hold unchanged for / `-1 <= l <= h <= 1`; ..." After: "The statement and its excess bound are argued with
   `l <= m <= h`, which a rounded midpoint need not satisfy (`l = h = 1/3`, p = 2 gives `m = 3/8`); the argument
   without
   that hypothesis is in `docs/api-3b.md`, section "Coordinate rounding and excess". The statement is unchanged. It
   holds
   for `-1 <= l <= h <= 1`; ..." (api-3b.md:128-140, finding at 173-176.) Statement text itself is unchanged.
9. Recorded in the row only.
10. Recorded in the row only: fail-stop stands; cites `lanes/c-sources1/report.md` item d (dirichlet.rst:71-74,
    group_init.c:115-127, group_clear.c:17-19). I added one inference of mine, flagged for you: such a modulus is
    word-sized and above the N-D22 cap 65536, so the cap returns LIMIT before group setup in the constructors; I
    did not
    check whether any other path reaches the predicate with such a modulus.

## Line shifts (other documents cite these by line)

- `docs/api-4.md`: +1 line at 100 (load comment), +1 at 106, +2 in F4, +2 in the poisson comment.
  Dump comment: old 105-106 (2 lines) is now 106-108 (3 lines). F4: old heading 173 is now 175; step 3 (old 178-180,
  three lines) is now 180-184 (five lines). Poisson comment: old 325-329 (five lines) is now 329-334 (six lines;
  the cost sentence old 327-328 is now 331-334, two lines longer). Every amended passage grew by at most two
  lines; everything after line 100 moved by +1 to +6.
- `docs/api-5.md`: line 33 grew to 33-34 (+1); everything after moved by +1 (the old line 80 is line 81).
- `docs/api-3.md`: 523-525 became 523-527 (+2); everything after moved by +2.
- `docs/SPEC.md`: new row N-D25 at line 948 only; rows N-D21 to N-D24 keep lines 944-947; later lines +1.

## Contradictions with a decision / observations (not resolved)

- Item 2: the brief says the headers ffun.h and rfun.h state the identity; they do not (dump.h does).
- Item 6: the brief says "LIMIT decided first for prec above the cap"; `docs/api-5.md` line 32 already states that.
- Item 1: `tests/test_ffun.c:216-220` allows 4 sum(rad)/M; the review's measured excess is 0.5 (1 + 2^-26.4) for a
  radius 1/2, L = 3, i.e. about 2^-26 relative, not 2^-30; so "of the order L 2^-30" is the review's own estimate
  and
  the observed excess is roughly 2^-27 per term at L = 3. The test factor 4 is far above either. Not resolved.
- N-D22 (SPEC line 945) still says "The predicate adf_char_is_canonical never turns a resource failure into 0" and
  "Source pending ... group-init failure semantics"; item 10 does not name N-D22 so it was not touched. The source
  is now
  on disk (c-sources1).

## N-D25 row (SPEC.md line 948)

| N-D25 | The open decisions of the milestone 3, 4 and 5 reviews and lane reports, taken in one pass (10 items) | Taken by the orchestrator on 2026-10-15 on TJO's instruction of that day, 00:00: "For any open question for me: just decide it and proceed and record the decision." Lane o-decisions1 recorded them; each is open to reversal by TJO. (1) F4 of `docs/api-4.md` (m4-review1 finding 1, MAJOR against the statement, bead adf-cx8): the transform's error statement gets the term rho_k for the upward 30-bit rounding of the radii in `acb_mul`, `acb_add`, `acb_div_ui`; the code's enclosure is sound; `tests/test_ffun.c:216-220` state the same allowance. (2) `docs/api-4.md` section 2: the dump/load identity holds when the caller limits and the D1 caps admit the dumped representation (else `LIMIT`), and load's cost includes clearing the old storage; both comments now say what `include/adelefeld/dump.h:288-303` says (lane f4-slice7, HEADER-FINDINGs 1 and 2). (3) The stall rule of `adf_tensor_poisson` (lane f4-slice6, G4; `src/poisson.c:509-512`) is recorded in N-D23. (4) Cost of `adf_tensor_poisson` (m4-review1 finding 3, MINOR, bead adf-l4v): the work units are evaluations at the working precision; the running time is bounded by the work cap times the cost of one evaluation at that precision (which grows with the bits), not by a clock; no code change and no time bound in the code. (5) Lane f4-slice3's finding stands as a cost item, with no text amendment: `acb_inv` loses the dependency for a wide canonical `A` in `adf_rfun_fourier`, so such a transform is `NOT_DETERMINED` at every precision; an exact interval inverse would certify it. (6) `docs/api-5.md` line 33 against line 81 (HEADER-FINDING of lanes t5-slice1 and t5-slice3): the headers' reading stands; `LIMIT` is decided first for `prec` above the cap; below it the working precision `prec + 32` of the delegated `adf_local_zeta_factor_at` is not itself subject to the cap; line 33 carries this exception. (7) N-D21: the restricted-product reading is implemented; a ball `0 mod 1/2` is a subset of `A_f` (lane q-slice3, bead adf-s1v). (8) `docs/api-3.md` Q1 (bead adf-nyk): the statement is unchanged; the text points to the argument of `docs/api-3b.md` ("Coordinate rounding and excess"), which does not need `l <= m <= h`, and gives the counterexample `l = h = 1/3`, precision 2, `m = 3/8`. (9) `docs/api-5a.md` and `docs/api-5b.md` are merged into one statements file only after the milestone 5 review (lane m5-review1) lands, because that review cites them by line; nothing else is done now. (10) `adf_char_is_canonical` keeps its fail-stop when FLINT's group setup returns 0 (`docs/api-3d.md:24-25`; lanes c-slice1, c-slice2). Lane c-sources1 (`lanes/c-sources1/report.md`, item d) settles the source: `dirichlet_group_init` returns 0 only for an odd prime factor of the modulus above 10^16 (`dirichlet.rst:71-74`, `group_init.c:115-127`), a power of 2 never gives 0, the source is silent on q = 0 and q = 1, and a group whose init returned 0 must not be cleared (`group_clear.c:17-19`). Such a modulus is word-sized (below 2^64) and above the cap 65536 of N-D22, so for the character constructors the cap returns `LIMIT` first; the predicate fail-stops if a group of such a modulus ever reaches it. |
