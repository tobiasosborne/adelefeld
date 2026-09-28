# Lane m0-gate-apply-docs: report

Date: 2026-09-28. Scope: apply the milestone 0 gate review (`docs/reviews/m0-gate/review.md`) to the files this
lane owns: `docs/SPEC.md`, `docs/PLAN.md`, `docs/PERF.md`, `docs/proofs/policies.md`, `proto/policies_checks.py`,
`tests/ref/` (all of it), `docs/sources.md`, `refs/README.md`.

## What was done

Findings applied, with the exact places:

- **G1 (scaled context).** `docs/SPEC.md` 4.1: the implicit global fallback is limited to `adf_fball` and types
  containing it, and does not apply to `adf_scaled`. `docs/SPEC.md` 4.4: new paragraph "Context compatibility for
  scaled values": a default binary operation needs the same context pointer, otherwise `ADF_DOMAIN` with the value
  output untouched; the result borrows that context; exact operands follow the same rule; the caller builds the
  `lcm(K, K')` context and no operation creates it. `docs/PLAN.md` section 4: new paragraph "Scaled context rule".
  `docs/PLAN.md` row 1.7 records the rule and its tests.
- **G11 (PLAN 1.8).** `docs/PLAN.md` row 1.8 now says local values retain raw numerator residues and their
  denominator; a result whose raw set cannot be represented in the shared caller-owned context, or that uses
  different context pointers, is global unless the caller supplies a target context; canonical cancellation alone
  does not force a local value global; equality and value printing use the canonical triple.
- **G2 (context life cycle).** `docs/PLAN.md` section 4 "Contexts": incomplete public struct, constructors
  `adf_modctx_new_*` taking `adf_modctx_struct **out` first, `adf_modctx_free`, success/failure states, caller
  ownership, no replace of `*out`, and the inline-allocation alternative. `docs/SPEC.md` 10.4 carries the same
  contract.
- **G3 (dump bindings).** `docs/SPEC.md` 10.2 and `docs/PLAN.md` section 5: the dump loader takes one caller-owned
  context binding per local-`adf_fball` or `adf_scaled` occurrence in traversal order; each binding matches the
  modulus and ordered blocks; repeated occurrences may share a pointer; `identical()` additionally needs the
  original context pointers.
- **G4 (C text is not a fixed point).** `docs/PLAN.md` section 5: parsing a printed value encloses the stored value;
  the fixed-point statement holds only for the exact-rational reference parser; a C round trip may widen and change
  the text on every pass; dump text is the identity-preserving form.
- **G5 (idele sign).** `docs/SPEC.md` section 5: new paragraph "Sign preservation": the real sign is validated on
  the result; on failure `ADF_NOT_DETERMINED` and the value output untouched; never `ADF_NOT_UNIT`; applies to
  product, inverse, powers and class conversion; the `1 +/- (1 - 2^-30)` witness is recorded.
- **G6 (pole status).** `docs/SPEC.md` 9.3.7 (local zeta factor row): exact input at a proved nonremovable pole is
  `ADF_DOMAIN`; a mixed or undecided ball is `ADF_NOT_DETERMINED`; both leave the value output untouched and no
  non-finite ball is stored. `docs/SPEC.md` 10.3 and `docs/PLAN.md` section 4 (status codes) say the same.
- **G14 (returned strings, kinds, predicates).** `docs/SPEC.md` 10.4 and `docs/PLAN.md` section 4: `get_str` and
  `dump_str` take `size_t *len` and return an allocated `char *` with a NUL at `s[len]` not counted in `len`;
  `adf_str_free` frees it; `adf_text_classify` returns an `ADF` status and writes an `adf_text_kind` enum only on
  `OK`; set predicates return `int` 0 or 1.
- **G16 (canonical triple reduces first).** `docs/proofs/policies.md` Summary 26: the canonical-column proof now
  reduces `A'` modulo `H'` first and gives `(R/g', H'/g', d'/g')`. The table of statements and the review record
  name the new check. `proto/policies_checks.py` gains `check_s26_centre`.
- **G9 (reference context precondition).** `tests/ref/adfref/policies.py` `scaled_add` now checks `x.K == y.K` on
  the non-exact path through `_context`, so `scaled_add` and `scaled_sub` raise `ValueError` on a mismatch as
  `scaled_mul` already did. New tests in `tests/ref/tests/test_policies.py` (class `CrossContext`); new mutant in
  `tests/ref/mutants.py`; `tests/ref/README.md` states the precondition and the lcm workaround.
- **Sources (item 5).** `docs/sources.md` table 1 gains 20 `uops-intel` rows, one per page fetched by
  `refs/fetch_intel.sh`, hashes from `refs/manifest-intel.sha256`. A note after table 1 records that the page
  `uops-intel/ADD_01_R64_R64.html` has an "AMD Zen 2" block (measured loop throughput 0.25 at line 1320,
  documented 0.25 at line 1330), which settles pending item 4. Pending item 4 is rewritten as settled.
  `refs/README.md` gains the `uops-intel` key, the `fetch_intel.sh` usage and the `manifest-intel.sha256` mention.
- **Decision ids (item 6).** `docs/SPEC.md` 15.2 states that `docs/conventions.md` writes the same twelve decisions
  as D1 to D12 and gives the correspondence to M0-D1 to M0-D12.
- **Version (item 7).** `docs/SPEC.md` and `docs/PLAN.md` are version 1.2 with a change log entry listing the
  findings applied. Headers now say the gate review is applied and the repaired contracts await re-review.
  `docs/PLAN.md` milestone 0 status table row "gate" records the same.

`docs/PERF.md` required no change: no finding names it, and its section 1a and 1b already cite `uops-intel` with
`refs/manifest-intel.sha256`.

## Files written

- `docs/SPEC.md` (version 1.2; sections 4.1, 4.4, 5, 9.3.7, 10.2, 10.3, 10.4, 15.2)
- `docs/PLAN.md` (version 1.2; header, sections 4, 5, rows 1.7, 1.8, milestone 0 status)
- `docs/proofs/policies.md` (Summary 26 proof, table of statements, review record)
- `proto/policies_checks.py` (`check_s26_centre`)
- `tests/ref/adfref/policies.py` (`scaled_add` context check and docstring)
- `tests/ref/tests/test_policies.py` (class `CrossContext`, 4 tests)
- `tests/ref/mutants.py` (one mutant: "scaled sum ignores a differing context (G9)")
- `tests/ref/README.md` (shared-context precondition and workaround)
- `docs/sources.md` (20 `uops-intel` rows, table-1 note, pending item 4)
- `refs/README.md` (`uops-intel` key, `fetch_intel.sh`, `manifest-intel.sha256`)

No file outside this list was changed. `docs/PERF.md` was read but not changed.

## Checks run

1. G9 red, before the fix:
   `python3 -B -m unittest tests.ref.tests.test_policies.CrossContext -v` (repository root).
   Result: 4 tests, 2 failures (`test_add_rejects_different_contexts`, `test_sub_rejects_different_contexts`,
   both "ValueError not raised"); `test_mul_rejects_different_contexts` and
   `test_lossless_conversion_then_same_context_add` passed.
2. G9 green, after the fix: same command. Result: 4 tests, OK.
3. Whole reference suite, as the brief gives it:
   `python3 -B -m unittest discover -s tests/ref/tests` (repository root).
   Result: `Ran 63 tests in 6.030s`, `OK` (was 59 tests before the 4 new G9 tests).
4. Mutation harness from `tests/ref`: `python3 -B mutants.py`.
   Result: `baseline: 62 tests, failures 0, errors 0`; `mutants killed: 19`; `mutants survived: 0`
   (was 18 killed before the new G9 mutant).
5. Vectors regenerated and compared:
   `python3 -B tests/ref/gen_vectors.py`, then `diff -rq` against a saved copy.
   Result: `VECTORS UNCHANGED`. The vectors are unaffected: `gen_vectors.py` always builds the two scaled
   operands with the same `K`, so the new precondition never fires in vector generation. The vector files were
   not rewritten in content (regeneration reproduced them byte for byte).
6. Proof checks: `python3 -B proto/policies_checks.py`.
   Result: `all checks passed`. New last line:
   `PASS check_s26_centre (S26, G16): old rule leaves (9,4,1) for the square of 3 + 4 Zhat; corrected rule gives
   (1,4,1); 960 raw triples agree with the corrected canonical triple`.
   The red side of G16 was shown separately: `canonical_triple_old(9,4,1)` gives `(9,4,1)`, `is_canonical_triple`
   False (violates `0 <= A < H`); `canonical_triple_new(9,4,1)` gives `(1,4,1)`, canonical True.
7. Intel manifest: `./fetch_intel.sh --check` in `refs/`.
   Result: all 20 pages `OK`, exit 0.
8. Hash check of the new rows: a script compared every `uops-intel` row of `docs/sources.md` against
   `refs/manifest-intel.sha256`. Result: `rows: 20`, `OK`, no missing row, no hash or path mismatch.
9. Line length, non-table lines over 116 characters: checked for the ten edited files. Result: no new overlong
   prose line; the remaining overlong lines are pre-existing table rows, quoted source lines or code-block
   padding (for example `docs/SPEC.md` 21, 156, 174, 308, 311, 448, 471, 520, 623; `docs/PLAN.md` 335).

## What is not done

- The gate review is applied but not re-reviewed. The header and the milestone 0 status say the repaired
  contracts await re-review. This lane does not claim the gate passed.
- `docs/PERF.md` was not changed (not needed by any finding).
- Findings that belong to other lanes were not touched here: G7, G8, G10, G12, G13 and the conventions side of
  G1, G2, G3, G4, G5, G6, G14 (files `docs/conventions.md`, `tests/golden/`, `proto/text_grammar.py`); G15
  (`docs/proofs/analysis.md`).
- No C code, no public header, no Julia binding was written, as at this milestone.
- The two new G9 tests exercise the reference only. The C-side scaled context rule of G1 is written in SPEC and
  PLAN but has no C implementation yet.

## Sources pending

- Settled by this lane: the uops.info Zen 2 register-register form for `add r64, r64` (pending item 4 of
  `docs/sources.md`), by `uops-intel/ADD_01_R64_R64.html`.
- Still pending, unchanged: a lawful copy of Tate's thesis (the "section 2.2" attribution in `docs/SPEC.md` 6);
  a source for the names "arithmetic" and "geometric" (made unnecessary by M0-D10 but still remarked in
  `docs/SPEC.md` 9.3.7); an open text with proofs of the domains of the p-adic sine, cosine, sinh and cosh series
  (`docs/SPEC.md` 9.3.2).
- Named by the gate review but outside this lane: the signed primitive quadratic Gauss sum evaluation for
  `W_chi = 1` (G15, `docs/proofs/analysis.md`); the catalogue's Hensel, reciprocity, Gamma, Haar and unit-group
  imports; the general-field background in `docs/seams.md`; the FLINT aliasing, cleanup and Dirichlet
  documentation and Nemo compatibility items in `docs/conventions.md`.

## Findings against the specification

- No new counterexample to `docs/SPEC.md` was found while applying the review. The gate review itself found no
  counterexample to the repaired finite enclosure, unit-coset, character, root or analytic bound formulas.
- G3, G5 and G6 were the review's findings against the specification, and they are applied as above: G3 qualifies
  the dump identity with caller-owned context bindings; G5 makes "rounded as usual" compatible with the idele sign
  invariant; G6 gives the pole case precise statuses. G4 and G11 were PLAN/statement repairs and are applied. G10
  and G15 lie in other lanes' files.
- One stale statement was corrected while reading: the note that the local-backend raw-storage decision was
  "pending the gate review" (CV-55) now records that the gate review accepted it, in `docs/SPEC.md` 4.1 and
  `docs/PLAN.md` section 4.
