# Lane f-repair3: the two minors of review f-review5 (sin, cos, sinh, cosh at a prime)

Review f-review5 (`docs/reviews/f1/review-lfunc-trig.md`, of lane f-slice7, merged `c09663a`) found no
blocker and no major, and two minors. You repair both. The reviewed functions are `adf_lball_sin`, `cos`,
`sinh`, `cosh` in `src/lfunc.c` (`parity_coefficient`, `parity_centre`, `parity_apply`, lines 690 to 813),
declared in `include/adelefeld/lfunc.h`; the statements F10 to F14 are at the end of `docs/api-1f4.md`; the
oracle is `proto/lfunc_trig_checks.py` with fixtures under `tests/ref/vectors/f-slice7/`; the tests are
`tests/test_lfunc_trig.c` and parts of `tests/test_rfunc_prime.c`. Decision N-D12 (`docs/SPEC.md` 15.4) and
the contract (`docs/SPEC.md` 9.3.2; `docs/proofs/functions.md` Definition 1, Lemma 5, Propositions 6, 7, 8,
Lemma 9, Proposition 10) do not change.

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md`, the review, the F10 to
F14 statements, `src/lfunc.c` lines 690 to 813 and the `exp` evaluator above them (the pattern of the family),
`tests/test_lfunc_trig.c`. `refs/src/` is NOT on disk in this container; the proofs under `docs/` are the
ground truth you cite, by file and line.

**You own:** `src/lfunc.c` (the parity evaluator only; `exp`, `log`, `Log` are reviewed code and do not
change), `docs/api-1f4.md` (F13 sentence; the F10/F11 text where it describes the loop, if it does),
`tests/test_lfunc_trig.c` (additions), `lanes/f-repair3/`. Everything else is read-only. No git command that
changes state, no `bd`. At most 2 cores; every program under `timeout`; build into
`BUILD=lanes/f-repair3/build` while you work.

## R1 (text). `docs/api-1f4.md` lines 422-423

The F13 sentence says `N = LONG_MIN` "returns LIMIT unless an exact-zero result ignores it". The code's order
is: input limits, then the domain check (`src/lfunc.c:759`), then the exact zero, then `|K|`. So exact 2 at
`p = 2` with `N = LONG_MIN` is `DOMAIN` and `2 Z_2` is `NOT_DETERMINED`, not `LIMIT`. Qualify the sentence so
that it states the code's order (the code is right; the text is wrong). One or two sentences; nothing else in
F13 changes.

## R2 (code). Pair the Horner steps

`parity_centre` runs Horner over every degree `k` from `L` down, including the degrees of the absent parity
whose coefficient is 0 (lines 727-735), so about half the modular multiplications are wasted (the review
measured 76 ms against 36 ms for the paired version at `p = 3`, `N = 2000`, `x = 3/2`). Rewrite the loop so
that it steps by two over the retained parity. The review's own proof that the pairing is exact (F' = kF,
A' = xA; F'' = (k-1)F', A'' = xA' + eps F''; so F'' = k(k-1)F, A'' = x^2 A + eps F''; reduction modulo `p^W`
commutes; for an odd top degree keep the final multiplication by `x`; the final division is unchanged) is
in the review; write it out in your own words as a comment at the loop, or as a new short statement F15 in
`docs/api-1f4.md` if the comment would exceed ten lines. The results must be IDENTICAL, residue for residue,
to the current code: same modulus `p^W`, same finite sum, same denominator division. No change to the term
count, the working precision, the domain checks, the exponent rule or the statuses.

Order of work (red-green, `lanes/f-repair3/redgreen.log`):

1. Before touching `src/lfunc.c`, build the CURRENT evaluator as a reference: copy the current `src/lfunc.c`
   into your lane directory, rename its external symbols (`adf_lball_sin` -> `ref_lball_sin`, etc.; `sed`
   over the four names suffices, keep the statics) and compile it into a small program that compares, for
   each of the four functions, the reference against the library on a few thousand inputs: `p` in 2, 3, 5,
   7, 13, 65537, `2^64 - 59`; exact inputs and balls; `N` from the domain edge up to 2000 at `p = 3` and up
   to 300 elsewhere; odd and even top degree `L` (both parities of the term count must occur: check that
   they do and say how you know). The comparison is `adf_lball_identical` on the result and equality of the
   status. This program is the test that decides the slice; run it before the change (it passes trivially)
   and after it.
2. Add a test to `tests/test_lfunc_trig.c` that pins a handful of exact residues at an odd AND an even top
   degree (the values from the current code, stated with their `p`, `N`, `L` in the test), so that a later
   change of the summation is caught by the suite.
3. The change. Keep `parity_coefficient` if it still serves; remove it if the paired loop makes it dead
   code (no dead code left behind).
4. Run: the comparison program (0 differences required; give the count of inputs); `timeout 300 make -j2
   BUILD=lanes/f-repair3/build lanes/f-repair3/build/test_lfunc_trig lanes/f-repair3/build/test_lfunc
   lanes/f-repair3/build/test_rfunc_prime` and run the three binaries (the stored fixtures of f-slice5 and
   f-slice7 pass through them); `python3 proto/lfunc_trig_checks.py` if it has a check mode (read its
   header; do not regenerate fixtures); one sanitizer build of `test_lfunc_trig` with `SAN=1` in
   `BUILD=lanes/f-repair3/build-san` and run it; the timing of the review's case once (`p = 3`, `N = 2000`,
   `x = 3/2`, the four functions, before and after; one run, a measurement and not a bound).
5. Mutation testing of `src/lfunc.c`, `lanes/COMMON-C.md` rule 5: `python3 tools/mutate/mutate.py --help`
   first; at most 60 mutants, `--limit`, `--seed 1`, 2 jobs, `--san`, bounded by 20 minutes (`timeout 1300`).
   Report the survivors in one line each; a survivor that is a gap of the tests gets a test.

Report: `lanes/f-repair3/result.md`, written once, at the end (the harness refuses the name `report.md` for
a Claude subagent); running notes in `lanes/f-repair3/progress.md` as you go. In it: the two repairs; every
check run with its command and its numbers; the mutation survivors; what is not done; findings against the
specification or the review. Give the same text as your final message.
