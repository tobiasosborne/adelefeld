# adelefeld: the public C interface of milestone S (proposal)

Written by the lane `s-design` on 2026-09-29 against `docs/conventions.md` 0.4, `docs/SPEC.md` 1.3,
`docs/PLAN.md` 1.3 and the headers of milestone 1 (`docs/api-m1.md`). Reviewed by a refute review
(`docs/reviews/s-design/review.md`, NOT READY before the repairs) and repaired by the lane `s-design-repair` on
2026-09-29: the decisions S-D3, S-D4, S-D6, S-D14 are restated, S-D16 to S-D19 are added, and the proposed edits
are replaced as the review asks (section 6). A proposal: nothing here is implemented, and nothing is approved.
The statements that the functions implement are in `docs/proofs/solvers.md` (cited as `solvers` with the number
of the statement); the reference algorithms and the oracles of the tests are in `proto/solvers_checks.py`.
TJO decided the nineteen questions of section 5 on 2026-09-29, S-D1 to S-D9 and S-D11 to S-D19 as recommended
and S-D10 otherwise; the decisions are written in `docs/SPEC.md` 15.3.

Contents: 1 headers and types; 2 functions of S.3; 3 functions of S.1; 4 functions of S.2; 5 decisions for
TJO (S-D1 to S-D19); 6 exact edits proposed for `docs/SPEC.md`, `docs/PLAN.md` and `docs/conventions.md`;
7 work packages for the implementation lanes.

Rules that hold for every function below unless its entry says otherwise (`conventions.md`):

- Statuses are those of `conventions.md` 3.1; no new status is proposed. Every function of this milestone is
  in the class "Reconstruction and solvers" of 3.2, whose row is `OK`, `NO_SOLUTION`, `NOT_UNIQUE`,
  `NOT_DETERMINED`, `LIMIT`. Two additions to that row are proposed in section 6 (`DOMAIN` for constructors
  and for a zero polynomial, `UNSUPPORTED` for an exact right-hand side); until they are decided the entries
  below mark them "(edit E-C1)".
- On every status other than `ADF_OK` every value output is untouched (4.3, CV-06). Report arguments
  (certificates, counts, flags) are written on every status for which the entry says so (4.3: "Optional report
  arguments ... are written"). Where an entry makes a value output a report (`sol` of `adf_linsolve_mod`), that is
  an explicit exception (decision S-D6), not a permission that exists.
- Aliasing (4.1): an output may be the same object as an input of the same type; two outputs must not alias.
- Argument order (2.2): outputs; reports; inputs; integer parameters; `prec`.
- No callbacks, no variadic functions, every function exported (section 12).
- Every intermediate exponent of the root functions (`e + j`, `k + s`, `2 k - s`, the sizes of `p^(k+s)`) is
  computed with checked `slong` arithmetic; see decision S-D18.

## 1. Headers and types

| Header (new) | Content |
|---|---|
| `include/adelefeld/resid.h` | `adf_resid`: a residue modulo `m` with nothing known at the other places; reconstruction from it (S.3) |
| `include/adelefeld/linsolve.h` | `adf_linsol`: the solution of a linear system modulo `N` with its certificate; the solver, the checker, the adelic form (S.1) |
| `include/adelefeld/roots.h` | `adf_rootlist`: roots at a prime and real roots with certificates and completeness (S.2) |

| Type | Data | Meaning | Predicate (`is_canonical`) | Init |
|---|---|---|---|---|
| `adf_resid` | `fmpz c, m` | the set `P(m, c)` of `solvers` 1.10: the rationals in `c + m Z_p` for every prime `p` dividing `m` | `m >= 1` and `0 <= c < m` | `(0, 1)`: every rational |
| `adf_recon_cert` | `fmpz Rp, Tp, R, T`; `int kind` | the certificate pair of `solvers` 1.3, or none | `kind` in `{0, 1}`; `kind = 0`: all four are 0 | `kind = 0` |
| `adf_linsol` | see section 3 | the set of solutions of `A x = b` modulo `N`, or the proof that it is empty | see section 3 | the solution of the empty system modulo 1 |
| `adf_rootlist` | see section 4 | a list of balls, each holding one root, with certificates; its scope PARTITION or SEED | see section 4 | the empty list |

Layouts (64 bit): `adf_resid_struct` 16 bytes (`c` at 0, `m` at 8); `adf_recon_cert_struct` 40 bytes (`Rp` 0,
`Tp` 8, `R` 16, `T` 24, `kind` 32). The layouts of `adf_linsol` and `adf_rootlist` are fixed by the
implementation lane of the header and then pinned in `tests/test_abi.c`; they own heap memory and have
`init` and `clear` (`conventions.md` 12.9).

Every type has the life cycle of `conventions.md` 2.3: `init`, `clear`, `set`, `swap`, `is_canonical`,
`identical`, and `adf_sizeof_<type>`, `adf_alignof_<type>`. Text and dump forms: decision S-D12.

## 2. Functions of S.3: reconstruction from a residue

In this section `ell = max(limit, 0)`, `T` is the denominator entry of the certificate pair of `solvers` 1.4 and
`X = floor(B / abs(T))`.

| Function | Set statement | Implements | Statuses and outputs | Cost |
|---|---|---|---|---|
| `int adf_resid_set_fmpz2(adf_resid_t x, const fmpz_t c, const fmpz_t m)` | `x = P(m, c)`; `c` is reduced into `[0, m)` | `solvers` D1.1 (the set depends on `c` modulo `m` only) | `OK`; `DOMAIN` (edit E-C1) if `m < 1`, `x` untouched | one division |
| `void adf_resid_get_fmpz2(fmpz_t c, fmpz_t m, const adf_resid_t x)` | the pair of `x` | | none | copies |
| `int adf_resid_set_rat(adf_resid_t x, const adf_rat_t q, const fmpz_t m)` | the `P(m, c)` that contains `q = n/d`: `c = n d^(-1)` modulo `m` | `solvers` L1.2 | `OK`; `DOMAIN` if `m < 1` or `gcd(d, m) > 1` (then `q` is in no `P(m, c)`, L1.2(1)); `x` untouched | one modular inverse |
| `int adf_resid_contains_rat(const adf_resid_t x, const adf_rat_t q)` | 1 if `q` is in `P(m, c)`, else 0 | `solvers` L1.2, P1.10(1) | predicate | one gcd, one reduction |
| `int adf_resid_set_fball_forget(adf_resid_t x, const adf_fball_t b)` | for `b = (A + H Zhat)/d` (canonical triple) with `H >= 1` and `gcd(d, H) = 1`: `x = P(H, A d^(-1) mod H)`, which contains every rational of `b` and more | `solvers` P1.10(4) | `OK`; `DOMAIN` if `H = 0` (an exact value is no residue) or `gcd(d, H) > 1`; `x` untouched | one modular inverse; a local `b` is recombined first |
| `int adf_resid_reconstruct(adf_rat_t q, adf_recon_cert_t cert, const adf_resid_t x, const fmpz_t A, const fmpz_t B, slong limit)` | decides `Sol(m, c, A, B)`: exactly one element, none, several | `solvers` P1.6, Algorithm R, P1.7 | `OK`: `q` is the only solution. `NO_SOLUTION`: proved; also for `A < 0` or `B < 1` (the box is empty). `NOT_UNIQUE`: two solutions were found (also inside a cut search). `NOT_DETERMINED`: exactly when `A < m <= 2 A B`, `abs(T) <= B`, `X > ell`, and the rounds `x <= ell` found fewer than two reduced pairs (`solvers` P1.7(3)): "uniqueness not certified". `q` untouched unless `OK`. `cert` (may be NULL) is written on every status: the pair if `0 <= A < m` and `B >= 1`, else `kind = 0`. `limit < 0` is taken as 0 | P1.7(4): one Euclidean algorithm on `(m, c)`, then at most `min(ell, X)` rounds |
| `int adf_resid_reconstruct_first(adf_rat_t q, int * count, adf_recon_cert_t cert, const adf_resid_t x, const fmpz_t A, const fmpz_t B, slong limit)` (decision S-D4) | `q` is a solution, the first in the order of Algorithm R (for `A >= m`: `c/1`) | `solvers` P1.6, P1.7 | `OK`: `q` written, a verified solution; `*count` is 1 if `q` is proved to be the only solution, 2 if a second one was found (also `A >= m`), 0 if the search was cut with one candidate found. `NO_SOLUTION`: proved, `q` untouched, `*count = 0`. `NOT_DETERMINED`: the search was cut before any solution was found, `q` untouched, `*count = 0`. `*count` and `cert` are written on every status | as above |
| `int adf_recon_cert_check(const adf_recon_cert_t cert, const adf_resid_t x, const fmpz_t A)` | 1 if `cert` has `kind = 1` and satisfies (C1) to (C4) for `(m, c, A)`, else 0 | `solvers` D1.3 | predicate | four multiplications, two reductions |
| `int adf_resid_verify_result(const adf_resid_t x, const fmpz_t A, const fmpz_t B, slong limit, int status, const adf_rat_t q, const adf_recon_cert_t cert)` | 1 if the claim "the function returned `status` (with `q` for `OK`)" is true by the tests of `solvers` P1.11; else 0 | `solvers` P1.11 | predicate | `NO_SOLUTION` and `OK` in the range `m <= 2 A B`, and `NOT_UNIQUE` for `A < m`: the cost of the search itself; the other cases: `adf_recon_cert_check` and a few comparisons |

Notes.

1. **No function of this header takes an `adf_fball` as the residue**, and `adf_fball_reconstruct`,
   `adf_adele_reconstruct` (`recon.h`) take no `adf_resid`. The only passage is
   `adf_resid_set_fball_forget`, whose name says what it does (`solvers` P1.10). A test of the implementation
   lane must show `5 + 6 Zhat`: `adf_fball_reconstruct` on `[0, 1]` gives `NO_SOLUTION`; after `forget`,
   `adf_resid_reconstruct` with `A = 1`, `B = 5` gives `NOT_UNIQUE` (`-1/1` and `1/5`), with `B = 4` gives
   `OK` and `-1/1`.
2. **`m = 1`, `m = 2`, `A = 0`.** All are admitted; the answers are those of `solvers` P1.6 and are in the
   table of `check_s3_edge`. `A = 0`: the only possible solution is `0/1`, exactly when `c = 0`.
3. **FLINT.** The function runs its own loop (S-D2). It never passes `A = 0` or `m <= 2` to
   `fmpq_reconstruct_fmpz_2` (`solvers` P1.9(6)).
4. **What the checker of a result does** (`solvers` P1.11). `adf_recon_cert_check` first; then the case of
   `solvers` P1.6 that the status claims: (b) `abs(T) > B`; (c) `2 A B < m` and `gcd(R, T)`; (d) the enumeration,
   whose cost is that of the search itself. A `NOT_UNIQUE` is certified by two different pairs that are both
   solutions (tested from Definition 1.1), or by repeating the complete enumeration until two are found. It is
   NOT certified by a second call with the bound `B` lowered below the first denominator: for `m = 2`, `c = 1`,
   `A = B = 1` both solutions `1/1` and `-1/1` have denominator 1, and with `B = 0` the box is empty.
   `NOT_DETERMINED` is certified by recomputing the four conditions of `solvers` P1.7(3).
5. **With `limit <= 0` the status is not "`2 A B >= m`".** `A >= m` gives `NOT_UNIQUE`, `abs(T) > B` gives
   `NO_SOLUTION`, `2 A B < m` gives the answer of `solvers` P1.6(c); `NOT_DETERMINED` is returned exactly for
   `A < m <= 2 A B` with `abs(T) <= B` (example: `m = 2`, `c = 1`, `A = 2`, `B = 1` gives `NOT_UNIQUE`).

Acceptance tests (each must fail for a wrong implementation, `PLAN.md` section 7):

| Function | The test must check |
|---|---|
| `adf_resid_reconstruct` | `solvers` P1.6: status and solution against the enumeration of D1.1 for every `m <= 36`, `c` inside and outside `[0, m)`, EVERY `A` from 0 to `2 m`, the values of `B` of `check_s3_complete`; P1.7(3) with the limits -1, 0, 1, 2, 5, with the reduced points of the rounds `x <= ell` recounted independently of the loop (`check_s3_limit`: a solver that returns `NOT_DETERMINED` where two solutions had been found must fail); the fixed cases of `check_s3_edge`; random fractions of 20, 64, 300 bits with `m > 2 A B`, each counted only when it was verified; the certificate with `adf_recon_cert_check`; outputs untouched on every other status; mutation testing of the comparisons `R <= A`, `abs(T) > B`, `2 A B < m`, `X > ell`, the count of two found solutions, and of the bounds of `y` |
| `adf_resid_reconstruct_first` | the `count` on every status against the enumeration: 1 only if `Sol` has one element, 2 only if it has at least two, 0 with `OK` only after a cut |
| `adf_resid_verify_result` | every result of `adf_resid_reconstruct` is accepted; changed claims (another status, another pair, a wrong certificate) are refused or true (`check_s3_verify`) |
| `adf_resid_set_fball_forget` | `solvers` P1.10(4): every rational `(A + H k)/d` of the ball is contained in the result; `1/5` and `5 + 6 Zhat` |
| all | aliasing; `is_canonical` after every `OK`; a wrapper test against `fmpq_reconstruct_fmpz_2` inside `2 A B < m` only, labelled as such |

## 3. Functions of S.1: linear systems modulo `N`

**The type `adf_linsol`.** One value holds the complete answer to one system: the modulus `N`, the sizes
`r`, `c`, a kind, and the matrices of `solvers` P2.6 to P2.8, all with entries in `[0, N)`:

| Field | Shape | Present | Meaning |
|---|---|---|---|
| `kind` | `int` | always | `ADF_LINSOL_COSET = 0`: the solutions are `x0 + S(G)`; `ADF_LINSOL_EMPTY = 1`: there is none |
| `N` | `fmpz` | always | the modulus, `N >= 1` |
| `r`, `c` | `slong` | always | the sizes of the system |
| `G` | `k` by `c` | always | the Howell form of the kernel (`solvers` D2.1, P2.4): the canonical generators |
| `E`, `V` | `e` by `r`, `e` by `c` | always | the certificate of completeness: `A V_i = E_i` (`solvers` P2.6) |
| `x0` | `c` by 1 | kind COSET | a particular solution |
| `y` | `r` by 1 | kind EMPTY | the certificate that there is no solution: `y^T A = 0`, `y^T b != 0` (`solvers` P2.7) |

The matrices are `fmpz_mat` owned by the value. Predicate (`adf_linsol_is_canonical`, what can be tested
without the system): `N >= 1`; `kind` in `{0, 1}`; the shapes above, a matrix that is not present has shape
0 by 0; entries in `[0, N)`; (K1), (K4), (K5) of `solvers` P2.6. Init value: `kind = COSET`, `N = 1`,
`r = c = 0`, no rows anywhere: the solution of the empty system modulo 1. Two values for the same
`(A mod N, N)` have identical `G`; `x0`, `E`, `V`, `y` are determined by Algorithm L of `solvers` 2.8, so
that `adf_linsol_identical` holds for two results of the library on the same input; a certificate made
elsewhere may differ in them and still be valid.

| Function | Set statement | Implements | Statuses and outputs | Cost |
|---|---|---|---|---|
| `int adf_linsolve_mod(adf_linsol_t sol, const fmpz_mat_t A, const fmpz_mat_t b, const fmpz_t N)` | the set of all `x` in `(Z/N)^c` with `A x = b` modulo `N`: the coset `x0 + S(G)`, or empty. `A` is `r` by `c`, `b` is `r` by 1, entries any integers | `solvers` Algorithm L, P2.8; P2.6 (kernel complete and canonical); P2.7 (none) | `OK`: `sol` written, kind COSET. `NO_SOLUTION`: proved; `sol` written, kind EMPTY, with `y` (decision S-D6: `sol` is the report of the function). `DOMAIN` (edit E-C1): `N < 1`, or `b` is not `r` by 1; `sol` untouched. `LIMIT`: `r + c > ADF_LINSOLVE_DIM_MAX` (S-D8), decided before any allocation; `sol` untouched. Never `NOT_DETERMINED`, never `NOT_UNIQUE`: several solutions are the normal case and are described by `G` | P2.5(4), P2.8(5): `O((r + c)^3 log N)` multiplications modulo `N` at most |
| `int adf_linsol_verify(const adf_linsol_t sol, const fmpz_mat_t A, const fmpz_mat_t b, const fmpz_t N)` | 1 if `sol` has the modulus and sizes of the system and (K1) to (K5) and (K6) or (K7) hold; else 0. This verifies a COMPLETE result: the conditions imply the whole answer (S-D17) | `solvers` P2.6(1), (2), P2.7(1), P2.8(3): the conditions imply the answer | predicate; never aborts on a value that satisfies `is_canonical` | P2.6(4): at most `r c (r + c) + k^2 c` multiplications; no elimination |
| `int adf_linsol_kind(const adf_linsol_t sol)` | the kind | | none | constant |
| `slong adf_linsol_kernel_rows(const adf_linsol_t sol)` | `k`, the number of canonical generators, `0 <= k <= c` | `solvers` P2.5(3) | none | constant |
| `void adf_linsol_kernel_order(fmpz_t n, const adf_linsol_t sol)` | the number of elements of the kernel, `(N/g_1) ... (N/g_k)`; for kind COSET also the number of solutions modulo `N` | `solvers` P2.6(1) | none | `k` divisions |
| `void adf_linsol_get_kernel(fmpz_mat_t G, const adf_linsol_t sol)` | a copy of `G` | | none; the output matrix is initialised by the caller and is set to the right shape by the function (`fmpz_mat_clear`, `fmpz_mat_init`) | copy of `k c` entries |
| `void adf_linsol_get_image_cert(fmpz_mat_t E, fmpz_mat_t V, const adf_linsol_t sol)` | copies of `E` and `V` | | none; as above | copy |
| `int adf_linsol_get_particular(fmpz_mat_t x0, const adf_linsol_t sol)` | a copy of `x0` | | returns 1 if the kind is COSET and `x0` was copied; else 0 and `x0` untouched. A predicate, not a status | copy |
| `int adf_linsol_get_dual(fmpz_mat_t y, const adf_linsol_t sol)` | a copy of `y` | | returns 1 if the kind is EMPTY and `y` was copied; else 0 and `y` untouched. A predicate, not a status | copy |
| `int adf_linsol_contains(const adf_linsol_t sol, const fmpz_mat_t x)` | 1 if kind COSET and `x` (a `c` by 1 integer vector) is a solution, that is `x - x0` is reduced to zero by `G`; else 0 | `solvers` L2.3(2) | predicate | `k c` multiplications |
| `int adf_linsolve_fball(adf_linsol_t sol, const fmpz_mat_t A, const adf_fball_struct * b, slong r)` | the set of all `x` in `Zhat^c` with `(A x)_i` in the ball `b[i]` for all `i`: the `x` whose class modulo `N = lcm(H_i)` is in `x0 + S(G)`, or empty; `sol` describes the modular solution coset, of which the set in `Zhat^c` is the inverse image (`solvers` P2.9) | `solvers` P2.9 | as `adf_linsolve_mod`; `UNSUPPORTED` (edit E-C1) if some `b[i]` is exact (`H = 0`); `DOMAIN` if `r` is not the number of rows of `A`. A local `b[i]` is used through its canonical triple | as above, for the matrix `A'` of P2.9 |
| `int adf_linsol_verify_fball(const adf_linsol_t sol, const fmpz_mat_t A, const adf_fball_struct * b, slong r)` | the check of `adf_linsol_verify` for the system `(A', b', N)` of P2.9 | `solvers` P2.9, P2.6 | predicate | as `adf_linsol_verify` |
| `int adf_linsol_get_fball(adf_fball_t x, const adf_linsol_t sol, slong j)` | the ball `x0[j] + rho_j Zhat`, `rho_j = gcd(N, G[1, j], ..., G[k, j])`: the set of the `j`-th coordinates of the solutions in `Zhat^c`. The product over `j` contains the set of solutions and is in general larger | `solvers` P2.10 | `OK`: `x` written, global backend, canonical; `NO_SOLUTION` on kind EMPTY; `DOMAIN` (edit E-C1) if `j` is outside `[0, c)`; `x` untouched | `k` gcds |

Notes.

1. **Zero matrix, `N = 1`, `r = 0`, `c = 0`** are ordinary inputs; their answers are in `solvers` P2.8(4) and
   in `check_s1_edge`. A `fmpz_mat` with zero rows or columns is a valid FLINT matrix
   (`flint-3.0.1:fmpz_mat.rst:356` to `359`: `fmpz_mat_is_empty` tests "if the number of rows or the number
   of columns in `mat` is zero"; `flint-3.0.1:nmod_mat.rst:20`: "Matrices having zero rows or columns are
   allowed"). The library's own loops treat the sizes 0 and do not pass an empty matrix to a FLINT routine
   whose entry does not admit it (`fmpz_mat_mul_blas` "will fail if the matrices are empty",
   `fmpz_mat.rst:606`). The Howell routines of FLINT return 0 at once on an empty matrix and read outside a
   NONEMPTY matrix with fewer rows than columns, which must be padded (`solvers` P2.11(2)).
2. **`N` is any integer `>= 1`**: composite, with square factors, of any size. No factorisation is made and
   no primality is assumed or tested (`solvers` 2.11).
3. **What is verified before the function returns.** Every result is passed through the conditions of
   `adf_linsol_verify` inside `adf_linsolve_mod` (`solvers` Algorithm L, step 5). A refusal cannot happen
   for Algorithm H (`solvers` P2.5, P2.6(3), P2.8); if the engine is FLINT (S-D7) the certificate is first
   tested for its dimensions, entries in `[0, N)` and (K1) to (K5), which do not use `b`, BEFORE step 3 of
   Algorithm L reduces `b` by it (that step divides by the pivots and assumes valid ones), and (K6) or (K7)
   last; a refusal starts Algorithm H (`solvers` P2.11, decision S-D7).
4. **Completeness of the kernel** is not tested "on small cases" only: (K4) is an identity that the checker
   tests for every system, and with (K1) to (K3) it implies that the generators generate the whole kernel
   (`solvers` P2.6(1)). The test on small cases (`PLAN.md` section 7) remains as the acceptance test of the
   implementation.
5. **Matrices with entries in balls** are not offered (`solvers` P2.9, last list).
6. **Unknowns.** For integer right-hand sides the unknowns are in `(Z/N)^c`. For finite-ball right-hand sides the
   unknowns are in `Zhat^c` and the output describes the modular solution coset whose inverse image is the set
   (`solvers` P2.9); the balls of `adf_linsol_get_fball` are its coordinate projections (P2.10).

Acceptance tests:

| Function | The test must check |
|---|---|
| `adf_linsolve_mod` | against the enumeration of `(Z/N)^c`: the status; `x0 + S(G)` is the set of all solutions (`solvers` P2.8(3)); `S(G)` is the whole kernel (P2.6(1)); `G` equals the Howell form computed from the list of all kernel elements (P2.4): all systems modulo 4 with `r, c <= 2`, random systems for `N` in 1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 16, 18 with `r, c <= 3`; the fixed cases of `check_s1_edge`; `N` of 64 to 122 bits with a planted solution; the same kernel from permuted and rescaled equations prints identically |
| `adf_linsol_verify` | `solvers` P2.6(1), P2.7(1): changed certificates (a generator removed or changed, a row of `E`, `V` removed or changed, `x0` or `y` changed, the kind flipped, a generator multiplied by a unit, the certificate of another matrix) are refused, or what they claim is true by enumeration; mutation testing of each of (K1) to (K7): a checker without one of them must accept a false certificate of the test set (`check_s1_checker_mutants` does this for the reference checker). A valid alternate certificate may pass; the test separates false claims from different valid certificates |
| `adf_linsolve_fball`, `adf_linsol_get_fball` | `solvers` P2.9, P2.10: membership of `A x` in the balls by exact rational arithmetic on integer points, also on `x + N z`; the residues of the coordinates; an exact ball gives `UNSUPPORTED`; local and global right-hand sides give identical results |
| engine | a wrapper test of `fmpz_mat_howell_form_mod` against Algorithm H, labelled as such; fuzzing of the matrix sizes 0 and 1; a certificate from FLINT with a false pivot is refused before step 3 |

## 4. Functions of S.2: roots

**The type `adf_rootlist`.** One value holds the roots of one polynomial at one place, with what proves them:

| Field | Present | Meaning |
|---|---|---|
| `place` | always | the prime `p`, or the real place (`adf_place_t`, `conventions.md` 7) |
| `scope` | always | `PARTITION` (0) or `SEED` (1). PARTITION: the listed balls and unresolved classes cover every root of the input at this place, and `complete = 1` exactly when `nu = 0`. SEED: the list of `adf_root_padic_from_seed`: `n = 1`, `nu = 0`, `complete = 0`; no statement about the other roots |
| `g` (`fmpz_poly`) | always | the polynomial that the certificates refer to: the normalised polynomial `g*` of the input (`solvers` L3.1(3)): the primitive integer squarefree part with positive leading coefficient (decision S-D13) |
| `reduced` (`int`) | always | 1 iff `f` has a repeated irreducible factor over `Q`, equivalently `deg gcd(f, f') > 0` for nonconstant `f`. Multiplication by a nonzero constant and removal of content alone do not set this flag. Root multiplicities are never returned (edit R12 of the closure check) |
| `n` | always | the number of roots in the list, `n >= 0` |
| `complete` (`int`) | always | 1: the list holds every root of the input at this place (`solvers` P3.5(3), P3.10) and has been produced by a run that closed every class; 0: it may not |
| `a[i]`, `K[i]`, `s[i]` | at a prime | the root certificate `(a, K, s)` of root `i` (`solvers` D3.2): the ball `a + p^K Z_p` holds exactly one root of `g`, in increasing order of `a` |
| `ua[i]`, `ue[i]`, `nu` | at a prime | the `nu` unresolved classes `ua + p^ue Z_p` (`solvers` P3.5) |
| `ball[i]` (`arb`) | at the real place | the isolating balls in increasing order; the exact end points of each satisfy the test of `solvers` P3.8; `arb_rel_accuracy_bits(ball[i]) >= max(prec, 2)` (exact balls allowed) |
| `count` | at the real place | the number of distinct real roots of `g` (`solvers` P3.9(1)); `complete = 1` exactly when `n = count` and the tests of RR passed |

Predicate (`adf_rootlist_is_canonical`, without the input polynomial): the shapes above; PARTITION: `complete = 1`
exactly when `nu = 0`; SEED: `n = 1`, `nu = 0`, `complete = 0`; at a prime (R1) for every certificate, the centres
increasing, the balls and classes pairwise disjoint (`solvers` P3.2(4)); at the real place finite balls with
`hi_i < lo_(i+1)`. Init value: the real place, scope PARTITION, `g = 1`, `n = 0`, `complete = 1` (the constant 1
has no root).

Two verifiers, with names that say what they check (decision S-D17):

| Function | Set statement | Implements | Statuses and outputs | Cost |
|---|---|---|---|---|
| `int adf_rootlist_verify_entries(const adf_rootlist_t L, const fmpz_poly_t f)` | 1 if `g` is the normalised polynomial of `f`; at a prime every listed certificate satisfies (R1) to (R3) for `g` and the listed balls and classes are pairwise disjoint; at the real place every ball passes the exact test of `solvers` P3.8 for `g` and the balls are pairwise disjoint. It verifies that each listed prime ball holds exactly one root and each real ball at least one root. It does NOT verify a flag `complete`, a count `n` or a coverage: an empty list for `f = X (X - 1)` at 3 with `complete = 1` passes | `solvers` P3.13(1), (4) | predicate | two evaluations modulo `p^(K+s)` for each root; two exact evaluations for each real root; one gcd |
| `int adf_rootlist_verify_complete(const adf_rootlist_t L, const fmpz_poly_t f, slong depth)` | 1 only if `complete = 1`, the scope is PARTITION and completeness has been independently verified. At a prime: the entries verify, `nu = 0`, Algorithm P is rerun on `g` through `depth`, it leaves no unresolved class, and every root ball of the rerun meets exactly one listed ball and every listed ball exactly one root ball (`solvers` P3.2(4)). At the real place: the entries verify and `n` and the number of balls both equal the count of the real roots of `g`, recomputed here (P3.9(1), or a Sturm chain), never read from the list. Returns 0 if verification fails or the depth given does not suffice. A SEED list never passes | `solvers` P3.13(2), (3), (5) | predicate | at a prime: the cost of the search itself; at the real place: one count and `2 n` exact evaluations |

| Function | Set statement | Implements | Statuses and outputs | Cost |
|---|---|---|---|---|
| `int adf_roots_padic(adf_rootlist_t L, const fmpz_poly_t f, adf_place_t p, slong prec_p, slong depth)` | the list of all roots of `f` in `Z_p`, each as a ball `a + p^K Z_p` with `K = max(prec_p, s + 1)` that holds exactly one root of `g` (`K` is sufficient for isolation, not the least isolating precision: `solvers` 3.11(5)); scope PARTITION | `solvers` Algorithm P, P3.5(1), (2), (3); P3.2; P3.3 | `OK`: `L` written, `complete = 1`; the list may be empty (no root in `Z_p`: this is an answer, not `NO_SOLUTION`). `NOT_DETERMINED`: classes remain unresolved at this `depth` (a larger `depth` may resolve them; with the squarefree part of S-D13 the depth `v_p(R) + 1` of P3.6 suffices when an integer identity `u g + v g' = R` is known); `L` untouched. `DOMAIN` (edit E-C1): `f = 0`, the place is not a prime, `prec_p < 1`, `depth < 0`. `UNSUPPORTED` (edit E-C1; S-D10): above the temporary bound of a slice that finds the roots modulo `p` by evaluation at every residue (`solvers` P3.7(1)); the library accepts every prime, and a later slice takes the route of P3.7(2) for larger primes. `LIMIT`: an exponent or size beyond the limits of S-D18, decided before allocation. `L` untouched on all of these | P3.5(7): for each class opened `p deg f` multiplications modulo `p`; for each root `O(log(prec_p))` Newton steps modulo `p^(2 K)` |
| `int adf_roots_padic_partial(adf_rootlist_t L, const fmpz_poly_t f, adf_place_t p, slong prec_p, slong depth)` (decision S-D15) | the certified roots found and the unresolved classes; every root of `f` in `Z_p` is in exactly one ball or class; scope PARTITION | `solvers` P3.5(1), (2), (5) | `OK`: `L` written, with `complete` 0 or 1. `DOMAIN`, `UNSUPPORTED`, `LIMIT` as above | as above |
| `int adf_root_padic_from_seed(adf_rootlist_t L, const fmpz_poly_t f, adf_place_t p, const fmpz_t a, slong prec_p)` | the seed condition, `s` and the certificate refer to `g`, the normalised polynomial of `f`: if `g'(a)` is not 0 and `v_p(g(a)) > 2 v_p(g'(a))` (the strong form), the one root `alpha` of `g` (and of `f`) with `v_p(alpha - a) > s = v_p(g'(a))`, as a list of one ball of precision `K = max(prec_p, s + 1)`; scope SEED: `n = 1`, `nu = 0`, `complete = 0` (other roots are not sought and no partition is claimed) | `solvers` (H2), P3.12, P3.2, P3.3 | `OK`: `L` written. `NOT_DETERMINED`: the seed does not satisfy the strong form for `g`; `L` untouched. `DOMAIN` (`f = 0`, `prec_p < 1`, not a prime), `UNSUPPORTED`, `LIMIT` as above. Example: `f = 27 X`, `p = 3`, `a = 0`: `g = X`, the certificate is `(0, K, 0)`, not `(0, K, 3)` | `O(log(prec_p))` Newton steps |
| `int adf_roots_real(adf_rootlist_t L, const fmpz_poly_t f, slong prec)` | the list of all distinct real roots of `f`, each in a ball that holds exactly one root, with `arb_rel_accuracy_bits(ball) >= max(prec, 2)` measured on the balls that are output, exact balls allowed (`flint-3.0.1:arb.rst:499` to `509`); scope PARTITION | `solvers` Algorithm RR, P3.10, P3.8 | `OK`: `L` written, `complete = 1`, `n = count`. `NOT_DETERMINED`: a test of P3.8 failed at this precision, or the two counts differ, or the accuracy of a ball after widening is below `max(prec, 2)` (decision S-D19); a larger `prec` may resolve it; `L` untouched. `DOMAIN` (edit E-C1): `f = 0`. A `prec` below 2 is taken as 2 (M1-D4) | one gcd of `f` and `f'`; FLINT's count and isolation; `2 n` exact evaluations of `g` at dyadic points of about `prec` bits |
| `slong adf_rootlist_length(L)`, `int adf_rootlist_is_complete(L)`, `slong adf_rootlist_unresolved_length(L)`, `adf_place_t adf_rootlist_place(L)`, `int adf_rootlist_scope(L)` | the fields | | none | constant |
| `void adf_rootlist_get_poly(fmpz_poly_t g, const adf_rootlist_t L)` | a copy of `g` | | none | a copy: `O(deg g)` coefficient copies, not constant |
| `int adf_rootlist_get_cert(fmpz_t a, slong * K, slong * s, const adf_rootlist_t L, slong i)` | the certificate of root `i` at a prime | `solvers` D3.2 | 1 if written, 0 if `L` is at the real place or `i` is outside `[0, n)` | copy |
| `int adf_rootlist_get_unresolved(fmpz_t a, slong * e, const adf_rootlist_t L, slong i)` | the unresolved class `i` | `solvers` P3.5 | 1 or 0 as above | copy |
| `int adf_rootlist_get_fball(adf_fball_t x, const adf_rootlist_t L, slong i)` (decision S-D14) | at a prime: the finite ball `a_i + p^(K_i) Zhat`, that is the set of the `x` in `Zhat` whose component at `p` lies in the ball `a_i + p^(K_i) Z_p` of root `i`. It is a CYLINDER: it says nothing about the components at the other primes beyond integrality, and it is not a set of roots of `f` in `Zhat` | `SPEC.md` 4.1 (precision at `p` is `v_p(H/d)`) | 1 if written (global backend, canonical: `(a_i, p^(K_i), 1)`), 0 if `L` is at the real place or `i` is outside `[0, n)` | one power |
| `int adf_rootlist_get_arb(arb_t x, const adf_rootlist_t L, slong i)` | at the real place: the ball of root `i` | `solvers` P3.10 | 1 or 0 | copy |

Notes.

1. **A root modulo `p` that does not lift.** `x^2 + 1` at 2: `adf_roots_padic` with `depth >= 1` returns
   `OK` with the empty list; with `depth = 0` it returns `NOT_DETERMINED`, and `_partial` returns the
   unresolved class `1 + 2 Z_2` (`solvers` 3.11(2)).
2. **Multiple roots.** Under S-D13 the functions work on the normalised polynomial `g`, so a multiple root of `f`
   is found as a simple root of `g` and `reduced = 1` says that the input had a repeated factor; multiplicities are never returned. If S-D13 is
   decided the other way, `g` is `f` with its content removed, a multiple root is never resolved, and
   `adf_roots_padic` returns `NOT_DETERMINED` for every `depth` (`solvers` P3.5(5)); for the real place the
   function then returns `UNSUPPORTED` for an input that is not squarefree, because FLINT's count and
   isolation ask for a squarefree argument (`solvers` P3.9(1), (2)).
3. **What certifies completeness.** At a prime the list is complete because Algorithm P has closed every
   class: the proof is the run itself, and `adf_rootlist_verify_complete` runs the search again, at the same
   cost. No shorter certificate is proposed for version 1. At the real place completeness rests on the count,
   which is FLINT's promise (S-D11), recomputed by the verifier.
4. **The prime 2** needs no separate function. Square roots at 2 are never found at level 0 and have
   `s >= 1` (`solvers` 3.11).
5. **Roots outside `Z_p`** are not returned; the caller applies the function to the reversed polynomial
   (`solvers` section 3, first paragraph).
6. **Real precision after widening** (decision S-D19). The accuracy promise of the balls of the engine
   (`arb_fmpz_poly.rst:98`) does not survive a widening (`solvers` P3.10(4)); the accuracy is measured on the
   balls that are output. A C test must measure the final stored ball with `arb_rel_accuracy_bits`, not assume
   the requested `prec`.

Acceptance tests:

| Function | The test must check |
|---|---|
| `adf_roots_padic`, `_partial` | `solvers` P3.2(1): for each ball, the number of `x` modulo `p^(K+s+3)` in `a + p^(s+1) Z_p` with `f(x) = 0` modulo `p^(K+s+3)` is `p^s`, by enumeration; P3.5(2): every approximate root of the enumeration lies in exactly one ball or class; P3.5(3), (5): the 31 named cases of `PADIC_CASES` with their numbers of roots and the depth limits 0, 1, 3, 8; random products with planted integer roots; `K = max(prec_p, s + 1)` and nested balls for growing `prec_p` (P3.3); the least isolating precision may be below `s + 1` (`check_s2_isolation`: `K` is not claimed minimal); polynomials with multiple factors give complete lists of distinct roots under S-D13; mutation testing of the conditions `g'(b) != 0`, `j > w - 2e`, `k > s`; overflow of `e + j`, `k + s`, `2 k - s` gives `LIMIT` and not a wrong list |
| `adf_root_padic_from_seed` | (H2): seeds with `v(g(a)) = 2 v(g'(a))` exactly are refused; Conrad's examples 4.2, 4.3, 4.4 with their residues; `f = 27 X` at 3 gives the certificate `(0, K, 0)` for `g = X`; every certificate of the seed function passes `adf_rootlist_verify_entries`, and no seed list passes `adf_rootlist_verify_complete` |
| `adf_rootlist_verify_entries`, `_complete` | changed lists (a certificate dropped or changed, two balls of one root, a class kept with `complete = 1`, a false `n`, a ball moved off its root, two balls merged, a ball stretched over two roots) are refused by the entries verifier or, if the list is only short, by the complete verifier; `f = X (X - 1)` at 3 with an empty list called complete passes the first and fails the second; the interval `[0, 4]` for `(X-1)(X-2)(X-3)` passes the first and fails the second (`check_s2_lists`); a depth one below the least depth that completes the run is refused |
| `adf_roots_real` | `solvers` P3.8: the 18 cases of `REAL_CASES` (close roots, multiple roots, rational and dyadic roots, no real root, degree 0 and 1) against a count by an independent Sturm chain in exact arithmetic and mpmath; each ball holds one root and the gaps hold none; the planted roots of `check_s2_real_planted` (repeated, `10^30`, `2^-40` apart, `X - 10^400`); `arb_rel_accuracy_bits` of every output ball measured; lists with a ball removed, merged or moved are refused by `adf_rootlist_verify_complete` |
| all | the zero polynomial; `adf_rootlist_get_fball` of a root gives the canonical triple `(a, p^K, 1)`; outputs untouched on every status other than `OK` |

## 5. Decisions for TJO

Each decision is a question, the recommendation of this lane, and the alternatives, stated so that each can be
chosen on its merits (the review found two of them described unfairly). Nothing in `solvers.md` depends on a
decision except where named. TJO decided them on 2026-09-29 on the recommendation of this lane, S-D10 otherwise;
the decision of each is the row of the same id in `docs/SPEC.md` 15.3, and the table below stays as the record
of the alternatives that were asked.

| Id | Question | Recommended | Alternative |
|---|---|---|---|
| S-D1 | Which type carries the input of the partial reconstruction? | A new type `adf_resid` `(c, m)`, meaning the set `P(m, c)` of `solvers` P1.10; no function of this problem takes an `adf_fball`; the only passage is `adf_resid_set_fball_forget` | Raw arguments `(c, m)` as `fmpz` with no type (less safe: nothing in the signature separates the two problems); a factored partial-ball type (it needs the primes of `m`, that is a factorisation); or wait for `adf_sball` of milestone 1F |
| S-D2 | Does the reconstruction call FLINT? | No: its own Euclidean loop, which gives both rows of the certificate pair (`solvers` P1.9). `fmpq_reconstruct_fmpz_2` is used in tests only, as a wrapper test inside `2 A B < m` | FLINT as a fast path for `2 A B < m`, `m > 2`, `A >= 1`, into temporaries, return value 1 accepted after four tests, own loop after return value 0 (fair; it saves work for large moduli) |
| S-D3 | Which status when the search limit stops the search of `solvers` Algorithm R? | `NOT_DETERMINED`, the "uniqueness not certified" of `SPEC.md` 9.2 and CV-51, returned exactly when `A < m <= 2 A B`, `abs(T) <= B`, `floor(B/abs(T)) > ell` and the rounds `x <= ell` found fewer than two reduced pairs (`solvers` P1.7(3)); the fast cases keep their statuses also for `limit = 0`. It is NOT true that with `limit = 0` the status is `NOT_DETERMINED` exactly when `2 A B >= m` (`m = 2`, `c = 1`, `A = 2`, `B = 1` returns `NOT_UNIQUE`) | `LIMIT` ("a limit given as argument", `conventions.md` 3.1) for a cut search, with `NOT_DETERMINED` reserved for `2 A B >= m` and no search; consistent if that meaning is chosen explicitly |
| S-D4 | Is a verified candidate returned when uniqueness is not certified? | Yes, by a second function `adf_resid_reconstruct_first` with `count` defined on every status (1 proved unique, 2 a second one found, 0 for a cut search with one candidate, and 0 with no `q` on `NO_SOLUTION` and `NOT_DETERMINED`); the strict function keeps CV-06 | Only the strict function: the caller gets no candidate under `NOT_DETERMINED` and `NOT_UNIQUE` (fair; a smaller interface) |
| S-D5 | `A < 0` or `B < 1` | `NO_SOLUTION`: the box is empty, as the empty interval `lo > hi` of `adf_fball_reconstruct` | `DOMAIN` (a fair convention) |
| S-D6 | Where does the certificate of "no solution" of a linear system go, given that CV-06 leaves value outputs untouched on `NO_SOLUTION`? | The argument `sol` of `adf_linsolve_mod` is declared a report of the function: it is written on `OK` and on `NO_SOLUTION`, untouched on `DOMAIN` and `LIMIT`. This is an explicit exception to CV-06 and needs the sentence of edit E-C2; it keeps `NO_SOLUTION` as the status of an empty solution set | (a) Return `OK` whenever the request "compute the solution set" was answered, with the kind EMPTY or COSET in `sol`: `OK` then means that the requested result set was computed, as the root functions return `OK` for an empty complete list; `NO_SOLUTION` is not returned by the solver, and a caller that tests only the status must read `adf_linsol_kind` to tell an empty set from a coset. (b) Keep `sol` an ordinary value output (untouched on `NO_SOLUTION`) and add a separate report argument `y` for the non-solvability vector, written on `NO_SOLUTION`. Each preserves CV-06 without an exception on a whole result |
| S-D7 | Engine of the Howell form | Algorithm H in the library's own code (`solvers` P2.5, proved); the checker (K1) to (K7) runs on every result before it is returned | FLINT's `fmpz_mat_howell_form_mod` or `nmod_mat_howell_form` on `[A^T \| I_c]` padded with `r` zero rows; then the dimensions and (K1) to (K5) are checked BEFORE the greedy reduction of `b` consumes the certificate, (K6) or (K7) afterwards, and Algorithm H runs if the checker refuses. Or the integer Hermite form of `solvers` P2.12 |
| S-D8 | Size limit of a system | `ADF_LINSOLVE_DIM_MAX = 4096` for `r + c`; above it `LIMIT`, decided from the sizes before any allocation (as M1-D5). The table of Algorithm H has `(r + c)^2` entries: 16.8 million at the bound. This is a size policy, not a measured memory or time budget, and it does not bound the bit length of `N`, the size of the entries or of the pending vectors; a second limit on the bit length of `N` is open | No limit other than memory; an allocation failure then aborts in FLINT (fair) |
| S-D9 | One computation modulo `N`, or one for each block of a context with recombination? | One computation modulo `N`; right-hand sides in the local backend are recombined first (`solvers` 2.11, last paragraph) | Block by block for word-size speed, then Algorithm H modulo `N` on the recombined generators to restore the canonical form (it may still save work despite the final canonicalisation) |
| S-D10 | Which primes does the root finder accept? | DECIDED (TJO, 2026-09-29), other than recommended: every prime. A bound on `p` is a property of a slice of the implementation, not of the library. The first slice finds the roots modulo `p` by evaluation at every residue (`solvers` P3.7(1)) and returns `UNSUPPORTED` above a bound that its header calls temporary; a later slice adds the route of `solvers` P3.7(2) (roots by a routine of FLINT, each tested by evaluation, completeness by `deg gcd(g, X^p - X)`) for larger primes, with no bound of one word `[source pending: flint-3.0.1 fmpz_mod_poly.rst, fmpz_mod_poly_factor.rst, nmod_poly.rst]`. The bound is then the point where the method changes, set by a benchmark. Recommended as it was asked: `p <= ADF_ROOTS_P_MAX = 2^20`: the roots modulo `p` are found by evaluation at every residue, so no routine for roots in `F_p` is trusted; larger primes give `UNSUPPORTED`. The threshold is not benchmarked | Every prime of one word: roots modulo `p` by FLINT, each tested by evaluation, completeness by `deg gcd(g, X^p - X)` (`solvers` P3.7(2)); needs `nmod_poly.rst` on disk |
| S-D11 | Is FLINT's count of real roots trusted? | Yes: `fmpz_poly_num_real_roots` on the squarefree part; the isolation is not trusted and is tested by exact signs. The acceptance test compares the count with an independent Sturm chain | The library's own Sturm chain in exact arithmetic (then Sturm's theorem must be on disk and proved or cited) |
| S-D12 | Text and dump forms of `adf_resid`, `adf_linsol`, `adf_rootlist` (`conventions.md` 2.3 asks them of every value type); `adf_resid` is also an input type | None in milestone S: the types are results and certificates, read through accessors, and `adf_resid` is built by the setters of section 2; the grammar of `conventions.md` 9 and 10 is not changed. Needs the sentence of edit E-C3, which lists all four types (`adf_resid`, `adf_recon_cert`, `adf_linsol`, `adf_rootlist`) | A value form `c mod m` for `adf_resid` and dump forms for all, as a separate work package with golden vectors and fuzzing |
| S-D13 | Do the root finders work on `f` or on its normalised polynomial `g` (`solvers` L3.1(3))? | On `g`, at the primes and at the real place: the list is then the list of distinct roots, it can always be completed, `reduced = 1` says that the input had a repeated factor; multiplicities are never returned, and every certificate refers to `g`. This changes the scope of the SPEC row, whose "multiple roots ... later" becomes "multiplicities ... later" (edit E-S4, an owner decision) | On `f` with its content removed: a multiple root in `Z_p` gives `NOT_DETERMINED` at every depth, and a polynomial that is not squarefree gives `UNSUPPORTED` at the real place (a chosen restriction, not a mathematical necessity: the real roots can also be handled by a squarefree factorisation that keeps the multiplicities) |
| S-D14 | In which type is a root at a prime returned? | As the certificate `(a, K, s)`, and on request as the `adf_fball` `a + p^K Zhat` with the meaning stated in its entry (a cylinder at `p`, see `adf_rootlist_get_fball`); `adf_lball` is added as a third accessor when milestone 1F.3 brings the type | The certificate only, with no `adf_fball` accessor (an immediate option that avoids the cylinder reading); or wait for `adf_lball`, so that milestone S depends on 1F.3, against M1-D8 |
| S-D15 | Are partial lists returned (certified roots with unresolved classes)? | Yes, by `adf_roots_padic_partial`, with `complete = 0` or 1, scope PARTITION; the strict function keeps CV-06. The list of the seed function has scope SEED (S-D16) | Only the strict function |
| S-D16 | What is the scope of the list that the seed function returns, and what is the polynomial its certificate refers to? | A field `scope` with values PARTITION and SEED. PARTITION: the balls and classes cover every root and `complete = 1` exactly when `nu = 0`. SEED: `n = 1`, `nu = 0`, `complete = 0`, no coverage claimed. Every seed condition, `s`, lifting step and certificate refers to `g`, the normalised polynomial; `f = 0` is refused before `g` is formed | Give seed results a separate type or function family without `complete`; or make the seed function return a covering unresolved partition (it would need Algorithm P, so it is no longer a seed) |
| S-D17 | What does "verified" mean for a list of roots and for a result? | Two names: `adf_rootlist_verify_entries` checks the listed certificates (or existence in the listed real intervals) and disjointness, and certifies no flag or count; `adf_rootlist_verify_complete` certifies `complete = 1` by rerunning Algorithm P to a given depth (prime) or recomputing the count (real). `adf_linsol_verify` verifies a complete result because its conditions imply the answer; `adf_resid_verify_result` verifies a claimed status | One function with a flag saying whether completeness is to be verified; or only the entries verifier and no verification of completeness (a caller then cannot check a `complete = 1`) |
| S-D18 | Resource limits and arithmetic of the root functions | Every intermediate exponent (`e + j`, `k + s`, `2 k - s`) is computed with checked `slong` arithmetic; a value that overflows `slong`, or that makes `p^(2K)` larger than a limit `ADF_ROOTS_BITS_MAX`, gives `LIMIT` before any allocation (as M1-D5). The seed path also checks `k0 = v_p(g(a)) - s` before materialising `p^k0`. If `k0 >= K`, it constructs `(a mod p^K, K, s)` directly, without constructing `p^k0` (the certificate follows from the Taylor argument of `solvers` P3.12(2): `v_p(g(a)) >= K + s` and `K > s`). Every other power of `p` is checked against `ADF_ROOTS_BITS_MAX` before allocation (edit R13 of the closure check). The value of the limit is a policy that is not benchmarked | No limit; then a valid `slong` precision can overflow an intermediate exponent in a literal C implementation and give a wrong list (fair only with a proof that no overflow occurs) |
| S-D19 | Final accuracy of real roots and its status | The balls that are output have `arb_rel_accuracy_bits >= max(prec, 2)` (exact allowed), measured on the final stored balls after any widening; if it fails, or a test of P3.8 fails, or the counts differ: `NOT_DETERMINED`, output untouched (`solvers` P3.10(4)) | Promise only that each ball isolates its root and report the achieved accuracy in a report argument; or widen less (a ball that stays accurate needs an exact endpoint test) |

## 6. Exact edits proposed

Nothing below is applied. "Replace" gives the present text and the new text. The edits E-S1, E-S2, E-S3, E-S5,
E-P1, E-P2 and E-C4 are replaced as `docs/reviews/s-design/review.md` (repairs R9, R10) asks; E-C5 is deferred until
the root-list invariants and the prototypes are frozen.

### 6.1 `docs/SPEC.md`

**E-S1**, section 9.1, table, row "Linear systems modulo `N`" (line 511). Replace

    | Linear systems modulo `N` | a particular solution and generators of the kernel, with the transformation matrices as certificate; or a certificate that there is none |

by

    | Linear systems modulo `N` (`N >= 1` arbitrary, no factorisation) | Exact integer matrices. For integer right-hand sides, unknowns are in `(Z/N)^c` with arbitrary `N >= 1`. Return a particular solution and the Howell generators of the complete kernel, with the certificate of `proofs/solvers.md` P2.6, or a separating vector proving non-solvability. For positive-radius finite-ball right-hand sides, unknowns are in `Zhat^c` and the output describes the inverse image of the modular solution coset of P2.9. A checker verifies both by matrix products |

**E-S2**, section 9.1, table, row "Polynomial roots at a given prime" (line 512), conditional on S-D13. Replace

    | Polynomial roots at a given prime | simple roots by Hensel lifting with certificate. A root modulo `p` need not lift: `x^2 + 1` has the root 1 modulo 2 and none modulo 4 |

by

    | Polynomial roots at a given prime | Distinct roots in `Z_p` of a nonzero integer polynomial. Certificates are for its normalised squarefree part `g`: `v_p(g(a)) >= k + s` and `v_p(g'(a)) = s < k`. Each listed ball contains exactly one root of `g` and therefore of `f`. A list is complete only when every search class has been resolved. A root modulo `p` need not lift: `x^2 + 1` has the root 1 modulo 2 and none modulo 4. Multiplicities are not reported (`proofs/solvers.md` Propositions 3.2 to 3.6, 3.12, 3.13) |

**E-S3**, section 9.1, table, row "Real roots" (line 513). Replace

    | Real roots | isolating balls with a completeness status |

by

    | Real roots | Isolating balls for the distinct real roots of a nonzero integer polynomial `f`. Use its squarefree part `g`. Each ball is either an exact point where `g` vanishes or has opposite nonzero signs of `g` at its exact end points. The balls are pairwise disjoint. The list is complete when its length is the certified count of the real roots of `g`. The final accuracy is `arb_rel_accuracy_bits >= max(prec, 2)`, tested on the output balls (`proofs/solvers.md` Propositions 3.8 to 3.10) |

**E-S4**, section 9.1, table, row "Multiple roots, roots at all primes" (line 514): only if S-D13 is
decided as recommended; an owner decision that L3.1 supports mathematically. Replace "Multiple roots, roots at
all primes" by "Multiplicities of roots, roots at all primes".

**E-S5**, section 9.2, second item (lines 526 to 530). Insert before the statements about the bounds
"The following claims assume `A >= 0` and `B >= 1`. If `A < 0` or `B < 1`, the solution set is empty." and
replace

    - **From partial data** (a residue modulo `m`, nothing known at other primes): the classical bounded problem: given `m > 0`
      and `c`, find a reduced fraction `n/d` with `gcd(d, m) = 1`, `n = c d mod m`, `|n| <= A`, `0 < d <= B`;
      `2 A B < m` is sufficient for uniqueness, and `2 A B = m` can admit two solutions. A separate function on a
      separate type. **[proved]** (`proofs/quotient.md` Proposition 13; uniqueness only, the algorithm is not proved)
      `1/5` is `5 mod 6` in this sense, and yet `1/5` is not in the adelic ball `5 + 6 Zhat`.

by

    - **From partial data** (a residue modulo `m`, nothing known at other primes): the classical bounded problem:
      given `m > 0` and `c`, find the reduced fractions `n/d` with `n = c d mod m`, `|n| <= A`, `0 < d <= B`; for a
      reduced fraction the congruence implies `gcd(d, m) = 1`. The following claims assume `A >= 0` and `B >= 1`.
      If `A < 0` or `B < 1`, the solution set is empty. `2 A B < m` is sufficient for uniqueness, not for
      existence, and `2 A B = m` can admit two solutions. For `A >= m` there are always several; for `A < m` the
      set of solutions is computed from two consecutive rows of the Euclidean algorithm, for every `A` and `B`. A
      separate function on a separate type (`adf_resid`). **[proved]** (`proofs/quotient.md` Proposition 13;
      `proofs/solvers.md` Propositions 1.5 to 1.8) `1/5` is `5 mod 6` in this sense, and yet `1/5` is not in the
      adelic ball `5 + 6 Zhat`.

**E-S6**, section 9.2, third item (line 531) (accepted by the review, read with `solvers` P1.7). Replace

    - Results: one verified candidate; none; several; or "uniqueness not certified".

by

    - Results: one verified candidate; none; several; or "uniqueness not certified". The last one is returned only
      when `m <= 2 A B` and the search limit given by the caller ended the search before the set of solutions
      was decided (`proofs/solvers.md` Proposition 1.7).

### 6.2 `docs/PLAN.md`

**E-P1**, section 6, milestone S, table (lines 321 to 323). Replace the three rows by

    | S.3 | Partial rational reconstruction from a residue (`adf_resid`), with certified one/none/several statuses and an explicit limited-search `NOT_DETERMINED` result; complete classification when the search is allowed to finish. Certificate pair and enumeration as in `proofs/solvers.md` section 1 |
    | S.1 | Systems modulo `N`: particular solution, canonical kernel (Howell form of `[A^T \| I]`), certificate checked by matrix products, vector of non-solvability; right-hand sides that are finite balls (`proofs/solvers.md` section 2). The Hermite form over `Z` is an equivalent engine (Proposition 2.12); FLINT's Smith form returns no transformations |
    | S.2 | Roots in `Z_p` by search and Hensel lifting, with certificates and a completeness status; real roots with a completeness status (`proofs/solvers.md` section 3) |

**E-P2**, section 7, table, row "Solvers" (the row that reads "the equation is verified; the kernel is
complete on small cases"). Replace its second cell by

    Compare every small modular solution set and kernel with direct enumeration. Check K1 to K7 as applicable.
    Changed certificates must be rejected when their asserted result or required canonical representation is
    false; valid alternate certificates may pass. Check R1 to R3 for each prime root certificate and
    independently enumerate compatible roots modulo `p^M`: a simple root of derivative valuation `s` has `p^s`
    representatives at sufficient `M`. Test root coverage, unresolved classes, real endpoint cases, real counts,
    and actual output accuracy separately

### 6.3 `docs/conventions.md`

**E-C1**, section 3.2, row "Reconstruction and solvers" (line 200) (accepted by the review). Replace

    | Reconstruction and solvers | `OK`, `NO_SOLUTION`, `NOT_UNIQUE`, `NOT_DETERMINED` ("uniqueness not certified", `SPEC.md` 9.2), `LIMIT` |

by

    | Reconstruction and solvers | `OK`, `NO_SOLUTION`, `NOT_UNIQUE`, `NOT_DETERMINED` ("uniqueness not certified", `SPEC.md` 9.2; a list of roots that is not proved complete), `LIMIT`, `DOMAIN` (a modulus below 1, the zero polynomial, a place of the wrong kind, a shape that does not fit), `UNSUPPORTED` (an exact right-hand side of a system, a prime above the temporary bound of a slice that finds the roots modulo `p` by evaluation at every residue; S-D10, and the library accepts every prime) |

Without this edit the functions of sections 2 to 4 cannot report invalid input: the row has neither code.
(`adf_adele_reconstruct` already returns `DOMAIN` for an infinite real ball, M1-D11; the row does not list
it.)

**E-C2**, section 4.3, after the table of CV-06: only if S-D6 is decided as recommended. It is an explicit
exception, not an existing permission. Add

    The argument `sol` of `adf_linsolve_mod` and `adf_linsolve_fball` is a report argument: it is written on
    `ADF_OK` (the coset of solutions) and on `ADF_NO_SOLUTION` (the vector that proves it), and untouched on every
    other status.

**E-C3**, section 2.3, after the table: only if S-D12 is decided as recommended (all four types are listed).
Add

    The types of milestone S (`adf_resid`, `adf_recon_cert`, `adf_linsol`, `adf_rootlist`) have no text form and
    no dump form in version 1; they are read and written through their accessors.

**E-C4**, section 6.8, first item, last sentence. Replace

    (quotient P13), "uniqueness not certified" (`2 A B >= m` and no search) is `ADF_NOT_DETERMINED`.

by

    (quotient P13; solvers P1.7), "uniqueness not certified" is `ADF_NOT_DETERMINED`. It is returned exactly when
    `A < m <= 2 A B`, `|T| <= B` (`T` the denominator entry of the Euclidean row that the algorithm reaches),
    `floor(B/|T|)` exceeds the search limit `ell = max(limit, 0)`, and the rounds `x <= ell` found fewer than two
    reduced pairs. The cases `A >= m`, `|T| > B` and `2 A B < m` keep their decided statuses also when `limit` is 0.

**E-C5**, section 5: new subsections 5.15 `adf_resid`, 5.16 `adf_linsol`, 5.17 `adf_rootlist`. DEFERRED: the header
lane writes them after the root-list invariants (scope, `g`, the two verifiers) and the prototypes of this file
are frozen, so that the invariants are not copied before they are repaired and settled.

## 7. Work packages for the implementation lanes

Rules of `CLAUDE.md` hold: the failing test first; mutation testing of the arithmetic; fuzzing where
untrusted input is read (none here: no text form). The reference oracle of every package is
`proto/solvers_checks.py`; the lanes copy its reference algorithms to `tests/ref/adfref/` and write vectors
to `tests/ref/vectors/`.

| WP | Files | Content | After | Parallel with | The tests must check |
|---|---|---|---|---|---|
| S.0 | `include/adelefeld/resid.h`, `linsolve.h`, `roots.h`; `include/adelefeld.h`; `tests/test_headers.c`, `tests/test_abi.c`; `docs/conventions.md` 5.15 to 5.17 | declarations only, with the contract of each function as its comment; layouts pinned | the decisions of section 5 | none | signatures, sizes, exported symbols (as work package 1.9) |
| S.3a | `src/resid.c`, `tests/test_resid.c`, `tests/ref/adfref/resid.py`, `tests/ref/vectors/resid.jsonl` | `adf_resid`, `adf_recon_cert`, the two reconstruction functions, the checkers | S.0 | S.1a, S.2a, S.2b | section 2, table of acceptance tests |
| S.1a | `src/howell.c`, `src/linsolve.c`, `tests/test_howell.c`, `tests/test_linsolve.c`, reference and vectors | Algorithm H (word and `fmpz` moduli), `adf_linsolve_mod`, `adf_linsol_verify`, accessors | S.0 | S.3a, S.2a, S.2b | section 3: `solvers` P2.4, P2.5 for `howell.c`; P2.6 to P2.8 for the solver and the checker |
| S.1b | `src/linsolve_fball.c`, `tests/test_linsolve_fball.c` | `adf_linsolve_fball`, `adf_linsol_verify_fball`, `adf_linsol_get_fball` | S.1a | S.2a, S.2b | `solvers` P2.9, P2.10 |
| S.2a | `src/roots_padic.c`, `tests/test_roots_padic.c`, reference and vectors | `adf_rootlist`, Algorithm P, the seed function, the two verifiers at a prime, checked exponent arithmetic | S.0 | S.3a, S.1a, S.2b | section 4: `solvers` P3.2 to P3.6, P3.12, P3.13, 3.11 |
| S.2b | `src/roots_real.c`, `tests/test_roots_real.c` | Algorithm RR, the two verifiers at the real place, the accuracy test of the output balls | S.0; shares `adf_rootlist` with S.2a: the struct and its life cycle are written in S.0 or by S.2a first | S.3a, S.1a | `solvers` P3.8 to P3.10 |
| S.4 | `bench/`, `docs/PERF.md` rows | one row each: reconstruction (word and 4096 bit modulus), Howell form (word modulus, `r + c = 64`), lifting to precision 1000 | S.3a, S.1a, S.2a | none | the contract of `PERF.md` section 7 |
| S.5 | `docs/reviews/mS/` | adversarial review of the code by a second model family: an input that gives a wrong status, a certificate that the checker accepts and that is false, a ball with no root or two | each of S.3a, S.1a, S.1b, S.2a, S.2b | none | none |

Order: S.0; then S.3a first (TJO, M1-D8: it is what a solver of rational systems needs first), with S.1a,
S.2a, S.2b in parallel if lanes are free; S.1b after S.1a; S.4 and S.5 last. Only the orchestrator commits.
