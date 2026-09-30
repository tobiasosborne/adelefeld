# Lane i-repair1: repairs after the review of milestone 2, and the pass over the documents

Part A. The review `docs/reviews/m2/review-slices-2-3.md` (codex gpt-6.1-sol) found no wrong enclosure
in slices 2 and 3 of milestone 2 and has the findings F1 (MAJOR) and F2 to F5 (MINOR). Its programs are
in `lanes/i-review2/` (`limit_alloc.c`, `findings.c`). See each finding reproduce BEFORE you change
anything (`lanes/i-repair1/redgreen.log`).

Decisions of the orchestrator:
- F1: in every function of `idele.h`, `idclass.h`, `idpow.h`, `idmap.h` that takes a `prec`, the test of
  `prec` against `ADF_IDELE_PREC_MAX` comes first, before the entry check of `ADF_CHECK_INVARIANTS`
  and before anything that reads an input (N-D8: decided from `prec` alone). Test: the reviewer's
  allocation counter in a test of the INV build.
- F2: the sentence of `idclass.h` gets the condition of Statement G.5 (the rational fits the precision
  of the class call). The sentence of `idele.h` on the norm ("each end is rounded once") is made true:
  EITHER the code rounds once (the exact end point of the input ball times the exact rational, one
  directed rounding; then the norm of an exact 5 at `prec` 2 is the exact 1), OR the sentence says
  what the code does. Take the first if Statement F and kernel B allow it without a change of any
  result that the vectors of `tests/ref/vectors/i-slice2/` fix; else the second; say which and why.
- F3: `idpow.h`: the exactness promise holds for `k > 0` (Statement K.4).
- F4: Statement L.5 of `docs/api-2.md` is corrected for `k < 0` (copies of the inverse) and `k = 0`.
- F5: Statement M.2 of `docs/api-2.md`: the ratio of the radii is stated in the right direction.

Part B. The pass over the documents. Lanes of the night reported these inconsistencies; none is a
defect of the code; the code stands and the documents follow:
1. `docs/conventions.md` 3.2 (the table of statuses of each class of functions): add rows for
   `adf_lball` (arithmetic; valuation, absolute value, decomposition; `pow_si`), for `adf_sball`
   (projection, operations, functions at a place, with `where`), for the series at a prime
   (`lfunc.h`), for the real functions on `arb`; add `LIMIT` to the row of unit cosets, ideles and
   classes. Read each header for the statuses it returns; the header is the source.
2. `docs/conventions.md` 5.7: the accessors are `adf_idclass_get_t`, `adf_idclass_get_unit` (as the
   code and as `adf_idele_get_unit`). 5.6: the exponent of a power is an `slong`. 5.7: the simple ball
   is formed from the normal form.
3. `docs/conventions.md` 2.2 and 7: `adf_places_t` is named and defined nowhere. A set of places is an
   array of `adf_place_t` with a length (as `sball.h` does); replace the name or define it as that.
4. `docs/PLAN.md` row 2.1: "`k = 1, -1` give `M_k = N`" contradicts `docs/proofs/ideles.md` P13.4
   (`M_k = Nbar`); the proof file holds. Row 1F.4 and `docs/SPEC.md` 9.3.2: the sentences that call the
   implementation "our wrapper around FLINT's centre evaluation" are replaced by what N-D9 says
   (`docs/SPEC.md` 15.4). Change the sentences only; keep a line in the change log of each document
   that names the decision or the finding (the documents have change logs at their head: follow their
   form).
5. `docs/proofs/functions.md`: the statements that lanes proved in `docs/api-1f.md` (L0 to L13, S1 to
   S9) and `docs/api-1f4.md` (F1 to F8 if present) are NOT copied into the proof file; add to its
   "Statement index and proof status" a table that names each of them, the file and line where it is
   proved, and its review status as the reviews under `docs/reviews/f1/` give it. The same for
   `docs/proofs/ideles.md` and the statements A to O of `docs/api-2.md`, with `docs/reviews/m2/` and
   `lanes/i-review1/result.md`.
6. `docs/PLAN.md` section 6: a status table for milestones S, 1F and 2 in the form of the table
   "Status of milestone 0", from `git log` and the result files in `lanes/` (what is done, what is
   left: for 1F the work packages 1F.5 to 1F.9 and `sin`, `cos`, `sinh`, `cosh` at a prime; for 2 the
   dump forms; text forms are in work in lane t-slice1).

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold for the headers
named; rule 5: no mutation run), `docs/SPEC.md` 15.4, the review, and for part B every header you
describe.

**You own:** `src/idele.c`, `src/idclass.c`, `src/idpow.c`, `src/idmap.c`, the four headers of part A
(sentences and the order of checks only), `tests/test_idele_maps.c`, `tests/test_idclass.c`,
`tests/test_idpow.c`, `tests/test_idmap.c` (additions), `docs/api-2.md` (sections 2 and 3 only; another
lane adds a section 4), `docs/conventions.md`, `docs/PLAN.md`, `docs/SPEC.md` (ONLY the sentences of
9.3.2 named above and its change log), `docs/proofs/functions.md` and `docs/proofs/ideles.md` (ONLY the
index tables), `lanes/i-repair1/`. Everything else is read-only. Another lane changes `src/text.c`,
`text.h` and `tools/adf/` at this moment.

Checks: `make clean && make -j2 check-all`, `make clean && make -j2 check SAN=1`,
`make clean && make -j2 check CC=clang`, `make clean && make -j2 check INV=1`,
`sh lanes/m1-headers/check_headers.sh` pass, each under `timeout 900`, in the foreground; the last line
of each. The reviewer's two programs again, with what they print. No line of prose above 116
characters in what you wrote.

Result: `lanes/i-repair1/result.md` and the same text as your final message: part A finding by
finding; part B item by item with the sentences changed (file and line); what is not done. Leave no
compiled binary in your lane directory.
