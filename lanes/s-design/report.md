# Report, lane s-design: the design of milestone S (solvers), with proofs

Date: 2026-09-29. Written by the lane (Claude Fable, 00:50 to 01:52, 535k tokens) as its final message; saved
to this file by the orchestrator, who added the last section. Base commit `0c80063`. No C was written, no
file outside the lane's ownership was changed.

## 1. What was written

| File | Lines | Content |
|---|---|---|
| `docs/proofs/solvers.md` | 1350 | section 0 (notation; 11 standard theorems with key, file, line); section 1 = S.3 (10 statements); section 2 = S.1 (12 statements); section 3 = S.2 (11 statements, one open); table of statements |
| `proto/solvers_checks.py` | 1956 | reference algorithms (`recon_partial`, `howell`, `linsolve_mod`, `linsol_check`, `padic_roots`, `newton_step`, `real_roots_ref`, `real_cert_ok`) next to brute-force oracles; 25 checks, of which 4 probe the installed FLINT 3.0.1 through `ctypes` and 1 probes python-flint |
| `docs/api-s.md` | 372 | three new headers (`resid.h`, `linsolve.h`, `roots.h`), types `adf_resid`, `adf_recon_cert`, `adf_linsol`, `adf_rootlist`; functions with set statement, statement implemented, statuses, cost; acceptance tests; 15 decisions S-D1 to S-D15; exact edits E-S1..E-S6 (SPEC), E-P1, E-P2 (PLAN), E-C1..E-C5 (conventions); 8 work packages |
| `lanes/s-design/checks_output.txt` | 26 | output of the last run |

Order followed: S.3 complete first, then S.1, then S.2.

## 2. Output of `python3 proto/solvers_checks.py` (exit status 0, one core, 28 s)

```
PASS check_s3_coprime: 465 problems (m <= 30), 85095 reduced pairs, 0 failures
PASS check_s3_certificate: 159175 small and 2000 large triples (m, c, A), 0 failures; accepted wrong pairs: 0
PASS check_s3_param: 177120 boxes (m <= 40, all c, all A < m), 2747820 lattice points, 0 failures
PASS check_s3_complete: 94256 problems (m <= 36): one 21124, none 24793, several 48339; 0 failures
PASS check_s3_limit: 239540 calls with limits 0, 1, 2, 5; NOT_DETERMINED 97264 times; 0 failures
PASS check_s3_ranges: 270600 problems (m <= 40). I: one 13016, none 24194, several 0; II: one 20342, none 5021, several 7514; III: one 36510, none 3851, several 133092; IV: one 0, none 0, several 27060; 0 failures
PASS check_s3_edge: 19 fixed cases, 3000 large random fractions, 0 failures
PASS check_s3_forget: 4628 rationals of balls (H <= 12, d <= 7), 356 witnesses of a proper inclusion, 0 failures
PASS probe_s3_flint: FLINT 3.0.1: 76146 calls, 36491 inside 2ND < m; outside: returns 0 although a solution exists 2380 times, returns one of several 22860 times; outputs changed at return 0: 32924 times; 0 failures
PASS check_s1_howell: 1869 row sets over Z/N, N in (1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 16, 18), up to 3 columns; 0 failures
PASS check_s1_canonical: 672 modules with changed generators; 32945 pairs of rows over (Z/N)^2, N in (4, 6, 8, 9, 12): 195 modules, 195 forms; 0 failures
PASS check_s1_solve: 4432 systems modulo 4 (all A, b with r, c <= 2) and 4232 random systems, N in (1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 16, 18), r, c <= 3; without solution: 3265; 0 failures
PASS check_s1_cert_sound: 8397 changed certificates: 6566 refused, 1831 accepted and true, 0 accepted and false
PASS check_s1_duality: 420 submodules of (Z/N)^r, every b of (Z/N)^r; 0 failures
PASS check_s1_edge: 13 fixed cases, 20540 single equations (N < 40), 60 systems, N up to 122 bits; 0 failures
PASS check_s1_adelic: 396 systems with right-hand sides (A + H Zhat)/d; 0 failures
PASS probe_s1_flint: FLINT 3.0.1: 2080 matrices, N up to 30 bits; 0 differences
PASS probe_s1_hnf: python-flint 0.8.0: 2016 matrices, 1527 rows with pivot N; 0 differences
PASS check_s2_certificate: 50 certificates of the named cases, 869 found by search in random polynomials; 0 failures
PASS check_s2_newton: 500 steps and requests; 0 failures
PASS check_s2_descent: 124 runs of the named cases (depth limits 0, 1, 3, 8), 150 random products with known integer roots; 0 failures
PASS check_s2_examples: 12 fixed cases; 0 failures
PASS check_s2_count_mod_p: 458 polynomials over F_p, p up to 257; 0 failures
PASS check_s2_real_completeness: 18 polynomials, 42 changed lists; 0 failures
PASS probe_s2_flint_real: FLINT 3.0.1: 51 calls (prec 16, 64, 200); balls widened once: 0; exact balls: 20; 0 failures
total time 28.0 s; failures: 0
```

Failures met while writing the checks were all errors of the tests, not of the statements: a "redundant row"
that was not a combination of the others; wrong expected values (kernel of `2x + 4y = 0` mod 8; the number of
roots of `(x^2-13)(x^2-17)(x^2-221)` in `Z_3` is 2, not 4; the residue mod 4 of a root in Conrad's example
4.4); `x^2 + 2` taken as rootless mod 3; an oracle modulus too small near a double root; touching closed
intervals in the real reference; one segfault from reading `flint_version` as a pointer. One of them changed
the design: the content of `f` at `p` is removed before certificates are formed (`27x` at 3 has `s = 0`, not
3). No statement was weakened to make a check pass.

## 3. Main design results

- **S.3.** The answer is exact for every `(m, c, A, B)`, from two consecutive rows `(R', T', R, T)` of the
  Euclidean algorithm (a "certificate pair", 4 checkable conditions; the proofs use only those conditions,
  not the algorithm). `A >= m`: always several. `|T| > B`: none, with no hypothesis on `2AB`. `2AB < m`: the
  row if `gcd(R,T) = 1`, else none (dividing a non-reduced row by its gcd gives a point outside the lattice;
  `gcd(R,T) = gcd(T,m)`). `m <= 2AB`: all lattice points of the box are `phi(x,y)`, at most two `y` per `x`,
  `x <= floor(B/|T|)`; enumerated in that many steps. "Uniqueness not certified" = `NOT_DETERMINED`, returned
  exactly when `m <= 2AB`, `|T| <= B`, the caller's search limit is below `floor(B/|T|)` and fewer than two
  solutions were found in the part searched. `gcd(d,m) = 1` follows from the other conditions. "None" occurs
  in every range with `A < m` (`Sol(4,2,1,B)` empty for all `B`). Own loop recommended over FLINT, which
  returns one row, no reason for 0, and writes its outputs before deciding. Input type: new `adf_resid`.
- **S.1.** One computation modulo `N`, no factorisation. Canonical kernel = Howell form; uniqueness and
  existence proved (Algorithm H, which avoids units). The certificate is the Howell form of `[A^T | I_c]`: a
  checker verifies echelon shape, `A V_i = E_i`, `A G_i = 0`, and the integer identity
  `prod(N/pivot) = N^c`; these imply by counting that the generators give the WHOLE kernel, for every case,
  at the cost of one matrix product. "No solution" is certified by a vector `y` with `y^T A = 0`,
  `y^T b != 0`. Adelic form: balls `(B_i + H_i Zhat)/d_i` reduce to a system modulo `lcm(H_i)`; exact
  right-hand sides are `UNSUPPORTED`. The integer Hermite form named in PLAN gives the same rows (P2.12,
  proved).
- **S.2.** Root certificate `(a, k, s)`: `k > s`, `v(f'(a)) = s`, `v(f(a)) >= k + s`, giving exactly one root
  in `a + p^(s+1) Z_p`. Newton step gives precision `2k - s`. Search by levels with a depth limit; list
  called complete only when every class is closed; a multiple root is never resolved and lands in an
  unresolved class. `x^2 + 1` at 2 gives the empty complete list at depth >= 1. Real roots: squarefree part,
  FLINT's count, FLINT's enclosures each re-tested by an exact sign change at exact end points, pigeonhole.

## 4. Table of statements with status

| No. | Content | Status |
|---|---|---|
| L1.2 | congruence gives `gcd(d,m) = 1` for a reduced fraction; local meaning | proved here |
| L1.4 | Euclidean algorithm gives a certificate pair; `gcd(R,T) = gcd(T,m)` | proved modulo Shoup 4.3 (proof on disk) |
| P1.5 | lattice points of the box from any certificate pair | proved here |
| P1.6 | complete answer for every `(m,c,A,B)` | proved here |
| P1.7 | Algorithm R, when `NOT_DETERMINED`, cost | proved here (cost modulo Shoup 4.4) |
| P1.8 | ranges in which "none" occurs; existence | proved here; claim 4 modulo Thue (proof on disk) |
| P1.9 | FLINT `fmpq_reconstruct_fmpz_2` | naive and one-limb paths read from source; multi-limb paths NOT proved, probed only |
| P1.10 | residue is not the adelic ball; forgetting map | proved here |
| L2.2, L2.3 | echelon span bound; Howell property, greedy membership, exact count | proved here |
| P2.4 | Howell form canonical over `Z/N` | proved here |
| P2.5 | Algorithm H: existence, correctness, cost | proved here |
| P2.6 | kernel certificate: soundness, canonical, existence | proved modulo first isomorphism theorem (on disk) |
| P2.7 | "no solution" vector: soundness, existence | soundness proved here; existence modulo structure theorem of finite abelian groups (on disk) |
| P2.8 | Algorithm L; zero matrix, `N=1`, `r=0`, `c=0` | proved here |
| P2.9, P2.10 | ball right-hand sides; solutions as a vector of balls | proved here (2.9 modulo fact (D) of quotient.md) |
| P2.11 | FLINT Howell form | read from sources; equality of normal forms NOT proved, probed on 2080 matrices |
| P2.12 | integer Hermite form gives the Howell form | proved here |
| L3.1 | content; squarefree part | proved here (Euclidean algorithm in `Q[X]` assumed) |
| P3.2 | root certificate gives exactly one root | proved modulo Conrad 4.1 (on disk) |
| P3.3 | Newton step, precision `2k - s` | proved here |
| P3.4, P3.5 | search: soundness, partition, completeness, multiple roots, termination | proved modulo Conrad 2.1, 4.1, completeness of `Z_p` |
| R3.6 | depth bound by valuation of a resultant | **OPEN**, source pending, not used |
| P3.7 | roots mod `p` complete: evaluation, or `deg gcd(g, X^p - X)` | proved modulo Shoup 7.12-7.14 (Euclidean algorithm in `F_p[X]` assumed) |
| P3.8 | count plus isolation gives completeness | proved modulo the intermediate value theorem (**source pending**) |
| P3.9 | FLINT for real roots | read from sources |
| P3.10 | Algorithm RR | proved modulo IVT and **FLINT's promise of the count** (Sturm, source pending) |
| 3.11 | the prime 2 and the examples of SPEC | proved by computation |

## 5. Where a source says something different

### 5.1 Against table 3 of `docs/sources.md` and the report of lane s-sources

1. **Thue's lemma, inequality reversed** (first S.3 row; note 4; finding 4 of the report). The row says
   existence for "`m > A B`". The source says `0 < r* <= n < r* t*` (`shoup-ntb:ntb-v2.txt:2184`): the modulus
   must be BELOW the product of the bounds. The conclusion "none in the range `AB < m <= 2AB`" is too narrow:
   none occurs in every range with `A < m`.
2. **Same row says "at least one `n/d`".** The source gives a pair `(r,t)`, not a reduced fraction with
   denominator prime to `m`. Counterexample to the row as worded: `m = 8, c = 3, A = B = 2` satisfies Thue's
   hypothesis and has no solution.
3. **Row "`fmpz_mod_mat` is a prime-field module" and note 5.** The module also has
   `fmpz_mod_mat_howell_form` and `_strong_echelon_form` (`fmpz_mod_mat.rst:278, 287`) with no condition on
   the modulus; the first is a call of `fmpz_mat_howell_form_mod` (`fmpz_mod_mat/howell_form.c:16`). The row
   says less than the source.
4. **Row "the same function for a prime modulus is `nmod_mat_howell_form`".** The entry
   (`nmod_mat.rst:716-723`) names no prime and the source uses gcds
   (`nmod_mat/strong_echelon_form.c:72, 91, 149`). Only the general sentence at `nmod_mat.rst:28-30` could be
   read so. The row says more than the entry cited.
5. **Note 3.** Sharper than stated: a return value 0 caused by a denominator above `D` still means "no
   solution" for every `N, D`; only a 0 caused by a non-reduced row loses its meaning.
6. **Row on the Sturm count.** True of `fmpz_poly_num_real_roots_sturm`. `fmpz_poly_num_real_roots` uses
   closed formulas up to degree 4; it throws for the zero polynomial and for non-squarefree degree 3 or 4,
   and returns 0 for `(x-1)^2` (`fmpz_poly/num_real_roots.c:34-40, 125-129, 158-159`).
7. **Row "kernel is `W Q U C`".** In the source it is the LEFT kernel `{v : vA = 0}` (`diss2up.txt:2099`);
   the design uses columns and transposes.
8. **Note 9** (completeness from the flags of `arb_calc_isolate_roots`): not used; it takes a function
   pointer (conventions 12.7).

### 5.2 Against SPEC, PLAN, conventions, quotient.md

No statement of `SPEC.md` or `quotient.md` was found false; no counterexample to them is reported.

1. SPEC 9.2 lists `gcd(d,m) = 1` as a condition; it follows from the others (L1.2).
2. SPEC 9.2 "the algorithm is not proved": now proved, for every `(m,c,A,B)`.
3. quotient.md P13 last sentence and conventions 6.8: "not certified when `2AB >= m` and no search". Without
   any search three cases are still decided exactly in that range. Edit E-C4.
4. PLAN S.1 names the Hermite form; over `Z/N` it is not canonical (`diss2up.txt:887`). Design uses the
   Howell form; P2.12 relates the two. Edit E-P1.
5. SPEC 9.1 "transformation matrices": the transformation is the right block of the Howell form of
   `[A^T | I]`, up to `r + c` rows, not unimodular. Edit E-S1.
6. conventions 3.2: the row "Reconstruction and solvers" has neither `DOMAIN` nor `UNSUPPORTED`. Edit E-C1.
7. conventions 4.3 (CV-06) leaves outputs untouched on `NO_SOLUTION`, while SPEC 9.1 asks for a certificate
   of it. Decision S-D6, edit E-C2.
8. conventions 2.3 asks text and dump forms of every value type. Decision S-D12, edit E-C3.
9. FLINT facts the implementation must respect: `fmpq_reconstruct_fmpz_2` writes outputs before deciding
   (`a=2, m=5, N=D=1` returns 0 and leaves `n = -3`); `N = 0` and `m <= 2` are outside its contract; the
   Howell functions read outside the matrix if it has fewer rows than columns (padding needed).

## 6. Sources pending

1. The intermediate value theorem (P3.8).
2. Sturm's theorem (behind FLINT's count, P3.9, P3.10).
3. `flint-3.0.1` `nmod_poly.rst` (only if S-D10 is decided for large primes).
4. The identity `u f + v g = Res(f, g)` over `Z[X]` (Remark 3.6, open, not used).
5. The Euclidean algorithm with Bezout identity in `Q[X]` and `F_p[X]` is used in L3.1 and P3.7 without a
   citation; the line in Shoup was not looked up.
6. The pending items of lane s-sources are unchanged; none is needed by the design.

## 7. What the lane is least sure of, in order (the refute review starts here)

1. **P3.5 claims 4 and 6** (a simple root is certified at level `s + 1` at the latest; termination). Longest
   argument in the file: the content `w` along a chain of classes, statement (W) `w >= 2e`, the infinite
   chain. Checked only on 31 named cases and 150 random products, small primes, depth up to 8.
2. **P3.4(3)**: the choice `j > w - 2e` and `K = max(prec, s + 1)`; that the ball of precision `K` lies inside
   the class where it was found rests on (W).
3. **P2.5, invariants (I1)-(I3) of Algorithm H**, above all (I3) when a pivot is replaced while vectors are
   pending. The algorithm multiplies by a Bezout coefficient and adds the annihilator multiple instead of
   using a unit; this is not the algorithm of the sources.
4. **P2.6(1), the counting argument**. Short; if wrong, the whole certificate is wrong. No accepted false
   certificate among 8397 mutated ones, small moduli only.
5. **P1.9**, the reading of FLINT's one-limb path, and everything about FLINT that is only probed. The design
   does not depend on it if S-D2 and S-D7 are decided as recommended.
6. **P3.10 rests on FLINT's count**; a wrong count would make a list wrongly complete.
7. **Interface**: `adf_fball` for a root at a prime (S-D14) and the argument `sol` written on `NO_SOLUTION`
   (S-D6) stretch the conventions; both are put as decisions, not decided.
8. **Line numbers** in `diss2up.txt` come from a two-column extraction; a statement may stand one line before
   or after the line cited. The cost bounds (P1.7(4), P2.5(4)) were counted by hand and not measured.

## 8. Decisions waiting for TJO (in `docs/api-s.md` section 5)

S-D1 type `adf_resid`; S-D2 own Euclidean loop; S-D3 `NOT_DETERMINED` vs `LIMIT` when the search limit cuts;
S-D4 second function returning a verified candidate; S-D5 empty box is `NO_SOLUTION`; S-D6 where the "no
solution" certificate goes; S-D7 engine of the Howell form; S-D8 `ADF_LINSOLVE_DIM_MAX = 4096`; S-D9 one
computation modulo `N`; S-D10 `ADF_ROOTS_P_MAX = 2^20`; S-D11 trust FLINT's count; S-D12 no text or dump
forms; S-D13 roots of the squarefree part; S-D14 output type of a root at a prime; S-D15 partial lists. The
implementation work packages (S.0 headers, S.3a, S.1a, S.1b, S.2a, S.2b, S.4 bench, S.5 review) depend on
these.

## Checks of the orchestrator before the merge (2026-09-29, 01:53)

- `python3 proto/solvers_checks.py` in the lane's worktree: exit 0, 30.4 s, `failures: 0`.
- Item 5.1.1 read in the source: Theorem 2.33 at `shoup-ntb:ntb-v2.txt:2184` has the hypothesis
  `0 < r* <= n < r* t*`. The lane is right and table 3 of `docs/sources.md` was wrong; the row and its note
  are corrected by the orchestrator in the commit after the merge. Items 5.1.2 to 5.1.8 were NOT checked by
  the orchestrator.
- The lane wrote three documents and one log, all in its ownership (`git status`).
- NOT checked by the orchestrator: any proof. The design is a DRAFT until the review by another model family
  (codex `gpt-6-astra`, refute mode, lane s-review) has judged it and TJO has taken the decisions S-D1 to
  S-D15. No implementation lane starts before that.
