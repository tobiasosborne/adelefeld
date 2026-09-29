# adelefeld: implementation plan, version 1.3

Date: 2026-09-28 (version 1.0: 2026-09-27). Status: **milestone 0: gate passed after the listed edits, applied
on 2026-09-28; nothing of the C library is implemented.** Read `SPEC.md` first (what is
built) and `PERF.md` (how speed and size are judged). Version 1.3 applies the edits E1 to E4 and the findings
C1 to C5 of the milestone 0 gate closure check (`reviews/m0-gate/closure.md`). Version 1.2 applies the milestone
0 gate review
(`reviews/m0-gate/review.md`); version 1.1 applied the milestone 0 proof and convention rounds. Draft 1 applies the
design review `reviews/astra-2026-09-27/review.md` (findings P1-P3, D1-D4, and the consequences of M1-M13 and
F1-F6), and TJO's decision that elementary functions belong to the basic package. Draft 2 applies review round 2
(`reviews/astra-2026-09-27-r2/review.md`) and adds the catalogue of functions (`SPEC.md` 9.3.7). Draft 3 applies
review round 3 (R1 to R6). Section numbers of `SPEC.md` refer to its version 1.3.

**Change log of version 1.3** (2026-09-28): the milestone 0 gate closure check (`reviews/m0-gate/closure.md`) is
applied. Section 4: the scaled shared-pointer check covers `adf_scaled_mul_tight` and unary and exact-scalar
operations (E1); a context constructor overwrites only the pointer slot of `*out` and version 1 offers no inline
context allocation (E2); the set predicates return `int` 0 or 1 and point comparison returns `ADF_CMP_EQUAL = 0`,
`ADF_CMP_DIFFERENT = 1` or `ADF_CMP_UNDECIDED = 2` (C1); the `NOT_DETERMINED` summary covers a required result
invariant that the computation cannot certify (C4). Section 8 records the gate review and the closure check; the
status of milestone 0 is "gate passed after edits, applied on 2026-09-28".

**Change log of version 1.2** (2026-09-28): the milestone 0 gate review (`reviews/m0-gate/review.md`) is applied.
Section 4: contexts have explicit constructors and a free function (G2); the implicit global fallback is limited to
`adf_fball` and types containing it, and a default scaled binary operation requires a shared context pointer (G1).
Section 5: the C value text is an enclosure, not a fixed point, and the dump loader takes one context binding per
occurrence (G3, G4). Rows 1.7 and 1.8 record the scaled context rule and the raw/canonical distinction (G1, G11).
The pole status, and the string and predicate signatures, follow `SPEC.md` 9.3.7 and 10.4 (G6, G14).

**Change log of version 1.1** (2026-09-28): status of milestone 0 per work package (section 6); types of section 4
updated for the exact unit (decision M0-D1), the opaque place handle and the content (M0-D8, seams R1 and R5) and
the scaled value with an exact field; rows 1.4, 1.6, 1.7, 1.8, 1F.9, 2.1, 2.3, 2.4, 3.1 adjusted to the decisions
M0-D1 to M0-D12 of `SPEC.md` section 15.2; new row 1.9 for the Julia-friendly interface (M0-D12); section 8 records
the four cross-family proof reviews with their verdict counts; section 9 has two new risks. After `conventions.md`
0.2: the local backend holds raw data (finding F7, CV-55), the residue range of unit cosets follows the conventions
(F8, CV-16), and section 5 agrees with `conventions.md` sections 9 and 10 (exact units `[1]`, `[-1]`; place label
`inf`; dump header `adf1 Q` with the archimedean count). The ids M0-D1 to M0-D11 are the orchestrator's decisions D1
to D11 of 2026-09-27 and M0-D12 is TJO's decision D12 (`SPEC.md` 15.2 says why the prefix).

## 1. Principles

1. **Contracts before code.** Milestone 0 fixes the mathematics, the conventions and the storage invariants in
   writing, and has them reviewed. Until then, production C is limited to a provisional build scaffold with no
   public types; disposable probes of mathematics or of library behaviour, and the benchmark harness, are allowed.
   The public header is frozen after work packages 0.3, 0.4 and 0.6 are reviewed.
2. **Vertical slices.** Each later milestone ends with something a user can type and see.
3. **Tests.** A new behaviour gets a test that defines it; a defect gets a regression test that fails before the
   fix. Corrections of text need no test. Mutation testing is measured by surviving mutants, fuzzing by coverage.
4. **Every formula in `src/` cites a proof in `docs/proofs/` or a source on disk** by file and line.
5. **Enclosure is the contract.** If the inputs contain the true values, the output contains the true result.
   Tightness is a quality measure with its own tests.
6. **A row in `PERF.md` before "finished"**, under the benchmark contract of `PERF.md` section 7.
7. **No hidden state.** Contexts are explicit and immutable.
8. **C for the library, Python for checking, Julia later.** Production code is C. Python is used for tests,
   reference oracles and proof checks only. The public C interface stays callable from Julia's `ccall` without C
   glue (`conventions.md` section 12). (TJO, 2026-09-27; `SPEC.md` M0-D12)

## 2. Repository layout

    include/adelefeld.h       the public interface (one header)
    src/                      one file per type or kernel
    tests/                    test_<module>.c; tests/ref/ holds the Python reference; tests/golden/ the fixed vectors
    bench/                    benchmarks; one dated output file per run, with the compiled hot loop where needed
    proto/                    Python prototypes that fix the mathematics before the C is written
    docs/                     SPEC.md, PLAN.md, PERF.md, proofs/, seams.md, conventions.md, sources.md, reviews/
    refs/                     fetch scripts and hashes for cited sources (the sources are not committed)
    lanes/                    briefs and reports of the parallel work lanes
    tools/                    the command-line driver `adf`
    Makefile                  all, check, fuzz, mutate, bench

Naming: public symbols `adf_<type>_<verb>`; FLINT's conventions for argument order (output first) and for
`init`, `clear`, `set`, `swap`; types `adf_x_struct`, `adf_x_t`, `adf_x_ptr`.

## 3. Layers

| Layer | Content |
|---|---|
| L0 kernels | residues modulo a fixed modulus: word-sized (`ulong` with `nmod_t`), multi-limb (`fmpz` with `fmpz_mod_ctx_t`), arrays of word-sized blocks for batches |
| L1 finite balls | `adf_rat`, `adf_fball` in the three policies, both backends; set predicates; `adf_lball`, `adf_sball` |
| L2 adeles, functions of one variable | `adf_adele`, `adf_cadele`; functions (section 9.3) through `f_at` with named places |
| L3 ideles | `adf_ucoset`, `adf_idele`, `adf_idclass`; norms; division; all-places `Log` enclosure; symbols |
| L4 quotient and characters | `adf_qclass`; the additive character; class-group characters with conductor |
| L5 analysis | finite and real test functions, weighted Fourier transform, Poisson summation, Tate integrals |
| L6 solving | reconstruction; module solvers; roots (optional for version 1) |
| L7 surface | value text, dump, command-line driver; later the interaction plane |

## 4. Types and contexts (proposal, not approved; to be frozen in work package 0.4)

Semantic value and physical storage are separate. The value carries its own radius and a tag for its backend.
Version 1.1 applies decisions M0-D1 (exact unit), M0-D8 (seams R1: opaque place handle; R5: the idele scale is read
through an accessor named "content") and the exact field of the scaled value (`SPEC.md` 4.4; `conventions.md` CV-14,
CV-15, CV-18, which the conventions draft states in full).

    typedef struct { fmpq_t q; } adf_rat_struct;                        /* exact global rational */

    typedef struct {                                                    /* (A + H Zhat)/d */
        fmpz_t A, H, d;                  /* d > 0; H >= 0. ADF_GLOBAL, H > 0: 0 <= A < H, gcd(A,H,d) = 1.
                                            ADF_LOCAL: raw data, no gcd condition (CV-55) */
        int backend;                     /* ADF_GLOBAL: A is the value. ADF_LOCAL: res is the value, A = 0 */
        const adf_modctx_struct *mctx;   /* ADF_LOCAL: word-sized coprime blocks q_i with product H; else NULL */
        ulong *res;                      /* ADF_LOCAL: residues modulo the blocks */
    } adf_fball_struct;                  /* a radius with a block above one word is ADF_GLOBAL as a whole */

    typedef struct {                                   /* exact = 0: s (u + K Zhat), K from the context */
        fmpq_t s; fmpz_t u;                            /* exact = 0: s > 0, 0 <= u < K */
        const adf_modctx_struct *mctx;                 /* exact = 1: the exact rational s (0 included), u = 0 */
        int exact;
    } adf_scaled_struct;

    typedef struct { arb_t inf; adf_fball_struct fin; } adf_adele_struct;
    typedef struct { acb_t inf; adf_fball_struct fin; } adf_cadele_struct;   /* the ring C x A_f */
    typedef struct { fmpz_t c, N; } adf_ucoset_struct;   /* c U(N): N >= 1, 1 <= c <= N, gcd(c,N) = 1; or N = 0,
                                                            c = +1 or -1: the exact unit (U(0) = {1}) */
    typedef struct { arb_t inf; fmpq_t r; adf_ucoset_struct u; } adf_idele_struct;   /* inf excludes 0; r > 0,
                                                                                         accessor "content" */
    typedef struct { arb_t t; adf_ucoset_struct u; } adf_idclass_struct;             /* t > 0 */
    typedef struct { ulong opaque; } adf_place_t;        /* a place; created and read only through functions */

Further types whose contracts are written in 0.4 and implemented in their milestones: `adf_lball` (one prime),
`adf_sball` (a finite set of places), `adf_qclass` (union of pieces modulo `Q`), `adf_ffun` (finite function: `D`,
`M`, array of `acb`), `adf_rfun` (sum of polynomial-Gaussian terms), `adf_char` (character with conductor and
parity), solver results with certificates, and results of integrals with their domain.

**Contexts.** `adf_modctx_struct` is incomplete in the public header. The library exports explicit constructors
named `adf_modctx_new_*`, each taking `adf_modctx_struct **out` as its first argument, and
`adf_modctx_free(adf_modctx_struct *ctx)`. A successful constructor allocates and fully initializes an immutable
context (blocks, reduction constants, recombination tree) and writes its pointer to `*out`; on failure `*out` is
untouched and no allocation is retained. The caller owns the returned context and frees it only after its borrowers
are gone. Passing NULL to free does nothing. A constructor never frees, mutates or reuses the context previously
pointed to by `*out`. On success it overwrites only the pointer slot; the caller must retain any previous owned
pointer separately. No public
by-value or array-of-one context type requires the layout of the incomplete struct. Value init functions stay
non-failing and require successfully constructed contexts where stated. Version 1 offers no inline context
allocation. Real
working precision is an argument of each operation (as in `arb`), not a field of the modulus context.

**Rules written down in 0.4 for every function:** which arguments may alias; who owns arrays and scratch space; the
state of the output after a failure; behaviour on invalid input (a non-finite real ball, a zero denominator);
limits on the size of parsed input; thread safety (values are not shared between threads; contexts may be).

A partial ball (`adf_sball`) carries the tag real or complex for its archimedean place. The implicit global
fallback applies only to `adf_fball` and to types containing it: an operation of theirs whose raw result needs other
blocks than those of its context returns a global value. It never writes into a context, and in version 1 it
creates none (`SPEC.md` 4.1; `proofs/policies.md` Proposition 24, Summary 26). Equality and printing go through the
canonical triple, never through raw residues.

**Scaled context rule.** A default binary operation on `adf_scaled` requires the same context pointer in both
inputs; if the pointers differ it returns `ADF_DOMAIN` with its value output untouched. The check precedes every
write, an aliased write included. The result borrows that input context; exact operands follow the same rule. The
shared-pointer check also applies to `adf_scaled_mul_tight`, including exact operands. It compares the two scaled
inputs, not the old context of the initialized output. Unary and exact-scalar operations borrow the scaled input's
context; an `adf_rat` scalar has no context to compare. To
combine different contexts the caller constructs a context with modulus `lcm(K, K')`, converts both operands to it
without loss and then calls the ordinary operation. No operation creates that context. A separately named
target-context operation may take an explicit caller-owned context; its conversion loss and output context are then
documented (`SPEC.md` 4.4; G1).

**The local backend holds raw data** (finding F7 of `conventions.md`). Version 1.0 of this plan put the invariant
`gcd(A, H, d) = 1` in the struct comment for both backends. `proofs/policies.md` Proposition 24 shows that
cancellation keeps the *set* of a local value in its context while its canonical triple leaves it, and Summary 26
that the canonical modulus can change after a sum. The struct comment now follows the accepted decision CV-55 of
`conventions.md` 5.3: a local value `(d; r_1, ..., r_k)` need not satisfy the gcd condition; its canonical triple is
derived when needed (the gcd blockwise, `policies.md` Lemma 18), and equality and printing go through it. A local
sum stays local with the denominator `lcm(d, e)`; a result whose raw form needs other blocks (a tight product with
`h` not dividing `d e`, an exact scalar `m/n` with `|m|` not dividing `d`) or that combines two different context
pointers is global. The milestone 0 gate review accepted CV-55 (raw local data); a backend whose invariant is
canonical data instead converts every local result with `gcd > 1` to the global backend (P24.4).

**Residues of unit cosets.** The header follows `conventions.md` 5.6 (CV-16, accepted by the gate review): residues
are stored and
printed in `1..N`, so the whole unit group is `[1 mod 1]`. `proofs/ideles.md` Definition 8 reduces them into
`[0, Nbar)` and writes the whole group `(0, 1)`. The sets and the equality test are the same (ideles Proposition
9.2); only the representative differs (finding F8 of `conventions.md`; the proof file is not changed here).

**Status codes.** `ADF_OK`; `ADF_UNIT_NOT_CERTIFIED` (the enclosure does not prove invertibility);
`ADF_NOT_UNIT` (proved not invertible); `ADF_NOT_DETERMINED` (not fixed by the inputs, or a required result
invariant not certified; for example a ball that meets a pole and also contains regular points); `ADF_NEEDS_SPLIT`;
`ADF_NOT_UNIQUE`;
`ADF_NO_SOLUTION`; `ADF_DOMAIN` (with the place; an exact input at a proved nonremovable pole); `ADF_LIMIT`
(resource limit); `ADF_PARSE`; `ADF_UNSUPPORTED`. The set predicates `equal_set`, `overlaps`, `contains` return
`int` 0 or 1, not a status. Point comparison returns `ADF_CMP_EQUAL = 0`, `ADF_CMP_DIFFERENT = 1`, or
`ADF_CMP_UNDECIDED = 2`.

**Modulus families offered by constructors:** arbitrary integer; list of pairwise coprime blocks; list of prime
powers; `k!`; powers of a primorial; exact (radius 0). A single prime power `p^k` as the radius of an `adf_fball`
still constrains all primes; one place alone is `adf_lball`.

## 5. Text forms (proposal, frozen in 0.4)

**Value form**: canonical, independent of the backend, for people and for exact exchange of finite parts.

    7/3                                  exact rational
    (3.14159 +/- 1e-5 ; 5/3 mod 6)       adele
    (* ; 2 mod 6)                        finite ball
    [p=5: 3 + O(5^4)]                    local ball
    (2.5 +/- 1e-9 ; 3/2 * [5 mod 36])    idele: real part ; scale times unit coset
    <1.25 +/- 1e-30 ; [5 mod 36]>        idele class
    {inf: 2.5 +/- 1e-9; p=5: 3 + O(5^4)} partial ball over the places inf and 5

Unit cosets are printed in canonical form: a modulus that is twice an odd number is halved (`[5 mod 6]` prints as
`[2 mod 3]`), then the residue is reduced. The dump keeps the modulus as supplied. The exact units (stored with
modulus 0, M0-D1) print as `[1]` and `[-1]`; residues are printed in `1..N`, so the whole unit group is `[1 mod 1]`
(`conventions.md` 9.4, 9.8). Places are labelled `p=5` and `inf` (seams R9, M0-D8). With real or complex parts,
parsing a printed value encloses the original stored value; it need not return it. The print-read-print fixed-point
statement applies only to the exact-rational reference parser. A C value-text round trip may widen the value and
change its text on every pass. Dump text is the identity-preserving form (`conventions.md` finding F6; G4).

A decimal real ball is read as an enclosure. **Dump form**: versioned; real balls as exact dyadic numbers
(`arb_dump_str`); backend and context recorded. Every dump starts `adf1 Q `: the version and the field (seams R9);
types with an archimedean part write the number of archimedean components before the real balls (1 for `Q`, seams
R3). A local finite ball is dumped with its raw data (`conventions.md` section 10). The loader takes one
caller-owned context binding per local-`adf_fball` or `adf_scaled` occurrence, in dump traversal order; every
binding must match that occurrence's modulus and ordered blocks, and repeated occurrences may share a pointer. Value
fields and backend are restored exactly relative to those bindings, and loading with `identical()` requires the
original context pointers (G3). The loader validates the whole text before any FLINT load function sees it, since
`arb_load_str` aborts the process on some malformed strings (M0-D9).

## 6. Work packages

Effort is given in three parts where it matters: proof and reference (P), implementation (I), adversarial testing
and measurement (T). S is up to a day, M a few days, L a week or more. These are estimates, not promises; packages
marked **gate** have an exit criterion instead of an estimate.

### Milestone 0: contracts (gate: reviewed by a second model family, no open blocker)

| WP | Content | Done when |
|---|---|---|
| 0.1 | Provisional scaffold: Makefile, test runner, local check; no public types, header empty | `make check` passes with one trivial test |
| 0.2 | Sources on disk with hashes: Tate's thesis; Hertogh's thesis and a pinned commit of the package; a textbook on adeles; a source for p-adic `exp`, `log`, `sin`, `cos`; uops.info pages used in `PERF.md`; FLINT and PARI documentation cited | `docs/sources.md` lists each source and what it is used for; every **[unverified]** and **[standard]** label in `SPEC.md` is either quoted or kept with a reason |
| 0.3 | Proofs in `docs/proofs/`: the list at the end of `proofs/precision.md`; `proofs/functions.md` (domains, radius rules, `Log`, roots with branches and precision loss, powers, truncation bounds of the series, all from review round 2, N1 to N6, rewritten stepwise); unit cosets; splitting; weighted transform; Poisson for the implemented class; the Tate integrals and the functional equation with the chosen signs; the formulas of the catalogue (`SPEC.md` 9.3.7) | each proof reviewed; each statement has a numerical check in `proto/` |
| 0.4 | `docs/conventions.md`: signs, measures, canonical forms, storage invariants, aliasing and ownership rules, status codes, text grammar | frozen; golden vectors written in `tests/golden/` |
| 0.5 | Benchmark harness under the contract of `PERF.md` section 7; matched rows for the word kernels | `bench/` reproduces the word rows with saved compiled loops |
| 0.6 | `docs/seams.md`: the interface tested on paper against a non-principal ideal (a field of class number 2) and against the place at infinity of `F_q(T)` | each public type is marked "unchanged", "generalises by ...", or "special to `Q`" |

**Status of milestone 0 on 2026-09-28** (from `git log` and the lane reports in `lanes/`):

| WP | State | Record |
|---|---|---|
| 0.1 | done | `7004de8`: Makefile, header-only test runner, one FLINT link test, empty public header |
| 0.2 | done, four sources pending | `c9d2cde`: 17 keys under `refs/src/` with hashes, `docs/sources.md` with 57 verbatim quotations; all 7 labelled statements of `SPEC.md` quoted in version 1.1, one attribution kept (Tate's thesis, section 2.2). Pending: Tate's thesis; a source for the names "arithmetic" and "geometric" (made unnecessary by M0-D10); a text with proofs for p-adic `sin`, `cos`, `sinh`, `cosh`; the uops.info register forms, now fetched by `refs/fetch_intel.sh` (`PERF.md` 1.1) |
| 0.3 | written, reviewed, repaired; new parts await the gate | functions `c070e75`, review `679fd48`, repair `08c63da`; policies, ideles, quotient `6168857`, review `6e32478`, repair `e18f8d5`; analysis `65aeea9`, review `1b8a199`, repair `e932a3d`; catalogue `c4f02b3`, review `46dabbe`, repair `6bcc1f3`. Verdicts in section 8. Added in repair, no second reader yet: functions Proposition 7b and Remark 15r |
| 0.4 | landed as a draft for the gate; not frozen | part A `1db7dad`: conventions draft 0.1 (reference parser and printer `proto/text_grammar.py` with 23 tests); part B `d599959`: conventions 0.2 (signs, measures, Gauss sum and root number, local gamma factors, class-group characters, reciprocity by exponent, raw local values, exact unit cosets, quotient invariant, opaque place handle, dump header with the field; 12 decisions decided, 48 proposed; 713 golden vectors; findings F7 to F10, applied in version 1.1 of `SPEC.md` and of this plan) |
| 0.5 | done | `5ae05db`: harness, six word rows, saved chain loops; first run `bench/results/2026-09-27T202944Z_word.txt`, second run `2026-09-27T221654Z_word.txt` (both `quiet_machine: no`) |
| 0.6 | done | `9f4be67`: 38 rows with verdicts, recommendations R1 to R9 (adopted, M0-D8), three findings on `SPEC.md` 3 (applied) |
| 1.1 | done early | `5ae05db`: Python reference, 59 tests, 18 of 18 mutants killed |
| gate | gate passed after edits, applied on 2026-09-28 | review `reviews/m0-gate/review.md` (GATE NOT PASSED, 16 findings); closure check `reviews/m0-gate/closure.md` (GATE PASSED AFTER THE LISTED EDITS); application reports `lanes/m0-gate-apply-docs/report.md`, `lanes/m0-gate-apply-conv/report.md`, `lanes/m0-closure-apply/report.md` |

### Milestone 1: the ring (L)

| WP | Content | Tests (see section 7) | PERF rows |
|---|---|---|---|
| 1.1 | Python reference `tests/ref/`, written from the proofs, not from the C | enumeration of small cases | none |
| 1.2 | `adf_rat`; `adf_fball`, tight, global: set, add, sub, neg, mul, scale by exact rationals; `equal_set`, `overlaps`, `contains` | enclosure, tightness witnesses, predicates | add, mul: chain and batch, word and 4096 bit |
| 1.3 | `adf_adele`, `adf_cadele`; conversion of `adf_rat` at a requested precision | exact arithmetic of tags; containment after conversion, including `1/3` and negatives | add, mul |
| 1.4 | Value text and dump, parser, printer | golden vectors; invalid and huge inputs; coverage-guided fuzzing; the dump loader validates the whole text before any FLINT load function (M0-D9), and the fuzzer never reaches `arb_load_str` with raw text | print, parse |
| 1.5 | Driver `adf`: evaluates expressions in the value form | the tables of `SPEC.md` typed at the prompt | none |
| 1.6 | Rational reconstruction from a full ball (milestone R) | progression cases; none, one, several candidates; end points of the closed real interval included (M0-D3) | one row |
| 1.7 | Scaled policy and absolute cap; the exact case of scaled values; the tight scaled product as a separately named operation (M0-D5); a default binary operation requires a shared context pointer and otherwise returns `ADF_DOMAIN`, with the caller constructing the `lcm(K, K')` context (G1) | same expression in all policies, containment after every step; the loss cases (factor `h = gcd(u, v, K)` of the default product); exact values untouched by the cap (M0-D2); conversion between contexts and the lossless `lcm`; cross-context addition and subtraction rejected; lossless conversion into `lcm(K, K')` then same-context arithmetic | add, mul |
| 1.8 | Local backend: `adf_modctx`, conversion both ways, batch kernels. Storage rules of `proofs/policies.md` Propositions 24, 25: numerator residues and `d` once per value; `d` inverted modulo a block only when coprime. Local values retain raw numerator residues and their denominator. Results whose raw set cannot be represented in the shared caller-owned context, or whose inputs use different context pointers, are global unless the caller supplies a suitable target context. Canonical cancellation alone does not force a local value global. Equality and value printing use the canonical triple (G11) | represented set unchanged by conversion, denominators included; a denominator sharing a factor with a block (`A = d = 2`, block 4); cancellation that keeps the set but not the canonical triple (`(2; 2)` in context `(4)`); the canonical `H` changing after a sum; equality and printing through the canonical triple | conversions; batch add and mul |
| 1.9 | Julia-friendly interface check (M0-D12): every public operation exported, no variadic functions, `adf_sizeof_<type>`, `adf_version_check`, `adf_str_free` | `nm -D` of the library against the declarations of the header; the sizes against the documented layouts; a program that loads the library with `dlopen` and calls a few functions through `dlsym` without the header's inline functions; a Julia `ccall` smoke test where Julia is installed | none |

### Milestone 1F: functions (L; split as the reviewer recommends)

| WP | Content | Tests |
|---|---|---|
| 1F.1 | `adf_sball` and `f_at`: partial balls over named places, with the real or complex tag; projection from adeles | the result names its places; failure at one place is reported with that place and no value |
| 1F.2 | Archimedean wrappers around `arb`/`acb`: `exp`, `log`, `log_abs`, trigonometric and hyperbolic functions, roots (odd and even degree), special functions | wrapper tests only: projection, guard digits, translation of statuses, balls crossing 0 or a branch cut. Not a test of `arb` |
| 1F.3 | `adf_lball`: arithmetic; decomposition `p^m w u`; valuation, absolute value, fractional part | enumeration modulo small prime powers |
| 1F.4 | Local `exp`, `log`, `Log`: our wrapper around FLINT's centre evaluation | the regression of review N4 (`exp 3` and `exp 12` at precision 8); the precision cases of `SPEC.md` 9.3.2 including the losses at `x = 3, 12` (`p = 3`) and `x = 2, 10` (`p = 2`); `log(-1) = 0` at 2; balls at the edge of the domain; uncertain zero |
| 1F.5 | Local roots: existence conditions, all branches with identifiers or a seed, precision formula and its guard; square roots at 2; degree divisible by `p` | 3 has no square root in `Q_2`, 9 has; counts `gcd(n, p-1)`; loss of one digit at 2; input balls outside the guard |
| 1F.6 | Powers: integer; rational through roots; principal units; (the quasi-character comes with milestone 3) | `exp(Log p) = 1` is not `p`; compatibility of principal-unit powers with integer powers |
| 1F.7 | `sin`, `cos`, `sinh`, `cosh` at a prime by power series with proved truncation | comparison with `exp` at `p = 5` and `13` (where `sqrt(-1)` is in `Q_p`); `sinh`, `cosh` against `exp` at every prime; independent exact truncations with tail bounds at `p = 2, 3`: `cos 4 = 9 mod 16`, `sin 3 = 3 mod 9`; a stated output precision is required, identities alone do not count |
| 1F.8 | All-places forms: the five series on values with finite part exactly 0; `Log` on ideles as the enclosure `4 Zhat` refined at named primes; the rational root of an exact rational (by integer root tests; both signs optional for even degree; 0; degree 1); `NOT_DETERMINED` for roots of degree at least 2 of ideles; branches are listed only over named places | after milestone 2. 1 has rational square roots `1` and `-1` and the function does not claim to list the adelic ones; 8 has the cube root 2; 2 has no square root |
| 1F.9 | Catalogue, Tier A, as the types arrive: Legendre, Jacobi, Kronecker and Hilbert symbols; local zeta factors; profinite power, and its finest-modulus variant as a separately named operation; binomial coefficients; content; cyclotomic action as two functions named by their formula, `z -> z^(1/u')` and `z -> z^(u')` (M0-D10). (Gauss sums and local constants: milestone 3 and 5; theta series: milestone 4) | `(1/2) = +1` against `(3/2) = -1`; Hilbert symbol: product formula on rationals, solvability modulo 16, 9, 25 on reduced coefficients, all 64 pairs of square classes at 2, undetermined cases for ideles, the unit part with the cofactor of the scale (`r = s = 3`, unit 1: `(3,3)_2 = -1`); profinite power: the criterion, the coarsening to `D`, negative exponents, canonical moduli, exponent 0 gives the exact unit, the finest modulus `canon(F)` with its CRT centre (`N = 5, c = 2, e = 2, M = 4`: `F = 120`, centre 49, not `c^e`); binomials: enclosure and the smallest ball by enumeration, `(a, N, k) = (0, 8, 4)` gives radius 2; cyclotomic action: the test vector of the specification; local factors at and near their poles: a ball containing a pole returns the pole status, never a finite or unbounded ball |

### Milestone 2: ideles (M)

| WP | Content | Tests |
|---|---|---|
| 2.1 | `adf_ucoset` with the exact units (modulus 0; printed `[1]`, `[-1]`; M0-D1), `adf_idele`, `adf_idclass`: multiply, invert, power. Power: the default enclosure `c^k U(N)`, and the smallest coset `chat^k U(M_k)` of `proofs/ideles.md` Proposition 13 as a separately named operation (M0-D6); exponent 0 gives the exact unit | exact coset identities at a fixed modulus; `gcd` rule at mixed moduli, including an exact factor (`gcd(0, N) = N`); the point 1 is in `x * x^-1`; powers against enumeration modulo small `M_k` (squares: `M_2 = 24` for `N = 1`); `k = 1, -1` give `M_k = N` |
| 2.2 | Absolute values, valuations at a named prime (by divisibility, no factorisation), norm | negative rationals; norm exact before rounding |
| 2.3 | Maps: rational to idele (exact unit `sign(q)`, M0-D1); idele to class (with the sign on the unit); idele to adele, both hulls | containment; tightness of the small hull; an exact unit gives the exact rational `r c` |
| 2.4 | Division of an adele by an idele (the smallest ball of `proofs/ideles.md` Proposition 19, M0-D7) or an exact rational | against multiplication by the inverse; the radius `gcd(|a| lcm(N, 2), M)/r` against enumeration, with `a = 0`, `M = 0`, odd `N`; status when the divisor is only an adele |

### Milestone 3: quotient and characters (M)

| WP | Content | Tests |
|---|---|---|
| 3.1 | `adf_qclass`: reduction with splitting, piece limit; `k` integers crossed give `k + 1` closed pieces (M0-D4); pieces are closed real balls that may exceed `[0,1]` by the rounding of the enclosure, with the invariant on the midpoint (`conventions.md` CV-45) | wrapping across an integer; several wraps; fractional radii; equality of the sets after translation by a rational; the piece count `k + 1`, one piece for a point; an end point that is not dyadic (`[0.9, 1]`), whose enclosure exceeds 1 |
| 3.2 | The additive character | a non-trivial phase at `(0 ; 1/3)`; ambiguity on a fractional radius; additivity; width of the result |
| 3.3 | `adf_char`: `t^s chi` with conductor and parity | conductor not dividing the modulus gives an enclosure or a status |
| 3.4 | Gauss sums | against `acb_dirichlet_gauss_sum` |

### Milestone 4: functions on the adeles (L)

| WP | Content | Tests |
|---|---|---|
| 4.1 | `adf_ffun`: arrays with `D`, `M`; sum, product with common refinement, exact translation and dilation | delta functions with `D` different from `M` |
| 4.2 | Weighted finite Fourier transform | the formulas of `SPEC.md` section 7 in exact cyclotomic arithmetic or certified balls; non-symmetric inputs |
| 4.3 | `adf_rfun`: polynomial-Gaussian sums; closure operations; transform | shifted (non-even) Gaussians, which tell the two sign conventions apart |
| 4.4 | Evaluation at adelic balls; Haar integral | enclosure across a jump |
| 4.5 | Poisson summation with proved tail bounds | two independent truncations; width target; convergence with precision |

### Milestone 5: Tate integrals (gate)

| WP | Content | Exit |
|---|---|---|
| 5.1 | Local integrals: unramified factors; ramified with the inverse character; infinity for both parities | closed forms |
| 5.2 | Global value for `Re(s) > 1`, zeta and primitive characters | compared with FLINT's values completed by hand; width shrinks with precision |
| 5.3 | Continuation by splitting and Poisson summation; poles | independent of `acb_dirichlet`; tested at and near poles |
| 5.4 | Functional equation as a computed identity | both sides overlap with finite width, for both parities and a non-real character |

### Milestone S: solvers (optional for version 1; gate)

**Order (TJO, 2026-09-28):** milestone S is worked directly after milestone 1 is closed, before milestones 1F
and 2 to 5. It depends on milestone 1 only (reconstruction, the local backend). Work package S.3 (partial
rational reconstruction, with a bound on the denominator) is what a solver of rational linear systems needs
first.

**Thin slices (TJO, 2026-09-29; `docs/workflow.md`):** the milestone is built as working slices, each end to
end (header, code, test against the reference `proto/solvers_checks.py`, a call through the driver or Julia).
Slice 1 is partial rational reconstruction in the range `2 A B < m`. The design (`docs/proofs/solvers.md`,
`docs/api-s.md`, draft 3, reviewed in `docs/reviews/s-design/`) is the map; a slice implements the part it needs.

| WP | Content |
|---|---|
| S.1 | Systems modulo `N`: particular solution, kernel, certificate (Hermite form with transformation; FLINT's Smith form returns no transformations) |
| S.2 | Simple roots at a prime by Hensel lifting; real roots with completeness status |
| S.3 | Partial rational reconstruction |

### Milestone 6: the second field (after version 1)

`F_q(T)` with the place at infinity stored separately; then number fields, after the choice of a backend for ideals,
class groups and units, with its guarantee recorded.

## 7. Acceptance tests: what counts

(P2) A test counts only if a wrong implementation of the claim would fail it.

| Claim | Test |
|---|---|
| Enclosure of sum and product | enumeration in small quotients; random rationals with shared factors; zero radii; negative and zero centres |
| Tightness | the four witness pairs of `proofs/precision.md`; the valuation formula `min(v(a)+v(M), v(b)+v(N), v(N)+v(M))` at each prime |
| Canonical form | the same set from different inputs prints identically; aliasing of arguments |
| Policies | one expression evaluated from the same inputs in all policies; containment checked after every operation; cases with intended loss |
| Backends | the set is unchanged by conversion in both directions |
| Ideles | coset identities; not "group axioms on balls", which fail for sets |
| Characters | non-trivial phases; a constant function must fail |
| Fourier transform | `D` different from `M`; weights; non-symmetric inputs |
| Poisson, integrals | two independent computations; certified tails; a result of unbounded width fails |
| Agreement with FLINT or `arb` | only where our path does not call the same routine; otherwise it is labelled a wrapper test |
| Functions | an independent oracle and a stated output precision; an identity that a constant function satisfies does not count; a result as large as the whole disc fails |
| Quotient | several wraps; endpoints with the gluing rule |
| Unit cosets | `[5 mod 6]` and `[2 mod 3]` are equal and print identically |
| Solvers | the equation is verified; the kernel is complete on small cases |
| Text | golden vectors; malformed and oversized input |

## 8. Review protocol

1. Documents: reviewed by a second model family (codex `gpt-6-astra`) before implementation; each round in
   `docs/reviews/`. Round 1: 2026-09-27, 28 findings, applied in draft 2 of the specification and draft 1 of this
   plan and of `PERF.md`. Round 2: 12 new findings and 11 remainders, applied in draft 3 of the specification and
   draft 2 of this plan and of `PERF.md`; verdict "milestone 0 may begin". Round 3: all round-2 items closed but one;
   six new findings (R1 to R6, two major), applied; closure check (`reviews/astra-2026-09-27-r3/closure.md`):
   ratify after three minor edits, applied. Design review of the documents is closed; the milestone-0 gates remain.
2. Each milestone: proofs to a second model family; code to an adversarial reviewer whose task is an input that
   breaks enclosure. Milestone 0, work package 0.3: four cross-family reviews in refute mode, in
   `docs/reviews/m0-proofs/`, each with the reviewer's own checks; all repairs applied (the review record at the end
   of each proof file lists them):

   | Proof files | Author | Reviewer | Valid | Minor | Invalid | Repair |
   |---|---|---|---|---|---|---|
   | `functions.md` (22 statements) | codex `gpt-6-astra` | Claude opus | 20 | 2 | 0 | `08c63da` |
   | `catalogue.md` (15) | codex `gpt-6-sol` | Claude opus | 11 | 4 | 0 | `6bcc1f3` |
   | `analysis.md` (15) | codex `gpt-6-astra` | Claude fable | 9 | 6 | 0 | `e932a3d` |
   | `policies.md`, `ideles.md`, `quotient.md` (51) | Claude opus | codex `gpt-6-astra` | 46 | 3 | 2 | `e18f8d5` |

   Total: 103 statements, 86 valid, 15 minor, 2 invalid. The two invalid verdicts (policies P24, P25, both claims
   about storage in the local backend, not about an enclosure radius) were agreed by the author and restated; no
   counterexample to an enclosure radius, sign, constant or formula was found in any review. Statements added in the
   repairs (functions Proposition 7b, Remark 15r) and the repaired statements went to the gate review of milestone 0
   (`lanes/m0-gate/brief.md`), which also reviews `conventions.md` and version 1.1 of the three documents.
3. Milestone 0 gate and closure check (2026-09-28). The gate review (`reviews/m0-gate/review.md`) returned GATE
   NOT PASSED: 16 findings (2 BLOCKER, 8 MAJOR, 6 MINOR) and 60 decisions of which 52 accepted, 8 rejected. The
   closure check (`reviews/m0-gate/closure.md`) re-judged all 24 items and returned GATE PASSED AFTER THE LISTED
   EDITS: G1-G16 9 CLOSED, 7 CLOSED WITH EDIT, 0 OPEN; rejected decisions 3 CLOSED, 5 CLOSED WITH EDIT; and
   five new findings (C1 MINOR, C2 MAJOR, C3 MINOR, C4 MINOR, C5 MAJOR) with edits E1 to E4, each with full
   replacement text. The edits and replacements are applied (`conventions.md` 0.4, `SPEC.md` 1.3, this plan 1.3,
   `proofs/analysis.md` P13): milestone 0 is gate passed after edits, applied on 2026-09-28. The closure's own
   checks (printing 15032 cases, contracts 878 checks, the reference and text suites) ran with 0 failures; they
   run again after the application (lane `lanes/m0-closure-apply/report.md`).
4. Statements about other people's work keep their label until quoted from a source on disk.

## 9. Risks

| Risk | Mitigation |
|---|---|
| Contracts frozen wrongly | milestone 0 is reviewed; the seams sketch (0.6) precedes the header |
| Rational normalisation and gcds dominate | measured on whole kernels (hypotheses of `PERF.md` section 5) before any redesign |
| Two backends and three policies multiply the test surface | one reference; the tight global ball is the oracle for all others |
| p-adic `sin`, `cos` are new code with no library to compare with | comparison with `exp` at `p = 5, 13`; independent exact truncations with tail bounds at other primes; stated output precision |
| FLINT's p-adic functions evaluate centres, on a smaller domain for `log` | our wrapper owns domain, precision and branches (1F.4, 1F.5) |
| The catalogue grows without limit | Tier A is fixed in `SPEC.md` 9.3.7; anything else is Tier B until a decision |
| Certified continuation of the Tate integral is research-grade work | milestone 5 is a gate, not an estimate; version 1 may ship `Re(s) > 1` first |
| Hertogh's rules differ from ours in detail | work package 0.2 precedes 1.2 |
| The interaction plane wants something the core cannot give | value text, dump and status codes are tested from milestone 1 |
| Development runs on a laptop that is not the baseline machine | `PERF.md` keeps one set of model floors per hardware profile (Zen 2 baseline; Intel laptop, provisional); a measurement is compared only with the floors of its own profile; harness runs on a shared machine carry `quiet_machine: no` |
| A FLINT routine aborts on malformed input (`arb_load_str`) | our loaders validate first (M0-D9); fuzzing targets our validators, never FLINT's loaders with raw text |

## 10. Out of scope for version 1

Number fields, and with them functions on extensions of `Q_p`; Tier B of the catalogue; groups beyond `GL_1`;
automorphic forms; roots at all primes of a general polynomial; multiple roots; the interaction plane
and visualisation (separate documents); any statement about the Riemann hypothesis.
