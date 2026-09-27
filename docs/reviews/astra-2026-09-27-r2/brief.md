# Brief: second review of the adelefeld design documents (round 2)

You reviewed draft 1 of these documents earlier today; your review is `docs/reviews/astra-2026-09-27/review.md`
(28 findings). The authors have revised everything. Your task now is a second, critical and constructive review of
the revised documents. Work in the repository root (the current directory). The project was renamed from `adelfeld`
to `adelefeld`.

## Read, in this order
1. `docs/reviews/astra-2026-09-27/review.md` (your round-1 findings)
2. `docs/SPEC.md` (draft 2)
3. `docs/proofs/precision.md` (new)
4. `docs/PLAN.md` (draft 1)
5. `docs/PERF.md` (draft 1), with `bench/baseline.c`, `bench/baseline_2026-09-27.txt`
6. `proto/precision_rules.py` (extended; run it)

## What to do
A. **Closure of round 1.** For each of the 28 findings, decide: RESOLVED, PARTLY (say what remains), NOT RESOLVED, or
   RESOLVED WRONGLY (the revision introduced an error). Check the revised text itself, not the authors' intent.
B. **New material.** The owner decided that elementary functions (`exp`, `sin`, `cos`, `log`, roots, powers, ...) are
   part of the basic package. Review `SPEC.md` section 9.3 and `PLAN.md` milestone 1F with full rigour:
   - the domains of convergence of the p-adic `exp`, `sin`, `cos`, `sinh`, `cosh`, `log` (p odd and p = 2), and of
     Iwasawa's extension of `log`;
   - the claim that `exp`, `sin`, `cos` are not functions on full adeles, and the claim about which points of our
     types lie in the domain;
   - the enclosure claim "output radius equals input radius" for these power series on their discs (prove or give a
     counterexample, for p odd and p = 2, and for `log` on `1 + p Z_p` and on the larger domain);
   - p-adic square roots and n-th roots: domain, choice of root, precision loss (in particular p = 2 and p dividing n);
   - `x^s` p-adically; what is sensible to offer;
   - the proposed test of p-adic `sin`, `cos` against `exp` "where sqrt(-1) exists in Q_p";
   - whether the design (functions act on named places and return a partial ball) is sound and natural; what a user
     would expect of `sin` applied to an adele; better alternatives if any;
   - what FLINT 3.0.1's `padic` module really provides (check the installed headers) and its domain conventions.
C. **The proofs** in `docs/proofs/precision.md`: check every step; Proposition 4 and 5 in particular; say what is
   missing.
D. **The complex type** `adf_cadele` = `C x A_f` (SPEC 4.1): is this the most useful meaning? Alternatives?
E. **PERF draft 1**: are the remaining floors and ratios now valid and correctly labelled? Are the new rows for
   elementary functions sound? Anything still wrong in section 1's hardware table?
F. **Anything else** you now see that round 1 missed.

You may compile and run small C or Python programs (FLINT 3.0.1, gmp, gcc, python3 stdlib); put them under
`docs/reviews/astra-2026-09-27-r2/checks/`. Do not leave compiled binaries there. Do not invent citations: when you
are not sure a paper or a result exists, say so.

## Output
Write your review to `docs/reviews/astra-2026-09-27-r2/review.md`, incrementally (write each section to the file as
you finish it). Do NOT edit any file outside `docs/reviews/astra-2026-09-27-r2/`. Keep it shorter than round 1: state
each point once. Structure:

1. **Closure table**: the 28 round-1 ids with their status and one line each.
2. **New findings table**: id (N1, N2, ...), location, severity (BLOCKER / MAJOR / MINOR), one-line statement.
3. **Findings in full**: only for items that are not RESOLVED, and for new findings; each with the quoted text, what
   is wrong, the replacement text, and the evidence (proof, counterexample, or a run with its output).
4. **What is right and should be kept** (short).
5. **Closing section, titled exactly "What this changes in the plan"**: the ordered list of changes you recommend,
   and a one-line overall verdict: may milestone 0 begin as planned, yes or no.

Write plainly and stepwise. Label every claim of yours as proved here, checked by a run, or from memory.
