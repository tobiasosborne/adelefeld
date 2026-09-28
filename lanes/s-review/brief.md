# Lane s-review: adversarial review of the design of milestone S (solvers)

**You own:** `docs/reviews/s-design/review.md`, `docs/reviews/s-design/review_checks.py`, other programs
under `docs/reviews/s-design/checks/`, and `lanes/s-review/`. Everything else is read only; do not edit the
design.

Object of review, written by a different model family (Claude Fable): `docs/proofs/solvers.md` (33
statements: partial rational reconstruction, linear systems modulo `N` with the Howell form and a
certificate, roots at a prime by Hensel lifting and real roots), its checks `proto/solvers_checks.py`, and
the interface with its 15 decisions and its proposed edits, `docs/api-s.md`. The author's report is
`lanes/s-design/report.md`; its section 7 lists what the author is least sure of, in order: START THERE.
Milestone S will implement these statements literally in C. A result must be what its statement says for
every admitted input: a list called complete must be complete, a certificate that the checker accepts must
imply the result, "none" must mean none.

Read first: `CLAUDE.md` (rules 3 and 4), `lanes/PROOFSTYLE.md`, `docs/SPEC.md` sections 9.1 and 9.2,
`docs/proofs/quotient.md` Propositions 11 to 13, `docs/conventions.md` sections 3 and 4.3,
`docs/sources.md` table 3. The sources are under `refs/src/` (`shoup-ntb`, `storjohann-thesis`,
`conrad-hensel`, `baker-padic`, `thorne-padic`, `flint-3.0.1`, `flint-src-3.0.1`).

Your task is to REFUTE. For each numbered statement:
1. Try to break it by your own independent computation in your checks file (do not import the author's
   functions; write your own brute force): every `(m, c, A, B)` with small `m`, with `A = 0`, `B = 1`,
   `m = 1, 2`, `c` negative or above `m`, `m` a prime power, `2 A B` equal to `m - 1`, `m`, `m + 1`; every
   matrix over `Z/N` of small size for `N` in 1, 4, 6, 8, 9, 12, 16, 36, with zero rows, zero columns,
   `r = 0`, `c = 0`, more rows than columns and fewer; polynomials with a multiple root, with a root modulo
   `p` that does not lift, with content divisible by `p`, with leading coefficient divisible by `p`, the
   zero polynomial, constants, `p = 2` and `p = 3` separately; real polynomials with a multiple root, with
   roots closer than the precision, with a root at an end point.
2. Read the proof step by step. Is each step justified? Is a hypothesis used that is not stated? Is every
   "exactly", "complete", "canonical", "unique" and "all" proved in both directions? For the certificates:
   construct a FALSE certificate that the stated checker accepts, or show why none exists.
3. Read every cited source at the cited place. Does it say what the design says it says? The author found
   table 3 of `docs/sources.md` wrong on Thue's lemma; judge the author's eight corrections of table 3
   (report, 5.1) and look for places where the author is wrong about a source.
4. Read the FLINT sources that the design relies on (P1.9, P2.11, P3.9, P3.10): is each stated promise of
   FLINT in the documentation or the code? Find an input for which FLINT does something the design does
   not expect. The installed library is FLINT 3.0.1; you may call it through `ctypes` or a C program.
5. Judge the author's checks: would a wrong formula or a wrong algorithm pass them? Name the check that
   proves least.
6. Judge the algorithms as specified in `proto/solvers_checks.py` against the statements: an input for
   which the reference algorithm and your brute force differ is a finding of the highest weight.
7. Judge each decision S-D1 to S-D15 of `docs/api-s.md`: is the recommendation sound, is the alternative
   stated fairly, is a decision missing? Judge each proposed edit of SPEC, PLAN and conventions.
8. Verdict for each statement: VALID; MINOR (true, but needs a stated repair: give the replacement text);
   INVALID (false, or a gap you cannot close: give the counterexample or the exact gap).

Rules: no git command that changes state, no `bd`; at most 2 cores; no computation above about 3 minutes;
a mutation sweep runs on this machine, so wait if `free -g` shows less than 6 GB available. Plain, sober
English; short sentences; numbers, not adjectives; no praise; lines at most 116 characters. Where you find
nothing, say what was tried, with counts.

Write `docs/reviews/s-design/review.md`: first line after the title the counts VALID / MINOR / INVALID and
one of the words `MAY BE IMPLEMENTED AFTER THE REPAIRS` or `NOT READY`; a summary table (statement, verdict,
one line); details for every statement that is not plainly VALID; the verdict on each correction of table 3
and on each decision and edit; a section "Repairs" with replacement texts ready to paste; a section
"Checks" with the command and the output of your own checks; what you did not examine. Then
`lanes/s-review/report.md` with the counts and the important items in one line each. Your final message
contains the complete text of `review.md`.
