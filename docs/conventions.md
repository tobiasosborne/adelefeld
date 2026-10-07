# adelefeld: conventions, version 0.4 (work package 0.4, parts A and B)

Date: 2026-09-28. Status: **draft with the milestone-0 gate review and its closure edits applied; nothing is
frozen.** Written by the lane
`m0-conventions`. Read `SPEC.md` first. This document fixes, as a contract, what `PLAN.md` section 4 and 5 leave as
a proposal: canonical forms and storage invariants, naming, aliasing and ownership, status codes, signs and
normalisations, the text grammar, and the rules of the foreign-function interface. Two programmers who follow it
should write interchangeable code.

## Change log

- **0.4, edit of 2026-09-30 (lane `i-repair1`, pass over the documents).** The code and the headers stand; the
  text follows them. 3.2: rows for `adf_lball`, `adf_sball`, the series at a prime (`lfunc.h`), the real functions
  on `arb` and at a place; `LIMIT` in the row of unit cosets, ideles and classes (each row is read from its
  header). 5.6: the exponent of a power is an `slong`. 5.7: the accessors are `adf_idclass_get_t` and
  `adf_idclass_get_unit`, as the code, and `adf_idele_get_unit`; the simple ball is formed from the normal form.
  2.2 and 7: `adf_places_t` was named and defined nowhere; a set of places is an array of `adf_place_t` and a
  length `n`, as in `sball.h` (finding of lane `f-slice1`, `docs/api-1f.md` line 246).
- **0.4 (2026-09-28), closure edits applied.** The edits of the milestone-0 gate closure check
  (`docs/reviews/m0-gate/closure.md`) are applied verbatim: E1 (4.6: the shared-pointer check covers
  `adf_scaled_mul_tight`, unary and exact-scalar operations; contexts are immutable, matching blocks permit
  explicit rebinding only), E2 (4.6, `SPEC.md` 10.4, `PLAN.md` 4: a constructor overwrites only the pointer slot),
  E3 (5.10: the reduction operation, not the midpoint predicate, encloses the constructed exact closed piece),
  C1 (`PLAN.md` 4: point comparison returns the three `ADF_CMP_*` values, it is not a Boolean predicate), C2
  (10.2, 12.10: `adf_ctx_desc_init`/`_clear` return void; inspection with `descs=NULL` writes the occurrence
  count only on `OK`; capacity in initialized descriptors; insufficient capacity is `ADF_LIMIT` with all outputs
  untouched; `size_t` covers binding counts, descriptor capacities and occurrence indices), C3 (3.2, 10.2:
  `adf_modctx_new_from_dump` may return `PARSE` and `LIMIT`, and validates the whole dump before the occurrence
  index), C4 (3.1, 3.2: `NOT_DETERMINED` also covers a result invariant the computation cannot certify; the
  adele-to-idele and inversion row carries it), C5 (3.2, 5.4: conversion between scaled contexts is the best
  enclosure with loss in `int *lost`, word blocks optional; `adf_scaled_set_context`; the R=0 case of the
  conversion from tight). The reference `proto/text_grammar.py`, its tests and the Python reference
  `tests/ref/` gained the matching contracts (point-comparison codes, `modctx_new_from_dump` statuses).
- **0.3 (2026-09-29), gate review applied.** The findings of `docs/reviews/m0-gate/review.md` that concern this
  document and its reference are applied: G1 (scaled arithmetic obeys the context rules; the implicit global
  fallback is limited to `adf_fball` and types containing it), G2 (the life cycle of `adf_modctx`: constructors
  `adf_modctx_new_*` and `adf_modctx_free`, no inline allocation), G3 (the dump loader takes one binding per
  context occurrence), G4 (C value-text round trips are not fixed points), G5 (idele and class arithmetic
  validate the real sign of their result), G6 (a pole gives `DOMAIN` only for an exact pole), G7 (polynomial
  coefficients: length `>= 0`, no exact-zero last coefficient), G8 (`max_items` bounds every context
  occurrence), G10 (the phase width test of 11.3), G12 (the global predicate display), G13 (quotient-piece
  constraints name the canonical triple), G14 (string and classification signatures), G15 (the real-character
  root number is marked `[source pending]`). CV-11, CV-12, CV-29, CV-30, CV-35, CV-37, CV-39 and CV-41 are
  replaced (by G1, G4, G14, G7, G3 and G7, G3, G2); the other 52 decisions are decided (section 13). The
  reference `proto/text_grammar.py`, its tests and golden vectors changed with G3, G4, G7 and G8. G9, G11, G16
  and the `PLAN.md` side of G1 are applied by another lane.
- **0.2 (2026-09-28), part B.** Filled from the reviewed proofs (`docs/proofs/analysis.md`, `policies.md`,
  `ideles.md`, `quotient.md`, `catalogue.md`, `functions.md`), the seams sketch (`docs/seams.md` section 5) and the
  sources on disk (`docs/sources.md`, `refs/src/`). Sections 6.1 to 6.8 written (additive character with the
  conversion to the unconjugated transform of the expositions on disk, measures, finite transform, Gauss sum and
  root number, local gamma factors, class-group characters, reciprocity by formula, the seams recommendations,
  operations fixed by decisions); 5.3 rewritten to agree with `policies.md` P24, P25 and Summary 26 (raw local
  values); 5.4 and the cap from `policies.md` P8 to P15; 5.10 from `quotient.md` (the invariant that replaces
  "inside `[0, 1]`"); section 7 makes the place an opaque handle; the value form labels the archimedean place `inf`;
  the dump header names the field and counts archimedean components; the orchestrator's decisions D1 to D11 are
  written as **DECISION** (section 13 has a column "decided / proposed"); FLINT documentation now on disk settles
  four points that were `[source pending]`. New golden files `psi_phases.tsv` and `gauss.tsv`.
- **0.1 (2026-09-27), part A.** First draft: everything except the signs and normalisations.

How to read it:

- **DECISION CV-nn** (decided by the orchestrator, with the decision number `D1` to `D11`) and **DECISION
  (proposed) CV-nn** (a choice made here, open for the gate review), each with a one-line reason. All are collected
  in section 13, which states for each its status after the gate review: decided (by the orchestrator, or accepted
  by the review) or replaced (with the finding that replaced it).
- Quotations are verbatim from the files on disk except that leading indentation and long runs of spaces from the
  PDF extraction are shortened, and control characters are written `\xNN`; " / " marks a line break.
- FLINT's conventions are cited from the installed headers of FLINT 3.0.1 as `/usr/include/flint/<file>:<line>`
  (version: `/usr/include/flint/flint.h:94-97`) and from its documentation on disk as
  `flint-3.0.1:<file>:<line>` (under `refs/src/`). Other sources are cited as `key:file:line` of `docs/sources.md`,
  and only what was read there is quoted. Behaviour observed by a probe program against `libflint.so.18.0.1` is
  marked **[probed]**; the probes are described in `lanes/m0-conventions/report.md`.
- The reference implementation of the value form and of the dump form is `proto/text_grammar.py`, with its tests in
  `proto/test_text_grammar.py`; the golden vectors are in `tests/golden/` (section 11).

Contents: 1 FLINT conventions we follow; 2 naming and argument order; 3 status codes; 4 aliasing, ownership, outputs
after a status, invalid input, threads, contexts; 5 canonical forms and storage invariants; 6 signs, measures,
normalisations, and operations fixed by decisions; 7 places; 8 text: general rules; 9 value form; 10 dump form;
11 golden vectors; 12 foreign-function interface; 13 table of decisions; 14 findings.

## 1. FLINT conventions we follow

| Convention | FLINT evidence |
|---|---|
| A type `x` has `x_struct`, `x_t` (an array of one struct), `x_ptr`, `x_srcptr` | `/usr/include/flint/arb_types.h:32-41` |
| `init` sets a valid value (zero), `clear` releases memory; every `init` is paired with one `clear` | `fmpz.h:69-70`, `fmpq.h:28-37` (`init` gives `0/1`), `arb.h:46-52` |
| `set(dest, src)`, `swap(a, b)`; swap exchanges contents in O(1) | `fmpz.h:151-152`, `fmpq.h:78-88`, `arb.h:139` |
| Outputs first, then inputs | `fmpz.h:361` (`fmpz_add(f, g, h)`), `arb.h:382` |
| Real working precision is the last argument of each operation, not a field of a context | `arb.h:382`, `arb.h:395` (`arb_add(z, x, y, prec)`, `arb_mul(z, x, y, prec)`) |
| A modulus context is passed explicitly as the last argument; values do not point to it | `fmpz_mod.h:74`, `fmpz_mod.h:109`; context init and clear `fmpz_mod.h:40-45`; `nmod_t` is a plain struct `flint.h:502-508` |
| A p-adic value keeps its own precision `N`; the context holds the prime | `padic.h:32-41`, `padic.h:68-80` |
| A canonical form has a predicate | `fmpq.h:117-118` (`fmpq_is_canonical`), `fmpz_mod.h:55` (`fmpz_mod_is_canonical`) |
| Strings are returned in memory from `flint_malloc` | `fmpz.h:349` (`fmpz_get_str`), `arb.h:168` (`arb_get_str`), `arb.h:1088` (`arb_dump_str`); allocator `flint.h:201-204` |
| Status of generic operations is a small `int`: `GR_SUCCESS 0`, `GR_DOMAIN 1`, `GR_UNABLE 2`, combined by bitwise or | `gr.h:98-101` |
| Header inline functions are also exported symbols (`FMPZ_INLINE` is `static __inline__` except in the one file that defines `FMPZ_INLINES_C`) | `fmpz.h:15-19`; **[probed]** `nm -D libflint.so` lists `fmpz_init`, `fmpq_init`, `arb_init`, `arb_swap` as `T` |

Two points where we deliberately differ:

1. **Values may point to a modulus context** (the local backend of `adf_fball` and the scaled policy, `PLAN.md`
   section 4), whereas FLINT's `fmpz_mod` elements do not. The reason is that a value's backend is part of the value
   (`SPEC.md` 4.1). The consequences are fixed in section 4.6.
2. **Status codes are not bit flags.** `SPEC.md` 10.3 asks for distinguishable statuses; a bitwise or of several
   meanings would not name one. The combination rule is a maximum (section 3.3). DECISION (proposed) CV-01: status
   codes are ordered integers combined by maximum; reason: one code per result, and a cheap, deterministic
   combination.

FLINT's documentation on disk states aliasing function by function (for example `flint-3.0.1:padic.rst:549`,
"Supports aliasing between ``rop`` and ``op``.", and `:492`, "Does not support aliasing between `y` and `z`."), and
says of underscore functions that they "may impose limitations on aliasing between the input and output variables"
(`flint-3.0.1:fmpq.rst:31-32`). A general rule for all modules is not among the files on disk
[source pending: FLINT 3.0.1 documentation, general section on aliasing]. We state our own rule in section 4.1 and
do not rely on FLINT's wording.

## 2. Naming, argument order, life cycle

### 2.1 Names

- Public symbols: `adf_<type>_<verb>` (`PLAN.md` section 2), e.g. `adf_fball_add`, `adf_idele_inv`.
  Types: `adf_<type>_struct`, `adf_<type>_t` (array of one), `adf_<type>_ptr`, `adf_<type>_srcptr`.
- Functions with a leading underscore, `_adf_<type>_<verb>`, are public but low level: they may take raw or
  non-canonical data and may forbid aliasing; each states its preconditions. As in FLINT.
- Functions at named places end in `_at` (`adf_adele_exp_at`); the all-places form has no suffix (`SPEC.md` 9.3.1).
- Macros: `ADF_` prefix, upper case. Status codes `ADF_<NAME>`.
- The verbs of set predicates are exactly `equal_set`, `overlaps`, `contains` (`SPEC.md` 4.2). DECISION (proposed)
  CV-02: there is no `adf_<type>_equal` for ball types; representation identity is `adf_<type>_identical` (same
  backend, context and fields); reason: `equal` would invite reading it as equality of points, which is undecided
  (`SPEC.md` 4.2).
- The comparison of points returns `int` with the fixed values `ADF_CMP_EQUAL = 0` (both exact and equal),
  `ADF_CMP_DIFFERENT = 1` (disjoint), `ADF_CMP_UNDECIDED = 2` (`SPEC.md` 4.2). It is not a status.

### 2.2 Argument order

Outputs; then optional output reports (`adf_place_t * where`, `int * lost`); then inputs; then integer parameters
(degree, limits, number of digits); then `slong prec`; then a context argument, if any. Example:

    int adf_sball_project(adf_sball_t y, adf_place_t * where, const adf_adele_t x,
                          const adf_place_t * places, slong n);
    int adf_sball_add(adf_sball_t z, adf_place_t * where, const adf_sball_t x, const adf_sball_t y, slong prec);

A set of places is an array of `adf_place_t` and a length `n` (`slong`), as in `sball.h`; there is no `adf_places_t`
type.

`prec` is the real working precision in bits, as in `arb` (`arb.h:382`); a `prec` below 2 is taken as 2 (M1-D4). It is an argument of every operation that
computes a real or complex ball, including the parser of the value form (a decimal is read into an `arb` at `prec`).
It is never stored in a context (`PLAN.md` section 4). Operations that involve no real or complex ball take no
`prec`.

### 2.3 Life cycle

For every value type `x` (a modulus context is not a value: it has constructors and `adf_modctx_free`, 4.6):

| Function | Contract |
|---|---|
| `adf_x_init(x)` | sets the init value of section 5 (a valid canonical value). Never fails; types that need a context take a successfully constructed one (4.6): `adf_scaled_init(x, ctx)` |
| `adf_x_clear(x)` | releases all memory owned by `x`. After it `x` must not be used except by `init` |
| `adf_x_set(y, x)` | `y` becomes a copy (same backend, same context pointer). `y` may be `x` |
| `adf_x_swap(x, y)` | exchanges contents; O(1), no allocation; context pointers travel with the values |
| `adf_x_is_canonical(x)` | returns 1 if the storage invariant of section 5 holds, else 0. Never aborts for an initialised object whose integer fields and tags hold any values, provided every pointer field is NULL or points to a live object of its kind (a context made by a constructor, a residue array of `k` words); a pointer to anything else is undefined behaviour, as for every C function (M1-D2, review of milestone 1, finding local R2) |
| `adf_x_identical(x, y)` | representation identity (type, backend, context blocks, all fields) |
| `adf_x_set_str`, `adf_x_get_str` | value form (section 9); `get_str` returns the allocated text and its byte length (8.1) |
| `adf_x_load_str`, `adf_x_dump_str` | dump form (section 10); `dump_str` returns the text and its byte length (8.1) |

The types of milestone S (`adf_resid`, `adf_recon_cert`, `adf_linsol`, `adf_rootlist`) have no text form and
no dump form in version 1; they are read and written through their accessors.

Functions that cannot fail return `void` (as `arb_add`). Functions that can fail return `int`, a status (section
3). Predicates return `int` 0 or 1.

## 3. Status codes

### 3.1 The list

DECISION (proposed) CV-03: the numeric values below, in increasing order of precedence; reason: the combination of
section 3.3 is then the maximum. `PLAN.md` section 4 gives the names; `SPEC.md` 10.3 the required distinctions.

| Code | Value | Exact meaning |
|---|---|---|
| `ADF_OK` | 0 | the output holds the result, which satisfies the function's contract (enclosure, exactness as stated) |
| `ADF_NOT_DETERMINED` | 1 | Valid inputs do not determine the requested quantity, or the computation cannot certify a required result invariant. More input or working precision, or a sharper enclosure algorithm, may resolve it |
| `ADF_UNIT_NOT_CERTIFIED` | 2 | an invertible value was required and the enclosure does not prove invertibility (for example every additive ball of positive radius, `SPEC.md` 4.5) |
| `ADF_NEEDS_SPLIT` | 3 | the true result is a union of several pieces and the function may return one piece only (`SPEC.md` 6); exceeding a piece limit given as argument is `ADF_LIMIT` |
| `ADF_NOT_UNIQUE` | 4 | several candidates satisfy the problem (reconstruction, solving, `SPEC.md` 9.2) |
| `ADF_NO_SOLUTION` | 5 | proved: no value satisfies the problem |
| `ADF_NOT_UNIT` | 6 | proved: the value is not invertible (for example the exact 0) |
| `ADF_DOMAIN` | 7 | proved: every point of the input lies outside the domain of the function (a ball that meets both the domain and its complement gives `ADF_NOT_DETERMINED`); or a constructor, parser or loader received data that violates the type's invariant (zero denominator, composite prime, non-finite real ball); or a stated context-compatibility requirement of the function fails (for example the two operands of a default `adf_scaled` operation have different context pointers, 5.4, gate finding G1). Reported with the place, where there is one |
| `ADF_UNSUPPORTED` | 8 | valid request that version 1 does not implement (a prime or modulus above one word where a word is required, a dump of an unknown version, a complex branch that is not specified) |
| `ADF_PARSE` | 9 | the text is not a sentence of the grammar of section 9 or 10 |
| `ADF_LIMIT` | 10 | a resource limit was reached: a limit of section 8.4, a piece limit given as argument, a size bound of an algorithm |

`ADF_NOT_DETERMINED` lacks the requested result certificate; `ADF_UNIT_NOT_CERTIFIED` lacks an input unit
certificate. `ADF_NO_SOLUTION`, `ADF_NOT_UNIT` and `ADF_DOMAIN` report proved failures. A function never returns a "proved" code on the basis of an enclosure that merely fails
to prove the opposite.

### 3.2 Which functions may return which

| Class of function | Possible statuses |
|---|---|
| Ring arithmetic of `adf_rat`, `adf_fball`, `adf_adele`, `adf_cadele` (add, sub, neg, mul, scale by exact rational), and `adf_qclass_add_rat` (translation by a rational, the identity on every class) | none: `void` |
| Ring arithmetic of `adf_scaled` (add, sub, neg, mul, `adf_scaled_mul_tight`, scale by exact rational) | `OK`, `DOMAIN` (the context-compatibility rule of 4.6 and 5.4, gate finding G1); the value output is untouched on `DOMAIN` |
| Set predicates (`equal_set`, `overlaps`, `contains`) | no status: they return `int` 0 or 1 (2.3; the other predicates likewise); exception: the three quotient set queries `adf_qclass_equal_set`, `adf_qclass_contains`, `adf_qclass_overlaps` return `OK` with the truth value written, or `LIMIT` with `truth` untouched (N-D21); the point comparisons keep their `CMP` codes |
| Division of `adf_rat` by an `adf_rat`; scaling by the inverse of an exact rational | `OK`, `NOT_UNIT` (divisor exactly 0) |
| Constructors from raw data (`_set_fmpz3`, `_set_arb_fball`, `adf_ucoset_set_fmpz2`, ...) | `OK`, `DOMAIN` |
| Raw context constructors (5.14) | `OK`, `DOMAIN`, `UNSUPPORTED` (a block above one word or more than 65536 blocks) |
| `adf_modctx_new_from_dump` | `OK`, `PARSE`, `LIMIT`, `UNSUPPORTED`, `DOMAIN` |
| Parsers of the value form (`_set_str`) | `OK`, `PARSE`, `LIMIT`, `UNSUPPORTED`, `DOMAIN`, `NOT_DETERMINED` (only the sign conditions of section 9.3 at the requested `prec`) |
| Loaders of the dump form (`_load_str`) | `OK`, `PARSE`, `LIMIT`, `UNSUPPORTED`, `DOMAIN` |
| Exact conversion to the local backend | `OK`, `DOMAIN` (unrepresentable), `UNSUPPORTED` (no word blocks) |
| Conversion between scaled contexts | `OK`; best enclosure; loss reported through `int *lost` |
| Conversion from tight to scaled | `OK` always; loss is reported in `int * lost` (`SPEC.md` 4.4: "says so") |
| Adele to idele; inversion of an adele-like value | `OK`, `UNIT_NOT_CERTIFIED`, `NOT_UNIT`, `NOT_DETERMINED` |
| Unit coset, idele, idele class arithmetic | `OK`, `NOT_DETERMINED` (the required real sign is not certified on the result, 5.7, gate finding G5), `LIMIT` (a `prec` above `ADF_IDELE_PREC_MAX`, decided from `prec` alone before every other status; for `adf_idele_pow` also a content power above `ADF_IDELE_POW_BITS_MAX` bits); `NOT_UNIT` only for an exact zero input where one is accepted (`adf_idele_set_rat`, `adf_idele_mul_rat` with `q = 0`); `DOMAIN` (`adf_idele_set_parts`, `adf_idclass_set_parts`; a valuation or absolute value at the archimedean place) (`idele.h`, `idclass.h`, `idpow.h`, `idmap.h`) |
| `adf_lball`: constructors and accessors (`set_rat`, `set_rat_ball`, `set_fball`, `get_prec`, `get_center`, `teichmuller`) | `OK`, `DOMAIN` (the archimedean place; an exact value has no precision; `p` divides the residue of `teichmuller`), `LIMIT` (an exponent above `ADF_LBALL_EXP_MAX`, or a power `p^k` above `ADF_LBALL_BITS_MAX` bits); the outputs are untouched on a status (`lball.h`) |
| `adf_lball` arithmetic (`neg`, `add`, `sub`, `mul`, `inv`, `div`) | `OK`, `DOMAIN` (two different primes; checked first), `LIMIT` (an input or the result outside the limits of `lball.h`), and for `inv`, `div`: `NOT_UNIT` (the exact 0), `UNIT_NOT_CERTIFIED` (a ball that contains 0) |
| `adf_lball` valuation, absolute value, decomposition (`valuation`, `abs`, `decompose`, `decompose_teich`, `frac`, `unit_mod`) | `OK`, `NOT_DETERMINED` (a ball that contains 0; `frac`: a ball with `N < 0`; `unit_mod`: `k` above the relative precision), `DOMAIN` (the exact 0 for `decompose`, `decompose_teich`, `unit_mod`; `k < 0`), `LIMIT` |
| `adf_lball_pow_si` | `OK`, `LIMIT`, and for `k < 0`: `NOT_UNIT` (the exact 0), `UNIT_NOT_CERTIFIED` (a ball that contains 0); `k = 0` gives the exact 1 for every `x` (`lball.h`) |
| `adf_sball`: constructors, projection, and accessors (`set_arb_lballs`, `project`, `get_place`, `get_lball`, `get_arb`) | `OK`, `DOMAIN` (a non-finite real ball, a non-canonical component, a repeated place, `n < 0`, an index or place that the value does not have), `LIMIT` (`project`: the projection to a prime, as `adf_lball_set_fball`); `where` names the place on a status other than `OK` (`sball.h`) |
| `adf_sball` operations (`neg`, `add`, `sub`, `mul`) | `OK`, `DOMAIN` (different sets of places or tags, checked first), `UNSUPPORTED` (a complex archimedean component), `LIMIT` (`prec` above `ADF_REAL_PREC_MAX`, decided first, `where` the archimedean place; an `adf_lball` limit at a prime); the combined status is the maximum of 3.3 and `where` the first place with it |
| Series at a prime (`adf_lball_exp`, `adf_lball_log`, `adf_lball_Log`, `lfunc.h`) | `OK`, `DOMAIN` (the input does not meet the domain; the exact 0 under `Log`), `NOT_DETERMINED` (the input meets the domain and its complement; a ball that contains 0 under `Log`), `LIMIT` (an exponent above `ADF_LBALL_EXP_MAX`, or a working modulus `p^W` above `ADF_LBALL_BITS_MAX` bits) |
| Real functions on `arb` (`adf_real_exp`, `_log`, `_log_abs`, `_sin`, `_cos`, `_sqrt`, `_root`, `rfunc.h`) | `OK`, `DOMAIN` (every point outside the domain; a non-finite input ball; the root of degree 0), `NOT_DETERMINED` (a ball that meets the domain and its complement; a result that `arb` returns non-finite, CV-08), `LIMIT` (`prec` above `ADF_REAL_PREC_MAX`, decided first) |
| Functions of `adf_sball` at one place (`adf_sball_exp_at`, `_log_at`, `_Log_at`, `_log_abs_at`, `_sin_at`, `_cos_at`, `_sqrt_at`, `_root_at`, with `where`) | as the two rows above, at the archimedean place (`prec` above the limit first) and at a prime through `lfunc.h` (where `prec` is an absolute `p`-adic precision, and no limit on it applies except those of `lfunc.h`); further `DOMAIN` (`v` is not a place of `x`; a root of degree 0), `UNSUPPORTED` (a complex tag at the archimedean place; `log_abs`, `sin`, `cos`, `sqrt`, `root` at a prime); `where` = the place |
| Functions at places (`_at`) and all-places functions (`SPEC.md` 9.3) | `OK`, `DOMAIN` (with place), `NOT_DETERMINED`, `NEEDS_SPLIT`, `UNSUPPORTED`, `LIMIT` |
| Quotient by `Q` | `OK`, `NEEDS_SPLIT`, `LIMIT`; the class character calls `adf_qclass_psi_tate`, `adf_qclass_psi_tate_strict` and `adf_qclass_psi_tate_phase` belong to the row below, not to this one, because they return `NOT_DETERMINED` (N-D21) |
| Characters, Gauss sums, local factors | `OK`, `NOT_DETERMINED` (also a mixed or undecided ball meeting a pole), `DOMAIN` (for an exact pole), `UNSUPPORTED`, `LIMIT` (`prec` above `ADF_REAL_PREC_MAX`, decided first; a stated bound of the algorithm, such as the 64 factors of the Gamma recurrence of `adf_local_zeta_factor_at`: `docs/design/local-zeta.md` Z4, N-D20; raw character constructors `adf_char_set_conrey`, `adf_char_set_conrey_acb`, `adf_char_set_str` and the character loaders: `LIMIT` for `q > ADF_CHAR_MOD_MAX = 65536` before FLINT group setup) |
| Reconstruction and solvers | `OK`, `NO_SOLUTION`, `NOT_UNIQUE`, `NOT_DETERMINED` ("uniqueness not certified", `SPEC.md` 9.2; a list of roots that is not proved complete), `LIMIT`, `DOMAIN` (a modulus below 1, the zero polynomial, a place of the wrong kind, a shape that does not fit), `UNSUPPORTED` (an exact right-hand side of a system, a prime above the temporary bound of a slice that finds the roots modulo `p` by evaluation at every residue, decision S-D10) |
| Integrals, Poisson summation | `OK`, `DOMAIN` (outside the stated half-plane, or at an exact pole), `NOT_DETERMINED` (also a mixed or undecided ball meeting a pole), `LIMIT`; the algebra of `adf_ffun` (`ffun.h`, `docs/api-4.md` 3 and 4, D1; `LIMIT` for the D1 caps) and the algebra and the real evaluation of `adf_rfun` (`rfun.h`, `docs/api-4.md` 5 and 6, D1) belong here: `DOMAIN` (dilation by 0; a non-finite real point), `NOT_DETERMINED` (`Re(A) > 0` of a result term not certified after rounding; a non-finite value), `LIMIT` (`prec` above `ADF_REAL_PREC_MAX`, decided first; the term, coefficient, work and bit caps of D1) |

In conversion or inversion, NOT_DETERMINED covers failure to certify the required real sign on the result;
UNIT_NOT_CERTIFIED and NOT_UNIT concern the input, as before. Failure leaves the value output untouched.

A function documents its own subset; it may not return a code outside its class's row.

### 3.3 Combining statuses

Over several places (an all-places function, `f_at` over a set `S`, a family such as the Hilbert symbols of
`SPEC.md` 9.3.7), and over the steps of a composite function:

1. The combined status is the **maximum** of the statuses, in the numeric order of section 3.1.
2. The reported place is the first place, in the canonical order of places (section 7), whose status equals the
   combined status.
3. The result is `ADF_OK` exactly when every part is `ADF_OK`.
4. A failure of the finite part as a whole that names no prime (the all-places root of an exact rational without a
   rational root: the failing prime is not computed, `SPEC.md` 15.4 N-D16) leaves the report `where` untouched; rule
   2 applies to the places that the function examines. Counterexample to "every `DOMAIN` of an all-places function
   carries a place": `adf_rat_root` of 2 with `n = 2` (finding 1 of lane `f-slice10`).

DECISION (proposed) CV-04: precedence by maximum, ties by canonical place order; reason: proved failures dominate
failures that more precision may repair, so a caller does not raise the precision in vain, and the reported place
does not depend on the order of evaluation.

The optional per-place variant (`SPEC.md` 9.3.1) returns one status per place in canonical order and the combined
status by the same rule.

## 4. Aliasing, ownership, state after a status, invalid input, threads, contexts

### 4.1 Aliasing

1. **Outputs may alias inputs of the same type**, for every public function without a leading underscore.
   `adf_fball_add(x, x, x)` is valid. DECISION (proposed) CV-05; reason: FLINT users expect it, and it is cheap for
   the global backend.
2. **Inputs may alias each other** freely (inputs are `const`).
3. **Two outputs of one function must not alias each other**, nor may an output alias a part of an input
   (`&x->fin` as output while `x` is an input). Undefined behaviour otherwise.
4. An output of one type never aliases an input of another type, except where a type contains the other and the
   function says so.
5. Underscore functions state their aliasing rules; by default they forbid aliasing of outputs with inputs.
6. A context argument is never an output and may be shared by any number of arguments.

Tests: every binary operation is tested with `(x, x, y)`, `(y, x, y)` and `(x, x, x)` (`PLAN.md` section 7,
"aliasing of arguments").

### 4.2 Ownership

- A value owns all memory it points to (its `fmpz` limbs, its `arb` mantissas, the residue array of a local
  `adf_fball`, the arrays of `adf_sball`, `adf_qclass`, `adf_ffun`, `adf_rfun`), except the modulus context.
- A value **borrows** its modulus context (`const adf_modctx_struct *`); it never frees or changes it.
- Arrays passed to a constructor (block lists, coefficient lists) are copied; the caller keeps ownership.
- Strings passed in are read during the call only; never retained.
- Strings returned (`_get_str`, `_dump_str`) are allocated with `flint_malloc` and carry their byte length (8.1,
  gate finding G14); the caller frees them with `adf_str_free(char *)`, which calls `flint_free`
  (`flint.h:201-204`), as for `arb_dump_str` (`arb.h:1088`) and `fmpz_get_str` (`fmpz.h:349`).
- Scratch space is allocated inside a call and released before it returns. There is no caller-visible scratch
  argument in version 1.

### 4.3 State of the outputs after each status

DECISION (proposed) CV-06: **on every status other than `ADF_OK` every output is left untouched** (bitwise the
same as before the call), with the single class of exceptions below; reason: a caller can retry at a higher
precision without re-creating aliased inputs, and the rule is the same for every function.

| Status | Output |
|---|---|
| `ADF_OK` | the result |
| any other | untouched. Optional report arguments (`where`, per-place status arrays) are written |

The argument `sol` of `adf_linsolve_mod` and `adf_linsolve_fball` is a report argument: it is written on
`ADF_OK` (the coset of solutions) and on `ADF_NO_SOLUTION` (the vector that proves it), and untouched on every
other status.

Exception: a function documented as **"enclosure with status"** writes a valid enclosure and returns
`ADF_NOT_DETERMINED`. The only such functions in version 1 are the complex archimedean wrappers when the input
ball meets a branch cut (`SPEC.md` 9.3.6: "returns the enclosure `acb` gives, with a status"). Where `SPEC.md` says
"an enclosure of all possible values, or a status" (characters on a coarse coset, `SPEC.md` 5; the additive
character on a fractional radius, `SPEC.md` 6; the profinite power, 9.3.7; the Kronecker symbol, 9.3.7), there are
two functions: the default returns `ADF_OK` with the enclosure, the `_strict` variant returns `ADF_NOT_DETERMINED`
and leaves the output untouched. DECISION (proposed) CV-07; reason: a status then never carries a value except in
the one case the specification demands it.

Implementation note (not a contract): the rule costs nothing for functions that cannot fail (`void`), and for
functions that fail before writing. Others compute into temporaries and `swap` at the end. A context constructor
is not an `init`: it writes `*out` only on success, and no initialised-but-failed context state exists (gate
finding G2, 4.6).

### 4.4 Invalid input

| Input | Behaviour |
|---|---|
| Non-finite real or complex ball (`arb_is_finite` false, `arb.h:134`) given to a constructor | `ADF_DOMAIN`, output untouched |
| Non-finite ball produced inside a computation from finite inputs | never stored; the function returns `ADF_NOT_DETERMINED` (or `ADF_DOMAIN` at a proved exact pole of a local factor or integral; a ball meeting a pole and also regular points gives `ADF_NOT_DETERMINED`, 6.4). DECISION (proposed) CV-08; reason: the invariants of section 5 require finite balls |
| Zero denominator, negative radius, modulus `<= 0` where `>= 1` is required, `gcd(c, N) != 1` for a unit coset, composite `p` | `ADF_DOMAIN` from constructors, parsers and loaders |
| Non-canonical value passed to a public function | precondition violation: behaviour undefined. With `-DADF_CHECK_INVARIANTS` every public function checks `adf_x_is_canonical` on entry and calls `flint_abort` with a message. DECISION (proposed) CV-09; reason: checking a gcd on every add would dominate the cost that `PERF.md` measures |
| Aliasing forbidden by 4.1 | undefined behaviour |
| Text input | never undefined: every byte string of every length gives a status (sections 8-10); fuzzed |

A value that came out of any public function, parser or loader with `ADF_OK`, or out of `init`, is canonical.

A status that a function returns for an input outside its contract (for example `ADF_DOMAIN` of
`adf_adele_reconstruct` for an infinite real ball, which exists so that FLINT does not abort) is a courtesy of the
release build and no promise (decision M1-D11).

### 4.5 Threads

- No global mutable state in the library (`SPEC.md` 10.1). Functions are reentrant.
- A value may be read concurrently by several threads; it must not be written while another thread reads or
  writes it.
- A context is immutable after construction (4.6) and may be read concurrently by any number of threads.
- FLINT keeps thread-local caches of constants (`ARB_DEF_CACHED_CONSTANT`, `arb.h:621-642`, declared with
  `FLINT_TLS_PREFIX`, `flint.h:156-160`) and registers cleanup functions (`arb.h:634`). These are FLINT's, per
  thread; they are not state of ours. A thread that used real functions should call FLINT's cleanup before it
  exits [source pending: FLINT documentation of `flint_cleanup`].
- The library never calls `flint_set_num_threads` and starts no threads.

### 4.6 Contexts: lifetime and construction

DECISION (proposed) CV-10: **modulus contexts are caller-owned with a stated lifetime, not reference counted.**

Construction and ownership (gate finding G2): an `adf_modctx_struct` is incomplete in the public header. The
library exports explicit context constructors named `adf_modctx_new_*`, taking `adf_modctx_struct **out` as their
first argument, and `adf_modctx_free(adf_modctx_struct *ctx)`. A successful constructor allocates and fully
initializes an immutable context and writes its pointer to `*out`. On failure `*out` is untouched and no
allocation is retained. The caller owns the returned context and frees it only after its borrowers are gone.
Passing NULL to free does nothing. A constructor never frees, mutates or reuses the context previously pointed
to by `*out`. On success it overwrites only the pointer slot; the caller must retain any previous owned pointer
separately.
No public by-value or array-of-one context type requires the layout of the incomplete struct.
Value init functions remain non-failing and require successfully constructed contexts where stated.
Version 1 offers no inline allocation of a context: a size query without an alignment query and without an
initialization ABI is not offered as a choice (the review allows inline allocation only if size, alignment,
signatures and the successful and failed states are all specified).

Lifetime contract: the caller constructs a context with `adf_modctx_new_*` (5.14), may use it for any number of
values, and calls `adf_modctx_free` only after every value that refers to it has been cleared or moved to another
context or backend. Freeing a context that a live value refers to is undefined behaviour. With
`-DADF_CHECK_INVARIANTS` a context counts the values that refer to it (atomically) and `adf_modctx_free` aborts if
the count is not zero.

Reasons:

1. FLINT's own contexts are caller-owned and have no count: `fmpz_mod_ctx_init`/`clear` (`fmpz_mod.h:40-45`),
   `padic_ctx_init`/`clear` (`padic.h:93-96`).
2. A count is mutable state inside an object that `SPEC.md` 10.1 calls immutable; it would have to be atomic, and
   every `init`, `set` and `clear` of a local value (the batch kernels of `PERF.md`) would write a shared cache
   line.
3. No operation needs to create a context: see the next rule.

Consequence: **no operation creates a context implicitly** (gate findings G1 and G2). Where `SPEC.md` 4.1 allows
a result that leaves its context to "fall back to the global backend or be given a new context", the implicit
global fallback applies only to `adf_fball` and types containing it. A default binary operation on `adf_scaled`
requires the same context pointer in both inputs; otherwise it returns `ADF_DOMAIN` with its value output
untouched. This check precedes writes, including aliased writes. The result borrows that input context. Exact
operands follow the same context rule. To combine different contexts, the caller constructs a context with
modulus `lcm(K, K')`, converts both operands to it without loss, and then calls the ordinary operation. No
operation creates this context. A separately named target-context operation may take an explicit caller-owned
context; its conversion loss and output context must be documented. DECISION CV-11 and CV-12 are **replaced** by
this rule (gate finding G1): their earlier wording sent such a result global, which `adf_scaled` cannot store
(its context is never NULL), and referred to contexts that no operation may create.

The shared-pointer check also applies to `adf_scaled_mul_tight`, including exact operands. It compares
the two scaled inputs, not the old context of the initialized output. Unary and exact-scalar operations
borrow the scaled input's context; an `adf_rat` scalar has no context to compare.

Contexts stay immutable after construction. A value that refers to a context gets and changes its context field
through functions of the library only; a hand write of the field, a bitwise copy of the value or a struct
assignment is outside this contract (decision M1-D10). Matching moduli and ordered blocks permit explicit rebinding;
they do not waive pointer-compatibility checks or the different-pointer global fallback below. For `adf_fball` and
types containing it, operations on two local values with different context pointers give a global result, even if
the blocks agree (the fallback above); reason: comparing block lists on every operation costs more than the
conversion it saves.

## 5. Canonical forms and storage invariants

For each type: the struct (a proposal to be frozen with the header), the invariant as a checkable predicate (this is
what `adf_x_is_canonical` returns), the init value, who establishes the invariant and who may assume it, the exact
zero, radius zero, signs, and which representative is stored.

General rule for every type: **every public function that returns `ADF_OK` leaves each output satisfying its
predicate; every public function may assume that each input satisfies its predicate** (section 4.4). The
predicates are established from raw data only by: `init`, the constructors `adf_x_set_*` (which validate and
return `ADF_DOMAIN`), the parsers and loaders, and `adf_x_canonicalise` after an underscore function.

`gcd` of rationals is as in `proofs/precision.md` (lines 9-11). `Zhat`, `Z_p`, `U(N)` are as in `SPEC.md`.

### 5.1 `adf_rat` (exact rational)

    typedef struct { fmpq_t q; } adf_rat_struct;

- Predicate: `fmpq_is_canonical(q)` (`fmpq.h:117-118`): denominator `> 0`, `gcd(num, den) = 1`.
- Init: 0, stored `0/1` (`fmpq.h:28-32`). Exact zero: `0/1`. The sign is on the numerator.
- Meaning: the rational `q` at every place at once; exact (`SPEC.md` 4.1).

### 5.2 `adf_fball` (finite ball), global backend

    typedef struct {
        fmpz_t A, H, d;
        int backend;                     /* ADF_GLOBAL = 0, ADF_LOCAL = 1 */
        const adf_modctx_struct * mctx;  /* ADF_GLOBAL: NULL */
        ulong * res;                     /* ADF_GLOBAL: NULL */
    } adf_fball_struct;

Meaning: the set `(A + H Zhat)/d`, centre `a = A/d`, radius `N = H/d` (`SPEC.md` 4.1).

Predicate `G(A, H, d)` (`SPEC.md` 4.1, D2), for `backend == ADF_GLOBAL`:

    mctx == NULL and res == NULL and d > 0 and H >= 0 and
    ((H > 0 and 0 <= A < H and gcd(A, H, d) = 1) or (H = 0 and gcd(A, d) = 1))

- Init: the exact 0, `(A, H, d) = (0, 0, 1)`. DECISION (proposed) CV-13; reason: as `fmpq_init` and `arb_init`, and
  `0 mod 1` would be a guess.
- Exact zero: `(0, 0, 1)`. Radius zero: `H = 0`, a single rational `A/d` in lowest terms.
- Signs: `d > 0`, `H >= 0`. For `H > 0` the stored centre is non-negative; for `H = 0` the sign is on `A`.
- Stored representative: for `H > 0`, the centre `a = A/d` is the unique element of `(a + N Z)` in `[0, N)`.
- The radius `N = H/d` is documented as the positive generator of the radius ideal `N Z`; radii are compared by
  inclusion (`contains`), never by size, and the precision is read per place, `v_p(N)` through a place handle
  (seams R2, adopted by D8). No public function needs a scalar radius beyond the constructors and getters of `Q`.

**Lemma 5.2 (uniqueness).** Two triples satisfying `G` that describe the same set are equal.

*Proof.* Radius zero: the set is one rational, and lowest terms with `d > 0` are unique. Radius positive: the set
determines `N` (`SPEC.md` 4.2, `equal_set` needs `N = M`), and its rational points are `a + N Z` (Lemma 1 of
`proofs/precision.md`). A triple `(A, H, d)` for the set needs `d a'` integral for some rational point `a'` and
`d N = H` integral; since `d (a' + k N) = d a' + k H`, the first condition does not depend on the choice of `a'`.
So the admissible `d` are the positive multiples of `d0 = lcm(den(a'), den(N))`. For `d = k d0` the triple is
`k (A0 + j H0, H0, d0)` for some integer `j`, whose gcd is at least `k`; so `gcd = 1` forces `k = 1`. At `k = 1` the
gcd is 1: a prime `l` dividing `A0`, `H0` and `d0` would make `(d0/l) a'` and `(d0/l) N` integers, against the
minimality of `d0`. With `H` and `d` fixed, `A` runs through `d a' + H Z`, and `0 <= A < H` picks one element.

### 5.3 `adf_fball`, local backend

Source: `docs/proofs/policies.md`, Definition 16 to Summary 26 (reviewed and repaired; P24 and P25 were the
repaired statements).

Meaning: the value `(d; r_1, ..., r_k)` in a context with pairwise coprime blocks `q_1, ..., q_k` of product `K`
is the set `(A + K Zhat)/d`, where `A` is any integer with `A = r_i mod q_i` for all `i` (policies Definition 16,
Lemma 17). The numerator `A` is not stored (`SPEC.md` 4.1).

Predicate `L(x)`, for `backend == ADF_LOCAL`, with `k = mctx->nblocks`:

    mctx != NULL and k >= 1 and H = K = q_1 ... q_k and A = 0 and res != NULL and d >= 1 and
    0 <= res[i] < q_i for all i

- **Raw local values; no gcd condition.** The predicate does not ask `gcd(A*, K, d) = 1` for the lift
  `0 <= A* < K`. In a fixed context distinct data give distinct sets (policies Lemma 17.2), so the local data are
  unique for their set without that condition. The canonical triple of `SPEC.md` 4.1, `(A*/g, K/g, d/g)` with
  `g = gcd(A*, K, d)`, is derived when needed; `g` is computed blockwise, `g = gcd(product of gcd(r_i, q_i), d)`
  (policies Lemma 18), without recombination. DECISION (proposed) CV-55; reason: policies P24 shows that
  cancellation keeps the *set* in the context while the canonical *triple* leaves it, and Summary 26 that the
  canonical modulus can change after a sum; a canonical-data invariant would force a conversion after most local
  sums, which is the operation the local backend exists to make cheap. (Version 0.1 had the gcd condition.)
- **Comparison and printing go through the canonical triple, never through raw residues** (policies P24.4).
  `adf_fball_equal_set` of a local and a global value compares sets; `adf_fball_identical` compares raw data.
  The value form prints the canonical triple (it does not show the backend); the dump writes the raw data.
- `A` is kept 0 so that `clear` and `swap` need no case distinction (CV-22). `H` is stored and equals `K`.
- **Which balls live in a context** (policies P19): `c + R Zhat`, `R > 0`, is a local value of a context of product
  `K` exactly when `K/R` and `c K/R` are integers; then `d = K/R`, `r_i = (c K/R) mod q_i`. Exact values (`R = 0`)
  are never local (policies Definition 16).
- **Conversion into a context** `adf_fball_set_local(y, x, ctx)`: `ADF_DOMAIN` unless P19 holds (the set would
  change; `PLAN.md` section 7); the best enclosure of policies P20 is a separately named operation,
  `adf_fball_set_local_enclose(y, lost, x, ctx)`, which reports loss. Conversion to global is recombination by CRT
  followed by canonical cancellation (policies Lemma 17.3, P24.1).
- **Operations at one context pointer** (policies P21 to P23, Summary 26), in the tight policy:

  | Operation | Result |
  |---|---|
  | negation | local, `(d; -r_i mod q_i)`, exact (P21.1) |
  | sum | local, `(L; (r_i L/d + s_i L/e) mod q_i)`, `L = lcm(d, e)`, exact and tight (P21.2) |
  | product | with `h = product of gcd(r_i, s_i, q_i)`: if `h = 1`, the blockwise product `(d e; r_i s_i mod q_i)` is tight and local; if `h > 1` and `h` divides `d e`, local `(d e/h; ...)` by the formula of P22.3; otherwise the tight product needs the derived blocks `q_i h_i` (P22.4) and is returned global (the fallback of 4.6, gate finding G1), or in a derived context passed by the caller. The blockwise product with `h > 1` is only an enclosure (P22.2) and is not the tight product |
  | exact scalar `m/n` | local `(n d/abs(m); sign(m) r_i mod q_i)` if `abs(m)` divides `d` (P23); otherwise global; `0` gives the exact 0 |

- **Inverses modulo blocks** (policies P25 and its rules for the C code): a modular inverse is computed only of an
  integer coprime to the modulus (`h/h_i` modulo `q_i`, `g/g_i` modulo `q_i/g_i`, and `d` modulo `q_i` only when
  `gcd(d, q_i) = 1`). The denominator is applied after recombination and never inverted modulo a block it shares a
  factor with (`SPEC.md` 4.1). A residue of the ball modulo `q_i` exists and may be reported only when
  `gcd(d, q_i) = 1` (P25.2); a solution of `d x = A mod q_i` is never reported as a residue of the value.
- Results that leave the context (a changed `K`, or two different context pointers) are global: the implicit
  fallback of 4.6 applies to `adf_fball` and types containing it (gate finding G1).

### 5.4 Scaled value (policy 2 of `SPEC.md` 4.4)

    typedef struct {
        fmpq_t s;                        /* exact = 0: s > 0.  exact = 1: the exact value, any sign */
        fmpz_t u;                        /* exact = 0: 0 <= u < K.  exact = 1: u = 0 */
        const adf_modctx_struct * mctx;  /* never NULL; K is the context's modulus */
        int exact;
    } adf_scaled_struct;

- Predicate: `mctx != NULL`, `exact` in `{0, 1}`, `fmpq_is_canonical(s)`; if `exact = 0`: `s > 0` and
  `0 <= u < K`; if `exact = 1`: `u = 0`.
- Meaning: `exact = 0`: the set `s (u + K Zhat) = s u + s K Zhat`; `exact = 1`: the rational `s`.
- The field `exact` is added to `PLAN.md` section 4. DECISION (proposed) CV-14; reason: Proposition 6(2) of
  `proofs/precision.md` produces the exact 0, and `s (u + K Zhat)` cannot hold it.
- Uniqueness: for `exact = 0` the set determines its radius `s K`, hence `s`, and then `u` modulo `K`; the stored
  `(s, u)` is unique without a normalisation step.
- Init: `adf_scaled_init(x, ctx)` gives the exact 0 in `ctx`.
- The scale is not promised to be canonical beyond `Q`: the tightness of the scaled sum rests on the gcd of two
  scales being a scale, which holds for `Q` (and `F_q[T]`) but not for number fields; no function returns "the
  canonical scale" of a result as part of the contract (seams R7, adopted by D8).

Operations (source: `docs/proofs/policies.md` section 2; `s, t > 0`, context `K`; a default binary operation
requires the same context pointer in both inputs, 4.6):

| Operation | Result | Source |
|---|---|---|
| sum | `g ((A u + B v) mod K + K Zhat)`, `g = gcd(s, t)`, `A = s/g`, `B = t/g`; tight | `SPEC.md` 4.4; precision P5(1) |
| product, default | `s t ((u v mod K) + K Zhat)`; an enclosure that loses the factor `h = gcd(u, v, K)` | `SPEC.md` 4.4; policies P10.2 |
| product, tight | `adf_scaled_mul_tight`: `(s t h) (((u v / h) mod K) + K Zhat)`, equal to the tight product | policies P10.3 |
| exact scalar `q` added | `q = 0`: unchanged; else `g = gcd(s, q)`, `g ((q/g + (s/g) u) mod K + K Zhat)`, the best scaled enclosure; `lost` set when `s` does not divide `q` | policies P8 |
| exact scalar `q` multiplied | `q != 0`: `abs(q) s ((sign(q) u mod K) + K Zhat)`, exact; `q = 0`: the exact 0 | policies P9 |
| two exact values | the exact result, `exact = 1` (the tag is kept) | policies Definition 4 |
| conversion from a tight ball `c + R Zhat` | `s* = gcd(c, R/K)`, `u* = (c/s*) mod K`; best enclosure; `lost` when `c K/R` is not an integer. For R=0, store exact=1, s=c, u=0 in the target context, with lost=0. | policies P7, L6 |
| conversion to context `K'` | `s' = s gcd(u, K/K')`, `u' = (s u/s') mod K'`; best; exact when `u K'/K` is an integer, always when `K` divides `K'` | policies P11 |
| two contexts `K`, `K'` | no default operation combines them; the caller rule of 4.6 (gate finding G1) applies | policies C12 |

```c
int adf_scaled_set_context(adf_scaled_t y, int *lost, const adf_scaled_t x, const adf_modctx_struct *ctx);
```

With initialized y and a successfully constructed ctx, this returns ADF_OK and stores the best enclosure
in ctx. If lost is non-NULL, write *lost=1 exactly when the set changes. Word blocks are optional.
Exact input keeps its rational s, exact=1 and u=0, with *lost=0 if requested. y may alias x; it borrows ctx.

DECISION CV-48 (D5): the product of `SPEC.md` 4.4 is the default `adf_scaled_mul`; the tight variant of policies
P10 is the separately named `adf_scaled_mul_tight`; reason (orchestrator): the specification's rule stays, and the
tight rule needs a gcd and an exact division that the default kernel avoids.

To combine two different context pointers, the caller constructs the context `lcm(K, K')` (5.14), converts both
operands with the row "conversion to context `K'`" (lossless, since `K` divides `lcm(K, K')`), and then calls the
ordinary operation. No operation creates this context (gate finding G1).

The absolute cap (policy 3) stores plain `adf_fball` values; the cap `C`, a positive rational, is an argument of the
capped operations, not a field (CV-23, proposed; reason: `SPEC.md` 4.4 says "the radius stays with each value").
After a tight operation with result radius `R > 0` the radius becomes `gcd(R, C)` (policies Definition 13, P14.1).
**An exact result keeps its exact tag; the cap never touches it.** DECISION CV-47 (D2); reason: `SPEC.md` 4.1
requires that an exact rational "stays exact under arithmetic with other exact values", and policies P14.2 shows
the literal `gcd(0, C) = C` would break it. The cap does act on every result of positive radius, including the
product of an exact scalar with a ball (P14.2). Invariant of capped values: `R` divides `C` (P15.1); the sum of two
capped values needs no cap (P15.3).

### 5.5 `adf_adele` and `adf_cadele`

    typedef struct { arb_t inf; adf_fball_struct fin; } adf_adele_struct;
    typedef struct { acb_t inf; adf_fball_struct fin; } adf_cadele_struct;

- Predicate: `arb_is_finite(inf)` (`arb.h:134`), respectively `acb_is_finite(inf)` (`acb.h:801`), and `fin`
  satisfies `G` or `L`.
- Init: `(0 ; 0)`, both coordinates exact 0.
- The two coordinates are independent; an adele with both coordinates exact is still an `adf_adele`, not an
  `adf_rat` (a real ball has a dyadic midpoint, `SPEC.md` 4.1).
- Radius zero at infinity: `arb_is_exact(inf)` (`arb.h:61`).
- The real ball is never normalised beyond what `arb` does; its midpoint and radius are whatever the operation
  produced.
- Archimedean operations are reached through the place (`f_at(..., place)`, `SPEC.md` 9.3.1); the field `inf` is a
  convenience special to `Q` (seams R3). `adf_cadele` is special to `Q`: its shape coincides with the adele ring of
  an imaginary quadratic field, and it is not that ring (seams R4; `SPEC.md` 4.1).

### 5.6 `adf_ucoset` (unit coset)

    typedef struct { fmpz_t c, N; } adf_ucoset_struct;

Meaning: `c U(N)`, the units of `Zhat` congruent to `c` modulo `N` (`SPEC.md` 5).

Predicate:

    ( N >= 1 and 1 <= c <= N and gcd(c, N) = 1 )  or  ( N = 0 and c in {1, -1} )

- **Exact units.** `N = 0` is admitted, with `U(0) = {1}` ("congruent modulo 0" is equality), so `[1 mod 0]` is the
  exact unit 1 and `[-1 mod 0]` the exact unit -1. DECISION CV-15 (D1); reason: three lanes needed it independently:
  `proofs/catalogue.md` Proposition 12 and `SPEC.md` 9.3.7 return "the exact 1" for exponent 0; `proofs/ideles.md`
  Proposition 13.6 shows that `P_0 = {1}` equals no coset with `N >= 1`; and the idele of an exact rational `q` has
  the exact unit `sign(q)`. The product rule `c c' mod gcd(N, N')` of `SPEC.md` 5 (ideles P11) holds unchanged with
  `gcd(0, N') = N'`; the inverse of an exact unit is itself; any power of an exact unit is exact.
- **Residue range 1 to `N`.** DECISION (proposed) CV-16; reason: a unit has no residue 0 except modulo 1, and the
  range `1..N` makes the whole unit group `[1 mod 1]` rather than `[0 mod 1]`. Note: `proofs/ideles.md` Definition 8
  reduces into `[0, Nbar)` and writes the whole group `(0, 1)`. The two choices describe the same sets (ideles P9.2:
  the canonical form is a complete invariant either way); only the stored and printed representative differs
  (finding F8).
- **Stored modulus as supplied.** The stored `N` is the one supplied to the constructor, parser or loader; it need
  not be normal. DECISION (proposed) CV-17; reason: `PLAN.md` 5 says the dump keeps the modulus as supplied, which
  is void if constructors normalise it.
- **Normal form** (used by the printer of the value form and by `equal_set`): remove from the modulus each prime
  whose residue field is `F_2` and which divides the modulus exactly once (seams R6, adopted by D8); for `Q` this is
  `N' = N/2` if `N = 2 mod 4`, else `N' = N`. Then `c'` is `c` reduced into `1..N'`; exact units are unchanged.
  `c U(N) = c U(N/2)` when `v_2(N) = 1` (`proofs/ideles.md` Lemma 7), and equal normal forms are equivalent to equal
  sets (ideles P9.2). `adf_ucoset_is_normal(x)` tests it.
- Init: the exact unit 1, `(c, N) = (1, 0)`.
- Containment, equality, overlap (ideles P9): `c U(N)` is inside `c' U(N')` exactly when `N'bar` divides `Nbar` and
  `c = c'` modulo `N'bar`; they meet exactly when `c = c'` modulo `gcd(N, N')`. For exact units: `[e mod 0]` is
  inside `c' U(N')` when `e = c'` modulo `N'bar` (`N' >= 1`), and two exact units are equal when equal.
- Product at moduli `N`, `N'`: `c c' U(gcd(N, N'))`, the smallest coset containing the product set (ideles P11).
- **Power.** `adf_ucoset_pow(y, x, k)`, the exponent `k` an `slong`, returns the enclosure `c^k U(N)` (`c^k` modulo
  the normal modulus, the inverse for `k < 0`), and the exact unit 1 for `k = 0`; `adf_ucoset_pow_tight(y, x, k)`
  returns the smallest coset `chat^k U(M_k)` of `proofs/ideles.md` Proposition 13 (both coincide for `k = 1` and
  `k = -1`, P13.4). DECISION CV-49 (D6); reason (orchestrator): the simple rule is the default; the tight modulus
  `M_k` needs the table of P13, including the primes `p` with `p - 1` dividing `k`, and is offered under its own
  name.

### 5.7 `adf_idele` and `adf_idclass`

    typedef struct { arb_t inf; fmpq_t r; adf_ucoset_struct u; } adf_idele_struct;
    typedef struct { arb_t t; adf_ucoset_struct u; } adf_idclass_struct;

- Idele predicate: `arb_is_finite(inf)` and `arb_is_nonzero(inf)` (`arb.h:240`: the ball excludes 0);
  `fmpq_is_canonical(r)` and `r > 0`; `u` satisfies 5.6.
- Class predicate: `arb_is_finite(t)` and `arb_is_positive(t)` (`arb.h:241`); `u` satisfies 5.6.
- **Result sign check** (gate finding G5): idele and idele-class arithmetic validates the required real sign on
  its result before committing it. A kernel may use sign-preserving endpoint bounds and a suitable enclosing
  midpoint-radius ball. If it cannot produce a finite ball with the required sign, it returns
  `ADF_NOT_DETERMINED` and leaves the value output untouched. Failure to preserve a sign under enclosure is never
  `ADF_NOT_UNIT`. The rule applies to multiplication and division, inversion, powers and class conversion alike:
  the predicate above must hold of the committed result, and an ordinary rounded product that loses the sign is
  not a result of this type.
- Signs (`SPEC.md` 5): the finite part is `r u` with `r > 0`; the sign of the finite coordinates is carried by the
  unit. The real sign is in `inf`. The class of an idele is `t = |x_inf| / r`, `u' = sign(x_inf) u`.
- The idele of an exact rational `q != 0`: `inf` an enclosure of `q` at `prec`, `r = |q|`, `u = [sign(q) mod 0]`.
- Init: the exact idele 1: `inf = 1` exact, `r = 1`, `u = [1 mod 0]`. Class init: `<1 ; [1 mod 0]>`.
- **Names** (seams R5, adopted by D8): the positive rational `r` is the **content** of the idele (for `Q` the
  positive generator of the content ideal); its accessor is `adf_idele_content`, never "scale". The class
  coordinates `(t, u')` and the sign rule are accessors special to `Q` (`adf_idclass_get_t`, `adf_idclass_get_unit`,
  as `adf_idele_get_unit` for the unit of an idele); class-level operations (multiply, norm, character value) do not
  expose them. The struct field keeps the short name `r`; the field name is not part of the contract.
- Idele to adele (`proofs/ideles.md` P16): the simple ball `r c'' + r N'' Zhat`, formed from the normal form
  `(c'', N'')` of the unit (`adf_adele_set_idele_simple`), and the smallest ball `r c' + r lcm(N, 2) Zhat` with
  `c'` odd (`adf_adele_set_idele`); both depend only on the set. The two are equal when `N''` is even; when `N''`
  is odd the radius of the simple ball is half that of the smallest, so it is the coarser set.
- **Division of an adele by an idele** `adf_adele_div_idele(z, x, y, prec)` returns, for `x = I x (a + M Zhat)` and
  `y = (Y, r, c U(N))`, the real part `I / Y` rounded as usual and the finite part
  `(a e)/r + (gcd(abs(a) lcm(N, 2), M) / r) Zhat`, with `e` odd and `e = c^-1` modulo `N`; this is the smallest ball
  containing all quotients (`proofs/ideles.md` P19). For an exact unit (`N = 0`, `c = +-1`) the finite part is
  `(a c)/r + (M/r) Zhat`, as for division by the exact rational `c r` (ideles P18). DECISION CV-50 (D7); reason
  (orchestrator): the smallest ball of P19 costs one gcd more than the simple ball and can be finer by a factor 2.

### 5.8 `adf_lball` (local ball at one prime)

    typedef struct {
        ulong p;      /* a prime */
        fmpq_t u;     /* see the predicate */
        slong v;      /* valuation of the centre; 0 when u = 0 */
        slong N;      /* ball: absolute precision, the radius is p^N.  exact: 0 */
        int exact;
    } adf_lball_struct;

The fields `u, v, N` follow `padic_struct` (`padic.h:32-41`: unit `u`, valuation `v`, absolute precision `N`) so
that the centre can be handed to FLINT; unlike `padic`, the prime is in the value, since a partial ball holds balls
at several primes. `u` is an `fmpq` only for exact values.

Predicate:

    p is prime and 2 <= p < 2^64 and exact in {0, 1} and fmpq_is_canonical(u) and
    exact = 1:  N = 0 and ( (u = 0 and v = 0) or (p divides neither numerator nor denominator of u) )
    exact = 0:  u is an integer and ( (u = 0 and v = 0) or (v < N and p does not divide u and 0 < u < p^(N - v)) )

- Meaning: exact: the rational `p^v u` of `Q_p`. Ball: `p^v u + p^N Z_p`.
- The stored ball centre `c = p^v u` is the unique element of `Z[1/p]` in `[0, p^N)` that lies in the ball.
  *Proof.* `Z[1/p] ∩ p^N Z_p = p^N Z`: an element `m/p^j` with valuation `>= N` times `p^(-N)` lies in
  `Z_p ∩ Z[1/p] = Z`. So the elements of `Z[1/p]` in the ball form one class `c + p^N Z`, and it meets `[0, p^N)` in
  one point. A ball containing 0 has `c = 0`, stored as `u = 0, v = 0`.
- Exact 0 (`exact = 1, u = 0`) and the ball `O(p^N)` around 0 (`exact = 0, u = 0`) are different values
  (`SPEC.md` 9.3.1: a stored centre 0 with finite precision is not the exact 0).
- Primes are one word in version 1 (part of DECISION CV-18, D8); reason: primality is then certified by `n_is_prime`
  (`ulong_extras.h:335`) and places fit a `ulong` (section 7). Larger primes: `ADF_UNSUPPORTED`.
- Init: `p = 2`, exact 0. DECISION (proposed) CV-19; reason: some prime must be chosen; 2 is the first.
- FLINT's reduced form of a `padic` is the same: "we consider a `p`-adic number `x = u p^v` to be in canonical form
  whenever either `p \nmid u` or `u = v = 0`, and we say it is reduced if, in addition, for non-zero `u`,
  `u \in (0, p^{N-v})`" (`flint-3.0.1:padic.rst:15-18`). So the centre of a ball can be passed to FLINT as a reduced
  `padic` with the same `u, v, N`.
- The prime is read through the place (section 7): `adf_lball_place(x)` returns the handle; the field `p` is not
  part of the contract (seams R1, D8).

### 5.9 `adf_sball` (partial ball over a finite set of places)

    typedef struct {
        int arch;                  /* ADF_ARCH_NONE = 0, ADF_ARCH_REAL = 1, ADF_ARCH_COMPLEX = 2 */
        acb_t inf;                 /* REAL: imaginary part exact 0; NONE: exact 0 */
        slong len;                 /* number of primes */
        adf_lball_struct * loc;    /* owned, primes strictly increasing */
    } adf_sball_struct;

- Predicate: `arch` in `{0, 1, 2}`; `acb_is_finite(inf)`; `arch = 1` implies the imaginary part is exact 0;
  `arch = 0` implies `inf` is exact 0; `len >= 0`; each `loc[i]` satisfies 5.8; `loc[i].p < loc[i+1].p`.
- The set of places is `{infinity}` (if `arch != 0`) together with the primes `loc[i].p`. The archimedean place
  carries the tag real or complex (`SPEC.md` 9.3.1); in the value form both are labelled `inf` and told apart by
  the syntax of the ball (9.2). Places are read through handles (section 7).
- Init: the empty set of places (`arch = 0`, `len = 0`).

### 5.10 `adf_qclass` (element or set modulo `Q`)

    typedef struct { int form; slong len; adf_adele_struct * piece; } adf_qclass_struct;
                                          /* form: ADF_QCLASS_LIFT = 0, ADF_QCLASS_PIECES = 1 */

Source: `docs/proofs/quotient.md` (reviewed) and `docs/proofs/analysis.md` Lemma 2.

- LIFT: `len = 1`; `piece[0]` is any adele (5.5); the value is its class modulo `Q` ("always available, always
  exact as a set", `SPEC.md` 6).
- PIECES: `len >= 1`; the value is the union of the images in `A/Q` of the pieces. Each piece is an adele
  `J x (m + N Zhat)` with a finite part inside `Zhat` of integer radius (quotient P5, P10 remark). In the PIECES
  invariant and order keys, `A, H, d` always denote the finite part's canonical global triple (5.2, `SPEC.md`
  4.1), including when its storage is local. Require `d = 1`, `H >= 0`, and the global centre range of 5.2; so
  the finite set is `m + N Zhat` with `N = H >= 1` and `0 <= m < N`, or `N = 0` and `m` an integer. The real
  midpoint must lie in `[0, 1]`. These are storage predicates only. A reduction operation must separately ensure
  that each output encloses its constructed exact closed piece; the predicate alone does not certify provenance,
  tightness or a bound on the amount of rounding (gate finding G13). The gluing rule: `(1 ; z)` is the point
  `(0 ; z - 1)` (quotient P3).
- **The invariant that replaces "inside `[0, 1]`".** DECISION CV-45 (D4): a piece's real part is a closed real ball
  whose **midpoint lies in `[0, 1]`**; the ball itself may reach beyond `[0, 1]` by the outward rounding of the
  enclosure. Its meaning is unchanged: every point `(s, z)` of a piece denotes its class in `A/Q`, which is defined
  for every real `s`, so a point with `s` slightly below 0 or above 1 is the class of `(s + 1, z + 1)` or
  `(s - 1, z - 1)`, a point near the other end of the domain; the enclosure stays an enclosure. A reduction
  operation must enclose its constructed exact closed piece `C_n` inside `[0, 1]`. The midpoint predicate alone
  does not certify that construction or its rounding error. Reason (orchestrator): `SPEC.md` 6 asks for pieces
  "inside `[0, 1]`", which an `arb` enclosure of an interval with a non-dyadic end point cannot keep (probed: the
  enclosure of `[0.9, 1]` exceeds 1 at `prec` 20, 53 and 128; finding F2).
- **Number of pieces** (D4, quotient P5, P6): reduction of `I x (a + N Zhat)` with integer `N` and shifted interval
  `[lo', hi'] = I - a` gives one closed piece `C_n` for each `n` with `floor(lo') <= n <= ceil(hi') - 1`, that is
  `k + 1` pieces when the open interval `(lo', hi')` contains `k` integers, and one piece when `lo' = hi'`. A
  fractional radius `A/B` is first split into the `B` balls `a + j A/B + A Zhat` (quotient P8, `B` is minimal). The
  count is that of the construction; pieces with the same finite class may merge afterwards (quotient P6 remark).
  When `hi - lo >= N` the image is all of `A/Q` (quotient P7).
- **Piece limit.** An argument of every function that produces pieces (`SPEC.md` 6); no default. More pieces than
  the limit: `ADF_LIMIT` (3.1). A function that may return one piece only returns `ADF_NEEDS_SPLIT` when the image
  needs more than one piece.
- **Canonical order of pieces** (storage and printing): increasing by the exact lower end of the real part, then
  its upper end, then `N`, then `m` (the canonical triple's `H` and `A`, never raw local data); no two pieces
  identical (CV-24, proposed; reason: any fixed order makes the printed form unique, this one is cheap). The
  value form orders by the printed real parts (9.4).
- **Equality of two unions** is decided by the canonical form of quotient P9 (common modulus `N'`, the sets `T_m` in
  `[0, 1)`), exactly when the end points are exact; for balls the function returns `ADF_CMP_UNDECIDED` unless the
  sets are certainly equal or certainly different.
- Translation by a rational changes nothing (quotient P10); the constructions of P5 and P8 give literally the same
  pieces for `X` and `X + q0`.
- Init: LIFT of the adele `(0 ; 0)`.

### 5.11 `adf_ffun` (finite test function)

    typedef struct { ulong D, M; acb_ptr f; } adf_ffun_struct;   /* f has length L = D M; f[j] = f(j/D) */

- Predicate: `D >= 1`, `M >= 1`, `D M < 2^62`, every `f[j]` finite. Meaning as in `SPEC.md` 7: zero outside
  `(1/D) Zhat`, constant on cosets of `M Zhat`.
- No normal form: a smaller `D` or `M` may describe the same function, and equality of ball values is undecided, so
  the stored `(D, M)` is kept. DECISION (proposed) CV-20; reason: a reduction to minimal `(D, M)` is not decidable
  on balls.
- Init: `D = M = 1`, `f[0] = 0` (the zero function).

### 5.12 `adf_rfun` (real test function)

    typedef struct { acb_poly_t P; acb_t A, B, C; } adf_rterm_struct;  /* P(x) exp(-pi A x^2 + B x + C) */
    typedef struct { slong len; adf_rterm_struct * term; } adf_rfun_struct;

- Predicate: `len >= 0`; for each term: `P` is a normalized `acb_poly` with length `>= 0` and finite
  coefficients; its last coefficient, if any, is not the exact zero ball; `A, B, C` finite; `Re(A) > 0`
  certified (`arb_is_positive` of the real part of `A`, `arb.h:241`) (`SPEC.md` 7). Length 0 is the zero
  polynomial. Terms may retain a zero `P`; term order is unchanged. The value grammar admits `P=[]` and removes
  exact trailing zero coefficients on input; the printer prints the resulting coefficient list. A ball merely
  containing zero is not trimmed (gate finding G7).
- No normal form; terms are kept in the stored order (CV-20).
- Init: `len = 0` (the zero function).

### 5.13 `adf_char` (quasi-character of the idele class group)

    typedef struct { ulong q; ulong n; int parity; acb_t s; } adf_char_struct;

Meaning: `t^s chi(u')` on `R_{>0} x Zhat^x` (`SPEC.md` 5), `chi` the Dirichlet character with Conrey label `n`
modulo `q`, as numbered by FLINT's `dirichlet` module (`dirichlet.h:37`, "conrey generator"; `dirichlet.h:114`
`dirichlet_char_log`). FLINT's documentation calls this label the "Conrey number"
(`flint-3.0.1:acb_dirichlet.rst:380`, "the *ui* version only takes the Conrey number *a* as parameter"); the
`dirichlet` module's own documentation is not on disk [source pending: FLINT 3.0.1 `dirichlet.rst`].

- Predicate: `q >= 1`; `1 <= n <= q`; `gcd(n, q) = 1`; the character is primitive, i.e. its conductor
  (`dirichlet_conductor_char`, `dirichlet.h:111`) equals `q`; `parity` equals `dirichlet_parity_char`
  (`dirichlet.h:110`), 0 even, 1 odd; `s` finite.
- The stored character is **primitive**; `q` is the conductor (`SPEC.md` 5: "a character carries its conductor").
  A constructor given a non-primitive `(q, n)` lowers it with `dirichlet_char_lower` (`dirichlet.h:136`).
  Note **[probed]** with python-flint 0.8.0: the label of the primitive character is not always `n mod f`:
  `(16, 9)` has conductor 8, and `(8, 1)` is the principal character.
- Init: the principal character, `q = 1, n = 1`, `s = 0`.
- **Value.** The stored `(chi, s)` is the quasi-character `omega(t, u') = t^s chi(u')` of `SPEC.md` 5, with
  `t = |x_inf| / r` the norm of the idele and `u' = sign(x_inf) u` (ideles P15); it is unitary when `Re(s) = 0`.
  DECISION (proposed) CV-58; reason: this is the family as `SPEC.md` 5 writes it, with no hidden conjugation.
- **The Tate integral of `L(s, chi)`** uses the character `omega_chi(x) = conj(chi(u'))` (`proofs/analysis.md`
  Proposition 11; `SPEC.md` 8), whose uniformizer value at `p` not dividing the conductor is `chi(p)`. In terms of
  this type it is the `adf_char` of `conj(chi)` with `s = 0`, the variable `s` of the integral being separate. The
  Conrey label of `conj(chi)` modulo `q` is `n^-1 mod q` (**[probed]** with python-flint on every character of
  modulus below 60: the exponents of `chi_q(n, x)` and `chi_q(n^-1, x)` add to 0 modulo the group exponent).
  Functions that compute `L(s, chi)` by the Tate integral take `chi` and form `omega_chi` themselves.

### 5.14 `adf_modctx` (modulus context)

Opaque (section 12). Contents, not part of the interface: `K >= 1` (an `fmpz`); `k >= 0` word blocks
`q_1, ..., q_k`, pairwise coprime, `2 <= q_i < 2^64`, in the order supplied, with product `K` when `k >= 1`; an
`nmod_t` per block (`nmod_init`, `nmod.h:212`); for each block whether it is a certified prime power and its prime;
recombination data. The type is incomplete in the public header (4.6, gate finding G2). A context has no init
value and no `init`/`clear`: it exists only after a successful constructor. A constructor writes its pointer to
the `adf_modctx_struct **out` first argument on success; on failure `*out` is untouched and the status below is
returned. `adf_modctx_free` releases a context. Constructors:

| Constructor | Blocks |
|---|---|
| `adf_modctx_new_blocks(out, q, k)` | as supplied; `ADF_DOMAIN` unless pairwise coprime and each `>= 2` |
| `adf_modctx_new_prime_powers(out, p, e, k)` | `p_i^e_i`, primes certified; `ADF_DOMAIN` on a composite or repeated prime; `ADF_UNSUPPORTED` if a power exceeds a word |
| `adf_modctx_new_fmpz(out, K)` | one block `K` if `2 <= K < 2^64`; none if `K = 1` or `K >= 2^64` (then only the scaled policy can use it) |
| factorial `k!` (`adf_modctx_new_factorial`), powers of a primorial | the prime powers of `K` in increasing order of the prime. DECISION (proposed) CV-21; reason: the factorisation is known, and prime-power blocks serve operations that name a prime |

The local backend needs `k >= 1`; conversion to the local backend into a context with `k = 0` returns
`ADF_UNSUPPORTED` (a scaled value may borrow a context with `k = 0`, 5.4).

## 6. Signs, measures, normalisations, and operations fixed by decisions

Sources: `docs/proofs/analysis.md` (reviewed and repaired: 9 VALID, 6 MINOR repaired, 0 INVALID),
`docs/proofs/catalogue.md`, `docs/sources.md` and the texts under `refs/src/`. `E(t) = exp(2 pi i t)`.

### 6.1 Additive character and Fourier transform

**The character** (`proofs/analysis.md` Definition 1). For every prime `p`, including 2, `fp_p(x)` is the unique
element of `Z[1/p]` in `[0, 1)` with `x - fp_p(x)` in `Z_p` (the p-primary fractional part `{x}_p` of `SPEC.md`
9.3.6). Then

    psi_p(x) = E(fp_p(x)),    psi_inf(x) = E(-x),    psi(x) = psi_inf(x_inf) * product over p of psi_p(x_p).

`psi` is a continuous character of `A`, trivial on `Q` (analysis Lemma 2); for a rational `a`, `psi_f(a) = E(a)`.
This is the standard character of the exposition on disk: "If F = R, let ψ(x) := e−2πix . (The minus sign is there
so that a global product" / "formula later on will hold.)" (`tate-poonen:notes.txt:693-694`), and at `Q_p`
"which is characterized by ψ|Zp = 1 and ψ(1/pn ) = e2πi/p for all n ≥ 1" (`tate-poonen:notes.txt:700`; the
extraction flattens the exponent `e^(2 pi i / p^n)`).

**The transform.** DECISION CV-54 (D11): the transform of `SPEC.md` 6 is kept,

    F f(y) = integral of f(x) conj(psi(x y)) dx,

with the self-dual measures of 6.2, so that `F F f(x) = f(-x)` (analysis Proposition 3). Reason (orchestrator):
it is Tate's original convention and the one the proofs use. The expositions on disk define the transform
**without** the conjugate: `tate-poonen:notes.txt:733-737`, "Definition 4.7. Fix a local field F , a nontrivial
additive character ψ on F , and a Haar / measure dx on F . Given f ∈ S , define the Fourier transform fb by / ... /
f (x) ψ(xy) dx." with the remark "(Tate originally took the complex conjugate of the additive character, but many
references since then have not done so.)" (`tate-poonen:notes.txt:740`); the adelic transform is the same,
"fb(y) :=   f (x) ψ(xy) dx," (`tate-poonen:notes.txt:1400`); and `tate-kudla:kudla-1.txt:705-706`, "The Fourier
transform / fˆ(x) =        f (y) ψ(xy) dy".

**Conversion between the two conventions.** Write `F_c` for ours and `F_u` for the unconjugated transform, with the
same `psi` and the same measure. Since `conj(psi(x y)) = psi(-x y)`:

    F_c f(y) = F_u f(-y) = F_u (f o (-1)) (y),        F_c = F_u^(-1),        F_u f(y) = F_c f(-y).

(Own calculation: substitute `x -> -x` in the integral; the last two follow from `F_u F_u f = f o (-1)`, which is
the self-duality of the measure for `F_u`, the same measure as for `F_c`.) The two agree on even functions and
differ by a reflection otherwise; the shifted Gaussians of `PLAN.md` 4.3 tell them apart. Every formula of this
document is in the convention `F_c`. The kernels are then `exp(+2 pi i x y)` at the real place and
`exp(-2 pi i fp(x y))` at the primes (`SPEC.md` 6).

**Phases on a ball** (analysis Lemma 2). On `a + N Zhat`: `N = 0` gives `{E(a)}`; `N = A/B > 0` in lowest terms
gives `{E(a) E(k/B) : 0 <= k < B}`, the `B`-th roots of unity times `E(a)`; an integer radius gives one phase. On
`I x (a + N Zhat)` the image is that finite set times `E(-I)`. The golden file `tests/golden/psi_phases.tsv` lists
the angles `t` with phase `E(t)` for finite balls.

**Criterion by duality** (seams R8, adopted by D8): the phase of a ball is determined exactly when the character is
trivial on its radius group; for `Q`, exactly when the finite radius is an integer (then only the real part adds
uncertainty). The functions carry the convention in their names: `adf_adele_psi_tate(z, x, prec)` returns an
enclosure of all phases (`ADF_OK`); `adf_adele_psi_tate_strict` returns `ADF_NOT_DETERMINED` when the finite radius
is not an integer (CV-07). DECISION (proposed) CV-59; reason: seams R8 asks for a named convention, and "tate" names
the convention of `SPEC.md` 6 without the unsourced section number. `psi` is constant on classes modulo `Q`, so the
same functions exist for `adf_qclass`.

### 6.2 Measures

(`proofs/analysis.md` Definition 1, Proposition 3, Lemma 2, Proposition 12.)

- Additive: `vol(Z_p) = 1` at every prime, Lebesgue measure at the real place, and their restricted product. So
  `vol(N Zhat) = 1/N` for `N > 0`, `vol` of a point is 0 (`SPEC.md` 9.3.7, Haar volume), and `A/Q` has volume 1
  with the fundamental domain `[0, 1) x Zhat`.
- These measures are self-dual for `psi` and `F_c`: `F_c F_c f(x) = f(-x)` (analysis P3; a rescaling `c` of one
  measure multiplies the double transform by `c^2`).
- Multiplicative: `d*x_p = (1 - 1/p)^(-1) dx_p / |x_p|_p`, so `vol*(Z_p^x) = 1`; `d*x_inf = dx / |x|`.
- On the units `Zhat^x`, the probability measure `du` (every finite quotient uniformly weighted) (analysis P12).

### 6.3 Finite Fourier transform

(`proofs/analysis.md` Proposition 4.) For `adf_ffun` with `D, M >= 1`, `L = D M`, `f_j = f(j/D)`:

    g_k = g(k/M) = (1/M) sum_(j=0)^(L-1) f_j E(-j k/L),    integral f = (1/M) sum_j f_j,
    integral |f|^2 = (1/M) sum_j |f_j|^2 = (1/D) sum_k |g_k|^2.

The transform `g = F_c f` is zero off `(1/M) Zhat` and constant modulo `D Zhat`, so it is stored as the
`adf_ffun` with `(D, M)` exchanged: `D' = M`, `M' = D`, `g_k = g(k/D')` for `0 <= k < L`. The second transform
has the weight `1/D` and returns `f(-x)`. These are inclusions of support and period, not minimality claims.

### 6.4 Gauss sums, root numbers, local constants

(`proofs/analysis.md` Lemma 8, Propositions 9, 10, 13.) Let `chi` be primitive of conductor `C`, extended by 0 on
non-units (for `C = 1`, `chi(n) = 1` for every `n`), and `chi(-1) = (-1)^e`.

- **Gauss sum, positive finite sign:** `tau(chi) = sum_(a mod C) chi(a) E(a/C)`. Then
  `sum_(a mod C) chi(a) E(m a/C) = conj(chi(m)) tau(chi)` for every integer `m`, `|tau(chi)| = sqrt(C)`,
  `tau(chi) tau(conj(chi)) = (-1)^e C`, and `tau = 1` for `C = 1`.
- It equals FLINT's Gauss sum for the primitive character: "G_q(a) = \sum_{x \bmod q} \chi_q(a, x)
  e^{\frac{2i\pi x}q}" (`flint-3.0.1:acb_dirichlet.rst:362`), with the same positive sign. **[probed]** for the 17
  characters of `tests/golden/gauss.tsv`, `acb_dirichlet_gauss_sum` at 256 bits lies in the golden balls.
- **Root number:** `W_chi = tau(chi) / (i^e sqrt(C))`, with `Lambda(s, chi) = W_chi Lambda(1 - s, conj(chi))`,
  `Lambda(s, chi) = C^((s+e)/2) pi^(-(s+e)/2) Gamma((s+e)/2) L(s, chi)`, `|W_chi| = 1`,
  `W_chi W_conj(chi) = 1` (analysis P13). The root number uses the positive finite Gauss sum although the finite
  Fourier kernel is negative (analysis P13, last sentence). For a real primitive character `W_chi = 1`
  [source pending: a local source or proof of the signed primitive quadratic Gauss sum evaluation]. Until that
  evaluation is supplied, compute `W_chi` by the same finite Gauss-sum formula used for other characters. The
  real-character golden vectors (`(3, 2)`, `(4, 3)`, `(5, 4)`, `(7, 6)`, `(8, 3)`, `(8, 5)`, `(8, 7)`,
  `(12, 11)`) are finite checks of examples, not its proof (gate finding G15).
- **Local constants at a prime** (analysis P9) for a quasi-character `eta` of `Q_p^x` with `alpha = eta(p)`, unit
  restriction `eta_0` and conductor exponent `a`, with the local functional equation
  `Z_p(F_c f, eta^(-1), 1 - s) = gamma_p(s, eta) Z_p(f, eta, s)`:

      a = 0:  gamma_p(s, eta) = (1 - alpha p^(-s)) / (1 - alpha^(-1) p^(s-1)),
              L_p(s, eta) = (1 - alpha p^(-s))^(-1)
      a > 0:  gamma_p(s, eta) = alpha^a p^(-a s) G_minus(eta_0^(-1)),    L_p(s, eta) = 1
              G_minus(eta_0^(-1)) = sum_(u mod p^a, p not dividing u) eta_0(u)^(-1) E(-u/p^a)
      epsilon_p = gamma_p L_p(s, eta) / L_p(1 - s, eta^(-1))

  The local sum `G_minus` has the **negative** sign of the finite kernel; it is not `tau`. At `p = 2` the ramified
  case needs `a >= 2`.
- **Real place** (analysis P10), `eta(x) = sign(x)^e`, `phi_e(x) = x^e exp(-pi x^2)`:
  `Z_inf(phi_e, eta, s) = pi^(-(s+e)/2) Gamma((s+e)/2)`,
  `gamma_inf(s, eta) = i^e pi^(s-1/2) Gamma((1-s+e)/2) / Gamma((s+e)/2)`, `L_inf(s, eta) = pi^(-(s+e)/2)
  Gamma((s+e)/2)`, `epsilon_inf = i^e`. Poles of the trivial factors: `s = 0, -2, -4, ...` at the real place,
  `s = 2 pi i k / log p` at `p`; a ball meeting a pole returns a status, never a finite ball (`SPEC.md` 9.3.7).
  For a local factor or integral, an exact input at a proved nonremovable pole returns `ADF_DOMAIN`. An input ball
  meeting a pole and also containing regular points returns `ADF_NOT_DETERMINED`. Both leave the value output
  untouched. The same rule applies if exclusion of poles is undecided. No non-finite ball is stored. A separately
  documented report may distinguish proved intersection with a pole from undecided intersection (gate finding G6).
- Names: `adf_char_gauss_sum` (returns `tau` of the stored primitive character), `adf_char_root_number` (`W`),
  `adf_local_gamma_at`, `adf_local_epsilon_at`, `adf_local_zeta_factor_at`. DECISION (proposed) CV-60; reason: the
  names say which object is meant; `tau` and `G_minus` differ in sign and must not share a name. The golden file
  `tests/golden/gauss.tsv` holds `e`, `tau` and `W` as balls.

### 6.5 Class-group characters

(`proofs/analysis.md` Proposition 11, 13; `proofs/ideles.md` Proposition 15; `SPEC.md` 5, 8.) The type is 5.13: the
stored `(chi, s)` is `t^s chi(u')`. For the Tate integral of `L(s, chi)` the idele class character is
`omega_chi = conj(chi(u'))`; its uniformizer value at `p` not dividing `C` is `chi(p)`, at `p` dividing `C` it is
`alpha_p = product over primes l dividing C, l != p, of chi_l(p)` (CRT factorisation `chi = product of chi_l`), its
unit restriction at `p | C` is `conj(chi_p)`, its real restriction `sign(x)^e`. The test vector: real part `phi_e`,
`eta_p^(-1) 1_(Z_p^x)` at ramified `p`, `1_(Z_p)` elsewhere; for `Re(s) > 1`,
`Z(f_chi, omega_chi, s) = pi^(-(s+e)/2) Gamma((s+e)/2) L(s, chi)`; `1_(Z_p)` at a ramified prime gives 0.
The value of a character on a unit coset `c U(N)` is one number exactly when the conductor divides `N`, or `N = 0`
(an exact unit); otherwise the default function returns an enclosure of all values and the `_strict` variant
`ADF_NOT_DETERMINED` (4.3, CV-07).

### 6.6 Reciprocity (cyclotomic action)

DECISION CV-53 (D10): both conventions are offered, as two functions named by their formula:

| Function | Action of the class `(t, u')` on a root of unity `z` of order `n` | Source |
|---|---|---|
| `adf_idclass_cyclo_exp_u(j, x, n)` | `z -> z^(u' mod n)`; returns `j = u' mod n` | the canonical isomorphism `Zhat^x -> Gal(Q^cyc/Q)` |
| `adf_idclass_cyclo_exp_uinv(j, x, n)` | `z -> z^(u'^(-1) mod n)`; returns `j = u'^(-1) mod n` | the global reciprocity map of Milne's notes |

Quotations (control characters of the extraction written `\xNN`, as in `docs/sources.md`): "In this case, the global
reciprocity map is the reciprocal of" / "\x1eW IQ ! ZO \x02 ! Gal.Qcyc =Q/;" / "where IQ ! ZO \x02 is the above
projection map, and ZO \x02 ! Gal.Q cyc =Q/ is the canonical" / "isomorphism (see I A.5c)."
(`milne-cft:CFT.txt:9883-9886`; the extraction drops the Greek letters and the superscripts); the canonical
isomorphism acts by `u`-th powers: "u D a0 C a1 p C a2 p C \x01 \x01 \x01 ; 0 \x14 ai \x14 p 1, and define" / ... /
"This defines an action of Z\x02" / "p on ˝p , and in fact an isomorphism of topological groups"
(`milne-cft:CFT.txt:3162-3166`, read as `zeta_(p^r)^u = zeta_(p^r)^(a_0 + a_1 p + ... + a_s p^s)`). Reason
(orchestrator): `SPEC.md` 9.3.7 asks for the convention in the name; the words "arithmetic" and "geometric" of
`SPEC.md` have no source on disk (`docs/sources.md`, sources pending, item 2), so the names give the exponent
instead, and this document does not use those words for them. `t` acts trivially. The action on all `n`-th roots is
determined exactly when the normal form of `n` divides the normal form of `N` (`proofs/catalogue.md` Proposition
15), always for an exact unit; otherwise `ADF_NOT_DETERMINED`. Test vector (catalogue P15;
`milne-cft:CFT.txt:9904-9908` with the Frobenius of `milne-cft:CFT.txt:1307`, "such that \x1b ˛ \x11 ˛ q mod mL for
all ˛ 2 OL . This \x1b is called the Frobenius element of"): the class of the idele with `p` at the place `p` and 1
elsewhere has `u' = p^(-1)` away from `p`, so for `gcd(p, n) = 1` `adf_idclass_cyclo_exp_uinv` returns `j = p` (`z
-> z^p`) and `adf_idclass_cyclo_exp_u` returns `j = p^(-1) mod n`.

### 6.7 Recommendations of the seams sketch

All nine recommendations of `docs/seams.md` section 5 are adopted: DECISION CV-57 (D8); reason (orchestrator): cheap
decisions now that avoid a break later (seams section 5). Where each is applied:

| Rec. | Content | Here |
|---|---|---|
| R1 | a place is an opaque handle | section 7 (CV-18 revised, CV-56); 5.8, 5.9; the place of `ADF_DOMAIN` |
| R2 | the radius is a rational documented as the positive generator of the radius ideal; compared by inclusion only | 5.2 |
| R3 | archimedean operations go through the place; the dump counts archimedean components | 10.1 |
| R4 | `adf_cadele` is special to `Q` | 5.5 |
| R5 | the idele scale is called the content | 5.7 |
| R6 | coset normal form stated by residue fields | 5.6 |
| R7 | the context holds an integer; no canonical scale is promised | 5.4 |
| R8 | the additive character is a named convention; its criterion by duality | 6.1 |
| R9 | the dump names the field; the value form has place labels as tokens (`p=5`, `inf`) | 9.2, 10.1 |

### 6.8 Operations fixed by decisions

- **Rational reconstruction from a full ball** intersects the finite progression with the **closed** real interval:
  for `I x (a + N Zhat)`, `I = [lo, hi]`, the candidates are `a + N k` with
  `ceil((lo - a)/N) <= k <= floor((hi - a)/N)` for `N > 0`, and `a` if `lo <= a <= hi` for `N = 0`
  (`proofs/quotient.md` P11); for an `arb` the interval is `[mid - rad, mid + rad]` with its exact dyadic end
  points. Statuses: one candidate `ADF_OK`; none `ADF_NO_SOLUTION`; several `ADF_NOT_UNIQUE`. From partial data
  (quotient P13; solvers P1.7), "uniqueness not certified" is `ADF_NOT_DETERMINED`. It is returned exactly when
  `A < m <= 2 A B`, `|T| <= B` (`T` the denominator entry of the Euclidean row that the algorithm reaches),
  `floor(B/|T|)` exceeds the search limit `ell = max(limit, 0)`, and the rounds `x <= ell` found fewer than two
  reduced pairs. The cases `A >= m`, `|T| > B` and `2 A B < m` keep their decided statuses also when `limit` is 0.
  DECISION CV-51 (D3); reason (orchestrator): a real ball is a closed set, and an end point of it may be the answer.
- Scaled product: 5.4 (CV-48, D5). Power of a unit coset: 5.6 (CV-49, D6). Division by an idele: 5.7 (CV-50, D7).
  Complete validation of dump text before FLINT: 10.2 (CV-52, D9).

## 7. Places

DECISION CV-18 (D8, seams R1), revised from version 0.1: **a place is an opaque handle.**

    typedef struct { ulong opaque; } adf_place_t;          /* 8 bytes, passed by value; the field is private */

    adf_place_t adf_place_inf(void);                        /* the archimedean place of Q */
    int         adf_place_prime(adf_place_t * v, ulong p);  /* ADF_DOMAIN unless p is prime (n_is_prime) */
    int         adf_place_is_archimedean(adf_place_t v);
    ulong       adf_place_prime_get(adf_place_t v);         /* 0 for the archimedean place */
    int         adf_place_cmp(adf_place_t v, adf_place_t w);   /* -1, 0, 1 in the canonical order */
    int         adf_place_equal(adf_place_t v, adf_place_t w);

- Every function that names a place takes an `adf_place_t`: `f_at`, `adf_lball`, `adf_sball`, valuation, absolute
  value, fractional part, Hilbert symbol, and the place reported with `ADF_DOMAIN` (3.3). User code creates and
  reads places only through these functions; the integer inside is not part of the contract, and no arithmetic or
  comparison on it is offered. Reason (orchestrator, from seams R1): in a number field the prime 3 can have two
  places, and in `F_q(T)` the infinite place is non-archimedean.
- The handle is a struct, not a bare `ulong`, so that C code cannot compare or compute with it by accident; its
  layout (one `ulong`) is fixed for the foreign-function interface (section 12). DECISION (proposed) CV-56; reason:
  the compiler then enforces "not an integer" at no run-time cost.
- In version 1 the primes of places are one word (CV-18 of version 0.1, kept): `adf_place_prime` with `p >= 2^64`
  is impossible by the type; larger primes in text are `ADF_UNSUPPORTED` (9.3).
- **Canonical order of places:** the archimedean place first, then the primes in increasing order
  (`adf_place_cmp`). It is the order of printing (9.4), of `adf_sball` storage (5.9), of per-place reports and of
  the tie-break of section 3.3. For fields with several places above one prime this order will need a tie-break
  among them (seams R1); version 1 has none to make.
- A set of places is an array of `adf_place_t` and a length `n` (there is no `adf_places_t` type, as `sball.h`
  says); a stored set (`adf_sball`, 5.9) is sorted in this order, without repetition; constructors accept the array
  in any order, sort it and reject repetitions with `ADF_DOMAIN`.

## 8. Text: general rules

Two forms (`SPEC.md` 10.2, `PLAN.md` 5). The **value form** is canonical, independent of the backend, for people
and for exact exchange of finite parts; a real ball in it is a decimal enclosure. The **dump form** is versioned,
keeps backend and context, writes real balls as exact dyadic numbers, and reading a dump gives back the identical
object.

### 8.1 Interface

    int adf_x_set_str(adf_x_t x, const char * s, size_t len, [slong prec,] const adf_text_limits_t * lim);
    char * adf_x_get_str(size_t * len, const adf_x_t x, [slong digits]);   /* value form, flint_malloc */
    int adf_x_load_str(adf_x_t x, const char * s, size_t len, const adf_modctx_struct * ctx,
                       const adf_text_limits_t * lim);           /* one context: the 10.2 convenience */
    int adf_x_load_str_binds(adf_x_t x, const char * s, size_t len, const adf_modctx_struct ** binds,
                             size_t nbinds, const adf_text_limits_t * lim);   /* one binding per occurrence */
    char * adf_x_dump_str(size_t * len, const adf_x_t x);         /* dump form, flint_malloc */

- The input is the `len` bytes at `s`; it need not be NUL-terminated, and a NUL byte inside it is an error
  (`ADF_PARSE`). DECISION (proposed) CV-25; reason: a NUL-terminated interface silently truncates at an embedded
  NUL, and bindings (section 12) pass lengths anyway.
- `prec` is present for the types with a real or complex part; `digits` likewise (default `ADF_DIGITS_DEFAULT = 20`,
  `1 <= digits <= 10^6`). A `prec` below 2 (zero or negative included) is taken as 2 by the parsers, as by every
  function (M1-D4, 2.2): `"(0.1 ; 0)"` read at `prec = -5, 0, 1` and `2` gives the same ball.
- `lim = NULL` means the defaults of 8.4.
- A returned string is an output in the order of 2.2: `len` receives its byte length. The `len` bytes are ASCII
  without embedded NUL; a terminating NUL is stored at `s[len]` and is not counted in `len`. The caller frees the
  pointer with `adf_str_free`. The text has no leading or trailing whitespace and no newline (gate finding G14).

### 8.2 Alphabet and whitespace

- Allowed bytes: `0x20` to `0x7E`, and TAB `0x09`, LF `0x0A`, CR `0x0D`. Any other byte (NUL, other control bytes,
  every byte `>= 0x80`, hence all non-ASCII UTF-8) gives `ADF_PARSE`. DECISION (proposed) CV-26; reason: one
  alphabet, no Unicode look-alikes of `-` or digits.
- Value form: whitespace (space, TAB, LF, CR) may stand before the first token, after the last, and between any two
  tokens; never inside a token. The printer writes exactly the templates of section 9.4 (single spaces).
- Dump form: exactly one space between tokens, nothing else (section 10).
- Keywords are case-sensitive.

### 8.3 Tokens of the value form

A token is one of: a number (section 9.1), a keyword (a maximal run of ASCII letters that must equal the keyword the
grammar expects: `mod O p inf i Q union ffun rfun term char D M P A B C q n s`), `+/-`, or one of the single
characters `( ) [ ] < > { } ; , : * + = ^`. The longest match is taken, so `+/-` is one token. A number starts with
a digit or with `-` directly followed by a digit. A letter directly after a number ends the number (`5mod6` is the
three tokens `5 mod 6`; `1 e5` is `1` followed by the unknown keyword `e`).

### 8.4 Limits

    typedef struct { size_t max_len; slong max_exp10; slong max_prec; slong max_items; } adf_text_limits_t;

| Limit | Default | Applies to |
|---|---|---|
| `max_len` | 1048576 bytes | the whole input, value form and dump |
| `max_exp10` | 100000 | the absolute value of the exponent of a decimal (`1e100001` is over) |
| `max_prec` | 100000 | the absolute value of `N` in `O(p^N)`, of the fields `v` and `N` of a dumped local ball |
| `max_items` | 1048576 | `D M` of `ffun`; the number of pieces, terms, places, polynomial coefficients, and the block count of every context occurrence (10.2) |

DECISION (proposed) CV-27; reason: each bounds the memory and time that a short input can demand (a 16-byte
`O(5^99999999999)` would otherwise ask for a 2^38-bit modulus). The limits are arguments, not global state.
Integers in the value form have no separate limit; `max_len` bounds them. The grammar is not recursive (no
production contains itself), so nesting depth is bounded by the grammar: `((((1))))` is simply `ADF_PARSE`.

The exponent digits of a decimal are compared with `max_exp10` as a number of any length (M1-D7). The Python
reference (`proto/text_grammar.py`) refuses an exponent of more than 18 digits whatever `max_exp10`; the C reader
has no such bound. Measured with the defaults (`adf_adele_set_str`, `prec = 64`): `(1e100000 ; 0)` is `ADF_OK`;
`(1e100001 ; 0)`, a 19-digit exponent and a 39-digit exponent, positive or negative, are `ADF_LIMIT`; `1e` with
the 38 exponent digits `00...05` is `ADF_OK` (leading zeros are skipped, the value is 5); with
`max_exp10 = 10^6` a 39-digit exponent is `ADF_LIMIT`. A coefficient zero does not excuse the exponent:
`(0e` with a 39-digit exponent `; 0)` is `ADF_LIMIT`.

One limit is a constant of the implementation and not a field of `adf_text_limits_t` (decision M1-D9,
`docs/SPEC.md` section 15): in a dumped `qclass` of the form `pieces` the binary exponent of the midpoint and of the
radius of the real ball of every piece is at most `ADF_DUMP_QCLASS_EXP_MAX = 2^20` in absolute value
(`include/adelefeld/dump.h`); a larger one is `ADF_LIMIT`. It is a limit of stage 4 of 8.5, compared on the digit
strings as a number of any length. Reason: the check of the range of a piece (10.2) forms the exact end points of
the ball, whose size grows with the exponents. The form `lift` forms no range and has no such limit, and neither
has any other body.

### 8.5 Order of checks

The first failing stage determines the status; the output is untouched (4.3).

1. `len > max_len`: `ADF_LIMIT` (checked before any byte is read).
2. A forbidden byte (8.2): `ADF_PARSE`.
3. The grammar (section 9.2 or 10): `ADF_PARSE`. For the dump: first the header; an unknown version is
   `ADF_UNSUPPORTED` at this point, before the body is read.
4. Limits on literals and counts (8.4): `ADF_LIMIT`. Checked on the digit strings, before any big number is
   formed. Every count limit is applied to every occurrence before any semantic check of stage 6, whatever field
   fails later: for a context this is its block count, including contexts nested in finite balls, adeles and
   quotient pieces (gate finding G8).
5. Word restrictions: a prime `p` or a character modulus `q` `>= 2^64`: `ADF_UNSUPPORTED`.
6. Semantic constraints (section 9.3, and the predicates of section 5 for the dump): `ADF_DOMAIN`.
7. Only in C, for a real or complex part: a sign condition that holds for the exact decimal interval but not for
   its enclosure at `prec` (section 9.3): `ADF_NOT_DETERMINED`.

DECISION (proposed) CV-28; reason: the status then does not depend on the order in which a parser happens to meet
the defects.

## 9. Value form

### 9.1 Lexical grammar (no whitespace inside)

    digit  = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;
    uint   = digit , { digit } ;                          (* leading zeros accepted, never printed *)
    int    = [ "-" ] , uint ;
    urat   = uint , [ "/" , uint ] ;
    rat    = int , [ "/" , uint ] ;
    exp10  = ( "e" | "E" ) , [ "+" | "-" ] , uint ;
    udec   = uint , [ "." , uint ] , [ exp10 ] ;
    dec    = [ "-" ] , udec ;
    sint   = [ "-" ] , uint ;

There is no `+` sign on a number (only in an exponent), no `.5`, no `5.`, no hexadecimal, no `inf` or `nan`.

### 9.2 Syntactic grammar (whitespace allowed between tokens)

    real      = dec , [ "+/-" , udec ] ;
    complex   = "(" , real , ")" , "+" , "(" , real , ")" , "*" , "i" ;
    fin       = rat , [ "mod" , urat ] ;
    ucoset    = "[" , int , [ "mod" , uint ] , "]" ;
    lcoord    = rat , [ "+" , "O" , "(" , uint , [ "^" , sint ] , ")" ] ;
    adele_v   = "(" , real , ";" , fin , ")" ;

    rat_v     = rat ;
    fball_v   = "(" , "*" , ";" , fin , ")"  |  rat , "mod" , urat ;
    cadele_v  = "(" , complex , ";" , fin , ")" ;
    ucoset_v  = ucoset ;
    idele_v   = "(" , real , ";" , urat , "*" , ucoset , ")" ;
    idclass_v = "<" , real , ";" , ucoset , ">" ;
    lball_v   = "[" , "p" , "=" , uint , ":" , lcoord , "]" ;
    sentry    = "inf" , ":" , ( real | complex )  |  "p" , "=" , uint , ":" , lcoord ;
    sball_v   = "{" , [ sentry , { ";" , sentry } ] , "}" ;
    qclass_v  = adele_v , "+" , "Q"  |  "union" , "(" , adele_v , { "," , adele_v } , ")" , "+" , "Q" ;
    ffun_v    = "ffun" , "(" , "D" , "=" , uint , "," , "M" , "=" , uint , ";" , complex , { "," , complex } , ")" ;
    rterm     = "term" , "(" , "P" , "=" , "[" , [ complex , { "," , complex } ] , "]" , "," , "A" ,
                "=" , complex , "," , "B" , "=" , complex , "," , "C" , "=" , complex , ")" ;
    rfun_v    = "rfun" , "(" , [ rterm , { "," , rterm } ] , ")" ;
    char_v    = "char" , "(" , "q" , "=" , uint , "," , "n" , "=" , uint , "," , "s" , "=" , complex , ")" ;

The start symbol of the parser of type `x` is `x_v`, and the whole input must be consumed. The languages of the
thirteen start symbols are pairwise disjoint, so the type of a text is determined by its syntax (9.7).

Meaning: `real` is the closed interval `[m - r, m + r]`, `m` the value of `dec`, `r` of `udec` (0 if absent);
`complex` is `re + im i`; `fin` is `a + N Zhat` (`N = 0` if `mod` is absent: the point `a`); `ucoset` is `c U(N)`
(`N = 0`, an exact unit, if `mod` is absent); `lcoord` is the exact rational if no `O`-term, else the ball
`a + p^N Z_p` (`O(p)` means `N = 1`); the idele `(x ; r * [c mod N])` is `x_inf = x`, finite part `r c U(N)`;
the class `<t ; u>`; the partial ball lists its places; the qclass is the class modulo `Q` of an adele, or the
union of pieces with the gluing rule; `ffun(D=, M=; f_0, ..., f_{L-1})` has `f_j = f(j/D)`; a term is
`P(x) exp(-pi A x^2 + B x + C)` with `P = [c_0, c_1, ...]`, `c_k` the coefficient of `x^k` (`P = []` is the zero
polynomial; exact trailing zero coefficients are removed on input, 5.12, gate finding G7); `char(q=, n=, s=)` is
`t^s chi(u')` with the Conrey label `n` modulo `q`.

### 9.3 Semantic constraints

| Where | Constraint | Status if violated |
|---|---|---|
| every `/` | denominator not 0 | `DOMAIN` |
| `ucoset` | `N >= 1`: `gcd(c, N) = 1`; `N = 0` or absent: `c = 1` or `c = -1` | `DOMAIN` |
| idele | `r > 0`; the exact interval of `real` excludes 0 | `DOMAIN` |
| idele class | the exact interval of `real` lies in `(0, infinity)` | `DOMAIN` |
| `lball_v`, `sentry` with `p=` | `p < 2^64` | `UNSUPPORTED` |
| | `p` prime (`n_is_prime`, `ulong_extras.h:335`) | `DOMAIN` |
| | the base inside `O(...)` equals `p`; `abs(N) <= max_prec` (the latter is stage 4: `LIMIT`) | `DOMAIN` |
| `sball_v` | at most one `inf` entry; no prime twice | `DOMAIN` |
| `qclass_v`, form `union` | each piece: the midpoint of `real` in `[0, 1]` (5.10, CV-45); the finite part's canonical triple has `d = 1` and the centre range of 5.2 (5.10) | `DOMAIN` |
| `ffun_v` | `D >= 1`, `M >= 1`; `D M <= max_items` (stage 4: `LIMIT`); number of values `= D M` | `DOMAIN` |
| `rterm` | the exact interval of the real part of `A` lies in `(0, infinity)` | `DOMAIN` |
| `char_v` | `q < 2^64` (`UNSUPPORTED`); `q >= 1`; `gcd(n mod q, q) = 1` | `DOMAIN` |
| decimals | `abs(exponent) <= max_exp10` (stage 4) | `LIMIT` |

The sign conditions (idele, class, `A`) are decided exactly on the decimal interval. A C parser that then rounds
the interval outward at `prec` checks the condition again on its `arb`; if it fails only there, the status is
`ADF_NOT_DETERMINED` (stage 7), and a higher `prec` will succeed. The condition on pieces concerns the midpoint;
a C parser checks it on the exact decimal midpoint and stores a midpoint rounded to nearest, which stays in `[0, 1]`
because 0 and 1 are representable.

Canonicalisation performed by the parser (non-canonical but valid input): leading zeros removed; fractions reduced;
`-0` becomes `0`; a finite centre reduced into `[0, N)`; `mod 0` dropped; a bare `a mod N` becomes `(* ; a mod N)`;
unit residues reduced into `1..N` (the stored modulus stays as written, CV-17; the printed one is normal); a local
centre reduced to its canonical representative (5.8), `O(p)` printed `O(p^1)`; partial-ball entries sorted by place;
duplicate pieces removed; a character lowered to its primitive character; `n` reduced; exact trailing zero
coefficients of `P` removed (5.12); decimals rewritten by 9.5.

### 9.4 Printing templates

`q(x)` prints a rational: `n` if the denominator is 1, else `n/d`, in lowest terms, `-` only before a negative
numerator, `0` for zero. `r(x)` prints a real ball by 9.5, `z(x)` a complex ball as `(r(re)) + (r(im))*i`.

| Type | Canonical text |
|---|---|
| `adf_rat` | `q(x)` |
| finite part `F` | `q(a)` if `N = 0`; else `q(a) mod q(N)` with `a` the centre in `[0, N)` |
| `adf_fball` | `(* ; F)` |
| scaled value (5.4) | the finite ball it denotes: `(* ; F)` with centre `s u` and radius `s K`, or `(* ; q(s))` if exact. The value form does not record the policy or the context; a scaled value is read by reading an `adf_fball` and converting it (which reports loss, 3.2) |
| `adf_adele` | `(r(x_inf) ; F)` |
| `adf_cadele` | `(z(x_inf) ; F)` |
| `adf_ucoset` | normal form (5.6): `[c mod N]`, `1 <= c <= N`; exact units `[1]` and `[-1]` |
| `adf_idele` | `(r(x_inf) ; q(r) * U)`, `U` the unit coset as above; `q(r)` also when `r = 1` |
| `adf_idclass` | `<r(t) ; U>` |
| local coordinate `L` | exact: `q(p^v u)`; ball: `q(c) + O(p^N)` with `c = p^v u` and `N` in signed decimal (`O(5^-2)`, `O(5^0)`, `O(5^1)`) |
| `adf_lball` | `[p=P: L]` |
| `adf_sball` | `{E; E; ...}` in canonical place order; `E` is `inf: r(x)` (real tag), `inf: z(x)` (complex tag) or `p=P: L`; no places: `{}` |
| `adf_qclass` | lift: `(r ; F) + Q`; pieces: `union(X, X, ...) + Q`, `X = (r ; F)`, sorted like 5.10 but by the exact end points of the *printed* real parts, each distinct printed piece once (printing can make two pieces equal or change their order; this keeps the text a fixed point) |
| `adf_ffun` | `ffun(D=D, M=M; z(f_0), z(f_1), ...)` |
| `adf_rfun` | `rfun(T, T, ...)`, `T = term(P=[z(c_0), ...], A=z(A), B=z(B), C=z(C))` with no exact-zero last coefficient (`P=[]` for the zero polynomial); zero function `rfun()` |
| `adf_char` | `char(q=q, n=n, s=z(s))` |

Examples (all in `tests/golden/`): `7/3`; `(3.14159 +/- 1e-5 ; 5/3 mod 6)`; `(* ; 2 mod 6)`; `[p=5: 3 + O(5^4)]`;
`(2.5 +/- 1e-9 ; 3/2 * [5 mod 36])`; `<1.25 +/- 1e-30 ; [5 mod 36]>` (`PLAN.md` 5, all canonical); `[5 mod 6]`
prints as `[2 mod 3]`.

### 9.5 Real balls: reading and printing

**Reading.** `dec +/- udec` denotes the exact rational interval `[m - r, m + r]`; without `+/-` the point `m`.
The Python reference keeps this interval exactly. A C parser produces an `arb` at `prec` that contains the interval
(enclosure: the contract). If `m` is a dyadic number whose odd mantissa has at most `prec` bits and `r` is a dyadic
number whose odd mantissa is below `2^30` (`MAG_BITS`, `mag.h:117`), the `arb` is exactly `[m +/- r]` (tightness:
a tested requirement). A decimal string is an enclosure on input, not a lossless form (`SPEC.md` 10.2).

**Printing.** Input: exact rationals `mid` and `rad >= 0` (for an `arb`, its dyadic midpoint and radius), and
`n = digits`. Notation: `X(y) = floor(log10 |y|)` for `y != 0`; `ceil2(E)` for `E > 0` is `E` rounded up to two
significant digits, `ceil(E / 10^(X(E) - 1)) * 10^(X(E) - 1)`; `round(y, q)` is `y` rounded to the nearest multiple
of `10^q`, ties to the even multiple.

1. If `rad = 0` and `mid` is a decimal with at most `n` significant digits (or 0): print `fmt(mid)`. Stop.
2. If `mid = 0`: print `0 +/- fmt(ceil2(rad))`. Stop.
3. `q = X(mid) - n + 1`; if `rad > 0`, `q = max(q, X(rad) - 1)`.
4. `M = round(mid, q)`; `R = ceil2(rad + |M - mid|)`; `q' = X(R) - 1` if `M = 0`, else
   `max(X(M) - n + 1, X(R) - 1)`.
5. If `M` is not a multiple of `10^q'`, set `q = q'` and repeat step 4.
6. Print `fmt(M) +/- fmt(R)`.

`fmt(y)` prints an exact decimal: `0` for zero; otherwise write `|y| = D 10^E` with `D` a positive integer not
divisible by 10, `k` the number of digits of `D`, `X = E + k - 1`. If `-4 <= X <= 20`, positional notation with no
superfluous zeros (`2.5`, `0.0001`, `1250`, `100000000000000000000`); else `d.ddd` `e` `X` with the digits of `D`, a
point only if `k > 1`, and the exponent without `+` and without leading zeros (`1e-5`, `1.5e-30`, `3e21`). A minus
sign precedes a negative number.

Properties (checked by the exact-rational reference parser; a C parser reads a decimal as a rounded ball and its
round trip is in 9.6, gate finding G4): the printed interval `[M - R, M + R]` contains `[mid - rad, mid + rad]`,
since `R >= rad + |M - mid|`; the loop of step 5 ends, because `q` increases strictly, and once `q >= X(rad) + 1`
(if `rad > 0`) and `q >= X(mid) - n + 2` the next `q'` is at most `q` (then `|M - mid| <= 10^q / 2` and
`rad < 10^q` give `X(R) <= q`, and `X(M) <= max(X(mid) + 1, q)`); printing is idempotent on its own output (a
printed text re-read and printed again is the same text), because the final `M` is a multiple of `10^q(M, R)` and
`R` has two significant digits.

**Constrained printing.** Where the type requires a sign condition of the real ball (the real part of an idele
excludes 0; the real part of a class and the real part of `A` in a term are positive), the printed interval must
satisfy it too, or the printed text could not be read back. The algorithm then takes a parameter `k >= 2`, the
number of significant digits of the radius: `ceil_k(E) = ceil(E / 10^(X(E) - k + 1)) * 10^(X(E) - k + 1)` replaces
`ceil2`; `X(rad) - k + 1` and `X(R) - k + 1` replace `X(rad) - 1` and `X(R) - 1` in steps 3 and 4; and
`n + k - 2` replaces `n` in steps 1, 3 and 4 (so `k = 2` is the unconstrained algorithm). The printer uses the least
`k >= 2` for which `[M - R, M + R]` satisfies the condition. Such a `k` exists: the exact interval satisfies the
condition with a positive margin (it is closed and the condition is open), and the printed interval exceeds the
exact one by at most `10^q + 10^(X(E) - k + 1)`, where `q = max(X(mid) - n - k + 3, X(rad) - k + 1)` in the first
pass and the loop of step 5 raises `q` only to `max(X(M) - n - k + 3, X(R) - k + 1)`; all of this tends to 0 as
`k` grows. Example: `(1 +/- 0.999999 ; 1 * [1])` needs `k = 6`, since `k = 2` would print `1 +/- 1`, which
contains 0.

The least level of a printed text can be lower than that of the value it came from (the reference found
`mid = 20396493/16` with a radius near `1.258e6`: level 4 prints `1275000 +/- 1259000`, whose least level is 3).
So the constrained printer **repeats**: print at the least level, read the text back exactly, print again, until
the text does not change; the result is that text. This ends: printing at a fixed level `k` is idempotent (the
argument above, with `k` for 2), so the least level of the text read back is at most the level used, and a
repetition happens only when it is strictly smaller; the levels decrease from the first one towards 2. The result
satisfies the condition, contains the value (each step is an enclosure) and is a fixed point of printing.

DECISION CV-29 (**replaced** by gate finding G4): this algorithm, with default `n = 20`; reason: it is exactly
specified (two implementations print the same text for the same `arb`), it never loses the enclosure, and like
`arb_get_str` (`arb.h:168`) it drops midpoint digits that the radius makes meaningless. The replaced part is the
C fixed-point promise of 9.6, which is false (G4); the algorithm itself is kept.

### 9.6 Round trips

- Types without a real or complex part (`adf_rat`, `adf_fball`, `adf_ucoset`, `adf_lball`, and `adf_sball` without
  archimedean place): `print(parse(t))` is canonical, and `parse(print(v))` equals `v` as a set; it is identical
  to `v` except for the unit modulus, which is printed normal (CV-17) and for the backend, which the value form
  does not carry.
- With real or complex parts, parsing a printed value encloses the original stored value, provided the limits of the reader admit the printed text (a value read at the limit `max_exp10` may print with an exponent one above it) and the printer did not refuse the value (M1-D6: a binary exponent above `ADF_PRINT_EXP_MAX` in absolute value gives NULL). The print-read-print
  fixed-point statement applies only to the exact-rational reference parser. Repeated C value-text round trips may
  widen the value and change its text on every pass. Dump text is the identity-preserving form. Constrained
  printing preserves the exact decimal interval's sign; at a specified C precision the parser may still return
  `NOT_DETERMINED` as in 9.3 (gate finding G4).
- Every expected output in `tests/golden/` is a fixed point of the reference: re-reading and printing it gives it
  back.

### 9.7 Type of a text

`adf_text_classify` returns an ADF status and writes an `adf_text_kind` through an output pointer only on
`ADF_OK` (gate finding G14). The enum is defined in the public header with one named constant for each supported
start symbol:

    typedef enum { ADF_TEXT_RAT, ADF_TEXT_FBALL, ADF_TEXT_ADELE, ADF_TEXT_CADELE, ADF_TEXT_UCOSET,
                   ADF_TEXT_IDELE, ADF_TEXT_IDCLASS, ADF_TEXT_LBALL, ADF_TEXT_SBALL, ADF_TEXT_QCLASS,
                   ADF_TEXT_FFUN, ADF_TEXT_RFUN, ADF_TEXT_CHAR } adf_text_kind;
    int adf_text_classify(adf_text_kind * kind, const char * s, size_t len, const adf_text_limits_t * lim);

It checks syntax only (stages 1 to 3): `ADF_PARSE` if no start symbol derives the text, `ADF_LIMIT` if
`len > max_len`. Semantic errors are found by the typed parser. The driver uses it; the typed parsers do not
coerce between types (`7/3` is not accepted by `adf_fball_set_str`). DECISION CV-30 is **replaced** by G14: one
text, one type, and the kind is a result channel distinct from the status.

### 9.8 Ambiguities of `PLAN.md` section 5, resolved

| # | Question left open | Resolution |
|---|---|---|
| A1 | How does an exact finite ball print? | `(* ; 7/3)`: the `mod` part is dropped for radius 0 (CV-31) |
| A2 | Is `7/3` a rational, a finite ball or an adele? | a rational (`adf_rat`); the finite ball is `(* ; 7/3)`, the adele needs a real part (9.7) |
| A3 | May a finite ball be written without `(* ; ...)`? | yes on input, `a mod N`; printed with it (CV-32) |
| A4 | Syntax of real balls | `m +/- r` only; no `[a, b]`, no `±`, no `inf`/`nan`; point without `+/-` (9.1, 9.2) |
| A5 | Which digits are printed for a real ball? | the algorithm of 9.5 (CV-29) |
| A6 | Precision of reading a decimal | the `prec` argument; containment is the contract, exactness for dyadic input a requirement (9.5) |
| A7 | Residue range of unit cosets, the modulus 1, exact units | `1..N`; `[1 mod 1]` is all units; `[1]`, `[-1]` are exact (CV-15, CV-16) |
| A8 | Is the scale of an idele printed when it is 1? | yes, `1 * [..]` (CV-33) |
| A9 | Local ball: which centre; how is `N` printed; exact local values | centre in `Z[1/p] ∩ [0, p^N)` (5.8); `O(p^N)` always with the exponent; exact value without `O`-term (CV-34) |
| A10 | Whitespace and case | 8.2, 8.3 |
| A11 | Does the value form record the backend? | no (`PLAN.md` 5); the dump does |
| A12 | Types without an example in `PLAN.md` 5 (`adf_cadele`, `adf_sball`, `adf_qclass`, `adf_ffun`, `adf_rfun`, `adf_char`) | forms of 9.2 (CV-35); places are labelled `p=5` and `inf` (seams R9, D8; version 0.1 used `R:` and `C:`) |
| A13 | Does the parser keep the unit modulus as written? | yes; printed normal (CV-17) |
| A14 | Negative radius, negative modulus, negative scale | excluded by the grammar (`udec`, `urat`, `uint`): `ADF_PARSE` |
| A15 | Zero denominator, composite `p`, non-unit residue | `ADF_DOMAIN` (9.3) |
| A16 | Leading zeros | accepted, never printed (CV-36) |

## 10. Dump form

### 10.1 Grammar

Tokens are separated by exactly one space `0x20`; no other whitespace anywhere; no leading or trailing space.

    dump    = "adf" , version , " " , field , " " , body ;   (* version "1"; any other version: UNSUPPORTED *)
    field   = upper , { nonspace } ;                  (* "Q" in version 1; any other field: UNSUPPORTED *)
    h       = "0" | [ "-" ] , hnz , { hdig } ;           (* integer in lower-case hexadecimal, no leading zeros *)
    hnz     = "1" | ... | "9" | "a" | ... | "f" ;   hdig = "0" | hnz ;
    arb     = h , " " , h , " " , h , " " , h ;   (* mid mantissa, mid exponent, rad mantissa, rad exponent *)
    acb     = arb , " " , arb ;
    arch    = h , { " " , arb } ;                       (* count of archimedean components, then real balls *)
    carch   = h , { " " , acb } ;                       (* count, then complex balls *)
    ctx     = h , " " , h , { " " , h } ;                (* K, k, then q_1 ... q_k *)
    fb      = "g" , " " , h , " " , h , " " , h          (* A H d *)
            | "l" , " " , h , " " , ctx , { " " , h } ;  (* d, context, then res_1 ... res_k *)
    lb      = h , " " , ( "x" , " " , h , " " , h , " " , h      (* p; exact: num(u) den(u) v *)
                        | "b" , " " , h , " " , h , " " , h ) ;  (* p; ball: u v N *)
    body    = "rat" , " " , h , " " , h
            | "fball" , " " , fb
            | "scaled" , " " , ( "x" , " " , h , " " , h | "s" , " " , h , " " , h , " " , h ) , " " , ctx
            | "adele" , " " , arch , " " , fb
            | "cadele" , " " , carch , " " , fb
            | "ucoset" , " " , h , " " , h                          (* c N *)
            | "idele" , " " , arch , " " , h , " " , h , " " , h , " " , h   (* inf, num(r) den(r), c N *)
            | "idclass" , " " , arb , " " , h , " " , h
            | "lball" , " " , lb
            | "sball" , " " , ( "n" | "r" , " " , arb | "c" , " " , acb ) , " " , h , { " " , lb }
            | "qclass" , " " , ( "lift" , " " , arch , " " , fb | "pieces" , " " , h , { " " , arch , " " , fb } )
            | "ffun" , " " , h , " " , h , { " " , acb }             (* D M, then D M values *)
            | "rfun" , " " , h , { " " , h , { " " , acb } , " " , acb , " " , acb , " " , acb }
            | "char" , " " , h , " " , h , " " , acb               (* q n s *)
            | "modctx" , " " , ctx ;

A count fixes the number of repetitions that follow it (the archimedean count in `arch` and `carch`; `k` in `ctx`
and in `fb`; the number of primes in `sball`;
of pieces in `qclass` (form `pieces`); `D M` values in `ffun`; the number of terms in `rfun`, and in each term the
length of `P`, which may be 0 (5.12)); a mismatch is `ADF_PARSE`. For `Q` the archimedean count must be 1
(`ADF_DOMAIN` otherwise).
The field descriptor and the archimedean count follow seams R9 and R3 (D8): a later field (`K`, `F3(T)`) can be
added without breaking version-1 files, and version 1 rejects it with `ADF_UNSUPPORTED`. `idclass` has no count (the
type is special to `Q`); `sball` keeps its tag `n`, `r`, `c`. DECISION CV-37 (**replaced** by gate findings G3 and
G7; the grammar itself is kept): this dump grammar; reason: one token per struct field, so that load and dump are a
direct transcription and identity is easy to test.

### 10.2 Rules

- **Strict.** The loader accepts only canonical text: the fields must satisfy the predicates of section 5
  (`ADF_DOMAIN` otherwise; the loader never canonicalises). For a local `fball` the predicate is `L` of 5.3 (raw
  data, no gcd condition): `adf1 Q fball l 2 6 2 2 3 0 0` is valid and denotes `(0 + 6 Zhat)/2 = 3 Zhat`. Dumping a
  loaded value gives back the same text, byte for byte. DECISION (proposed) CV-38; reason: `PLAN.md` 5 requires "the
  identical object", and a loader that repaired input would make two texts denote one dump.
- **Real balls** are written as `arb_dump_str` (`arb.h:1088`) writes them. **[probed]** on FLINT 3.0.1: four
  lower-case hexadecimal integers, midpoint mantissa and exponent, radius mantissa and exponent, the value being
  `mantissa * 2^exponent`, each mantissa odd (or `0 0` for zero); `arb_dump_str` of 0.5 is `1 -1 0 0`, of -0.75 is
  `-3 -2 0 0`. Non-finite values use a zero mantissa with a non-zero exponent (`+inf` as `0 -1`, `nan` as `0 -3`).
  The documentation agrees and leaves the special values unspecified: "The format consists / of four hexadecimal
  integers representing the midpoint mantissa, / midpoint exponent, radius mantissa and radius exponent (with
  special / values to indicate zero, infinity and NaN values), / separated by single spaces. The returned string
  needs to be deallocated / with *flint_free*." (`flint-3.0.1:arb.rst:288-293`). Our loader accepts an `arb` field
  only if both mantissas are odd or the pair is `0 0`, and the radius mantissa is positive and below `2^30`
  (`mag.h:117`); anything else is `ADF_DOMAIN` (non-finite or not canonical).
- **The loader validates the whole text before any FLINT load function sees any of it.** DECISION CV-52 (D9); reason
  (orchestrator; finding F5): FLINT's `arb_load_str` aborts the process on some malformed strings. `arb_load_str`
  (`arb.h:1087`) "Returns a nonzero value if *str* is not formatted correctly" (`flint-3.0.1:arb.rst:297-298`), but
  the probe below shows that it does not always return. **[probed]** In FLINT 3.0.1 `arb_load_str` aborts the
  process (`SIGABRT`) on `"1 0 0 0 5"` (a fifth token), on a zero midpoint mantissa with an exponent other than `0,
  -1, -2, -3` (`"0 5 0 0"`, `"0 1 0 0"`, `"0 -4 0 0"`), on a zero radius mantissa with an exponent other than `0,
  -1` (`"1 0 0 1"`, `"1 0 0 -2"`), and on an odd radius mantissa of more than 30 bits (`"1 0 40000001 0"`, `"1 0
  7fffffff 0"`). It accepts and silently changes `"2 0 0 0"` (to `1 1 0 0`), `"A 0 0 0"` (to `5 1 0 0`), `"1 0 -1
  0"` (a negative radius mantissa becomes positive) and `"1 0 40000000 0"`, and accepts a trailing space. Every
  `arb` group of every valid vector in `tests/golden/dump.tsv` loads and dumps back identically. A loader may build
  the `arb` from the validated tokens directly, or pass exactly the four validated tokens; the validation of this
  section excludes every aborting case found.
- **Polynomial coefficients** (gate finding G7): the stored `P` length of a term must already be the normalized
  length of 5.12: a nonempty coefficient list whose last coefficient is the exact zero ball is `ADF_DOMAIN`, and
  length 0 is the zero polynomial. The loader never trims.
- **Contexts** (gate finding G3). A local `fball` (also inside `adele`, `cadele`, `qclass`) and a scaled value
  record their context (`ctx`). A dump loader takes an array of caller-owned context bindings and its length.
  There is one binding per local-fball or scaled occurrence, in dump traversal order, including nested pieces.
  Every binding must match that occurrence's modulus and ordered blocks. Repeated occurrences may share a
  pointer. A validation/inspection function reports the number of occurrences and their context descriptors
  without constructing a value. The caller explicitly constructs any missing contexts before loading. Value
  fields and backend are restored exactly relative to those bindings. `identical()` with the original value
  additionally requires binding each occurrence to its original context pointer. A binding count other than the
  number of occurrences, a `NULL` binding, or a binding that does not match its occurrence is `ADF_DOMAIN`.
  The one-context call `adf_x_load_str(x, s, len, ctx, lim)` remains a convenience for dumps whose occurrences
  all use that context: it stands for the array with `ctx` at every occurrence (a dump without contexts needs no
  binding, and any `ctx` is accepted). The inspection function and the descriptor are:

        typedef struct { fmpz_t K; slong k; ulong * q; } adf_ctx_desc_t;   /* one context occurrence */
        int adf_x_dump_inspect(size_t * nctx, adf_ctx_desc_t * descs,
                               const char * s, size_t len, const adf_text_limits_t * lim);

  `adf_ctx_desc_init(adf_ctx_desc_t *d)` and `adf_ctx_desc_clear(adf_ctx_desc_t *d)` return void;
  init sets K=1, k=0, q=NULL; clear releases its owned integer and block array.
  With descs=NULL, ignore the incoming *nctx and write the occurrence count only on OK.
  Otherwise incoming *nctx is capacity in initialized descriptors. Validate the whole dump first;
  insufficient capacity returns ADF_LIMIT with *nctx and every descriptor untouched.
  On OK replace the first required descriptors, releasing their old contents, write that count to *nctx,
  and leave the rest untouched. Other failures also leave all outputs untouched. Statuses are loader statuses.
  `adf_modctx_new_from_dump(out, s, len, occurrence, lim)` constructs the context
  recorded at one occurrence: `occurrence` is 0-based in dump traversal order, a `modctx` dump has exactly one,
  and an index out of range is `ADF_DOMAIN`. Validate the whole dump in the order of 8.5 before checking the
  occurrence index or allocating a context. DECISION CV-39 is **replaced** by G3: the caller owns contexts
  (CV-10), so the loader cannot create one behind the caller's back, and one context does not cover all valid
  dumps.
- **Unit moduli** are dumped as stored (as supplied, CV-17).
- **Version and field.** This is version 1, written `adf1`, for the field `Q`: every dump starts `adf1 Q `. A
  reader of version 1 rejects any other version and any other field with `ADF_UNSUPPORTED`. A later version must be
  able to read `adf1`.

## 11. Golden vectors

### 11.1 File format

DECISION (proposed) CV-46: the following format; reason: plain text, one vector per line, readable by C and Python
with a few lines of code, and able to carry any byte string.

- Files `tests/golden/*.tsv`, ASCII, LF line ends. A line starting with `#` is a comment; an empty line is ignored.
- A vector is `input<TAB>expected`, exactly one TAB.
- `expected` is the canonical output text, or `!` followed by a status name without `ADF_` (`!PARSE`, `!DOMAIN`,
  `!LIMIT`, `!UNSUPPORTED`, `!NOT_DETERMINED`).
- In `input` a backslash starts an escape: `\\` backslash, `\t` TAB, `\n` LF, `\r` CR, `\xHH` the byte with
  hexadecimal value `HH`. Any other backslash sequence is an error in the file.
- An input of the form `@gen:PREFIX|UNIT|COUNT|SUFFIX` (each part escaped as above; a literal `|` is `\x7c`) stands
  for `PREFIX` followed by `COUNT` copies of `UNIT` followed by `SUFFIX`; it is used for inputs longer than a line
  should be.

### 11.2 Files

| File | Input | Expected |
|---|---|---|
| `<type>.tsv` for the thirteen types of 9.2 | a text given to the parser of that type (limits of 8.4, defaults) | canonical text or status |
| `dispatch.tsv` | a text | the type name (`rat`, `fball`, ...) of 9.7, or a status |
| `realball_read.tsv` | a real ball in the value form | the exact interval as `lo hi` (two rationals in lowest terms), or a status |
| `realball_print.tsv` | `mid rad digits`, `mid` and `rad` dyadic rationals | the text printed by 9.5 |
| `dump.tsv` | a dump text | the same text if valid, or a status |
| `psi_phases.tsv` | a finite ball in the value form | the angles `t` in `[0, 1)` of the finite phases `E(t)` of `psi` (6.1), increasing, or a status |
| `gauss.tsv` | an `adf_char` in the value form | `e=<parity> tau=<complex ball> W=<complex ball>` (6.4); computed by `lanes/m0-conventions/gen_gauss.py` (mpmath, 60 digits), not by hand; the balls enclose the exact values |

The counts are in `tests/golden/README.md`.

### 11.3 How a C test uses them

1. Status vectors: the status must be equal. The output must be untouched (4.3).
2. Types without real or complex parts: the printed text must be equal to `expected`.
3. Types with real or complex parts, read at `prec = 128`: every part that is not real or complex must print equal
   to the corresponding part of `expected`. For each real ball: if the input's decimal midpoint and radius satisfy
   the exactness condition of 9.5 at `prec = 128`, the printed ball must be equal to the expected one; otherwise the
   C `arb` must contain the exact interval of the input ball (as in `realball_read.tsv`), and its printed text,
   re-read exactly, must contain it too. The C test decides which case applies from the exact decimal it has read.
4. `realball_print.tsv`: set the `arb` exactly (every vector has a midpoint with at most 128 bits and a radius
   mantissa below `2^30`) and compare the printed text. Which FLINT call sets the radius exactly matters here.
   The FLINT documentation promises only an upper bound for all three candidates: `arf_get_mag` (`arf.rst:403-405`),
   `mag_set_fmpz_2exp_fmpz` and `mag_set_ui_2exp_si` (`mag.rst:147-151`; `mag_set_fmpz`, `mag.rst:131-134`, says it
   "may be inexact even if x is exactly representable"). Probed with a mantissa of 30 bits
   (`lanes/m1-text/report.md`, "Bugs found", item 1): `arf_get_mag` and `mag_set_fmpz_2exp_fmpz` add one unit
   and are not exact; `mag_set_ui_2exp_si` is exact for a mantissa below `2^30` (probed on 1, 3, 2^29, 2^29 + 1, 2^29 + 7,
   2^30 - 1; not documented, checked by `tests/test_text_adele.c`). The dump loader builds the `mag` from its
   mantissa and exponent fields and uses none of the three (`src/dump.c`, `dp_mag_set_exact`).
5. `dump.tsv`: load with one binding per context occurrence (10.2), then dump; the text must be identical;
   loading an invalid dump must leave the output untouched and return the status.
6. `psi_phases.tsv`: the default enclosure must contain every listed phase. For a single phase, bound numerical
   error by a shrinking precision-dependent tolerance. For several phases, compare with the rectangular hull of
   all listed phases and bound only the excess due to numerical evaluation. The unavoidable width of that hull
   is not an error. The strict variant returns `ADF_OK` exactly for singleton images (gate finding G10).
7. `gauss.tsv`: the balls computed by `adf_char_gauss_sum` and `adf_char_root_number` must overlap the expected
   balls, have radius below `2^-60` at `prec = 128`, and the parity must agree.

## 12. Foreign-function interface

Added on 2026-09-27 at the request of TJO: a Julia layer will call the C library through `ccall`. Each rule is a
contract of the public interface. Python stays for tests and proof checks only.

1. **Every public operation is an exported function.** No operation exists only as a macro or only as a
   `static inline` function. Inline fast paths may exist in addition, in the FLINT manner: the header defines
   `ADF_INLINE` as `static inline` (FLINT writes `static __inline__`, `fmpz.h:15-19`; the headers of this library use
   the C99 keyword on purpose, so that no compiler extension is used: `include/adelefeld/common.h:36-44`,
   `lanes/m1-common/report.md`, "Findings against the specification"), and one source file defines `ADF_INLINES_C` so that the same functions are
   also compiled as exported symbols (FLINT: `fmpz.h:15-19`; **[probed]** `nm -D libflint.so` lists `fmpz_init`,
   `fmpq_init`, `arb_init`, `arb_swap` as exported). Accessor macros (as `arb_midref`) have function equivalents.
   A test lists the exported symbols with `nm -D` and compares them with the header's function declarations.
2. **No variadic functions.** Printing takes explicit arguments; there is no `adf_printf`.
3. **Status is a plain `int` return value** with the numeric values of 3.1; predicates return `int` 0 or 1;
   comparisons return `int` with the values of 2.1. No status is returned through `errno` or a global. An extra
   result is written through an output pointer and only on `ADF_OK` (`adf_text_kind` of 9.7, gate finding G14).
4. **Structs.** Value types have a documented fixed layout (section 5), so that a binding can allocate them inline
   as Nemo.jl does for FLINT types. DECISION (proposed) CV-40: value structs are public with fixed layout, and the
   library exports `size_t adf_sizeof_<type>(void)` for every type, which a binding compares with its own layout at
   load time; reason: inline allocation avoids a heap object per number, and the size check turns a layout mismatch
   into an error at load time instead of memory corruption. The modulus context is **opaque and incomplete**
   (gate finding G2, replacing CV-41): its fields and layout are not part of the interface, there is no public
   by-value or array-of-one context type, and a binding obtains every context from the exported
   `adf_modctx_new_*` constructors and releases it with `adf_modctx_free` (4.6, 5.14). Version 1 offers no inline
   context allocation. Reason (CV-41): the context holds internal tables (reduction constants, recombination
   tree) whose layout should be free to change.
5. **Places** are the 8-byte struct `adf_place_t` of section 7, passed by value; a binding treats it as an opaque
   bits type and creates it only through `adf_place_inf` and `adf_place_prime`.
6. **Contexts are explicit; no hidden global state** (4.5, 4.6). A binding must keep a context alive as long as any
   value refers to it (in Julia: the value object holds a reference to the context object).
7. **No callbacks.** Version 1 has no function-pointer parameters. If a later function needs one (an integrand), it
   takes a plain C function pointer and a `void *` user-data argument, never a closure.
8. **Strings.** Input: `(const char *, size_t)`, never retained (8.1). Output: `char *` from `flint_malloc` with
   its byte length written through a `size_t *` output and a terminating NUL at `s[len]` (gate finding G14);
   freed by the caller with the exported `adf_str_free(char *)`, which calls `flint_free` (`flint.h:204`).
   DECISION (proposed) CV-43; reason: a binding need not locate FLINT's allocator.
9. **Arrays.** An array output is either caller-allocated with its length as the next argument, or owned by an
   output value (5.9 to 5.12) and freed by that value's `clear`. No function returns a bare heap array.
10. **Integer types.** `slong` and `ulong` are FLINT's `mp_limb_signed_t` and `mp_limb_t` (`flint.h:110-111`),
   64 bits on the supported platforms (`FLINT_BITS 64`, `flint.h:193`). DECISION (proposed) CV-42: version 1
   supports 64-bit platforms only; reason: places, blocks and primes are one word (CV-18), and the layouts below
   are stated for 64 bits. `size_t` is used for byte lengths and for binding counts, descriptor capacities and
   occurrence indices.
11. **FLINT types across the interface.** `fmpz`, `fmpq`, `arb`, `acb` and `padic` values may be passed by pointer,
    since Nemo.jl wraps the same types [unverified: Nemo.jl's FLINT version and layout were not read]. The interface
    assumes FLINT 3.0.1 (`flint.h:94-97`) and these layouts, measured on x86-64 Linux with the installed headers
    **[probed]**:

    | Type | Definition | Size (bytes) |
    |---|---|---|
    | `fmpz` | `typedef slong fmpz;` (`flint.h:510`); small values inline, large values a tagged pointer (`COEFF_IS_MPZ`, `fmpz.h:44`) | 8 |
    | `fmpq` | `{ fmpz num; fmpz den; }` (`flint.h:513-518`) | 16 |
    | `mag_struct` | `{ fmpz exp; mp_limb_t man; }` (`arb_types.h:21-26`) | 16 |
    | `arf_struct` | `{ fmpz exp; mp_size_t size; mantissa_struct d; }`, two inline limbs (`arf_types.h:22`, `:44-50`) | 32 |
    | `arb_struct` | `{ arf_struct mid; mag_struct rad; }` (`arb_types.h:32-37`) | 48 |
    | `acb_struct` | `{ arb_struct real; arb_struct imag; }` (`acb_types.h:21-26`) | 96 |
    | `padic_struct` | `{ fmpz u; slong v; slong N; }` (`padic.h:32-36`) | 24 |

    A binding must link against a FLINT whose `flint_version` string (`flint.h:105`) has the same major and minor
    version; the library exports `const char * adf_flint_version_compiled(void)` and checks `flint_version` against
    it in `adf_version_check(void)`, which returns `ADF_OK` or `ADF_UNSUPPORTED`. DECISION (proposed) CV-44; reason:
    a FLINT of another minor version may change these layouts.
12. **Memory of FLINT types inside our values** is FLINT's: our `clear` calls `fmpz_clear`, `arb_clear` and so on.
    A binding must not free the inner FLINT objects of an `adf` value itself.

## 13. Decisions, for review

"decided" means decided by the orchestrator on 2026-09-28 (D1 to D11) and accepted by the milestone-0 gate
review; "decided (gate)" means proposed here and accepted by that review; "replaced (Gn)" means rejected by it
and replaced by finding `Gn` of `docs/reviews/m0-gate/review.md` (whose text is in force, as amended by the edits
E1 to E4 and the findings C1 to C5 of the closure check `docs/reviews/m0-gate/closure.md`). 60 decisions: 52
decided (12 of them as D1 to D11), 8 replaced.

| Id | Section | Decision | Reason (short) | Status |
|---|---|---|---|---|
| CV-01 | 1 | statuses are ordered integers, not bit flags | one code per result | decided (gate) |
| CV-02 | 2.1 | no `_equal` for ball types; `_identical` for representation | point equality is undecided | decided (gate) |
| CV-03 | 3.1 | numeric values `OK 0` ... `LIMIT 10` in order of precedence | combination is a maximum | decided (gate) |
| CV-04 | 3.3 | combined status is the maximum; place is the first in canonical order | proved failures dominate; deterministic place | decided (gate) |
| CV-05 | 4.1 | outputs may alias inputs of the same type | FLINT habit; cheap | decided (gate) |
| CV-06 | 4.3 | outputs untouched on every non-OK status | retry at higher precision | decided (gate) |
| CV-07 | 4.3 | "enclosure with status" only at branch cuts; otherwise default and `_strict` variants | a status never carries a value except where `SPEC.md` asks | decided (gate) |
| CV-08 | 4.4 | a non-finite result is never stored; `NOT_DETERMINED` | invariants need finite balls | decided (gate) |
| CV-09 | 4.4 | non-canonical input is a precondition violation; checked in debug builds | gcd checks would dominate the kernels | decided (gate) |
| CV-10 | 4.6 | contexts caller-owned with stated lifetime, no reference count | as FLINT; no shared writes; no hidden state | decided (gate) |
| CV-11 | 4.6 | no operation creates a context; the implicit global fallback is only for `adf_fball` and types containing it | a created context would have no owner | **replaced (G1)** |
| CV-12 | 4.6 | a default `adf_scaled` operation requires the same context pointer in both inputs | block comparison per operation is too costly | **replaced (G1)** |
| CV-13 | 5.2 | init of a finite ball is the exact 0 | as `fmpq_init`, `arb_init` | decided (gate) |
| CV-14 | 5.4 | scaled value gets a field `exact` | the exact 0 of precision P6(2) must be stored; policies Definition 4 | decided (gate) |
| CV-15 | 5.6 | unit coset with `N = 0`, `c = 1` or `-1`, is an exact unit | needed by three lanes (catalogue P12, ideles P13.6, exact ideles) | **decided (D1)** |
| CV-16 | 5.6 | unit residues in `1..N` | no residue 0 for units; `[1 mod 1]` | decided (gate) |
| CV-17 | 5.6 | unit modulus stored as supplied; normal form for printing and equality | the dump keeps the modulus as supplied | decided (gate) |
| CV-18 | 7 | a place is an opaque handle; primes of places are one word | seams R1 | **decided (D8)** |
| CV-19 | 5.8 | init of a local ball is the exact 0 at `p = 2` | some prime must be chosen | decided (gate) |
| CV-20 | 5.11 | no normal form for `adf_ffun`, `adf_rfun` | minimality is undecidable on balls | decided (gate) |
| CV-21 | 5.14 | family contexts use prime-power blocks, increasing prime | factorisation known; prime-named operations | decided (gate) |
| CV-22 | 5.3 | local backend keeps `A = 0` | an unused field cannot be misread | decided (gate) |
| CV-23 | 5.4 | the absolute cap is an argument | only the radius is per value | decided (gate) |
| CV-24 | 5.10 | order of quotient pieces | unique printed form | decided (gate) |
| CV-25 | 8.1 | inputs are `(pointer, length)`; NUL inside is `PARSE` | no silent truncation | decided (gate) |
| CV-26 | 8.2 | ASCII alphabet; everything else `PARSE` | no look-alike characters | decided (gate) |
| CV-27 | 8.4 | limits `max_len`, `max_exp10`, `max_prec`, `max_items` with defaults | short input cannot demand huge work | decided (gate) |
| CV-28 | 8.5 | fixed order of checks | status independent of parser internals | decided (gate) |
| CV-29 | 9.5 | real-ball printing algorithm, 20 digits by default, constrained printing | exact and enclosing; idempotent only for the exact reference (G4) | **replaced (G4)** |
| CV-30 | 9.7 | classification by syntax; typed parsers do not coerce; kind is separate from status | one text, one type | **replaced (G14)** |
| CV-31 | 9.8 | exact finite ball prints `(* ; 7/3)` | no `mod 0` in output | decided (gate) |
| CV-32 | 9.8 | bare `a mod N` accepted as finite ball | natural input | decided (gate) |
| CV-33 | 9.8 | idele content printed even when 1 | uniform template | decided (gate) |
| CV-34 | 9.8 | local ball printing: canonical centre, explicit exponent, exact without `O` | unique text | decided (gate) |
| CV-35 | 9.2 | forms for `cadele`, `sball`, `qclass`, `ffun`, `rfun`, `char`; `P` has length `>= 0`, no exact-zero last coefficient | not in `PLAN.md` 5; matches `acb_poly` normalization | **replaced (G7)** |
| CV-36 | 9.8 | leading zeros accepted, never printed | friendly input, unique output | decided (gate) |
| CV-37 | 10.1 | dump grammar: one token per field, hexadecimal; `P` lengths may be 0 and must be normalized | direct transcription | **replaced (G3, G7)** |
| CV-38 | 10.2 | strict loader, never canonicalises | identical object | decided (gate) |
| CV-39 | 10.2 | the loader takes one binding per context occurrence; one context is a convenience | caller owns contexts; one context does not cover every dump | **replaced (G3)** |
| CV-40 | 12 | value structs have fixed layout; `adf_sizeof_<type>` exported | inline allocation, checked layout | decided (gate) |
| CV-41 | 12 | the modulus context is opaque and incomplete; `adf_modctx_new_*` and `adf_modctx_free` | internal tables may change | **replaced (G2)** |
| CV-42 | 12 | 64-bit platforms only | word places and blocks | decided (gate) |
| CV-43 | 12 | `adf_str_free` exported | bindings need not find FLINT's allocator | decided (gate) |
| CV-44 | 12 | FLINT version check exported | layouts may change between minor versions | decided (gate) |
| CV-45 | 5.10 | quotient pieces: midpoint in `[0, 1]`; the ball may exceed it by rounding; `k + 1` pieces | outward rounding (finding F2); quotient P6 | **decided (D4)** |
| CV-46 | 11.1 | golden file format | plain, escapable | decided (gate) |
| CV-47 | 5.4 | the cap never touches an exact value | `SPEC.md` 4.1; policies P14 | **decided (D2)** |
| CV-48 | 5.4 | scaled product: `SPEC.md` rule by default, `adf_scaled_mul_tight` for policies P10 | the rule stays; the tight one needs a gcd | **decided (D5)** |
| CV-49 | 5.6 | unit-coset power: `c^k U(N)` by default, `adf_ucoset_pow_tight` for ideles P13 | the tight modulus needs the table of P13 | **decided (D6)** |
| CV-50 | 5.7 | division of an adele by an idele: the smallest ball of ideles P19 | finer by up to a factor 2 | **decided (D7)** |
| CV-51 | 6.8 | reconstruction intersects with the closed real interval | a real ball is closed | **decided (D3)** |
| CV-52 | 10.2 | the dump text is validated completely before any FLINT load function | `arb_load_str` aborts (F5) | **decided (D9)** |
| CV-53 | 6.6 | reciprocity: two functions named by the exponent (`_exp_u`, `_exp_uinv`) | no source on disk for the words arithmetic and geometric | **decided (D10)** |
| CV-54 | 6.1 | transform against `conj(psi)` kept; conversion to the unconjugated transform recorded | Tate's original; the proofs use it | **decided (D11)** |
| CV-55 | 5.3 | raw local values: no gcd condition; comparison and printing through the canonical triple | policies P24, Summary 26: canonical data would force conversions after sums | decided (gate) |
| CV-56 | 7 | `adf_place_t` is a one-word struct, not a bare integer | the compiler enforces opacity | decided (gate) |
| CV-57 | 6.7 | seams R1 to R9 adopted: `inf` label, content, dump field `Q` and archimedean count, R6 wording, no canonical scale, named character convention | cheap now, avoids a break later | **decided (D8)** |
| CV-58 | 5.13 | `adf_char` value is `t^s chi(u')`; the L-function integral forms `conj(chi)` itself | the family as `SPEC.md` 5 writes it | decided (gate) |
| CV-59 | 6.1 | additive character functions `adf_adele_psi_tate`, `_strict` | named convention (seams R8) | decided (gate) |
| CV-60 | 6.4 | names `adf_char_gauss_sum` (tau), `adf_char_root_number`, `adf_local_gamma_at`, ... | `tau` and `G_minus` differ in sign | decided (gate) |

The gate review accepted CV-55 (raw local values: a departure from the canonical-data comment of `PLAN.md` 4,
with consequences for the C kernels), CV-06 (outputs untouched on failure: a cost in every function that can fail
after writing), CV-10 (caller-owned contexts), CV-16 (the residue range of unit cosets differs from
`proofs/ideles.md` Definition 8), CV-09 (undefined behaviour on non-canonical input) and CV-58 (the value
convention of `adf_char`) as written here. The eight rejected decisions carry the finding that replaces them in
the table above.

## 14. Findings against the specification, the plan and the proofs

Status after part B and the milestone-0 gate review. "Resolved" means a decision of the orchestrator settles it;
the amendment of `SPEC.md` and `PLAN.md` is done by another lane.

- **F1** (`SPEC.md` 5, `PLAN.md` 4 against `SPEC.md` 9.3.7, `proofs/catalogue.md` P12, `proofs/ideles.md` P13.6).
  Unit cosets were defined with `N >= 1`, but the exponent 0 gives "the exact 1", and the idele of an exact rational
  has an exact sign unit. Resolved by D1 (CV-15).
- **F2** (`SPEC.md` 6). "Pieces are closed intervals inside `[0, 1]`" cannot hold for `arb` enclosures when an end
  point is not dyadic: **[probed]** the enclosure of `[0.9, 1]` built as `arb_set_fmpq(19/20)` plus
  `arb_add_error(1/20)` has upper end above 1 at `prec` 20, 53 and 128 (the radius is a 30-bit number rounded up,
  `mag.h:117`). Resolved by D4 (CV-45). Also `SPEC.md` 6 "one piece for each integer it crosses" counts `k` pieces
  where the construction gives `k + 1` (`proofs/quotient.md` P6 remark); resolved by D4.
- **F3** (`PLAN.md` 4). `adf_scaled_struct` cannot hold an exact value, which `proofs/precision.md` P6(2) produces
  and `proofs/policies.md` Definition 4 keeps as an exact tag. Resolved by CV-14 (decided).
- **F4** (`SPEC.md` 4.1, "or is given a new context"). With caller-owned contexts no operation creates a context;
  version 1 uses the global fallback (CV-11), or a derived context passed by the caller. `proofs/policies.md` P22.4
  and P24.3 describe the derived contexts that this excludes by default. Gate finding G1 limits the implicit
  fallback to `adf_fball` and types containing it; scaled values use the target-context rule of 5.4.
- **F5** (FLINT 3.0.1). `arb_load_str` aborts the process on some malformed strings and silently changes others
  (**[probed]**, 10.2), although its documentation promises a nonzero return (`flint-3.0.1:arb.rst:297-298`).
  Resolved by D9 (CV-52).
- **F6** (`PLAN.md` 5). The value form of a real ball is lossy on output as well as input; printing an idele without
  regard to its sign condition could produce a text that reads back as `DOMAIN`. Handled by 9.5 (constrained
  printing) and 9.6 (after gate finding G4 the fixed-point property holds for the exact-rational reference only;
  C value-text round trips may widen).
- **F7** (`PLAN.md` 4, struct comment `docs/PLAN.md:60`: "`H > 0: 0 <= A < H, gcd(A,H,d) = 1`" for both backends).
  For the local backend `proofs/policies.md` P24 shows that the set of a local value stays in its context after
  cancellation while its canonical triple leaves it. Resolved: the gate review accepted raw local values (CV-55);
  the canonical-data comment of `PLAN.md` 4 is applied by another lane.
- **F8** (`proofs/ideles.md` Definition 8 against CV-16). The proof reduces unit residues into `[0, Nbar)` and
  writes the whole unit group `(0, 1)`; this document stores and prints residues in `1..N` and writes it `[1 mod
  1]`. The sets and the equality test are the same; only the representative differs. The gate review accepted
  CV-16 (residues `1..N`); `proofs/ideles.md` Definition 8 should follow.
- **F9** (`proofs/catalogue.md` P15 and `SPEC.md` 9.3.7 against D10). Both use the names "arithmetic" and
  "geometric" for the two cyclotomic conventions; no source on disk uses them (`docs/sources.md`, pending item 2).
  The functions are named by their exponent (CV-53); the proofs' statements are unaffected.
- **F10** (`SPEC.md` 5 against `proofs/analysis.md` P11). `SPEC.md` 5 writes the class-group quasi-characters as
  `t^s chi(u')`; the Tate integral of `L(s, chi)` uses `conj(chi(u'))`. Not a contradiction (the latter is the
  character of `conj(chi)`), but the specification should say which object the type stores; proposed CV-58.
- No statement of the reviewed proofs contradicts a decided convention of this document. The sign conventions of
  6.1 to 6.5 are those of `proofs/analysis.md`; `tau` has the positive finite sign and agrees with FLINT
  (`flint-3.0.1:acb_dirichlet.rst:362`, probed on 17 characters), while the local `G_minus` of analysis P9 has the
  negative sign; the two are named differently (CV-60).
