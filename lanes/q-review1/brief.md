# Lane q-review1: adversarial review of the design of milestone 3, first half (`docs/api-3.md`)

**You own:** `lanes/q-review1/` only (your review is `lanes/q-review1/review.md`, your checks
`lanes/q-review1/review_checks.py` and other programs beside it). Everything else is read-only; do not edit
the design. No git command that changes state, no `bd`. At most 2 cores; every program under `timeout`; none
over 170 s. Leave no files in `/tmp` and none outside your lane directory.

Object of review, written by a different model family (codex `gpt-6-astra`, lane d-quotient): `docs/api-3.md`
(876 lines: the type `adf_qclass` and its contracts, 40 declarations for work package 3.1 (the quotient
`A/Q`: lift, reduction to the fundamental domain with splitting, a piece limit, set equality, containment and
overlap, translation, group arithmetic, text and dump) and 3.2 (the additive character: phases as `acb`
enclosures and as exact rational angles, local and class entry points), the new statements Q1 to Q5 with
proofs (section 4), acceptance tests and faults (5), three decisions D3-1 to D3-3 (6), thin slices (7), five
findings F1 to F5 against SPEC and conventions (8)), and its oracle `proto/quotient3_checks.py` (700 lines, 15
check groups). The author's report is `lanes/d-quotient/report.md`. The implementation lanes will implement
these declarations and statements literally in C (the first slice, lift only, is being written now). A result
must be what its statement says for every admitted input: a set called equal must be equal, a reduction must
represent exactly the same subset of `A/Q` before rounding and a superset after it, a phase enclosure must
contain every phase of the input set.

Read first: `CLAUDE.md` (rules 3 and 4), `lanes/COMMON.md`, `docs/SPEC.md` section 6 (line 386 on) and the
decisions of section 15 that name the quotient (M0-D4 among them), `docs/proofs/quotient.md` (all),
`docs/proofs/analysis.md` where the additive character is defined, `docs/conventions.md` 5.10, 3, 4, 6, 9.2,
9.4, 10, and the status table of 3.2; `docs/reviews/s-design/review.md` as the form of such a review (a table
of verdicts, then details, repairs, checks).

Your task is to REFUTE. In this order, and if the time is short the first three items matter most:
1. **Q1 to Q5, by your own independent computation** in `review_checks.py` (do not import the author's
   functions; write your own brute force in exact rationals with `fractions.Fraction`):
   - Reduction: a subset of `A/Q` is given by an adele ball `(real interval ; a + N Zhat)` with `N` a
     non-negative rational. Decide membership of a point of `A/Q` (a pair of a real number and a class of
     `Q/Z`-type finite data: work with rational points `(t ; q)` and the action of `Q` by simultaneous
     translation) in the original set and in the union of the reduced pieces, for every small case: intervals
     with rational end points crossing 0, 1, 2, 3 integers, of length 0, ending exactly at an integer;
     finite radii `N` in 0, 1, 2, 3, 1/2, 1/3, 2/3, 3/2; centres with denominators 1, 2, 3, 4, 6. The two
     sets must be EQUAL before rounding (Q1 is about the rounding: after it, a superset with the midpoint in
     `[0, 1]` and the stated bound on the growth). Count the pieces: `k` integers crossed give `k + 1`
     (M0-D4): for which inputs does the design's count differ from the count of your brute force?
   - Q2 (set equality, containment, overlap are decidable for stored balls, "including points and spill"):
     for pairs of small piece lists compare the design's algorithm (as section 2.3 and Q2 state it: implement
     it yourself from the text) with brute-force membership on a fine rational grid plus the end points. A
     pair on which they differ is a finding of the highest weight. Zero-radius fibres, pieces that exceed
     `[0, 1]` by rounding, a piece equal to another translated by 1.
   - Q3 (sum and negation of quotient sets), Q4 (phase extrema by four distances, additivity, width), Q5
     (full image with fractional or zero finite radius): the same method; for Q4 compute the true set of
     phases `exp(2 pi i theta)` over the input set by exact rational angles at the end points and the
     critical points, and compare the hull.
2. **The proofs** of Q1 to Q5, step by step: is each step justified, is a hypothesis used that is not stated,
   is every "exactly", "equal", "all" proved in both directions?
3. **The interface against the standing documents**: for each of the 40 declarations, does its status list
   agree with the status table of `conventions.md` 3.2 and with SPEC 6 and 3.1 (the design itself reports
   that D3-1 and D3-2 need exceptions: are there others it does not report)? Aliasing rules, outputs on
   failure, `where`. Two declarations whose contracts contradict each other. A function the PLAN rows 3.1,
   3.2 ask for that is missing. The sign of the additive character against `conventions.md` section 6 and
   against `refs/src/tate-poonen/notes.txt:693-700, 733-740` (open the source: does it say what the design
   says it says?). The test of PLAN 3.2 "a non-trivial phase at `(0 ; 1/3)`": what phase does the design
   give, and is it right under the convention?
4. **The author's findings F1 to F5**: is each real? Reproduce each witness yourself. A finding that is wrong
   is a finding against the design.
5. **The author's checks**: would a wrong formula pass them? Mutate the oracle in a scratch copy in your lane
   directory (six mutants: a sign of the character, an off-by-one in the piece count, a closed end point made
   open, the rounding inward, the midpoint invariant dropped, the finite radius ignored in the phase) and say
   which check group notices each. Name the check group that proves least.
6. **The decisions D3-1, D3-2, D3-3**: is the recommendation sound, is the alternative stated fairly, is a
   decision missing? Give your own recommendation for each in two lines.
7. Verdict for each of Q1 to Q5 and for each subsection of sections 1 to 3: VALID; MINOR (true, but needs a
   stated repair: give the replacement text); INVALID (false, or a gap you cannot close: give the
   counterexample or the exact gap).

Write `lanes/q-review1/review.md`: first line after the title the counts VALID / MINOR / INVALID and one of
`MAY BE IMPLEMENTED AFTER THE REPAIRS` or `NOT READY`; a summary table; details for everything that is not
plainly VALID, each with the input, what the design says, what is true and why, and the command that
reproduces it; the verdict on F1 to F5 and on D3-1 to D3-3; a section "Repairs" with replacement texts ready
to paste; a section "Checks" with the commands and outputs of your own checks (counts, and what would have
made a case fail); what you did not examine. Where you find nothing, say what was tried, with counts. No
praise. Keep notes in `lanes/q-review1/progress.md` as you go. Finish with `lanes/q-review1/report.md`: the
counts and each finding in one line.
