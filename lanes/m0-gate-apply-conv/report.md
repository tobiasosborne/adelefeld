# Lane m0-gate-apply-conv: report

Work package: apply the findings of `docs/reviews/m0-gate/review.md` that concern `docs/conventions.md`, the
reference `proto/text_grammar.py`, its tests and `tests/golden/`, and mark the 60 decisions of section 13.
Everything below was run from the repository root unless stated. Numbers are measured, not estimated.

## What was done

`docs/conventions.md` is now version 0.3 with a change log entry naming the applied findings. Applied here:
G1, G2, G3, G4, G5, G6, G7, G8, G10, G12, G13, G14, G15 (its `docs/conventions.md` location; the
`docs/proofs/analysis.md` half belongs to another lane). The 8 rejected decisions CV-11, CV-12, CV-29, CV-30,
CV-35, CV-37, CV-39, CV-41 are marked **replaced (Gn)** in the decision table with their replacement text in
force; the remaining 52 are marked decided (12 as D1 to D11 with "accepted by the gate review" in the intro of
section 13, 40 as "decided (gate)"). Per finding:

- **G1** (4.6, 5.3, 5.4, 3.1, 3.2): the implicit global fallback is limited to `adf_fball` and types containing
  it; a default binary `adf_scaled` operation requires the same context pointer in both inputs and returns
  `ADF_DOMAIN` (value output untouched, check before writes) otherwise; the result borrows that input context;
  exact operands follow the rule; combining contexts is the caller's `lcm(K, K')` construction plus lossless
  conversion; a separately named target-context operation documents loss and output context. Added a scaled
  arithmetic row to 3.2 (`OK`, `DOMAIN`), removed set predicates from the void row (they return `int` 0 or 1),
  added the context-compatibility case to `ADF_DOMAIN` in 3.1.
- **G2** (4.6, 5.14, 2.3, 4.3, 4.5, 12.4): `adf_modctx_struct` incomplete; constructors `adf_modctx_new_*` with
  `adf_modctx_struct **out` first; `adf_modctx_free`, NULL-safe; success allocates, fully initialises an
  immutable context and writes `*out`; failure leaves `*out` untouched and retains no allocation; a constructor
  never replaces a previous `*out`; no by-value or array-of-one context type; value init stays non-failing.
  Version 1 offers no inline context allocation (see "replacement texts" below). 5.14's constructors renamed
  `init` to `new`; 12.4's `adf_sizeof_modctx`/`adf_modctx_t` removed (CV-41 replaced).
- **G3** (10.2, 8.1): the loader takes an array of caller-owned context bindings and its length, one per
  local-fball or scaled occurrence in dump traversal order including nested pieces; each binding must match its
  occurrence's modulus and ordered blocks; repeated occurrences may share a pointer; an inspection function
  reports occurrence count and descriptors without constructing a value; `identical()` additionally requires
  the original context pointers. The one-context `adf_x_load_str` remains as the convenience; a binding-count
  mismatch, `NULL` binding or mismatch is `ADF_DOMAIN`. `adf_modctx_init_load_str` (undefined for a dump with
  two contexts) is replaced by `adf_modctx_new_from_dump(out, s, len, occurrence, lim)`.
- **G4** (9.6 second bullet, 9.5 properties, CV-29 marker, 14/F6): the print-read-print fixed point is scoped
  to the exact-rational reference parser; repeated C value-text round trips may widen and change the text every
  pass; parsing a printed value still encloses the stored value; dump text is the identity-preserving form;
  constrained printing preserves the sign of the exact interval, with `NOT_DETERMINED` still possible at a
  given `prec`.
- **G5** (5.7, 3.2 idele row, 4.4 row): idele and idele-class arithmetic validates the required real sign on
  its result before committing it; a kernel may use sign-preserving endpoint bounds and a suitable enclosing
  midpoint-radius ball; failure to produce a finite ball with the required sign returns `ADF_NOT_DETERMINED`
  with the value output untouched; never `ADF_NOT_UNIT`. `NOT_DETERMINED` added to the row; the rule is stated
  for multiplication and division, inversion, powers and class conversion.
- **G6** (6.4 poles paragraph, 3.2 rows, 4.4 row): an exact input at a proved nonremovable pole gives
  `ADF_DOMAIN`; a ball meeting a pole and also containing regular points, or with undecided pole exclusion,
  gives `ADF_NOT_DETERMINED`; both leave the output untouched; no non-finite ball is stored; a report may
  distinguish proved from undecided intersection. The rows read "DOMAIN for an exact pole;
  NOT_DETERMINED for a mixed/undecided ball".
- **G7** (5.12, 9.2 grammar, 9.3, 9.4, 10.1, 10.2, CV-35/CV-37 markers): `P` is a normalised `acb_poly` with
  length `>= 0` and no exact-zero last coefficient; length 0 is the zero polynomial; terms may keep a zero `P`;
  the value grammar admits `P=[]` and removes exact trailing zero coefficients on input; dumps must already
  have the normalised length (nonempty with exact-zero last coefficient: `ADF_DOMAIN`, length 0 accepted);
  a ball merely containing zero is not trimmed. Both parsers and the golden vectors changed together.
- **G8** (8.4, 8.5 stage 4, 10.2 counts): every count limit, in particular the block count of every context
  occurrence including nested ones, is checked before any semantic check, whatever fails later.
- **G10** (11.3 item 6): the default enclosure must contain every listed phase; a single phase is bounded by a
  shrinking precision-dependent tolerance; several phases are compared with the rectangular hull of the listed
  phases and only the numerical excess is bounded; the strict variant returns `ADF_OK` exactly for singleton
  images.
- **G12** (5.2): the display of `G(A, H, d)` is parenthesised as the review's replacement.
- **G13** (5.10, 9.3 row): in the PIECES invariant and order keys `A, H, d` denote the finite part's canonical
  global triple including local storage; `d = 1`, `H >= 0`, the 5.2 centre range, midpoint in `[0, 1]`; these
  are storage predicates only, and a reduction operation separately ensures enclosure of the constructed exact
  closed piece. Order keys `N, m` are the canonical `H, A`.
- **G14** (8.1, 2.3, 4.2, 9.7, 12.3, 12.8, 3.2): `get_str`/`dump_str` take `size_t *len` and return the
  allocated string; the `len` bytes are ASCII without embedded NUL, a terminating NUL is at `s[len]` and not
  counted; freed with `adf_str_free`. `adf_text_classify` returns an ADF status and writes an `adf_text_kind`
  (one named constant per start symbol, enum listed) through an output pointer only on `ADF_OK`. Set predicates
  return `int` 0 or 1 in their own 3.2 row.
- **G15** (6.4): "For a real primitive character `W_chi = 1`" is kept but marked
  `[source pending: a local source or proof of the signed primitive quadratic Gauss sum evaluation]`; until
  supplied, `W_chi` is computed by the finite Gauss-sum formula of other characters, and the real-character
  golden vectors are finite checks, not a proof.

## Files written

- `docs/conventions.md` (version 0.3; findings above; decision table: 60 rows, 52 decided, 8 replaced).
- `proto/text_grammar.py` (G3: `_ctx_occurrences`, `dump_contexts`, `dump_load_check`; G7: empty `P`, trailing
  exact-zero trimming in the value form, strict dump rule; G8: context block counts in stage 4; docstring).
- `proto/test_text_grammar.py` (new `TestGateFindings`, 4 tests: G8, G3, G7, G4).
- `tests/golden/rfun.tsv` (14 -> 19 vectors; `P=[]` moved from `!PARSE` to accepted; 6 normalisation vectors).
- `tests/golden/dump.tsv` (161 -> 165 vectors; the `rfun` `P`-length-0 vector flipped `!DOMAIN` -> roundtrip;
  added a trailing exact-zero-top `!DOMAIN` vector, a contains-zero-top roundtrip vector, the two-context
  quotient dump of G3 and the raw-piece dump of G13).
- `tests/golden/realball_print.tsv` (23 -> 26 vectors: `1 1/8 20 -> 1 +/- 0.13`, the Arb rereadings
  `1 279172875/2147483648 20 -> 1 +/- 0.14` and `1 601295423/4294967296 20 -> 1 +/- 0.15`).
- `tests/golden/realball_read.tsv` (28 -> 30 vectors: `1 +/- 0.13 -> 87/100 113/100`, `1 +/- 0.14 -> 43/50 57/50`).
- `tests/golden/README.md` (counts: 727 vectors, 327 of them a status; note on the hand-added vectors).
- `lanes/m0-gate-apply-conv/report.md` (this file).

## Checks: commands and results

Evidence of each finding was reproduced before the change (the reviewer's check or a two-line computation):

- G1, set computation (script): `2 Zhat + 3 Zhat = Zhat` contains 1; no set `s (u + 2 Zhat)` or
  `s (u + 3 Zhat)` equals `Zhat`; `lcm(2, 3) = 6` is in neither input.
- G2, read conventions 0.2 lines 108, 220, 268, 674, 683, 1339: no init value or failed-init state for
  `adf_modctx`; no alignment query.
- G3, `tg.dump_roundtrip` of `adf1 Q qclass pieces 2 1 1 -2 0 0 l 1 2 1 2 0 1 3 -2 0 0 l 1 3 1 3 0`:
  accepted, byte-identical; occurrences `(2, (2,))` and `(3, (3,))`; one `ctx` argument cannot match both.
- G4, `python3 -B docs/reviews/m0-gate/checks/roundtrip.py`: `1 0 10a3d70b -1f` prints `1 +/- 0.14`;
  `1 0 23d70a3f -20` prints `1 +/- 0.15`; `C_read_print_fixed_point_failures=2`;
  `tg.print_real(1, 1/8, 20)` = `1 +/- 0.13`.
- G4, `cc -O2 -Wall -Wextra docs/reviews/m0-gate/checks/flint_probe.c -o /tmp/gate_flint_probe -lflint
  -lgmp -lmpfr && /tmp/gate_flint_probe`: exit 0, flint 3.0.1, `read_decimal_1=1 0 10a3d70b -1f`,
  `read_decimal_2=1 0 23d70a3f -20` (the checks directory was not written).
- G5, the same probe: `input_nonzero=1 product_nonzero=0`, `input=1 0 3fffffff -1e`,
  `product=1 0 30000001 -1c`; the exact product set has lower end `2^-60 > 0`.
- G6, set computation: 0 and 1/8 both lie in `[-1/4, 1/4]`; 0 is a pole, 1/8 regular.
- G7, the same probe and reference calls: `zero_polynomial_length=0`,
  `trailing_zero_polynomial_length=1`; `P=[(0)+(0)*i]` printed 1 coefficient,
  `P=[(1)+(0)*i, (0)+(0)*i]` printed 2, `P=[]` returned `!PARSE`.
- G8, `tg.dump_roundtrip(s, tg.Limits(max_items=1))` on the 4 review dumps: all 4 returned their own text
  (accepted), not `!LIMIT`.
- G10, `tg.psi_phases(b"(* ; 0 mod 1/2)")`: `0 1/2`, i.e. +1 and -1; any rectangle containing both has
  real radius >= 1.
- G12, Boolean evaluation on `A=1, H=0, d=-1`: `(C and P) or E` = True, `C and (P or E)` = False.
- G13, `tg.dump_roundtrip` of `adf1 Q qclass pieces 1 1 1 -1 0 0 l 2 6 2 2 3 0 0` and a gcd computation:
  accepted; raw `(0, 6, 2)`, `g = 2`, canonical `(0, 3, 1)`, so `d = 1` holds only canonically.
- G14, read conventions 0.2 lines 153, 915, 926, 1164, 1353: set predicates under `void` versus 2.3
  `int 0 or 1`; no length output for returned strings; no kind enum for `classify`.
- G15, `W_chi W_conj(chi) = 1` with `conj(chi) = chi` (analysis P13): `W_chi^2 = 1`, leaving `+1` or `-1`;
  the choice needs a signed quadratic Gauss sum evaluation not on disk.

Red-green (rule 2). Red command `python3 -B -m unittest proto/test_text_grammar.py`: FAILED, 30 tests,
failures=6, errors=1. The failing artefacts:
G3 `test_g3_context_occurrences_and_bindings` (ERROR: no inspection/binding entry points existed);
G4 `realball_print.tsv:22` "expected '1 +/- 0.13', got '1 +/- 0.14'" (the vector asserted the refuted
fixed-point claim and was repaired to `1 +/- 0.14`, with the rereading `1 +/- 0.15` added);
G7 `rfun.tsv:10-14` (5 vectors: `P=[0]` must print `P=[]`, `P=[1,0]` must print `P=[1]`, `P=[]` must parse),
`dump.tsv:165` (`rfun` with `P` length 0 asserted accepted), `dump.tsv:166` (trailing exact-zero top asserted
`!DOMAIN`), `test_g7_polynomial_normalisation`, `test_fixed_points`;
G8 `test_g8_context_block_counts_have_limits` (all 4 dumps accepted at `max_items=1`; also `7 2 2 3` gave
`!DOMAIN` where stage order demands `!LIMIT`).
Green after repairing `proto/text_grammar.py` and the two vector files:
`python3 -m unittest proto/test_text_grammar.py`: 30 tests OK in 12.6 s (the suite had 26 tests before this
lane; 4 tests added). Note on G3: its two-context dump was already accepted by the reference (the review says
so); the defect it shows is in the documented loader interface, which cannot serve it (G3 evidence above); the
red that failed was the missing inspection/binding capability. The G3 golden vector passed from the start and
locks the behaviour.

Golden inventory after the change: 727 vectors, 327 of them a status (was 713/328). Counted with a script over
`tests/golden/*.tsv`, excluding comments and blank lines.

Mutation check `python3 lanes/m0-conventions/mutants.py` (usage: `mutants.py [first] [last]`, 31 mutants, each
mutant run as a failfast unittest on a copy in a temp dir), run in six ranges under 2 minutes each with
`PYTHONUNBUFFERED=1 timeout 115`:

| Range | Result |
|---|---|
| 0 7 | killed 8, survived 0 |
| 8 15 | killed 8, survived 0 |
| 16 23 | killed 7, survived 1 (index 23, "NUL accepted (8.2)") |
| 24 25 | killed 2, survived 0 (index 24 by the harness's designed 90 s timeout) |
| 26 28 | killed 3, survived 0 |
| 29 30 | killed 2, survived 0 |

Total: 30 killed (indices 17 and 24 by the designed timeout kill), 1 survived. The survivor is pre-existing and
documented as an equivalent mutant by the author of the harness (`lanes/m0-conventions/report.md` line 114:
"1 survived, the equivalent 'NUL accepted' of part A"): with NUL admitted in `_prep`, the grammar still rejects
every NUL string with `PARSE`, so no test can distinguish it. This lane did not cause it (no `_prep` or NUL
vector change) and did not weaken any test to hide it.

Structural checks of `docs/conventions.md` (script): 0 non-table lines over 116 characters (table rows keep the
file's existing style and can exceed, as in 0.2); decision table has 60 rows, status cells: 40 "decided (gate)",
12 "decided (Dn)", 8 "replaced (G1, G1, G2, G3, G3+G7, G4, G7, G14)"; no stale `adf_modctx_init`,
`adf_modctx_clear`, `adf_modctx_t`, `adf_sizeof_modctx`, `init_load_str` or `print(parse(print(v)))` references
(grep returned nothing).

`docs/reviews/m0-gate/checks/contracts.py` (read-only) no longer runs to completion: it exits 1 at its G9 record
because lane m0-repair-analysis has already repaired `scaled_add` in `tests/ref/adfref/policies.py` (the script
asserts the old, defective behaviour). My evidence for the G9-adjacent records was reproduced with the snippets
in the table above instead.

## Replacement texts: completions (rule 3)

None of the replacement texts was wrong. Five were incomplete and are completed in the document:

1. G1 ends with an instruction ("changes a public function signature and requires checking the resulting header
   proposal"). The signature change is recorded (3.2 scaled row, 5.4 preamble); no header proposal exists in
   this lane's files, so that check is listed under "not done".
2. G2 leaves open whether inline context allocation is kept ("if retained ... specify size AND alignment ...").
   Resolved decisively: not offered in version 1 (4.6 states this and why).
3. G3 does not state the status of a binding error. Used `ADF_DOMAIN`, the status of the rule it replaces
   ("otherwise, or if ctx is NULL, the status is `ADF_DOMAIN`", conventions 0.2 line 1264). The inspection
   function is completed with a descriptor type, init/clear and a two-call pattern (10.2).
4. G14 does not fix the position of `size_t *len`. Placed before inputs, per 2.2 ("Outputs; then ... inputs").
   The enum constants are named `ADF_TEXT_RAT` ... `ADF_TEXT_CHAR`, one per start symbol of 9.2.
5. G13's replacement does not restate what a piece means and the gluing rule; both kept from 5.10 unchanged and
   merged with the replacement text.

## What is not done

- The header proposal check at the end of G1 (no header file under this lane's ownership).
- `lanes/m0-conventions/write_golden.py` does not contain the vectors this lane added; regenerating
  `tests/golden/` from it would drop them. Noted in `tests/golden/README.md`; the script belongs to another
  lane.
- The `docs/SPEC.md`, `docs/PLAN.md`, `docs/proofs/policies.md`, `docs/proofs/analysis.md` and `tests/ref/`
  sides of the review (PLAN G1 and G4, G9, G11, G16, the G15 half in `analysis.md`) belong to lane
  `m0-gate-apply-docs` and were not touched.
- No C code exists yet, so the C-side contracts added or repaired here (G4, G5, G7 dump strictness, G14
  signatures) are encoded in the golden vectors and the reference only; a future C test uses them via
  `docs/conventions.md` 11.3.
- No claim about the 60 decisions beyond their marking: the interface contracts repaired by G1, G2, G3 and G14
  still need the follow-up review the verdict asks for.

## Sources pending

- A local source or proof of the signed primitive quadratic Gauss sum evaluation (G15; marked in 6.4).
- Inherited and unchanged: the FLINT general aliasing section, FLINT `flint_cleanup` documentation, FLINT
  `dirichlet.rst`, Tate's section number and sign attribution, the arithmetic/geometric reciprocity names, the
  analytic background and Gamma bounds listed in `docs/proofs/analysis.md`, and the Nemo.jl layout/version
  compatibility item (12.4 keeps it unverified).

## Findings against the specification

No new counterexample to `docs/SPEC.md` was found while applying the review. As the review itself records, the
findings applied here are against conventions, its reference and its acceptance tests (G4 refutes the C text
identity of conventions 9.6 and PLAN 1.8, not SPEC's enclosure statement; G5 refines SPEC 5's "rounded as
usual" for the idele invariant; G6 assigns statuses to SPEC's pole requirement; G10 refutes a conventions
acceptance test; G13 fixes a storage predicate to say what SPEC 4.1 already calls the canonical triple). The
SPEC and PLAN amendments named by the review are the other lane's work.
