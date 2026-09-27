# adelfeld: implementation plan, draft 0

Date: 2026-09-27. Status: **plan for review; nothing is implemented.** Read `SPEC.md` first (what is built) and
`PERF.md` (how speed and size are judged). This document says in what order, in which files, and what counts as done.

## 1. Principles

1. **Vertical slices.** Each milestone ends with something a user can type and see, not with a layer.
2. **A failing test precedes every change.** Unit tests, property tests against a slow exact reference, fuzzing of
   the parser, mutation testing of the arithmetic (as in `riemann-channel/zst`).
3. **Every formula in `src/` cites a source on disk** by file and line, or a proof in `docs/proofs/`.
4. **Enclosure is the contract.** For every inexact operation: if the inputs contain the true values, the output
   contains the true result. Tightness is a quality measure, enclosure is a correctness requirement.
5. **A row in `PERF.md` before "finished".** Object, operation, size, floor and its kind, measured, ratio.
6. **No hidden state.** Contexts are explicit arguments. No global variables apart from FLINT's own caches.

## 2. Repository layout

    include/adelfeld.h        the public interface (one header)
    src/                      one file per type or kernel
    tests/                    test_<module>.c, one executable each; tests/ref/ holds the Python reference
    bench/                    benchmarks; one dated output file per run
    proto/                    Python prototypes that fix the mathematics before the C is written
    docs/                     SPEC.md, PLAN.md, PERF.md, proofs/, sources.md
    refs/                     fetch script and hashes for cited sources (sources themselves not committed)
    tools/                    the command-line driver `adf`
    Makefile                  all, check, fuzz, mutate, bench

Naming: public symbols `adf_<type>_<verb>`, FLINT's conventions for argument order (output first) and for
`init`/`clear`/`set`/`swap`. Types follow FLINT's pattern `adf_x_struct`, `adf_x_t` (array of one), `adf_x_ptr`.

## 3. Layers

| Layer | Content | Precision handling |
|---|---|---|
| L0 kernels | residues modulo a fixed modulus: word-sized (`nmod`), multi-limb (`fmpz_mod`), tuple of small prime powers (vectorised) | none: exact arithmetic in a finite ring |
| L1 profinite balls | `adf_pball` in the tight and the capped policy; conversion between global and local storage | section 4.2 and section 15 of the specification |
| L2 adeles and ideles | `adf_adele`, `adf_adele_c`, `adf_idele`, `adf_idclass`; absolute values; reduction modulo `Q`; characters | real part by `arb`/`acb` |
| L3 functions | test functions, Fourier transform, Poisson summation, zeta integrals | `acb` and finite arrays |
| L4 solving | linear systems, Hensel and Newton, rational reconstruction | built on L1-L2 |
| L5 surface | text form, command-line driver, later the interaction plane | none |

## 4. Core data types (proposal)

    typedef struct {
        fmpz_t modulus;            /* N > 0 */
        int    policy;             /* ADF_TIGHT or ADF_CAPPED */
        int    storage;            /* ADF_GLOBAL or ADF_LOCAL */
        slong  nfac; ulong *q;     /* local storage: prime powers q_i < 2^32 with product N */
        /* precomputed: reduction constants, CRT tree */
        slong  prec;               /* working precision in bits for the real place */
    } adf_ctx_struct;

    typedef struct { fmpq_t centre; fmpq_t radius; } adf_pball_struct;      /* tight: radius 0 means exact */
    typedef struct { fmpq_t scale; fmpz_t res; }     adf_cball_struct;      /* capped: scale * (res mod ctx N) */
    typedef struct { arb_t inf; adf_pball_struct fin; } adf_adele_struct;
    typedef struct { acb_t inf; adf_pball_struct fin; } adf_adele_c_struct;
    typedef struct { arb_t inf; fmpq_t r; fmpz_t c; fmpz_t N; } adf_idele_struct;   /* inf excludes 0; gcd(c,N)=1 */
    typedef struct { arb_t t;   fmpz_t c; fmpz_t N; } adf_idclass_struct;           /* t > 0 */

Status codes returned by every function that can fail: `ADF_OK`, `ADF_NOT_UNIT` (ball contains a non-invertible
element), `ADF_TOO_COARSE` (radius too large for the operation, for example reduction modulo `Q`),
`ADF_INEXACT_COMPARE`, `ADF_DOMAIN`, `ADF_PARSE`.

Modulus families offered by constructors (decision 3): arbitrary integer; list of prime powers; `k!`; powers of a
primorial; a single prime power (the p-adic case); exact (radius 0).

## 5. Text form (proposal)

One line per value, readable by a person and exact for a machine.

    7/3                                  exact rational, as an adele
    (3.14159 +/- 1e-5 ; 5/3 mod 6)       adele: real ball ; centre mod radius
    (* ; 2 mod 6)                        finite adele only (no real place)
    (2.5 +/- 0 ; 3/2 * [5 mod 36])       idele: real part ; rational times unit class
    <1.25 +/- 1e-30 ; [5 mod 36]>        idele class

The exact round trip uses a second form in which real balls are written as exact dyadic numbers. Grammar, and a
fuzzer for the parser, are part of work package 1.6.

## 6. Work packages

Sizes: S is up to a day, M a few days, L a week or more, for one person with agent support. They are estimates of
effort, not promises.

### Milestone 0: skeleton (S)

| WP | Content | Done when |
|---|---|---|
| 0.1 | Makefile, header, empty library, test runner, continuous local check | `make check` passes with one trivial test |
| 0.2 | `refs/`: fetch script and hashes for Tate's thesis, Hertogh's thesis, one textbook on adeles, FLINT documentation | sources on disk, hashes recorded, `docs/sources.md` lists what each is used for |
| 0.3 | `docs/proofs/precision.md`: written proofs of the two rules of section 4.2 and of tightness | reviewed by a second model family |

### Milestone 1: the ring (L)

| WP | Content | Test | PERF row |
|---|---|---|---|
| 1.1 | Python reference `tests/ref/adele_ref.py`: exact, slow, obviously correct | the brute-force checks of `proto/` | none |
| 1.2 | `adf_pball`, tight policy, global storage: set, add, sub, neg, mul, multiply and divide by exact rationals, overlap and containment tests | C against the reference on random inputs; section 4.2 table | add, mul at 62 and 4096 bits |
| 1.3 | `adf_ctx` and the capped policy | capped result contains tight result, always | add, mul at 62 and 4096 bits |
| 1.4 | Local storage and CRT both ways | round trip; local and global agree on every operation | conversion; tuple add and mul |
| 1.5 | `adf_adele`, `adf_adele_c`: the real or complex ball joined on; embedding of `Q` | `(q ; q)` arithmetic agrees with `fmpq` | add, mul at 128 bits + 62 bits |
| 1.6 | Text form, parser, printer, fuzzer | round trip is the identity on 10^6 random values | print, parse |
| 1.7 | Driver `adf`: evaluates an expression in the text form | the examples of the specification typed at the prompt | none |

Decision point 1b, after 1.5: whether to write a fixed-precision small ball kernel (see `PERF.md` section 4, point 2).

### Milestone 2: ideles (M)

| WP | Content | Test |
|---|---|---|
| 2.1 | `adf_idele`, `adf_idclass`: multiply, invert, power | group axioms on random values; radius never degrades |
| 2.2 | Absolute values at each place, idele norm | product formula on random rationals |
| 2.3 | Maps: idele to adele, idele to class, rational to idele | compatibility with multiplication |
| 2.4 | Division of an adele by an idele | agrees with multiplication by the inverse |

### Milestone 3: quotient and characters (M)

| WP | Content | Test |
|---|---|---|
| 3.1 | Reduction modulo `Q` into `[0,1) x Zhat` | two adeles differing by a random rational reduce to overlapping values |
| 3.2 | The standard additive character, sign convention quoted from Tate | value 1 on random rationals; additivity |
| 3.3 | Characters of the idele class group: `t^s` times a Dirichlet character (`dirichlet`, `acb_dirichlet`) | multiplicativity; agreement with FLINT's character values |
| 3.4 | Gauss sums | against `acb_dirichlet_gauss_sum` |

### Milestone 4: functions (L)

| WP | Content | Test |
|---|---|---|
| 4.1 | Finite part: functions on `(1/D)Z / M Z`, arithmetic, translation, dilation | against the reference |
| 4.2 | Real part: polynomial times Gaussian, exact Fourier transform | double transform is reflection |
| 4.3 | Fourier transform on `A`, Haar integral | Plancherel identity to the ball |
| 4.4 | Poisson summation | both sides overlap, for random test functions |

### Milestone 5: Tate integrals (M)

| WP | Content | Test |
|---|---|---|
| 5.1 | Local zeta integrals: Euler factor at unramified primes, finite sums at ramified ones | against the closed forms |
| 5.2 | Global integral for the Gaussian and the indicator of `Zhat` | completed zeta function, against `acb_dirichlet` |
| 5.3 | The same with a Dirichlet character | completed L-function, against `acb_dirichlet` |
| 5.4 | Functional equation as a computed identity | both sides overlap on a grid in the critical strip |

### Milestone 6: solving (M, may run in parallel with 3-5)

| WP | Content | Test |
|---|---|---|
| 6.1 | Rational reconstruction from an adelic ball | recovers random rationals of bounded height; refuses when the ball is too coarse |
| 6.2 | Linear systems modulo `N`, Hermite and Smith forms | against `fmpz_mat` |
| 6.3 | Roots of an integer polynomial: real roots, roots modulo `N`, Hensel lifting with certificate | against factorisation modulo primes |

### Milestone 7: the second instance (L)

| WP | Content |
|---|---|
| 7.1 | Extract the interface "global field" from the code of milestones 1-3 |
| 7.2 | `F_q(T)`: places are monic irreducible polynomials and the point at infinity; radius is a polynomial |
| 7.3 | Restate and pass the tests of milestones 1-3 |

Number fields follow only after 7.3, and need ideals, class groups and units from outside FLINT.

## 7. Review protocol

1. This plan, the specification and `PERF.md` are reviewed by a second model family (codex, `gpt-6-astra`) before any
   C is written: `docs/reviews/`.
2. Each milestone: the proofs and the precision rules go to a second model family; the code goes to an adversarial
   reviewer with the instruction to find an input that breaks enclosure.
3. Statements about other people's work stay labelled unverified until quoted from a source on disk.

## 8. Risks

| Risk | Consequence | Mitigation |
|---|---|---|
| The tight rule costs a gcd per product | the natural policy is slow | the capped policy; measure both |
| Fractional radii complicate every kernel | more code paths | store a common denominator once; kernels see integers only |
| Two storage forms double the test surface | bugs in the conversion | one reference, every operation tested in both forms |
| The abstraction "global field" is designed too early or too late | rewrite at milestone 7 | write milestones 1-3 for `Q` plainly, extract the interface afterwards from working code |
| Hertogh's rules differ from ours | milestone 1 rests on a wrong rule | work package 0.2 and 0.3 precede 1.2 |
| `arb` dominates the cost of small adeles | x13 to x83 of floor at 128 bits | decision point 1b |
| The interaction plane wants something the core cannot give | retrofit | the three constraints of section 9 of the specification are tested from milestone 1 (text round trip, status codes, no global state) |

## 9. Out of scope for version 1

Number fields; groups beyond `GL_1`; automorphic forms; the interaction plane and visualisation (separate
documents); any statement about the Riemann hypothesis.
