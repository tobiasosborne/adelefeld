# adelefeld: implementation plan, draft 1

Date: 2026-09-27. Status: **plan; nothing is implemented.** Read `SPEC.md` first (what is built) and `PERF.md` (how
speed and size are judged). Draft 1 applies the design review `reviews/astra-2026-09-27/review.md` (findings P1-P3,
D1-D4, and the consequences of M1-M13 and F1-F6), and TJO's decision that elementary functions belong to the basic
package. Section numbers of `SPEC.md` refer to its draft 2.

## 1. Principles

1. **Contracts before code.** Milestone 0 fixes the mathematics, the conventions and the storage invariants in
   writing, and has them reviewed, before the first C file.
2. **Vertical slices.** Each later milestone ends with something a user can type and see.
3. **Tests.** A new behaviour gets a test that defines it; a defect gets a regression test that fails before the
   fix. Corrections of text need no test. Mutation testing is measured by surviving mutants, fuzzing by coverage.
4. **Every formula in `src/` cites a proof in `docs/proofs/` or a source on disk** by file and line.
5. **Enclosure is the contract.** If the inputs contain the true values, the output contains the true result.
   Tightness is a quality measure with its own tests.
6. **A row in `PERF.md` before "finished"**, under the benchmark contract of `PERF.md` section 7.
7. **No hidden state.** Contexts are explicit and immutable.

## 2. Repository layout

    include/adelefeld.h       the public interface (one header)
    src/                      one file per type or kernel
    tests/                    test_<module>.c; tests/ref/ holds the Python reference; tests/golden/ the fixed vectors
    bench/                    benchmarks; one dated output file per run, with the compiled hot loop where needed
    proto/                    Python prototypes that fix the mathematics before the C is written
    docs/                     SPEC.md, PLAN.md, PERF.md, proofs/, seams.md, conventions.md, sources.md, reviews/
    refs/                     fetch script and hashes for cited sources (the sources are not committed)
    tools/                    the command-line driver `adf`
    Makefile                  all, check, fuzz, mutate, bench

Naming: public symbols `adf_<type>_<verb>`; FLINT's conventions for argument order (output first) and for
`init`, `clear`, `set`, `swap`; types `adf_x_struct`, `adf_x_t`, `adf_x_ptr`.

## 3. Layers

| Layer | Content |
|---|---|
| L0 kernels | residues modulo a fixed modulus: word-sized (`ulong` with `nmod_t`), multi-limb (`fmpz` with `fmpz_mod_ctx_t`), arrays of word-sized blocks for batches |
| L1 finite balls | `adf_rat`, `adf_fball` in the three policies, both backends; set predicates; `adf_lball`, `adf_sball` |
| L2 adeles, functions of one variable | `adf_adele`, `adf_cadele`; elementary functions (section 9.3) |
| L3 ideles | `adf_ucoset`, `adf_idele`, `adf_idclass`; norms; division; `log` and roots on ideles |
| L4 quotient and characters | `adf_qclass`; the additive character; class-group characters with conductor |
| L5 analysis | finite and real test functions, weighted Fourier transform, Poisson summation, Tate integrals |
| L6 solving | reconstruction; module solvers; roots (optional for version 1) |
| L7 surface | value text, dump, command-line driver; later the interaction plane |

## 4. Types and contexts (proposal, to be frozen in work package 0.4)

Semantic value and physical storage are separate. The value carries its own radius and a tag for its backend.

    typedef struct { fmpq_t q; } adf_rat_struct;                        /* exact global rational */

    typedef struct {                                                    /* (A + H Zhat)/d */
        fmpz_t A, H, d;                  /* d > 0; H >= 0; H > 0: 0 <= A < H, gcd(A,H,d) = 1 */
        int backend;                     /* ADF_GLOBAL, ADF_LOCAL */
        const adf_modctx_struct *mctx;   /* ADF_LOCAL: blocks q_i with product H; else NULL */
        ulong *res;                      /* ADF_LOCAL: residues of A modulo the blocks */
    } adf_fball_struct;

    typedef struct {                                                    /* s (u + K Zhat), K from the context */
        fmpq_t s; fmpz_t u; const adf_modctx_struct *mctx;
    } adf_scaled_struct;

    typedef struct { arb_t inf; adf_fball_struct fin; } adf_adele_struct;
    typedef struct { acb_t inf; adf_fball_struct fin; } adf_cadele_struct;   /* the ring C x A_f */
    typedef struct { fmpz_t c, N; } adf_ucoset_struct;                       /* c U(N), gcd(c,N) = 1, N >= 1 */
    typedef struct { arb_t inf; fmpq_t r; adf_ucoset_struct u; } adf_idele_struct;   /* inf excludes 0; r > 0 */
    typedef struct { arb_t t; adf_ucoset_struct u; } adf_idclass_struct;             /* t > 0 */

Further types whose contracts are written in 0.4 and implemented in their milestones: `adf_lball` (one prime),
`adf_sball` (a finite set of places), `adf_qclass` (union of pieces modulo `Q`), `adf_ffun` (finite function: `D`,
`M`, array of `acb`), `adf_rfun` (sum of polynomial-Gaussian terms), `adf_char` (character with conductor and
parity), solver results with certificates, and results of integrals with their domain.

**Contexts.** `adf_modctx`: immutable; blocks, reduction constants, recombination tree; reference counted or
caller-owned with a stated lifetime. Real working precision is an argument of each operation (as in `arb`), not a
field of the modulus context.

**Rules written down in 0.4 for every function:** which arguments may alias; who owns arrays and scratch space; the
state of the output after a failure; behaviour on invalid input (a non-finite real ball, a zero denominator);
limits on the size of parsed input; thread safety (values are not shared between threads; contexts may be).

**Status codes.** `ADF_OK`; `ADF_UNIT_NOT_CERTIFIED` (the enclosure does not prove invertibility);
`ADF_NOT_UNIT` (proved not invertible); `ADF_NOT_DETERMINED` (the value is not fixed at this precision);
`ADF_NEEDS_SPLIT`; `ADF_NOT_UNIQUE`; `ADF_NO_SOLUTION`; `ADF_DOMAIN` (with the place); `ADF_LIMIT` (resource limit);
`ADF_PARSE`; `ADF_UNSUPPORTED`.

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

A decimal real ball is read as an enclosure. **Dump form**: versioned; real balls as exact dyadic numbers
(`arb_dump_str`); backend and context recorded. Reading a dump gives back the identical object.

## 6. Work packages

Effort is given in three parts where it matters: proof and reference (P), implementation (I), adversarial testing
and measurement (T). S is up to a day, M a few days, L a week or more. These are estimates, not promises; packages
marked **gate** have an exit criterion instead of an estimate.

### Milestone 0: contracts (gate: reviewed by a second model family, no open blocker)

| WP | Content | Done when |
|---|---|---|
| 0.1 | Makefile, header, empty library, test runner, local check | `make check` passes with one trivial test |
| 0.2 | Sources on disk with hashes: Tate's thesis; Hertogh's thesis and a pinned commit of the package; a textbook on adeles; a source for p-adic `exp`, `log`, `sin`, `cos`; uops.info pages used in `PERF.md`; FLINT and PARI documentation cited | `docs/sources.md` lists each source and what it is used for; every **[unverified]** and **[standard]** label in `SPEC.md` is either quoted or kept with a reason |
| 0.3 | Proofs in `docs/proofs/`: the list at the end of `proofs/precision.md`; enclosure of each elementary function; unit cosets; splitting; weighted transform; Poisson for the implemented class; the Tate integrals and the functional equation with the chosen signs | each proof reviewed; each statement has a numerical check in `proto/` |
| 0.4 | `docs/conventions.md`: signs, measures, canonical forms, storage invariants, aliasing and ownership rules, status codes, text grammar | frozen; golden vectors written in `tests/golden/` |
| 0.5 | Benchmark harness under the contract of `PERF.md` section 7; matched rows for the word kernels | `bench/` reproduces the word rows with saved compiled loops |
| 0.6 | `docs/seams.md`: the interface tested on paper against a non-principal ideal (a field of class number 2) and against the place at infinity of `F_q(T)` | each public type is marked "unchanged", "generalises by ...", or "special to `Q`" |

### Milestone 1: the ring (L)

| WP | Content | Tests (see section 7) | PERF rows |
|---|---|---|---|
| 1.1 | Python reference `tests/ref/`, written from the proofs, not from the C | enumeration of small cases | none |
| 1.2 | `adf_rat`; `adf_fball`, tight, global: set, add, sub, neg, mul, scale by exact rationals; `equal_set`, `overlaps`, `contains` | enclosure, tightness witnesses, predicates | add, mul: chain and batch, word and 4096 bit |
| 1.3 | `adf_adele`, `adf_cadele`; conversion of `adf_rat` at a requested precision | exact arithmetic of tags; containment after conversion, including `1/3` and negatives | add, mul |
| 1.4 | Value text and dump, parser, printer | golden vectors; invalid and huge inputs; coverage-guided fuzzing | print, parse |
| 1.5 | Driver `adf`: evaluates expressions in the value form | the tables of `SPEC.md` typed at the prompt | none |
| 1.6 | Rational reconstruction from a full ball (milestone R) | progression cases; none, one, several candidates | one row |
| 1.7 | Scaled policy and absolute cap | same expression in all policies, containment after every step; the loss cases | add, mul |
| 1.8 | Local backend: `adf_modctx`, conversion both ways, batch kernels | represented set unchanged by conversion, denominators included | conversions; batch add and mul |

### Milestone 1F: elementary functions (M to L)

| WP | Content | Tests |
|---|---|---|
| 1F.1 | Real place: the functions of `SPEC.md` section 9.3 through `arb` and `acb`, on `adf_adele` and `adf_cadele`, returning partial balls | agreement with `arb`; result type names its places |
| 1F.2 | `adf_lball`: arithmetic; `exp`, `log`, square root, Teichmüller lift through FLINT's `padic` | enclosure on random balls against exact rational points; domain statuses at the edge of the disc |
| 1F.3 | p-adic `sin`, `cos`, `sinh`, `cosh` by power series with proved truncation | identities `sin^2 + cos^2 = 1`, `exp` addition formulas, to the ball; comparison with `exp` over an extension where `sqrt(-1)` exists in `Q_p` (`p = 1 mod 4`) |
| 1F.4 | `adf_sball`: partial balls over named places; functions place by place | projection from adeles; failure at one place reported with that place |
| 1F.5 | After milestone 2: `log`, roots and powers on ideles | compatibility with products: `log(x y) = log x + log y` to the ball |

### Milestone 2: ideles (M)

| WP | Content | Tests |
|---|---|---|
| 2.1 | `adf_ucoset`, `adf_idele`, `adf_idclass`: multiply, invert, power | exact coset identities at a fixed modulus; `gcd` rule at mixed moduli; the point 1 is in `x * x^-1` |
| 2.2 | Absolute values, valuations at a named prime (by divisibility, no factorisation), norm | negative rationals; norm exact before rounding |
| 2.3 | Maps: rational to idele; idele to class (with the sign on the unit); idele to adele, both hulls | containment; tightness of the small hull |
| 2.4 | Division of an adele by an idele or an exact rational | against multiplication by the inverse; status when the divisor is only an adele |

### Milestone 3: quotient and characters (M)

| WP | Content | Tests |
|---|---|---|
| 3.1 | `adf_qclass`: reduction with splitting, piece limit | wrapping across an integer; fractional radii; equality of the sets after translation by a rational |
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
| Agreement with FLINT | only where our path does not call the same routine |
| Solvers | the equation is verified; the kernel is complete on small cases |
| Text | golden vectors; malformed and oversized input |

## 8. Review protocol

1. Documents: reviewed by a second model family (codex `gpt-6-astra`) before implementation; each round in
   `docs/reviews/`. Round 1: 2026-09-27, 28 findings, applied in draft 2 of the specification and draft 1 of this
   plan and of `PERF.md`.
2. Each milestone: proofs to a second model family; code to an adversarial reviewer whose task is an input that
   breaks enclosure.
3. Statements about other people's work keep their label until quoted from a source on disk.

## 9. Risks

| Risk | Mitigation |
|---|---|
| Contracts frozen wrongly | milestone 0 is reviewed; the seams sketch (0.6) precedes the header |
| Rational normalisation and gcds dominate | measured on whole kernels (hypotheses of `PERF.md` section 5) before any redesign |
| Two backends and three policies multiply the test surface | one reference; the tight global ball is the oracle for all others |
| p-adic `sin`, `cos` are new code with no library to compare with | identities, and comparison with `exp` where `sqrt(-1)` is in `Q_p` |
| Certified continuation of the Tate integral is research-grade work | milestone 5 is a gate, not an estimate; version 1 may ship `Re(s) > 1` first |
| Hertogh's rules differ from ours in detail | work package 0.2 precedes 1.2 |
| The interaction plane wants something the core cannot give | value text, dump and status codes are tested from milestone 1 |

## 10. Out of scope for version 1

Number fields; groups beyond `GL_1`; automorphic forms; roots at all primes; multiple roots; the interaction plane
and visualisation (separate documents); any statement about the Riemann hypothesis.
