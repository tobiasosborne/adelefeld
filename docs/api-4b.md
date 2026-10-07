# Slice 4d: real test functions `adf_rfun`

The type, its predicate and the formulas are those of `docs/api-4.md` sections 1, 2, 5 and 6, conventions 5.12 and
`docs/proofs/analysis.md` Proposition 5 (P5, lines 174-212). The header is `include/adelefeld/rfun.h`, the code
`src/rfun.c` and the end of `src/text.c`, the test `tests/test_rfun.c`. A member of an rfun chooses one point of
every parameter ball and is the function sum_k P_k(x) exp(-pi A_k x^2 + B_k x + C_k) on real x. "Encloses" means:
every member of the result is the exact operation applied to some member of the inputs, and conversely every
exact operation on members of the inputs is a member of the result. Every ball operation used contains the exact
operation on every point of its input balls (`refs/src/flint-3.0.1/arb.rst:6-12`, `acb.rst:6-12`); an
enclosure statement below follows from that rule applied step by step, because each step is one such operation.
Not in this slice: derivative, transform, integrals and norms (4e), dump forms, `adf_ffun`, tensors.

## Statuses, caps and order of checks

Every arithmetic and evaluation call checks, in this order: `prec > ADF_REAL_PREC_MAX` gives `LIMIT`; under INV the
entry predicates; the caps of D1 give `LIMIT`; the domain (`DOMAIN`); then the computation, where a nonfinite
result or a result term without certified `Re(A) > 0` gives `NOT_DETERMINED`. The work is done at
`p = max(prec, 2)` in a fresh term array, committed only after the last check, so every status other than `OK`
leaves the outputs untouched (api-4.md section 1). The caps (D1, SPEC 15.4 N-D23) are
`ADF_RFUN_TERMS_MAX = ADF_RFUN_COEFFS_MAX = 2^16` terms and coefficients in each input and in the result,
`ADF_RFUN_WORK_MAX = 2^20` work units, and `ADF_RFUN_BITS_MAX = 2^20` bits for the square of the numerator and of
the denominator of an exact rational. A work unit is one dense coefficient multiply-add: the shift of a polynomial
of length n costs n(n - 1)/2, a product of lengths n and m costs n m, a dilation and an evaluation one per
coefficient. The status row is "Integrals, Poisson summation" of conventions 3.2 (one clause added).

## Statements

1. `adf_rfun_init`, `_clear`, `_set`, `_swap`: the zero function is `len = 0, term = NULL`; clear releases each
   term (`acb_poly_clear`, `acb_clear`) and the array (`flint_free`); set is a deep copy without rounding, a
   no-op for `y == x`; swap exchanges the two structs. Check: `test_rfun lifecycle`; SAN with leak detection.
2. `adf_rfun_is_canonical`: 1 exactly for conventions 5.12: `len >= 0`, a non-NULL array when `len > 0`, each
   P with finite coefficients and a last coefficient that is not the exact zero ball (`acb_is_zero`), finite
   `A, B, C`, `arb_is_positive(Re A)`. Check: `test_rfun lifecycle` (raw fields).
3. `adf_rfun_identical`: equal length and, term by term, `acb_poly_equal` and `acb_equal` of A, B, C. It is
   representation identity: `x + y` and `y + x` are not identical and evaluate to overlapping balls.
   Check: `test_rfun order`, `aliasing`.
4. `adf_rfun_set_terms`: `LIMIT` for `n > 2^16`, decided before a term is read, then for more than `2^16`
   coefficients in all; `DOMAIN` for `n < 0`, `terms == NULL` with `n > 0`, or a term that fails statement 2;
   otherwise a deep copy, made before the old storage is released (so an overlapping `terms` is still read
   correctly; the header states non-overlap as the precondition). Check: `test_rfun lifecycle`, `caps`
   (2^16 terms accepted, 2^16 + 1 refused with a one-element array).
5. `adf_rfun_set_str`: the stages of conventions 8.5 over the whole text: length and alphabet; the grammar
   `rfun_v` (9.2); stage 4: the number of terms and each coefficient list against `max_items` (as
   `proto/text_grammar.py` `_check_limits`, the "items" nodes) and every decimal exponent against `max_exp10`;
   stage 6: the exact decimal interval of `Re(A)` must lie in `(0, infinity)` (`DOMAIN` otherwise, as
   `_check_real(..., "positive")`); stage 7: `Re(A)` is built by the reader of the idele class
   (`tx_real_ball_signed`, positive): the enclosure at `prec` first, then kernel B on the exact end points; if
   both fail, `NOT_DETERMINED`. Coefficients and the other parts are the enclosing balls of 9.5. Exact trailing
   zero coefficients are removed by `_acb_poly_normalise`, which strips coefficients identical to zero
   (`acb_poly.rst:52-54`); a decimal zero with zero radius is the only text that gives an exact zero ball, so
   this is the trimming of `_trim_zero_coeffs`. Terms are neither combined nor sorted. The D1 caps are not
   applied by the reader (api-4.md section 1: text keeps the caller limits of 8.4).
   Check: `test_rfun golden` (19 rows), `texts` (164 reference texts), `reader_cases`.
6. `adf_rfun_get_str`: the template of 9.4 with `digits`; `Re(A)` is printed by the constrained printer of 9.5
   with the condition "positive" (as `_fmt_complex(A, "positive")` in the reference), every other part by the
   printer of 9.5. NULL with `*len = 0` when a part fails the exponent bound M1-D6 or the constrained printer
   passes its work bound (N-D11). Check: `test_rfun golden`, `texts`; driver `rfun-text`.
7. `adf_rfun_add`: the terms of x, then the terms of y, copied exactly; `prec` is only checked. The sum of two
   members is a member, so the result encloses. Check: `test_rfun closure` (add), `order`, `caps`.
8. `adf_rfun_mul`: for i over x, then j over y, the term `P_i Q_j` (`acb_poly_mul`), `A_i + A'_j`,
   `B_i + B'_j`, `C_i + C'_j` (P5). Each is one ball operation, so the term encloses the product of the two
   member terms. `Re(A_i + A'_j) > 0` holds exactly for members (P5 proof step 1); the ball sum can lose the
   certificate at small `prec`, then `NOT_DETERMINED`. Check: `test_rfun closure` (mul), `statuses`, `order`.
9. `adf_rfun_translate_rat`: with `q = n/d`, the term `P(x - q)`, `A`, `B + 2 pi A q`, `C - B q - pi A q^2`
   (P5). `pi A` is one product; each product with q is a multiplication by n followed by a division by d
   (api-4.md section 5), each with q^2 a multiplication by n^2 and a division by d^2. `P(x - q)` is the Horner
   scheme `a_j += c a_(j+1)` (i = 0..L-2, j = L-2..i) with `c = -q`, each step `(a_(j+1) (-n))/d`; it computes
   the Taylor shift `P(x + c)` (the identity of `acb_poly_taylor_shift`, `acb_poly.rst:391-395`). Proof: pass i
   is synthetic division by `x - c` of the polynomial stored in `a_i, ..., a_(L-1)`: `b_(L-1) = a_(L-1)`,
   `b_j = a_j + c b_(j+1)`, so `a_i` receives the remainder `Q_i(c)` and `a_(i+1), ...` the quotient
   `Q_(i+1) = (Q_i - Q_i(c))/(x - c)`, with `Q_0 = P`. Then `P(x) = sum_k Q_k(c) (x - c)^k`, hence
   `P(y + c) = sum_k Q_k(c) y^k`: the final `a_k` are the coefficients of `P(y + c)`.
   A is copied, so `Re(A) > 0` is never lost. Rounding enters through pi and through a division by d when d is
   not a power of 2, and at `prec` when a result needs more bits; with dyadic q and exact dyadic inputs,
   `P(x - q)` and A are exact, and B and C are exact when `q = 0`. Check: `test_rfun closure` (translate by 0,
   1/3, -7/2 and a 2000-bit q; exactness asserted where these conditions hold), `shifted_gaussian`.
10. `adf_rfun_dilate_rat`: `DOMAIN` for `h = 0`, also on the zero function (after `prec` and the caps);
    otherwise with `h = n/d` the term `P(h x)`, `A h^2`, `B h`, `C` (P5): `A h^2 = (A n^2)/d^2`,
    `B h = (B n)/d`, and the coefficient `p_j w_j` with `w_0 = 1`, `w_j = (w_(j-1) n)/d`; each `w_j` contains
    `h^j`. `Re(A h^2) = Re(A) h^2 > 0` for members; the ball can lose it (`NOT_DETERMINED`).
    Check: `test_rfun closure` (h = 1, -1, 2/3, -5, 2000-bit), `statuses`, `shifted_gaussian`.
11. `adf_rfun_dilate_idele`: as statement 10 with `h = a->inf`, the real component of the idele (only it is
    used). `h^2` is formed by interval squaring: `[l^2, u^2]` with l and u the lower and upper bounds of `|h|`
    rounded outward (`arb.rst:425-433`, `arf.rst:590`, `arb.rst:481-485`); since the idele's real ball excludes
    0 (conventions 5.7), every point of `[l^2, u^2]` is the square of a point of h or lies between two such
    squares, and `[l^2, u^2]` is the image of h under squaring up to the outward rounding. The product `h h`
    of two independent copies would be wider: for `h = 1 +/- 0.875` it contains negative numbers, while the
    interval square is `[1/64, 225/64]` and keeps `Re(A h^2) > 0` certified. The powers `h^j` of the
    coefficients and `B h` are ball products; they enclose. Decision (the design is silent): the dependency
    between `A h^2`, `B h` and the coefficients is lost, so the result encloses all members of a family that is
    larger than {x(h t) : h in a->inf}; the alternative, a parametrised family, is not representable in the
    struct (CV-20, api-4.md section 1: "loss of dependencies between parameter balls can enlarge the family").
    A ball near 0 (`2^-100 +/- 2^-101`) gives `h^2` in `[2^-202, 9 2^-202]`, positive. Check: `test_rfun
    statuses` (both signs, exact -2 identical to `dilate_rat` by -2, near 0), `aliasing`.
12. `adf_rfun_reflect`: odd coefficients and B negated, exactly; `Re(A)` unchanged. Reflecting twice gives
    the identical value. `adf_rfun_conj`: every coefficient and A, B, C conjugated, exactly; conjugating twice
    gives the identical value. Both: `OK` or `LIMIT`. Check: `test_rfun closure` (reflect, conj).
13. `adf_rfun_eval`: `DOMAIN` for a nonfinite x (after `prec` and the caps). For each term with a nonzero P:
    `P(x)` by Horner (`acb_poly_evaluate_horner`, `acb_poly.rst:446-449`) with x as a ball, `x^2` by the
    interval squaring of statement 11 (`[0, u^2]` when x contains 0), `E = C - pi A x^2 + B x`, `exp(E)`
    (`acb.rst:658-661`), then the sum over terms. Each step encloses on all points of its inputs, so the result
    contains `phi(t)` for every t in x and every member: the image of the ball, not a sample. A term with
    `P = 0` contributes the exact 0 and is skipped (its exponential may overflow without changing the value).
    A nonfinite sum is `NOT_DETERMINED` (`exp(10^300)`). Check: `test_rfun values` (12 functions at 40 points,
    of which 4 are balls with sampled values, plus the shifted Gaussian), `statuses`, `shifted_gaussian`.
14. Layout queries `adf_sizeof_rfun`, `_alignof_rfun`, `_sizeof_rterm`, `_alignof_rterm`: header-inline and
    exported through `src/inlines.c`. Check: `test_rfun lifecycle`, `tests/test_exports.sh`, Julia `rfun.jl`.

## Decisions where the design is silent

- Stage 7 of the reader uses kernel B (the reader of the idele class) before `NOT_DETERMINED`. Alternative:
  only the enclosure at `prec`, which returns `NOT_DETERMINED` more often (for `1 +/- (1 - 10^-59)` at every
  `prec`, since a `mag` radius has 30 bits).
- Work units: translation charges the n(n - 1)/2 multiply-adds that it performs, not `(deg + 1)^2`.
  Alternative: charge `(deg + 1)^2`, which refuses lengths between about 1024 and 1448.
- The caps apply to each input and to the result of every operation, including reflection and conjugation.
- The bit cap applies to the squares `n^2`, `d^2` that translation and dilation form.
- The test of exactness: rounding is asserted absent only where the formula has no pi part, every input is an
  exact dyadic ball, q is dyadic, and the result has fewer than 100 bits.

## Cost

Set, add, reflect, conj: O(total size). Product: one `acb_poly_mul` per pair. Translation: O(sum (deg + 1)^2)
ball operations with integers of the size of q. Dilation: O(total coefficients). Evaluation: O(total
coefficients) plus one exponential per term. Avoidable cost: `pi` and `pi A` are recomputed per term in the
translation (`acb_const_pi` is cached by FLINT, the product is not).

# Slice 4e: transform, derivative, integral and norm of `adf_rfun`

The functions are those of `docs/api-4.md` section 5 (derivative, R1, R2) and section 6 (integral, norm), with
analysis Proposition 5 (P5, `docs/proofs/analysis.md:174-212`). Header `include/adelefeld/rfun.h` (appended
declarations), code at the end of `src/rfun.c`, test `tests/test_rfun_fourier.c`, vectors
`tests/ref/vectors/f4-slice3/` (`lanes/f4-slice3/gen_vectors.py`). The convention is that of conventions 6.1
(CV-54): `F f(y) = integral f(x) conj(psi_inf(x y)) dx` with `psi_inf(x) = E(-x)`, so the real kernel is
`exp(+2 pi i x y)`; P5 step 4 states "Substituting z=B+2 pi i y gives the positive Fourier sign". With it,
`F(T_q phi)(y) = integral phi(x - q) E(x y) dx = E(q y) F phi(y)` (substitute `x -> x + q`), the sign the test
asserts, and `F(D_h phi)(y) = |h|^-1 F phi(y/h)` (P5 step 6; SPEC 7). The words "encloses" and "member" are those
of slice 4d above; every ball operation used contains the exact operation on all points of its inputs
(`refs/src/flint-3.0.1/arb.rst:6-12`, `acb.rst:6-12`).

## Statuses, caps and order of checks

As in slice 4d: `prec > ADF_REAL_PREC_MAX` gives `LIMIT` first; then under INV the entry predicate; then the caps
of D1 (`LIMIT`); then the computation at `p = max(prec, 2)` in fresh storage, committed only after the last
check. `NOT_DETERMINED`: a nonfinite result, a result term without certified `Re(A') > 0`, or a root of R2 that
is not certified (statement 2). No `DOMAIN` on canonical input. Every status other than `OK` leaves `y` or `z`
untouched (sentinel bytes in the test).

## Statements

1. `adf_rfun_derivative`: for each term the coefficient k of `P' + (B - 2 pi A x) P` is
   `(k + 1) p_(k+1) + B p_k - 2 pi A p_(k-1)` (`acb_poly_derivative`, `acb_poly_scalar_mul`, `acb_poly_shift_left`,
   `acb_poly_add`, `acb_poly_sub`; `acb_poly.rst:140-146, 250-289, 563-569`); A, B, C are copied. Proof of the
   formula: `(P f)' = P' f + P f'` and `f' = (B - 2 pi A x) f` for `f = exp(-pi A x^2 + B x + C)`. A zero P stays
   zero; a nonzero P has length `L + 1` (its top coefficient `-2 pi A p_(L-1)` is not the exact zero, since
   `Re(A) > 0` and `p_(L-1)` is not the exact zero). Cap: the result has at most `2^16` coefficients, so a term of
   length `2^16 - 1` passes and `2^16` does not. Check: `test_rfun_fourier vectors` (`rterm_derivative`, every
   member), `covariance` (central differences at 300 bits with `h = 2^-50`; the formula at `A = 1 + i`), `caps`.
2. `adf_rfun_fourier` (R1, R2). For a term, with `w = 1/(2 pi A)`:
   `A' = 1/A` (`acb_inv`), `B' = i (B A')` (`acb_mul_onei`, exact), `C' = C + B^2 w/2` (= `C + B^2/(4 pi A)`), and
   `Q(y) = A^(-1/2) sum_j p_j G_j(y)` with `G_j(y) = H_j(B + 2 pi i y)` of P5. Derivation of the recurrence the
   code uses: from `z = B + 2 pi i y`, `dG_j/dy = 2 pi i H_j'(z)`, so `H_j'(z) = G_j'(y)/(2 pi i)`; and
   `z w = w B + (2 pi i w) y = w B + (i/A) y`. Then `H_(j+1) = H_j' + z w H_j` (P5) becomes
   `G_0 = 1`, `G_(j+1) = G_j'/(2 pi i) + (w B) G_j + (i/A) y G_j`, which builds Q by polynomial arithmetic
   without a composition (R1 step 2: "Expand by polynomial arithmetic"). The degree of `G_j` is j with top
   coefficient `(i/A)^j`, so `deg Q = deg P`. The amplitude `A^(-1/2)` is formed as `1/sqrt(A)` with
   `acb_sqrt_analytic(r, A, 1, p)` and `acb_inv`. Branch (R2): `acb.rst:590-596` documents
   `sqrt(a+bi) = u/2 + ib/u, u = sqrt(2(|a+bi|+a))`; for `a > 0` this is `u' + i v'` with
   `u' = sqrt((|A| + a)/2) > 0` and `v' = b/(2 u')`, the root of R2 step 1. A canonical A box lies in
   `Re > 0` and does not meet the branch cut; `acb.rst:598-601` says that with `analytic` set the result contains
   NaN if z touches the cut, so the root encloses the R2 root of every member. Certificate: the root must be
   finite with `Re > 0` certified (`arb_is_positive`), else `NOT_DETERMINED`. The predicate check of the result
   term (slice 4d `rf_result_ok`) certifies `Re(A') > 0` (R2 step 4: "Re(1/A)>0 still needs the output-ball
   predicate check"), finite parameters and coefficients, else `NOT_DETERMINED`. Enclosure: each step is one
   ball operation on enclosures of the exact quantities of P5, so Q, A', B', C' enclose those of every member;
   the dependency between the four balls is lost (the family can grow, api-4.md section 1). Zero terms are kept
   (their A', B', C' are computed and checked). F^2: the roots of A and 1/A are reciprocal (R2 step 3), so the
   second transform has amplitude 1 and equals the reflection (P5 step 5).
   Work: one unit per multiply-add, `2 L^2 - L + 1` for a term of length `L > 0` (the step `j -> j + 1` costs
   `3 j + 2`, the sum `p_j G_j` costs `j + 1`, the amplitude `L`), 0 for `L = 0`; the sum over terms must not
   exceed `2^20`: `L = 724` passes, `L = 725` does not. Exactness: for `A = 1, B = 0` and `A = 4, B = 0` with
   `deg P <= 1` every operation is exact (`sqrt(1) = 1`, `sqrt(4) = 2`), so `F(exp(-pi x^2))` is identical to its
   input and `F(x exp(-pi x^2)) = i y exp(-pi y^2)` exactly. Check: `test_rfun_fourier exact_cases`, `sign_test`
   (the shifted Gaussian: `B` contains `2 pi/3`, `C` contains `-pi/9`; the transform at `y = 1/4` contains
   `exp(-pi/16) E(1/12)`, has a positive imaginary part and does not meet its conjugate; end to end from
   `adf_rfun_translate_rat`), `vectors` (60 functions: every member's oracle transform, `F^2` against the
   reflection, values at four points, the quadrature transform at two points), `covariance` (dilation by 2/3,
   -5, 3, -1/2 and translation by 1/3, -7/2, 5/8, 16 functions at 3 points; `E(-q y)` is disjoint), `statuses`,
   `aliasing`, `caps`; driver `rfun-fourier`; Julia `rfun_fourier.jl`.
3. `adf_rfun_integral`. Claim: for `Re(A) > 0`, complex B, C and `j >= 0`,
   `M_j = integral_R x^j exp(-pi A x^2 + B x + C) dx = A^(-1/2) exp(C + B^2/(4 pi A)) h_j` with `h_0 = 1`,
   `h_1 = B/(2 pi A)`, `h_(j+1) = (j h_(j-1) + B h_j)/(2 pi A)`; explicitly
   `h_j = j! sum_(r=0)^floor(j/2) B^(j-2r)/[r! (j-2r)! (4 pi A)^r (2 pi A)^(j-2r)] = H_j(B)`.
   Proof. (a) `M_0`: P5 step 3 with `z = B`, the root positive for `A > 0` and continued to `Re(A) > 0` (R2).
   (b) Put `f(x) = exp(-pi A x^2 + B x + C)`, so `f' = (B - 2 pi A x) f`. Then
   `(x^j f)' = j x^(j-1) f + B x^j f - 2 pi A x^(j+1) f`. Integrate over `[-T, T]` and let `T -> infinity`: the
   boundary terms `T^j |f(+-T)| <= T^j exp(-pi Re(A) T^2 + |Re B| T + Re C)` tend to 0 and every integrand is
   integrable, so `0 = j M_(j-1) + B M_j - 2 pi A M_(j+1)` (with `j M_(j-1) = 0` for `j = 0`). Divide by
   `2 pi A != 0` and by `M_0`. (c) The closed form: the generating function of R1 step 1,
   `sum_j H_j(z) t^j/j! = exp(w z t + w t^2/2)` with `w = 1/(2 pi A)`, gives `H_j' = j w H_(j-1)` (differentiate in
   z) and so `H_(j+1)(B) = w (j H_(j-1)(B) + B H_j(B))`: the same recurrence and start, hence `h_j = H_j(B)`, and
   the integral of a term, `M_0 sum_j p_j h_j`, is the formula of api-4.md section 6 and equals `F phi(0)`.
   The code runs the recurrence in acb (`acb_addmul`, `acb_mul_si`), forms `exp(C + B^2 w/2)` (`acb_exp`) and the
   amplitude of statement 2 (root certificate as there); a term with `P = 0` contributes the exact 0 and is
   skipped; the zero function gives the exact 0. A nonfinite sum is `NOT_DETERMINED` (`C = 10^300`). Cost:
   `O(total coefficients)` plus one exp and one root per term; not charged against the work cap (at most
   `3 * 2^16` units). Check: `test_rfun_fourier vectors` (every member's `F phi(0)` from the oracle; our own
   transform evaluated at 0 by the other recurrence; the quadrature value of the oracle within 1e-40),
   `exact_cases` (A = 4: 1/2), `statuses`; driver `rfun-fourier`.
4. `adf_rfun_norm2`: `adf_rfun_conj` then `adf_rfun_mul(phi, conj phi)`, which forms the term `P_k conj(P_l)`,
   `A_k + conj(A_l)`, `B_k + conj(B_l)`, `C_k + conj(C_l)` for every ordered pair `(k, l)` (all cross terms; for
   real x `|phi(x)|^2 = phi(x) conj(phi(x))`, and `conj(phi)` on real x is the function with conjugated
   coefficients and parameters), then the integral of statement 3. `Re(A_k + conj A_l) = Re A_k + Re A_l > 0` for
   members; the product returns `NOT_DETERMINED` if the ball loses it. For members the value is real and
   nonnegative; the result is the real part of the enclosure intersected with `[0, infinity)` by
   `arb_nonnegative_part` (`arb.rst:417-423`: an exact copy if nonnegative, otherwise a ball `[r/2 +/- r/2]`). A
   provably negative enclosure cannot occur (it contains the nonnegative true value); the code calls
   `flint_abort` there, the internal defect of api-4.md section 6. Caps: those of the product (`len^2 <= 2^16`
   pairs, `(total coefficients)^2 <= 2^20` work, `2^16` result coefficients): 256 terms pass and 257 do not, a
   term of length 1024 passes and 1025 does not. Check: `test_rfun_fourier vectors` (the oracle's sum over every
   ordered pair of `rterm_product(t_k, conj t_l)` transformed at 0, for every member; mul and conj of slice 4d;
   the quadrature of `|phi|^2`), `exact_cases` (the closed form `(2 Re A)^(-1/2) exp(2 Re C + (Re B)^2/(2 pi Re A))`
   of one Gaussian, real and complex), `caps`, `statuses`; driver `rfun-fourier`.

## Decisions where the design is silent

- The root: `acb_sqrt_analytic` and `acb_inv` (api-4.md R2 step 4 allows `acb_sqrt` or `acb_rsqrt_analytic`),
  because perfect squares then give exact roots. Alternative: `acb_rsqrt_analytic`, one call.
- The transform is computed in the variable y by the recurrence for `G_j` (statement 2), not by expanding
  `sum p_j H_j(z)` and composing with `z = B + 2 pi i y`. Same cost order, no composition step.
- The integral uses the value recurrence `h_j` (O(L) per term), not the transform polynomial (O(L^2)); the
  test compares the two.
- Work of the transform: the multiply-adds performed, `2 L^2 - L + 1` per term. Work of the integral and the
  derivative: not checked, since it is at most `3 * 2^16 < 2^20`. Norm: the caps of `adf_rfun_mul`; the integral
  of the product is not charged in addition.
- A zero polynomial term of the transform keeps its new parameters and is checked like any other term, so it can
  give `NOT_DETERMINED` (api-4.md section 1: the predicate holds for every term).
- Tightness stated in the test: radius at most `2^-96 (1 + |v|)` for exact inputs at 128 bits, `2^-2 (1 + |v|)`
  for ball inputs; for `Re(A) = 10^-30` integrals, norms and values `2^-16 (1 + |v|)` and `F^2` by containment
  only, because the exponents and coefficients there have size `2^100` and lose their low bits at 128 bits
  whatever the method.

## Cost

Derivative O(total coefficients). Transform O(sum (deg + 1)^2) ball operations plus one root and one inverse per
term. Integral O(total coefficients) plus one exp and one root per term. Norm: the product, `len^2` terms and
`(total coefficients)^2` multiply-adds, then their integrals. Avoidable costs: `pi` and `2 pi A` are recomputed per
term in the derivative and the integral; the norm forms the conjugate as a full copy and integrates both `(k, l)`
and `(l, k)`, which are conjugates (half of the work, but the design asks for every ordered pair).
