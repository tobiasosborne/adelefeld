# Lane s-design-repair: the repairs of the design of milestone S after its review

Read `CLAUDE.md`, `lanes/COMMON.md`, `lanes/PROOFSTYLE.md`. Then, completely:
`docs/reviews/s-design/review.md` (the review: 28 VALID, 3 MINOR, 2 INVALID, NOT READY; its sections "Proof
audit and counterexamples", "Reference algorithm failure and interface contradictions", "Sources and the
eight corrections of table 3", "Decisions", "Proposed edits", "Repairs" R1 to R10, "Judgment of the author's
checks"), `lanes/s-design/report.md`, and the three files of the design: `docs/proofs/solvers.md`,
`proto/solvers_checks.py`, `docs/api-s.md`. The reviewer's programs are under `docs/reviews/s-design/`.
The sources are under `/home/tobias/Projects/adelefeld/refs/src/` (read only; CLAUDE.md rules 3 and 4).

**You own:** `docs/proofs/solvers.md`, `proto/solvers_checks.py`, `docs/api-s.md`, `docs/sources.md` (table 3
and its notes only), `lanes/s-design-repair/`. You change nothing under `docs/reviews/`, and not
`docs/SPEC.md`, `docs/PLAN.md`, `docs/conventions.md`, no header, no code.

## Rules of a repair

- Every finding of the review is answered: repaired as the review proposes; repaired otherwise, with the
  reason; or contested, with the proof or the counterexample that shows the review wrong. No finding is
  left without an answer. The table of answers is the first section of your report.
- A replacement text of the review is not pasted blindly: read it against the source it cites and against
  the statements around it. The reviewer can be wrong.
- Red-green for the checks: for every finding that the review shows by an input (the overflow of
  `real_roots_ref` at `X - 10^400`; the two surviving mutants of `check_s3_limit` and
  `check_s2_real_completeness`; the accepted interval `[0, 4]` for `(X-1)(X-2)(X-3)`; `f = 27 X` at 3;
  `m = 2, c = 1, A = B = 1`; `m = 2, c = 1, A = 1, B = 3, limit = 1`), first a check that fails on the
  present file, seen failing and logged, then the repair. The two mutants of the reviewer
  (`docs/reviews/s-design/checks/mutation_checks.py`) must be killed by your checks afterwards: run it.
- The other weaknesses of the checks that the review lists ("Other limitations") are repaired too: a count
  that is printed is the count of what was verified; nothing is skipped silently.
- Never weaken a statement to make a check pass. A statement that was false (P3.9(1), 3.11(5)) is
  replaced by a true one, and the places that used the false one are found and read again.

## The work

1. The statements: R1 (D2.1), R2 (P2.11), R3 (Remark 3.6 becomes Proposition 3.6, with its proof), R4
   (P3.9), R5 (3.11); the table of statements at the end of `solvers.md` gains the three definitions and
   shows the new status of each statement; a section "Review record" at the end, in the form of the other
   proof files (`docs/proofs/quotient.md` has one), names the review and lists the repairs.
2. The reference algorithms and checks: R6, R8 (the accuracy after widening), R9 (the statuses of the
   limited search, exactly the four conditions of P1.7(3)), and the limitations of the checks.
3. The interface `docs/api-s.md`: R7 (normalisation to `g`, scope PARTITION or SEED, the two verifiers),
   R8, R9 (`count` on every status), the prototypes that must return `int`, the cost of `_get_poly`; the
   decisions S-D3, S-D4, S-D6, S-D14 restated as the review asks (S-D6: both alternatives stated
   neutrally); the missing decisions added as S-D16 and following (seed scope; meaning of verification;
   resource limits and checked `slong` arithmetic for `e + j`, `k + s`, `2k - s`; final real accuracy and
   its status); the edits E-S1, E-S2, E-S3, E-S5, E-P1, E-P2, E-C4 replaced by R10 and R9; E-C5 deferred.
4. `docs/sources.md` table 3: the seven corrections of the author's report 5.1 that the review judged
   RIGHT are applied to the rows and notes (the first, Thue's lemma, is applied already: read the row and
   leave it); also the two the review adds (note 2: `1/1` and `-1/1` are not a common multiple of a pair;
   the row on Shoup's bound at line 4202 keeps its hypothesis that a bounded lattice point exists). The
   quotes themselves do not change. `python3 lanes/s-sources/check_quotes.py` must give 0 failures.
5. At the end: `python3 proto/solvers_checks.py` (exit 0, under 3 minutes), the reviewer's
   `python3 docs/reviews/s-design/review_checks.py <suite>` for each of its suites (they must still run;
   a FINDING line that your repair removes is named in the report), `python3 -m pytest -q proto`.

## Report

`lanes/s-design-repair/report.md` if you can write it, and in every case your final message: the table of
answers (finding, answer, where); the red and green logs by name; the output of the final runs; what is
not done; every place where you hold the review to be wrong; the absolute path of your worktree.
