# Lane s-decisions: write the decisions of TJO of 2026-09-29 into the documents

Documents only. No code, no build, no test run. Read `docs/SPEC.md` section 15 (the form of a row),
`docs/api-s.md` sections 5 and 6, `HANDOFF.md` lines 40 to 46.

**You own:** `docs/SPEC.md`, `docs/PLAN.md`, `docs/conventions.md`, `docs/api-s.md`,
`docs/proofs/solvers.md` (ONLY the two sentences named in item 3), comments in
`include/adelefeld/fball.h`, `scaled.h`, `recon.h` (item 4; comments only, no declaration changes),
`lanes/s-decisions/`.

## What TJO decided on 2026-09-29

- S-D1 to S-D9 and S-D11 to S-D19: accepted as recommended in `docs/api-s.md` section 5.
- S-D10: NOT as recommended. Decided: the root finder accepts every prime. A bound on `p` is a property of
  a slice of the implementation, not of the library: the first slice finds the roots modulo `p` by
  evaluation at every residue (`solvers` P3.7(1)) and returns `UNSUPPORTED` above a bound that its header
  calls temporary; a later slice adds the route of `solvers` P3.7(2) (roots by a routine of FLINT, each
  tested by evaluation, completeness by `deg gcd(g, X^p - X)`) for larger primes, with no bound of one
  word (FLINT's `fmpz_mod_poly`, `[source pending: flint-3.0.1 fmpz_mod_poly.rst, fmpz_mod_poly_factor.rst,
  nmod_poly.rst]`). The bound then is the point where the method changes, set by a benchmark.
- M1-D10 and M1-D11: accepted as proposed.

## What to do

1. `docs/SPEC.md` section 15: M1-D10 and M1-D11: replace "PROPOSED by the orchestrator, 2026-09-29; not yet
   accepted by TJO" by "proposed by the orchestrator and accepted by TJO, 2026-09-29". Add rows S-D1 to
   S-D19 in the form of the rows that are there: the question in a few words, the decision in one or two
   sentences (from the column "Recommended" of `docs/api-s.md` section 5, shortened, nothing added), the
   sections concerned. S-D10 with the text decided above.
2. `docs/api-s.md`: in the head of the file and in section 5, say that the decisions were taken on
   2026-09-29 and how (one sentence; the table stays as the record of the alternatives). The row S-D10:
   add the decision. Wherever the file names `ADF_ROOTS_P_MAX` as a property of the library (section 4,
   section 6 edit E-C1, section 7), change the sentence so that it agrees with the decision; list each
   place in your report.
3. Apply the exact edits of `docs/api-s.md` section 6 to `docs/SPEC.md`, `docs/PLAN.md`,
   `docs/conventions.md`: E-S1 to E-S6, E-P1, E-P2, E-C1 to E-C4. E-C5 stays deferred. For each: find the
   present text (the line numbers may have moved), check that it is the text the edit quotes, replace. If
   the present text differs from the quoted one, do not apply the edit; report both texts. In E-C1 and
   wherever else an edit names a prime above `ADF_ROOTS_P_MAX`, write it as the decision says (a
   temporary `UNSUPPORTED` of a slice). `docs/proofs/solvers.md` line 1304 to 1307 (after Proposition 3.7)
   and its line near 1615: one sentence each so that they agree with S-D10 as decided; no other change of
   that file.
4. M1-D10 and M1-D11 in the headers: one sentence each, in the comment at the head of
   `include/adelefeld/fball.h` and `scaled.h` (M1-D10: values that refer to a context are made and changed
   by functions of the library only), of `recon.h` (M1-D11: a status for an input outside the contract is a
   courtesy of the release build), and in `docs/conventions.md` 4.6 (M1-D10) and 4.4 (M1-D11). Name the
   decision in each.
5. Lines at most 116 characters. No other rewording. The report lists every edit with file and line, and
   every edit that was not applied with the reason. Do NOT create `report.md` before the work is done.
