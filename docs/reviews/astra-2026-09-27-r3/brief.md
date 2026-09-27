# Brief: third review of the adelefeld design documents (round 3, ratification check)

You reviewed these documents twice today (`docs/reviews/astra-2026-09-27/review.md`,
`docs/reviews/astra-2026-09-27-r2/review.md`). The authors applied round 2 and added a catalogue of functions. The
owner wants to ratify the documents as the basis of milestone 0 if no major defect remains. Work in the repository
root. This round is short and focused.

## Read
1. `docs/reviews/astra-2026-09-27-r2/review.md` (your round-2 findings and their replacement texts)
2. `docs/SPEC.md` (draft 3): all of it, with most attention to sections 4.1, 5, 6, 9.2 and the rewritten 9.3
3. `docs/PLAN.md` (draft 2): sections 1, 4, 5, milestone 0, milestone 1F, section 7
4. `docs/PERF.md` (draft 2)
5. `docs/proofs/precision.md` (extended: Proposition 3 zero cases, Proposition 4 rewritten, new Proposition 6)
6. `proto/precision_rules.py` (run it; new checks at the end)

## What to do
A. **Closure of round 2.** For each of the 11 round-1 remainders (M6, M9, M10, M12, D2, P1, P2, F1, F2, F3, F4, F6)
   and each new finding N1 to N12: RESOLVED, PARTLY, NOT RESOLVED, or RESOLVED WRONGLY. Judge the text as written.
   Check in particular that your formulas were transcribed correctly into SPEC 9.3.2 to 9.3.4 (domains, radius
   rules, the Log image formula and its 2-adic exception, root existence conditions, the precision formula and its
   guard, root counts).
B. **The new catalogue, SPEC 9.3.7, and PLAN 1F.9.** Check every row of Tier A with full rigour:
   - Hilbert symbol: the definition, the precision needed at odd p, at 2 and at the real place, the claim that at an
     odd prime where both entries are units the symbol is 1, the product formula, and what is determined for two
     ideles of finite precision (which places need data; is the all-places family determined?). The formulas used
     are in `proto/precision_rules.py` (function `hilbert`); check them, and check whether the prototype's test by
     solvability modulo p^k is a valid test of the definition.
   - profinite power `a^x`: is it well defined for `a` a unit of Zhat and `x` in Zhat; is the criterion
     "`c^M = 1 mod N`" correct and complete for the value to be determined modulo N; what happens for negative
     exponents, for the canonical form of unit cosets (U(2N) = U(N)), and what should be returned when the criterion
     fails (is there a largest modulus at which the value is determined, and can it be found without factorising?).
   - binomial coefficient: does `binom(x, k)` map Zhat to Zhat; is "known modulo N / gcd(N, k!)" a valid enclosure;
     is it tight; give the tight rule if it is simple.
   - local zeta factors, Gauss sums and local constants, content, theta series with its Poisson identity (check the
     identity and its hypotheses), the cyclotomic action (state the two conventions and their sources if you know
     them; do not invent citations).
   - Is any function in Tier A misplaced (should be Tier B), and is any natural, cheap function missing from Tier A
     that a user of adeles over Q would expect in a basic package? Keep this list short and justified.
C. **Proofs.** Check Proposition 3's zero cases, the rewritten Proposition 4 and the new Proposition 6, step by step.
D. **Anything that would make ratification wrong.** Internal contradictions between SPEC, PLAN and PERF; promises
   that cannot be kept; a milestone-0 exit that cannot be evaluated.

You may compile and run small C or Python programs; put them under `docs/reviews/astra-2026-09-27-r3/checks/`; leave
no compiled binaries. Do not invent citations.

## Output
Write to `docs/reviews/astra-2026-09-27-r3/review.md`, incrementally. Do NOT edit any file outside
`docs/reviews/astra-2026-09-27-r3/`. Be brief: one line per resolved item; full text only for what is not resolved
and for new findings. Structure:

1. **Closure table** (round-2 items).
2. **New findings table**: id (R1, R2, ...), location, severity (BLOCKER / MAJOR / MINOR), one line.
3. **Findings in full**, each with quoted text, what is wrong, replacement text, evidence.
4. **Closing section, titled exactly "Ratification"**: a one-line verdict, one of
   - RATIFY (no change needed),
   - RATIFY AFTER MINOR EDITS (list them; no further review needed),
   - DO NOT RATIFY (list the blockers);
   followed by the ordered list of edits.

Label every claim of yours as proved here, checked by a run, or from memory.
