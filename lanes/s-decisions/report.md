# Lane s-decisions: report

Work of 2026-09-29 (three attempts of the runner; the date 2026-09-30 that the model wrote here was false: orchestrator). Documents only: no code, no build, no test run,
no git command that changes state.

## What was done

The decisions of TJO of 2026-09-29 are written into the documents. S-D1 to S-D9 and S-D11 to S-D19 stand as
recommended in `docs/api-s.md` section 5. S-D10 stands otherwise than recommended: the root finder accepts every
prime, and a bound on `p` is a property of a slice of the implementation, not of the library. M1-D10 and M1-D11
are accepted as proposed. The exact edits of `docs/api-s.md` section 6 (E-S1 to E-S6, E-P1, E-P2, E-C1 to
E-C4) are applied; E-C5 stays deferred.

## Files written, edit by edit

Line numbers are those of the text as it is now.

### `docs/SPEC.md`

- Line 511: E-S1, the row "Linear systems modulo `N`" of 9.1, replaced by the quoted new row.
- Line 512: E-S2, the row "Polynomial roots at a given prime", replaced (S-D13 decided as recommended, so the
  condition of the edit holds).
- Line 513: E-S3, the row "Real roots", replaced.
- Line 514: E-S4, "Multiple roots, roots at all primes" replaced by "Multiplicities of roots, roots at all primes".
- Lines 526 to 535: E-S5, the second item of 9.2, replaced. The sentence "The following claims assume
  `A >= 0` and `B >= 1` ..." is inside the new text that the edit itself quotes.
- Lines 535 to 537: E-S6, the third item of 9.2, replaced by the two-line form of the edit.
- Line 875: M1-D10, question cell: "PROPOSED by the orchestrator, 2026-09-29; not yet accepted by TJO"
  replaced by "proposed by the orchestrator and accepted by TJO, 2026-09-29".
- Line 876: M1-D11, the same replacement in the question cell.
- Lines 878 to 905: new subsection 15.3 "Decisions of TJO, 2026-09-29 (milestone S)": three lines of text, the
  table header (line 884), and nineteen rows S-D1 to S-D19 (lines 886 to 904), one row per line, in the form of
  the rows of 15.2 (Id, Question, Decision, Where).

### `docs/PLAN.md`

- Lines 326 to 328: E-P1. The three rows S.1, S.2, S.3 of the milestone S table are replaced by the three rows
  of the edit, in the order the edit gives them (S.3, S.1, S.2). The edit does not quote the present text; the
  present three rows are those of the milestone S table of section 6.
- Line 354: E-P2, the second cell of the row "Solvers" of section 7. The cell of the edit is written over six
  lines in `api-s.md`; here it is one line, as a table cell must be. The words are those of the edit, unchanged.

### `docs/conventions.md`

- Lines 151 to 152: E-C3, the two sentences added after the table of 2.3 (S-D12 decided as recommended, so the
  condition of the edit holds).
- Line 203: E-C1, the row "Reconstruction and solvers" of 3.2. The `UNSUPPORTED` cell is written as the
  decision S-D10 says: "a prime above the temporary bound of a slice that finds the roots modulo `p` by
  evaluation at every residue, decision S-D10". The edit quoted "`ADF_ROOTS_P_MAX`".
- Lines 270 to 272: E-C2, the three sentences added after the table of CV-06 in 4.3 (S-D6 decided as recommended).
- Lines 992 to 995: E-C4, the last sentence of the first item of 6.8, replaced.
- Lines 301 to 303: M1-D11, one sentence added at the end of 4.4 (after its table), naming the decision.
- Lines 366 to 368: M1-D10, one sentence added in 4.6 after "Contexts stay immutable after construction.",
  naming the decision.

### `docs/api-s.md`

- Lines 10 to 11: one sentence in the head: TJO decided the nineteen questions on 2026-09-29, S-D10 otherwise
  than recommended, and the decisions are written in `docs/SPEC.md` 15.3.
- Line 214: section 4, the row of `adf_roots_padic`: "`UNSUPPORTED` (edit E-C1): `p > ADF_ROOTS_P_MAX` (S-D10)"
  replaced by the temporary-`UNSUPPORTED`-of-a-slice wording.
- Lines 263 to 265: one sentence in section 5: the date, how the decisions were taken, and the fact that the
  table stays as the record of the alternatives that were asked.
- Line 278: row S-D10. The decision stands at the front of the cell, with the source-pending list of the
  decision; the recommendation is kept after it, as it was asked.
- Line 383: E-C1, new row: "`a prime above `ADF_ROOTS_P_MAX`" replaced by the same temporary-bound wording as
  in `conventions.md` 3.2.

The brief names three places of `api-s.md` that hold `ADF_ROOTS_P_MAX` as a property of the library: section 4
(line 212), the edit E-C1 (line 379), and section 7. All three were changed except section 7, which does not
mention `ADF_ROOTS_P_MAX` or any bound on `p`: before the edits, `grep -n "ADF_ROOTS_P_MAX" docs/api-s.md`
returned lines 212, 274 and 379 only. Nothing was changed in section 7. The name now survives in the file only
in the recommendation of the row S-D10, which is kept as the record of the alternative that was asked.

### `docs/proofs/solvers.md` (only the two sentences named in item 3)

- Lines 1304 to 1307: the three sentences after Proposition 3.7 are replaced by four lines (one sentence, wrapped
  over them) that say what S-D10 as decided says. Line 1308, the source-pending line inside the range of the
  brief, now names `fmpz_mod_poly.rst`, `fmpz_mod_poly_factor.rst` and `nmod_poly.rst`; the function names that
  the old line listed are kept in brackets.
- Lines 1616 to 1617: in the list "Not proved and stated as such", "the larger-prime `nmod_poly` alternative of
  decision S-D10 (source pending)" is replaced by "the route 2 of decision S-D10, which the later slice takes
  for larger primes (source pending)".

No other line of that file was touched: `git diff docs/proofs/solvers.md` shows two hunks.

### Comments in the headers (item 4; comments only, no declaration changed)

- `include/adelefeld/fball.h`, lines 20 to 21: M1-D10, "A value that refers to a context gets and changes its
  context field through functions of the library only (decision M1-D10)."
- `include/adelefeld/scaled.h`, lines 24 to 25: the same sentence for M1-D10.
- `include/adelefeld/recon.h`, lines 26 to 28: M1-D11, "A status returned for an input outside the contract of
  the function (for example ADF_DOMAIN for an infinite real ball) is a courtesy of the release build and no
  promise (decision M1-D11)."

## Checks that were run, with their results

1. Present text of every edit against the quoted text of `docs/api-s.md` section 6, compared as bytes. Command:
   a Python script that collects every block of `api-s.md` indented by four spaces and tests
   `block in '\n'.join(open(f).read().split('\n'))` for `f` in SPEC, PLAN, conventions. Result before the
   edits: the present text of E-S1 (168 characters), E-S2 (173), E-S3 (59), E-S5 (123), E-S6 (80), E-P2 (the
   cell), E-C1 (137) and E-C4 (96) is in the named file, byte for byte. No edit was applied to a text that
   differs from the quoted one.
2. New text of the edits, same script, after the edits. Result: E-S1 (552 characters), E-S2 (515), E-S3 (498),
   E-S5 (109) and E-S6 (110) are in `docs/SPEC.md`; E-P1 (823) is in `docs/PLAN.md`; E-C2 (229) and E-C3 (186)
   are in `docs/conventions.md`; E-C1 (374) is not in `docs/conventions.md`, on purpose, because its
   `UNSUPPORTED` cell was reworded as the decision says; E-P2 (562) differs from the block of `api-s.md` only
   in the line wrapping (six lines there, one table cell here), the words are equal. Checked separately: the cell
   of `PLAN.md` line 354, without `| Solvers | ` and without the trailing ` |`, equals the block of the edit
   joined with single spaces.
3. Table of SPEC 15.3: 19 rows begin with `| S-D`, each has 5 vertical bars (four columns) and each ends with
   `|`.
4. Line lengths. Command: `git diff -U0`, added lines only, lengths measured with a Python script that skips
   table rows and source-pending citations. Result: no line above 116 characters. The one exception in the
   tree is the source-pending line `docs/proofs/solvers.md` 1308, 149 characters; lines of that kind already
   reach 1190 characters in this tree.
5. Trailing whitespace on added lines, same command: nothing found.
6. `git status --short` and `git diff --stat` at the end: eight files changed (`docs/PLAN.md` 8 lines,
   `docs/SPEC.md` 58, `docs/api-s.md` 12, `docs/conventions.md` 14, `docs/proofs/solvers.md` 7,
   `include/adelefeld/fball.h` 2, `include/adelefeld/scaled.h` 2, `include/adelefeld/recon.h` 3) and this
   report. No file outside the list of the lane was touched. No git command that changes state, no `bd`.

## What is not done

- E-C5 (subsections 5.15 to 5.17 of `conventions.md`) stays deferred, as the brief says.
- The head of `docs/api-s.md`, lines 20 to 24, still says of the row "Reconstruction and solvers" that the two
  additions are "proposed in section 6 ... until they are decided the entries below mark them '(edit E-C1)'".
  The additions are now decided and applied in `conventions.md` 3.2, so that sentence is stale. Item 5 of the
  brief forbids any other rewording, so it stands. The orchestrator may want one sentence there.
- Note 2 of section 4 of `docs/api-s.md` (lines 233 to 236) describes what happens "if S-D13 is decided the
  other way". S-D13 was decided as recommended, so that text describes a case that does not occur. It was left,
  since the brief asks for no other rewording.
- No benchmark was run to set a bound on `p`. The decision says the bound is the point where the method changes,
  set by a benchmark; that is work of an implementation lane (`docs/PERF.md`).
- The proofs, the sources and the code were not touched: no `src/`, no `tests/`, no `refs/`, no `docs/proofs/`
  other than the two sentences named in item 3.

## Sources pending

- `[source pending: flint-3.0.1 fmpz_mod_poly.rst, fmpz_mod_poly_factor.rst, nmod_poly.rst]`: the route of
  `solvers` P3.7(2) for larger primes, which S-D10 as decided leaves to a later slice. The files are not on
  disk. `refs/src/flint-3.0.1/` holds 22 files; `ls refs/src/flint-3.0.1 | grep mod_poly` returns nothing, so
  there is no `fmpz_mod_poly.rst`, no `fmpz_mod_poly_factor.rst` and no `nmod_poly.rst` there. The proof of
  P3.7 itself is modulo (Roots), which is on disk, and was not changed.
- No other source is quoted by the text this lane wrote. Every other statement is either a decision of TJO or a
  copy of the text of `docs/api-s.md`.

## Findings against the specification

None. No statement of `docs/SPEC.md` was found wrong, and no test was weakened.

Two things the orchestrator should know:

1. Lines longer than 116 characters in the table of decisions. A row of a Markdown table is one line, so the rows
   S-D1 to S-D19 of `SPEC.md` 15.3 run from 190 to 625 characters. The rows of 15.2 that are already there do
   the same: the row M1-D1 is 2043 characters long, and 18 lines of section 15.2 are longer than 116
   characters. I kept the form of the rows that are there, as item 1 asks, and shortened the decisions to the
   length item 1 allows.
2. The order of the rows S-D1 to S-D19 in `SPEC.md` 15.3 is the order of the table of `api-s.md` section 5, not
   the order of the milestones (S.3, S.1, S.2). The brief asked for no reordering.
