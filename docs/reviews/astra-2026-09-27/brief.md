# Brief: critical and constructive review of the adelfeld design documents

You are the design reviewer for a new project, `adelfeld`: a C library on FLINT in which the adeles of Q are a
first-class number type with ball arithmetic. Nothing is implemented. Your review decides what is changed before any
C is written. Work in the repository root (the current directory).

## Read, in this order
1. `docs/SPEC.md` (what is to be built)
2. `docs/PLAN.md` (order, files, acceptance tests)
3. `docs/PERF.md` (lower bounds and reference measurements), with `bench/baseline.c` and `bench/baseline_2026-09-27.txt`
4. `proto/precision_rules.py` (brute-force check of the precision rules)

The project's performance metric: a measured number counts only as a ratio to a derived lower bound; proved bounds,
hardware-model bounds and best-known algorithm costs are three different kinds and must never be confused.

## What to do
Be critical and constructive: for every defect, say what is wrong, why, and what to write instead.

A. **Mathematics.** Check every mathematical statement in SPEC.md, including those labelled [standard] and
   [checked]. In particular: the two precision rules of section 4.2 and their tightness (prove them or give a
   counterexample; treat rational centres and rational radii, radius 0, negative centres); the claim that at finite
   precision the finite part of an adele of Q is one ball `a + N Zhat` with rational `a`, `N`; the idele description
   `A_f^x = Q_{>0} x Zhat^x` and the precision claim of section 5; the claim of section 4.3 about non-units; the
   reduction modulo Q of section 6; the description of test functions and the finite Fourier transform in section 7
   (support and periodicity after transform, normalisation of Haar measure); the acceptance test of section 7; the
   count of idempotents; section 8. State the correct sign and normalisation conventions where the documents leave
   them open, and say which source fixes them.
B. **Design.** Is `(real ball ; centre mod radius)` the right representation? Are the tight and capped policies
   (SPEC section 15, PLAN section 4) sound, and is "scale times residue" the right capped form? Is the split into
   global and local storage right? What is missing from the type list? Are the structs of PLAN section 4 right for
   FLINT 3 (use of fmpq, fmpz, arb, nmod, fmpz_mod, padic conventions)? Is the abstraction "global field / restricted
   product" of SPEC section 3 placed at the right moment (PLAN milestone 7)? What would make the types awkward for
   number fields or for F_q(T) later?
C. **Plan.** Order of work packages, missing work packages, acceptance tests that do not test what they claim,
   tests that are missing (enclosure, tightness, policies against each other), unrealistic sizes.
D. **Performance.** Check every floor in PERF.md: is it a valid lower bound under its stated model, is its kind
   (PROVED / MODEL / BEST-KNOWN) right, is a tighter valid floor available, is the measurement the matching kind
   (latency against latency, throughput against throughput)? Check the hardware figures you can (Zen 2: latency and
   throughput of mul, adc; AVX2 facts). Check the design conclusions of PERF.md section 4. Derive the missing rows of
   section 5 where you can (CRT conversion, finite Fourier transform of length D*M, rational reconstruction, vector
   kernels for moduli below 2^32 on AVX2), each with kind and assumptions. You may compile and run small C or Python
   programs to check a claim (FLINT 3.0.1, gmp, gcc, python3 with stdlib are installed); put them under
   `docs/reviews/astra-2026-09-27/checks/`.
E. **Prior art.** Statements labelled [unverified] are from memory. Say which you believe are wrong or imprecise.
   Do not invent citations: when you are not sure a paper or a result exists, say so.

## Output
Write your review to `docs/reviews/astra-2026-09-27/review.md`, incrementally (write each section to the file as you
finish it, so that nothing is lost if you are interrupted). Do NOT edit SPEC.md, PLAN.md, PERF.md or any file outside
`docs/reviews/astra-2026-09-27/`. Structure:

1. **Verdict table**: one row per finding, with an id (M1.., D1.., P1.., F1.., A1..), the location (file, section),
   severity (BLOCKER: wrong and would be built upon; MAJOR; MINOR), and a one-line statement.
2. **Findings in full**, each with: what the document says (quoted), what is wrong or weak, the corrected text or
   design to use instead, and the evidence (proof, counterexample, or the check you ran with its output).
3. **What is right and should be kept** (short).
4. **Proposed replacement texts**: for each BLOCKER and MAJOR, the exact paragraph or table row to substitute.
5. **Closing section, titled exactly "What this changes in the plan"**: the ordered list of changes you recommend
   before milestone 1 starts.

Write plainly, stepwise, without jargon where an elementary statement will do. Label every claim of yours as proved
here, checked by a run, or from memory.
