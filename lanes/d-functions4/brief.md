# Lane d-functions4: design of milestone 4, functions on the adeles, before code

A design document before code, the map for thin slices (`docs/workflow.md` rules 1 and 3), as `docs/api-3.md`
(lane d-quotient) and `docs/api-3c.md` (lane d-char) are for milestone 3: read `lanes/d-char/brief.md` and
`docs/api-3c.md` for the form and the length (at most 500 lines; declarations with full comment blocks,
statements with proofs, acceptance tests, slices; nothing twice). No change of `src/` or `include/`. Write for
an implementer who will build it in lanes of about an hour each, and for a reviewer who will try to refute you.

Milestone 4 (`docs/PLAN.md` lines 325-333): 4.1 `adf_ffun`: arrays with `D`, `M`; sum, product with common
refinement, exact translation and dilation (test: delta functions with `D` different from `M`); 4.2 the
weighted finite Fourier transform (the formulas of `docs/SPEC.md` section 7 in exact cyclotomic arithmetic or
certified balls; non-symmetric inputs); 4.3 `adf_rfun`: polynomial-Gaussian sums, closure operations,
transform (shifted, non-even Gaussians, which tell the two sign conventions apart); 4.4 evaluation at adelic
balls and the Haar integral (enclosure across a jump); 4.5 Poisson summation with proved tail bounds (two
independent truncations; width target; convergence with precision).

What is fixed and NOT yours to change: `docs/SPEC.md` section 7 (lines 443-468: the finite part `f_j = f(j/D)`,
`0 <= j < L = D M`; the three formulas of the transform, the integral and the norm; the real part as finite sums
of `P(x) exp(-pi A x^2 + B x + C)` with `Re(A) > 0`, parameters kept as balls; the operations); the structs
`adf_ffun_struct { ulong D, M; acb_ptr f; }` and `adf_rfun_struct { slong len; adf_rterm_struct *term; }`
(`docs/conventions.md` 5.11:760-770, 5.12:771-784; CV-20: no normal form), the value forms `ffun(D=D, M=M;
z(f_0), ...)` and `rfun(term(P=[...], A=, B=, C=), ...)` (conventions 9, lines 1262-1263; 11.3), the goldens
`tests/golden/ffun.tsv` (18 rows) and `rfun.tsv` (19 rows) (`tests/golden/README.md`), the sign conventions of
section 6.1 and 6.3 (the finite Fourier transform, 901-911: `E(-jk/L)`), the measures of 6.2; the proofs
`docs/proofs/analysis.md` Proposition 4 (150: the weighted finite transform), Proposition 5 (174: real
polynomial-Gaussians, closure, the transform), Lemma 6 (213: a computable Gaussian series bound), Proposition
7 (260: Poisson summation over `Q` and theta series), Lemma 14 (627), Proposition 15 (652: explicit truncation
and quadrature certificates); `proto/analysis_checks.py` (`check_finite_fourier`, `check_real_transform`,
`check_real_closure`, `check_tail_bounds`, `check_poisson`, `check_theta`: the exact checks that exist; your
oracle may import it); the status table of conventions 3.2 (which row these functions belong to: say it);
the types they are evaluated on (`include/adelefeld/adele.h`, `fball.h`, `sball.h`, `idele.h` for dilation by a
certified idele) and the character `include/adelefeld/psi.h`, `docs/api-3b.md` (the kernel `E(theta)`; the
finite transform's `E(-jk/L)` is the phase evaluator with a negated angle, `docs/api-3.md` 3.4:487-511).
Milestone 5 (Tate integrals, `docs/SPEC.md` section 8, PLAN 335-343) consumes this milestone: read section 8
and `analysis.md` Propositions 9 to 12 so that the test function class and its integral are what section 8
needs (the test vector of conventions 6.5; `1_(Z_p)` at a prime, `phi_e` at infinity), and say in one
paragraph what milestone 5 will call.

**You own:** `docs/api-4.md` (new), `proto/functions4_checks.py` (new; exact arithmetic for the finite part
(cyclotomic phases as exact rationals modulo 1, sums by `python-flint` `acb` or `mpmath` at 60 digits with a
stated margin), exact closure algebra for the real terms, Poisson both sides numerically with the tail bound of
Lemma 6; may import `proto/analysis_checks.py` and `proto/quotient3_checks.py`), `lanes/d-functions4/`.
Everything else is read-only (a defect of SPEC, the conventions, the proofs or the goldens is a finding with
the counterexample computed by your script). No git command that changes state, no `bd`. At most 2 cores;
every program under `timeout`; none over 120 s. Other lanes write `src/char.c`, `src/text.c`, `src/dump.c`,
`src/qclass_arith.c` in other worktrees: not your files.

## What `docs/api-4.md` contains

1. **The two types**: predicates (`D, M >= 1`, `L = D M` fitting the resource bounds: state them; the real
   terms with `Re(A) > 0` certified), set statements (which function on `A_f`, on `R`, on `A` a value means;
   a tensor `phi x f_fin` and finite sums of tensors: is a product type `adf_afun` needed for 4.4 and 4.5, or
   do the two types with a pairing suffice? decide and say why), init values, `is_canonical`, layout queries,
   the missing typed declarations of `text.h` and `dump.h` (bodies `ffun`, `rfun`: does conventions 10 define
   them? if not, propose them in the style of the others and mark it a decision).
2. **4.1 and 4.3, the algebra**: sum and product with common refinement (`D' = lcm`, `M' = lcm`: the cost
   charged, the resource bound), exact translation by a rational (`f(x - q)`: the array permutation when `q`
   has denominator dividing `D`, else the refinement), dilation by an exact non-zero rational and by a
   certified idele (SPEC 7: `D_a f(x) = f(a x)`; dilation by 0 leaves the class; "balls precise enough to fix
   the array operation uniquely", else `NOT_DETERMINED`), negation of the argument; for `adf_rfun` the closure
   operations of Proposition 5 with the parameter balls (translation, dilation, product, derivative if needed
   for the Tate integrals; polynomial arithmetic in `acb`), statuses and where the balls widen.
3. **4.2 and 4.3, the transforms**: `hat f(k/M) = (1/M) sum_j f_j E(-jk/L)` with the kernel through the phase
   evaluator and the error bound of `L` certified terms (and when `L` is large, what is promised: a cost
   bound, `LIMIT` above a cap: a decision); the second transform as the reflection with the factor `1/D`; the
   real transform of a term by Proposition 5 (the formula with `A, B, C` and `P`: write the new parameters and
   the new polynomial explicitly, and the branch of `sqrt(A)` for complex `A`: prove the choice); the two sign
   conventions told apart by a shifted Gaussian (the PLAN test).
4. **4.4, evaluation and integral**: `f(x)` at an adele ball (the finite part: the set of cosets `j/D + M Zhat`
   the finite ball meets, by exact arithmetic on the triple; an enclosure of all values across a jump; the
   real part: `phi` on an `arb` by `acb` arithmetic), on an `adf_sball`; the Haar integral `(1/M) sum_j f_j`
   times `integral of phi` (Proposition 5 gives the Gaussian integral in closed form: write it); the norm.
5. **4.5, Poisson**: `sum_(q in Q) f(q) = sum_(q in Q) hat f(q)` for tensors (Proposition 7): the set of `q`
   with `f_fin(q) != 0` is `(1/D) Z` modulo the period, so the left side is a theta-like series over `j/D + M
   n`; the truncation by Lemma 6 with explicit `N` for a target width; both sides computed independently; the
   statuses when `Re(A)` is a ball near 0.
6. **Statements to add to `analysis.md`** (numbered steps, proofs written out) for whatever 1 to 5 needs that
   the propositions do not state: the refinement formulas, the array permutation of a translation, the
   `acb` error bounds of the finite transform, the branch of the square root, the evaluation enclosure across
   a jump.
7. **Acceptance tests and faults**: per function what a test must check so that it fails for a wrong
   implementation (exact small cases from the oracle; the goldens; the PLAN tests of rows 4.1 to 4.5); six
   faults per work package (the sign of the kernel; `1/D` for `1/M`; the reflection dropped; the refinement
   by `max` instead of `lcm`; the translation permuting the wrong way; the tail bound off by the factor of
   Lemma 6; a jump sampled at one point).
8. **Decisions for TJO**: few and real (the caps; the branch of the root; whether a tensor type is added).
9. **Thin slices**: in the order of PLAN 4.1 to 4.5, each the smallest piece a user can call (driver `adf`
   command, Julia `ccall`), with its functions, oracle group and decisions; the first slice (`adf_ffun`
   lifecycle, text, sum, the transform against the goldens) in one lane of about an hour.
10. **Findings** against SPEC, PLAN, conventions, proofs, goldens, each with the counterexample.

`proto/functions4_checks.py` must run: `timeout 120 python3 proto/functions4_checks.py` prints each group with
its counts and ends with the number of checks; every example of the design is computed by it. It provides the
oracle functions for the implementation lanes: `ffun_transform(D, M, f)` exact (cyclotomic) or certified;
`refine(D, M, f, D2, M2)`; `translate`, `dilate`; `rterm_transform(P, A, B, C)`; `evaluate(f, finite ball)` as
the exact set of array indices met; `poisson_sides(tensor, N)` with the tail bound.

## Report

`lanes/d-functions4/report.md`, written once, at the end; notes in `lanes/d-functions4/progress.md` as you go.
In the report: the interface in twenty lines; the decisions; the findings; what the oracle checks, with numbers
and what would have made a check fail; what is not designed and why; sources pending. No praise, no summary of
what the files already say. Aim to finish within about ninety minutes.
