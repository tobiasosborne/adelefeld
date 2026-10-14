# Lane o-decisions1: the open decisions of the milestone 3, 4 and 5 reviews, taken and recorded

TJO (2026-10-15, 00:00): "For any open question for me: just decide it and proceed and record the decision."
The orchestrator has taken the decisions below. You record them: one new row `N-D25` in `docs/SPEC.md`
section 15.4 (the form of the rows N-D21 to N-D24 at lines 944 to 947: one table row, the id, a title cell,
a long cell with the decision, who took it and when, and "open to reversal by TJO"), and the amendments of
the design texts that each decision names. No code, no build, no test. You change the TEXT of statements
only where a decision says so; you do not touch any other statement, proof or convention.

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `docs/SPEC.md` 15.4 (lines 80 to 90 and 940 to 947),
`docs/workflow.md` rule 7, and for each item the passage it names, with twenty lines of context.

**You own:** `docs/SPEC.md` (ONLY: the new row after line 947; the one sentence of N-D21 named in item 7; the
one addition to N-D23 named in item 3), `docs/api-4.md`, `docs/api-5.md`, `docs/api-3.md`,
`lanes/o-decisions1/`. Everything else is read-only. No git command that changes state, no `bd`.

The decisions (each with its record in the review or lane report; quote the source in the row):

1. `docs/api-4.md` statement F4 (lines about 176 to 181) and review m4-review1 finding 1 (MAJOR against the
   statement; `docs/reviews/m4/review-m4.md`; bead adf-cx8): the transform's error statement gets a term for
   the 30-bit radius rounding of `acb_mul`, `acb_add`, `acb_div_ui` (the code's enclosure is sound; the tests
   of `test_ffun` state the allowance: read them for the term they use and state the same). Amend the
   statement; keep its structure.
2. `docs/api-4.md` section 2, the comment at lines about 105 to 106 ("Dumping then loading is identical")
   and the load comments near 109, and lane f4-slice7's two HEADER-FINDINGs (`lanes/f4-slice7/report.md`,
   `docs/api-4a.md` or `4b.md` or `4c.md`, find which records them): the identity holds when the D1 caps
   admit the dumped representation; load's cost includes clearing the old storage. Amend both comments in
   the design so that they state what the headers `include/adelefeld/ffun.h` and `rfun.h` state (read the
   headers; the header is the contract that was implemented).
3. The stall rule of `adf_tensor_poisson` (lane f4-slice6, `lanes/f4-slice6/result.md` line about 137, G4;
   `docs/api-4b.md` or wherever the lane wrote it): a precision doubling from 64 bits that does not halve the
   largest radius stops with `NOT_DETERMINED`. Taken. Add one sentence to the N-D23 row of `docs/SPEC.md`
   (line 946) recording it, after "the theta witness)".
4. Review m4-review1 finding 3 (MINOR; bead adf-l4v): the cost statement of `adf_tensor_poisson`
   (`docs/api-4.md` lines about 324 to 329) omits precision. Decision: amend the cost statement so that the
   work units are evaluations at the working precision, and the running time is bounded by the work cap
   times the cost of one evaluation at that precision (which grows with the bits), not by a clock; no code
   change, no time bound in the code. Amend the comment.
5. Lane f4-slice3's finding (`lanes/f4-slice3/result.md`): `acb_inv` loses the dependency for a wide
   canonical `A` in `adf_rfun_fourier`, so such a transform is `NOT_DETERMINED` at every precision.
   Decision: this stands as a cost item (an exact interval inverse would certify it); record it in the row
   only; no text amendment.
6. The HEADER-FINDING of lanes t5-slice1 and t5-slice3 (`docs/api-5a.md`, search HEADER-FINDING;
   `lanes/t5-slice1/report.md`, `lanes/t5-slice3/result.md`): `docs/api-5.md` line 33 ("guards count against
   the precision cap") against line 80 (the trivial calls delegate to `adf_local_zeta_factor_at`, which works
   at `prec + 32`): both cannot hold at the cap. Decision: the headers' reading stands: `LIMIT` is decided
   first for `prec` above the cap; below it, the delegated factor's internal working precision `prec + 32`
   is not itself subject to the cap. Amend line 33 of `docs/api-5.md` with this exception, one clause.
7. Bead adf-s1v and the "Waits for TJO" sentence at the end of N-D21 (`docs/SPEC.md` line 944: "a finite
   ball with a fractional radius (`0 mod 1/2`) is a coset in the product of the `Q_p` that is not inside
   `A_f`"): lane q-slice3 showed that `(1/2) Zhat` IS in `A_f` by the restricted product (every component is
   in `Z_p` except at 2). Decision: the library implements the restricted-product reading; such a ball is a
   subset of `A_f` and its class in the quotient is the one SPEC 6 gives. Replace that sentence of N-D21 by
   the decision (one or two sentences; keep the rest of the row).
8. Bead adf-nyk: `docs/api-3.md` lines about 521 to 525: the rounding argument assumes `l <= m <= h`, which a
   rounded midpoint need not satisfy (`l = h = 1/3`, precision 2 gives `m = 3/8`); `docs/api-3b.md` has the
   argument without that hypothesis (find it). Decision: `docs/api-3.md` points to the argument of
   `docs/api-3b.md` and states the counterexample in one sentence; the statement itself is unchanged.
9. `docs/api-5a.md` and `docs/api-5b.md` are merged into one statements file only after the milestone 5
   review (lane m5-review1, running) lands, because the review cites them by line. Record in the row; do
   nothing else.
10. The predicate `adf_char_is_canonical` fail-stops when FLINT's group setup returns 0 (`docs/api-3d.md`
    lines 24 to 25; lanes c-slice1, c-slice2). Decision: the fail-stop stands. Whether such a failure is
    reachable for a word-sized modulus is answered by lane c-sources1 (running now): if
    `lanes/c-sources1/report.md` exists in your tree, cite what it says; otherwise write "source pending:
    `refs/src/flint-3.0.1/dirichlet.rst`, being fetched by lane c-sources1".

Rules of the text: plain sober English, lines at most 116 characters (the SPEC rows are one long line each,
as the existing rows are: that is the one exception), files end with a newline, mathematics in plain text, no
LaTeX. Every amended passage keeps its line count within two lines of the original where other documents
cite it by line (`docs/api-4.md` 105-106, 176-181, 324-329; `docs/api-5.md` 33; `docs/api-3.md` 521-525):
prefer rewording a line to inserting lines; if you must insert, say so in the report with the old and new
line numbers of what moved.

Report: `lanes/o-decisions1/report.md`, written at the end: for each item, the passage before and after
(quoted), the file and lines; the N-D25 row; anything you found that contradicts a decision (quote it; do not
resolve it). Aim to finish within about forty minutes.
