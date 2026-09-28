# Milestone 0 gate closure

Reviewed on 2026-09-28: conventions 0.3, SPEC and PLAN 1.2, and their changed reference code and tests.
Locations below are in the reviewed files, before the listed edits. `conv` means `docs/conventions.md`.
The application reports were read, but the verdicts below come from the text and code themselves.

| Item | Verdict | Evidence / remaining edit |
|---|---|---|
| G1 | CLOSED WITH EDIT | conv 3.2, 4.6, 5.4; SPEC 4.4; PLAN 4, 1.7. E1 removes a conflicting clause. |
| G2 | CLOSED WITH EDIT | conv 4.6, 5.14, 12.4; SPEC 10.4; PLAN 4. E2 resolves two lifetime ambiguities. |
| G3 | CLOSED WITH EDIT | conv 8.1, 10.2; SPEC 10.2. Bindings work; C2 and C3 complete inspection. |
| G4 | CLOSED | conv 9.5-9.6; PLAN 5. Exact-reference fixed points only; 15032 printer cases below. |
| G5 | CLOSED WITH EDIT | conv 5.7; SPEC 5. Sign checks added; C4 reconciles their status with 3.1-3.2. |
| G6 | CLOSED | conv 3.2, 4.4, 6.4; SPEC 9.3.7, 10.3; PLAN 4 agree on exact/mixed poles. |
| G7 | CLOSED | conv 5.12, 9.2-9.4, 10.2; both parsers normalize only exact trailing zeros. |
| G8 | CLOSED | conv 8.4-8.5; `_ctx_occurrences` checks every context before semantics; 12 checks. |
| G9 | CLOSED | `scaled_add` checks `_context`; subtraction inherits it. Three mismatch checks pass. |
| G10 | CLOSED | conv 11.3(6) now bounds excess over the phase hull, not its unavoidable width. |
| G11 | CLOSED | PLAN 1.8 keeps raw data local; canonical cancellation alone does not force fallback. |
| G12 | CLOSED | conv 5.2: common conditions now apply to both parenthesized radius cases. |
| G13 | CLOSED WITH EDIT | conv 5.10 names canonical keys; E3 removes the surviving provenance claim. |
| G14 | CLOSED WITH EDIT | conv 8.1, 9.7, 12 fix strings and kind/status channels; C1 fixes PLAN's compare. |
| G15 | CLOSED WITH EDIT | conv 6.4 marks the source pending; E4 adds the missing note in analysis P13. |
| G16 | CLOSED | policies S26 reduces the centre first; the 960-case regression passes. |
| CV-11 | CLOSED WITH EDIT | Global fallback has the right scope; E1. |
| CV-12 | CLOSED WITH EDIT | Distinct scaled pointers are rejected, even for exact operands; E1. |
| CV-29 | CLOSED | The retained printer encloses; only exact-rational rereading has a fixed point. |
| CV-30 | CLOSED | conv 9.7 defines the 13 enum constants and the separate status return. |
| CV-35 | CLOSED | Empty coefficient lists and exact trailing-zero normalization agree with storage. |
| CV-37 | CLOSED WITH EDIT | Dump coefficients and occurrence bindings agree; inspection needs C2, C3. |
| CV-39 | CLOSED WITH EDIT | Each occurrence has its own binding; inspection needs C2, C3. |
| CV-41 | CLOSED WITH EDIT | Heap constructors resolve incomplete-type allocation; E2 clarifies ownership. |

Counts: G1-G16: 9 CLOSED, 7 CLOSED WITH EDIT, 0 OPEN. Rejected decisions: 3 CLOSED, 5 CLOSED WITH EDIT.
These are conditional closures, not a claim that the edits have already been made. C5 is an additional issue.

## Remaining edits to the original findings

E1. In conv 4.6:328-329 replace the sentence beginning "Other context rules" through `adf_x_identical` with:

> Contexts stay immutable after construction. Matching moduli and ordered blocks permit explicit rebinding;
> they do not waive pointer-compatibility checks or the different-pointer global fallback below.

After conv 4.6:326 add this scope clarification; it also applies to the scaled rule in SPEC 4.4 and PLAN 4:

> The shared-pointer check also applies to `adf_scaled_mul_tight`, including exact operands. It compares
> the two scaled inputs, not the old context of the initialized output. Unary and exact-scalar operations
> borrow the scaled input's context; an `adf_rat` scalar has no context to compare.

E2. Replace "A constructor does not replace or free a previous `*out`" in conv 4.6:294 and the equivalent
clauses in SPEC 10.4:751 and PLAN 4:118 with:

> A constructor never frees, mutates or reuses the context previously pointed to by `*out`. On success it
> overwrites only the pointer slot; the caller must retain any previous owned pointer separately.

In PLAN 4:120-121 replace the conditional inline-allocation sentence with:

> Version 1 offers no inline context allocation.

The remainder of G2 is closed: free(NULL), failure without a retained allocation, non-failing value init,
copy/swap of borrowed pointers, and the lifetime through every borrower are explicit. No context is a value.

E3. Replace conv 5.10:660-661's sentence beginning "What the invariant guarantees" with:

> A reduction operation must enclose its constructed exact closed piece `C_n` inside `[0, 1]`.
> The midpoint predicate alone does not certify that construction or its rounding error.

For example, midpoint 1/2 and radius 100 satisfy the predicate. That does not establish how a reducer obtained
the ball. The revised first bullet already makes this distinction; the later sentence must agree.

E4. Add after `docs/proofs/analysis.md:578`, before the proof of Proposition 13:

> For real primitive chi, the identities above imply W_chi^2 = 1; they do not select its sign.
> The further assertion W_chi = 1 is
> [source pending: a local source or proof of the signed primitive quadratic Gauss sum evaluation].
> Until supplied, compute W_chi by the finite Gauss-sum formula, including for real characters.
> The real-character golden vectors check examples, not the universal signed evaluation.

The current P13 does not itself assert the universal W_chi = 1. This is an explicit scope note, not a
replacement of its correct functional equation. Its C=1 calculation is unaffected.

## New findings and full replacement text

C1. MINOR, introduced by G14: PLAN 4:164-165 puts `compare` among Boolean predicates. SPEC 4.2 and
conv 2.1 require three results. Overlapping non-exact balls need UNDECIDED = 2. Replace those two lines with:

> The set predicates `equal_set`, `overlaps`, `contains` return `int` 0 or 1, not a status.
> Point comparison returns `ADF_CMP_EQUAL = 0`, `ADF_CMP_DIFFERENT = 1`, or `ADF_CMP_UNDECIDED = 2`.

C2. MAJOR, introduced by G3: conv 10.2:1379-1381 does not say what inspection does with insufficient
descriptor capacity. The two-context witness with one initialized descriptor has no specified outcome.
Keep the descriptor and inspection signature, but replace the paragraph through "statuses ... loader" with:

> `adf_ctx_desc_init(adf_ctx_desc_t *d)` and `adf_ctx_desc_clear(adf_ctx_desc_t *d)` return void;
> init sets K=1, k=0, q=NULL; clear releases its owned integer and block array.
> With descs=NULL, ignore the incoming *nctx and write the occurrence count only on OK.
> Otherwise incoming *nctx is capacity in initialized descriptors. Validate the whole dump first;
> insufficient capacity returns ADF_LIMIT with *nctx and every descriptor untouched.
> On OK replace the first required descriptors, releasing their old contents, write that count to *nctx,
> and leave the rest untouched. Other failures also leave all outputs untouched. Statuses are loader statuses.

Also replace conv 12.10:1482's final sentence with:

> `size_t` is used for byte lengths and for binding counts, descriptor capacities and occurrence indices.

C3. MINOR, introduced by G2/G3: conv 3.2:172's `adf_modctx_new_*` row prohibits PARSE and LIMIT, although
the new `adf_modctx_new_from_dump` consumes arbitrary text. Replace that row with these two rows:

| Class of function | Possible statuses |
|---|---|
| Raw context constructors (5.14) | `OK`, `DOMAIN`, `UNSUPPORTED` (a prime power above one word) |
| `adf_modctx_new_from_dump` | `OK`, `PARSE`, `LIMIT`, `UNSUPPORTED`, `DOMAIN` |

Add after conv 10.2's `adf_modctx_new_from_dump` description:

> Validate the whole dump in the order of 8.5 before checking the occurrence index or allocating a context.

C4. MINOR, exposed by G5: conv 3.1:146 defines NOT_DETERMINED only by insufficient input precision.
In the G5 witness the sign is determined by the inputs; the result enclosure loses it. Also, the inversion
row at conv 3.2:178 still excludes the new result status. Replace the NOT_DETERMINED meaning with:

> Valid inputs do not determine the requested quantity, or the computation cannot certify a required result
> invariant. More input or working precision, or a sharper enclosure algorithm, may resolve it.

Replace the first sentence after the status table at conv 3.1:157-158 with:

> `ADF_NOT_DETERMINED` lacks the requested result certificate; `ADF_UNIT_NOT_CERTIFIED` lacks an input unit
> certificate. `ADF_NO_SOLUTION`, `ADF_NOT_UNIT` and `ADF_DOMAIN` report proved failures.

Replace the adele-to-idele/inversion row of 3.2 with:

| Class of function | Possible statuses |
|---|---|
| Adele to idele; inversion of an adele-like value | `OK`, `UNIT_NOT_CERTIFIED`, `NOT_UNIT`, `NOT_DETERMINED` |

Add immediately below the table:

> In conversion or inversion, NOT_DETERMINED covers failure to certify the required real sign on the result;
> UNIT_NOT_CERTIFIED and NOT_UNIT concern the input, as before. Failure leaves the value output untouched.

C5. MAJOR, retained conflict exposed by G1: conv 3.2:175 says conversion to a scaled context rejects loss
and can require word blocks. Conv 5.4:474 and SPEC 4.4 prescribe the best enclosing conversion, and scaled
values also allow contexts without blocks. Take s=1, u=1, K=2 and target K'=1. The formula gives scale 1,
residue 0: Zhat, strictly containing 1 + 2 Zhat. One clause requires OK with loss, the other DOMAIN.
In 3.2 replace the conversion-to-local-or-scaled row with:

| Class of function | Possible statuses |
|---|---|
| Exact conversion to the local backend | `OK`, `DOMAIN` (unrepresentable), `UNSUPPORTED` (no word blocks) |
| Conversion between scaled contexts | `OK`; best enclosure; loss reported through `int *lost` |

Add after the operation table in conv 5.4:

```c
int adf_scaled_set_context(adf_scaled_t y, int *lost, const adf_scaled_t x, const adf_modctx_struct *ctx);
```

> With initialized y and a successfully constructed ctx, this returns ADF_OK and stores the best enclosure
> in ctx. If lost is non-NULL, write *lost=1 exactly when the set changes. Word blocks are optional.
> Exact input keeps its rational s, exact=1 and u=0, with *lost=0 if requested. y may alias x; it borrows ctx.

This completes the existing enclosing-conversion contract, including exact operands; it changes no formula.
The conversion-from-tight row likewise handles R=0 by storing the exact rational c with lost=0.
Append this last sentence verbatim to the conversion-from-tight result cell in conv 5.4:473:

> For R=0, store exact=1, s=c, u=0 in the target context, with lost=0.

## Verdict

**GATE PASSED AFTER THE LISTED EDITS.** Apply E1-E4 and C1-C5 verbatim. They need no further design review.
There is no remaining arithmetic or enclosure counterexample requiring a new decision.

Milestone 1 may then write the public header for adf_rat, global adf_fball, adf_adele, adf_cadele and text
forms, followed by the scaled policy and local backend with adf_modctx. The global arithmetic layouts and
value-text signatures can proceed now using C1's comparison constants. Context construction, scaled
conversion and dump inspection declarations must incorporate the listed edits before they are frozen.

For shared scaled inputs, the result can always borrow their context. The exact tag covers zero products;
the tight product changes scale, not context. Checking pointers before writing and committing a temporary
preserve aliased inputs on failure. Pointer identity and lifetime need future C tests: the Python reference
models modulus equality and context-free exact rationals, not those C contracts.

At poles the documents now agree: exact proved pole -> DOMAIN; mixed or undecided exclusion ->
NOT_DETERMINED; value output untouched. There is no pole sentinel in finite storage. These are text-contract
checks; production pole functions and a public header do not yet exist.

## Commands and outputs

Working directory: repository root. `c=docs/reviews/m0-gate/closure-checks` and
`g=docs/reviews/m0-gate/checks` abbreviate paths below. Output files retain complete output.
Python used `-B`; subprocess suites also used `PYTHONDONTWRITEBYTECODE=1`. At most two processes ran
concurrently. Each suite was bounded by `timeout 170s`; no package was installed.

| Command | Output / result |
|---|---|
| `git diff a46b161 HEAD -- docs proto tests` (read by file groups) | Revised contracts and code inspected. |
| `timeout 170s python3 -B $c/printing.py` | 12000 random + 3000 constrained + 26 golden + 6 hard; 0 failures. |
| `python3 -B $c/contracts.py` | 60 binding, 12 limit, 806 scaled checks; 878 total, 0 failures. |
| `timeout 170s python3 -B -m unittest proto/test_text_grammar.py` | 30 tests, 0 failures, 12.609 s. |
| `timeout 170s python3 -B -m unittest discover -s tests/ref/tests` | 63 tests, 0 failures, 6.029 s. |
| `timeout 170s python3 -B proto/policies_checks.py` | 14 groups, 0 failures; S26: 960 raw triples. |
| `timeout 170s python3 -B tests/ref/mutants.py` | Baseline 62 tests; 19 mutants killed, 0 survived. |
| `python3 -B $c/audit.py` | 16 findings, 8 decisions; first 2 overlong report lines, then 0. |
| `cc -O2 -Wall -Wextra $g/flint_probe.c -o $c/flint_probe -lflint -lgmp -lmpfr` | Exit 0, 0 diagnostics. |
| `$c/flint_probe` | Exit 0; FLINT 3.0.1; output below. |

Independent printer: exact Fraction arithmetic, independently implemented decimal rounding and formatting;
seed 2026092803. All printed intervals contain their input and are exact-parser fixed points. Maximum observed
rounding iterations: 2; radius digits: 46; constrained fixed-point passes: 3. These counts are evidence, not
a uniform complexity bound. The finite decreasing-level argument in conv 9.5 supplies termination.

Hard cases: 20396493/16 +/- 1258000 at digits=2 reaches `1280000 +/- 1270000`; its negative reaches the
negative midpoint with the same radius. At digits=20, 1 +/- (1-2^-30) prints `1 +/- 0.9999999991` under
the nonzero constraint; 1 +/- 1023/1024 prints `1 +/- 0.9991`. Both negative cases also preserve sign.

The fresh installed-FLINT probe reports:

```text
flint=3.0.1
input_nonzero=1 product_nonzero=0
input=1 0 3fffffff -1e
product=1 0 30000001 -1c
read_decimal_1=1 0 10a3d70b -1f
read_decimal_2=1 0 23d70a3f -20
zero_polynomial_length=0
trailing_zero_polynomial_length=1
```

The last two decimal balls print `1 +/- 0.14` and `1 +/- 0.15`; their exact reference rereadings are fixed
points. C rereading is deliberately outside that claim. The dump-field convention is documented locally at
`refs/src/flint-3.0.1/arb.rst:288-293`; the probe results are observations, not quotations from that source.

Source pending: the signed quadratic evaluation in E4. Existing source-pending background items are unchanged;
this closure adds no external theorem and does not rerun the original gate's full source or analytic audit.
The final run of every check exited 0. The first structural audit exited 1 for the two line lengths above.
The contracts check first passed 875 checks, then 878 after adding the explicit C5 witness.
`reviewed-inputs.sha256` records the input files; the temporary compiled probe executable was removed.
