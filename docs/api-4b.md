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
