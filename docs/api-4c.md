# Slice 4f: evaluation and additive integrals of test functions

The functions are those of `docs/api-4.md` section 6 (statement E1, steps 1 to 5; the integrals and norms) with
decision D3 (`docs/api-4.md:399-401`; SPEC 15.4, N-D23) and analysis Proposition 4 (P4,
`docs/proofs/analysis.md:150-173`). Header `include/adelefeld/tensor.h`, code `src/tensor.c`, test
`tests/test_tensor.c`, vectors `tests/ref/vectors/f4-slice5/` (`lanes/f4-slice5/gen_vectors.py`, from
`proto/functions4_checks.py`), driver fixture `tests/driver/tensor-eval`, Julia `tests/julia/tensor.jl`.

The three `adf_ffun_*` functions of this slice (`eval`, `integral`, `norm2`) are declared in `tensor.h`, not in
`ffun.h`, because slice 4b appended to `src/ffun.c` and `ffun.h` in a parallel lane. They may move later; the ABI
does not depend on the header.

"Encloses" and "member" are those of slice 4d (`docs/api-4b.md`): a member of an ffun chooses one point in each
ball `f[j]`; a member of an rfun chooses one point in each parameter ball. Every ball operation used contains the
exact operation on all points of its inputs (`refs/src/flint-3.0.1/arb.rst:6-12`, `acb.rst:6-12`).

## Statuses, caps and order of checks

1. `prec > ADF_REAL_PREC_MAX` gives `LIMIT` (api-4.md:29).
2. The size preflight of D1 (api-4.md:36-43, "Preflight arithmetic sizes before INV"), `LIMIT`:
   - an ffun with `D M > 2^20` (decided as `D > 2^20 / M`, without forming `D M`);
   - an rfun with more than `2^16` terms or coefficients;
   - a finite ball whose raw `A`, `H` or `d` has more than `2^20 - 128` bits;
   - an lball centre of an sball whose numerator or denominator has more than `2^20 - 128` bits;
   - more than `2^20` work units in `tensor_eval_sball` (decision 1 below).
3. Under INV, the entry predicates of every input (`adf_ffun_is_canonical`, `adf_rfun_is_canonical`,
   `adf_fball_is_canonical`, `adf_adele_is_canonical`, `adf_sball_is_canonical`); a failure aborts.
4. `DOMAIN`:
   - a nonfinite raw real input (the real part of an adele, which INV rejects first);
   - the COMPLEX tag of an sball (E1 step 5: no complex-to-real coercion).
5. The computation at `p = max(prec, 2)` in temporaries.
6. `NOT_DETERMINED`:
   - a nonfinite result;
   - the bound of E1 step 5 not certified (`alpha` not certified positive, or `R` not finite).
   During the computation, `tensor_norm2` can also return the `LIMIT` of the real norm of slice 4e (the cap of
   its product of `phi` with `conj(phi)`).
7. The result is swapped into `z` after the last check. Every status other than `OK` leaves `z` untouched
   (sentinel bytes in the test).

## Statements

1. `adf_ffun_eval` (E1 steps 1-3). Let `(A, H, d)` be the canonical global triple of `x` (`adf_fball_get_fmpz3`,
   `include/adelefeld/fball.h:156-162`; a CRT recombination for a local `x`), so `x = (A + H Zhat)/d`, `d > 0`,
   `H >= 0`. Write `L = D M`.
   - Step 1, the gcd criterion. `x` meets the coset `j/D + M Zhat` iff `g = gcd(H D, M d D)` divides `A D - j d`.
     Proof:
     - Multiplication by `d D` is injective on `A_f`. So the two sets meet iff `A D + H D Zhat` meets
       `j d + M d D Zhat`, that is iff `A D - j d` lies in `H D Zhat + M d D Zhat`.
     - By Bezout, `n Zhat + m Zhat = gcd(n, m) Zhat` (`gcd(0, m) = |m|` covers `H = 0`).
     - An integer `k` lies in `g Zhat` iff `k/g` lies in `Zhat ∩ Q = Z`, that is iff `g | k`.
   - Step 2, the support. `x` lies in `(1/D) Zhat` iff `d | D A` and `d | D H`. Proof:
     - `x` lies in `(1/D) Zhat` iff `D x` lies in `Zhat`.
     - The point `u = 0` of `D x` is `D A/d`, a rational; it lies in `Zhat` iff it is an integer.
     - If `D A/d` is an integer, the difference of the points `u = 1` and `u = 0`, `D H/d`, must be one too.
     - Conversely, if both are integers, `D A/d + (D H/d) Zhat` lies in `Zhat`.
   - The image of `x` under a member of `f`:
     - The cosets `j/D + M Zhat`, `0 <= j < L`, partition `(1/D) Zhat`, and `f` is `f[j]` on the coset `j` and 0
       off `(1/D) Zhat`.
     - So the image is the set of the `f[j]` with `j` met, together with 0 when `x` is not inside `(1/D) Zhat`.
     - The image is never empty. If no index is met, `x` is disjoint from `(1/D) Zhat`, and the image is `{0}`:
       the exact 0.
   - Step 3, the hull (`tn_box`, `tn_set_interval`). The result is a rectangle that contains every selected ball
     and, when `x` leaves the support, the exact 0. So it contains the image under every member.
     - Per part, each lower endpoint `mid - rad` is rounded down at `p` and each upper endpoint `mid + rad` is
       rounded up; then the minimum and the maximum are taken.
     - One ball containing `[a, b]` is then formed. Its midpoint is `(a + b)/2`, rounded, and its radius is
       `(b - a)/2` plus half the ulp of the rounded sum. The radius is exact when `b - a` fits a mag of 30 bits.
     - Enclosure: the distance between the midpoint and `(a + b)/2` is at most half an ulp, and the radius covers
       it.
     - A single ball, or the exact 0 alone, is copied unchanged: a point ball gives `f[j]` exactly.
   - Check: `test_tensor`:
     - `eval_vectors`: 1603 balls times 3 families; containment, tightness, exclusion, the exact 0;
     - `local_vectors`: 56 local balls of the context with blocks 8, 9, 5, equal to the global result;
     - `exact_cases`:
       - E1 step 3 exactly: `[1, 5] = 3 +/- 2`, `[0, 6] = 3 +/- 3`, 1/5 gives exactly 0;
       - point balls;
       - the jump of PLAN 4.4;
       - prec 2, 53, the cap, 0, -5.
2. `adf_tensor_eval`. For every point `t = (t_inf, t_f)` of the adele `x`:
   - `phi(t_inf)` lies in `adf_rfun_eval(phi, x->inf)` (slice 4d);
   - `f(t_f)` lies in the hull of statement 1;
   - so `phi(t_inf) f(t_f)` lies in their `acb_mul`.
   This holds for every member of `phi` and of `f`. The hull is computed before the real value, and `z` is written
   only after both.
   - Check: `tensor_vectors`:
     - 5 rfuns at 4 real points and 4 finite balls, against `rvalue_ball` of the oracle times `f[j]`;
     - equality with the product of the two component calls;
     - a real ball of radius `2^-10`: the values at both ends.
3. `adf_tensor_eval_sball` (E1 step 4, D3). A completion of the sball `x` is a finite adele `t` with `t_p` in the
   component `B_p` at each supplied prime `p`, and any `t_q` at the other primes. The component is
   `B_p = a_p + p^e Z_p`, or `{a_p}` for an exact local point.
   - Keep `j` iff, at every supplied `p`, `v_p(j/D - a_p) >= min(e, v_p(M))`, or `>= v_p(M)` for an exact point.
     Proof:
     - `j/D + M Zhat` is the product over all primes `q` of `j/D + q^(v_q(M)) Z_q`.
     - Two balls of `Q_p` meet iff the larger contains the smaller, that is iff the difference of their centres has
       valuation at least the smaller exponent. A point lies in a ball iff its difference with the centre has
       valuation at least the exponent.
     - Necessity: project a common point to `Q_p`.
     - Sufficiency (local independence): take `s_p` in `B_p ∩ (j/D + p^(v_p(M)) Z_p)` at each supplied `p`, and
       `t_q = j/D` at every other prime `q`. Then `t` lies in `j/D + M Zhat` and is a completion.
   - The value 0 is always attained, not only admitted:
     - Finitely many primes are supplied, so some prime `q` is absent.
     - The completion with `t_q = q^(-v_q(D) - 1)` lies outside `(1/D) Zhat`.
     - So the hull of the kept `f[j]` and of 0 is the hull of the exact image over all completions.
   - Missing primes impose nothing. No prime is factored: only the supplied primes and `v_p` of the machine words
     `j`, `D` and `M` are used.
   - The valuation is found without forming `p^v` for the centre `c = p^v u` (`u = 0`, or `v_p(u) = 0`;
     `include/adelefeld/lball.h:72-80`):
     - `v_p(c) = v`, and `v_p(j/D) = v_p(j) - v_p(D)`.
     - If the two differ, `v_p(j/D - c)` is their minimum.
     - Only when they are equal (then `|v| <= 63`, since `j, D < 2^62`) is `D c - j` formed exactly, and
       `v_p(j/D - c) = v_p(D c - j) - v_p(D)`.
     - `j = 0` or `c = 0` gives the valuation of the other term (`+infinity` if both are 0).
   - Real part: with the tag REAL, `adf_rfun_eval` on the real component; with NONE, statement 4. The product is
     formed as in statement 2.
   - Check:
     - `sball_vectors`: 91 tuples at the places `{2, 3}`, `{5}` and `{}`, against the oracle's
       `evaluate_partial` and against the generator's own transcription of the step 4 rule (they agree on every
       record).
     - 49 adele projections to `{inf, 2, 3}`: the adele hull lies in the sball hull, and the index sets are
       equal in 46 of 49 (the three others are at `(D, M) = (12, 5)`, whose prime 5 is not supplied).
     - `sball_cases`: the oracle example `{2, 5}` and zero, at 3 with `1 + 3 Z_3`; the exact point `1/4` at 2 gives
       exactly 0.
4. E1 step 5, the tag NONE (`tn_bound`). For real `t` and a term `P(t) exp(-pi A t^2 + B t + C)`:
   - `|exp(...)| = exp(-pi Re(A) t^2 + Re(B) t + Re(C)) <= exp(-alpha t^2 + beta |t| + gamma)`, with
     `alpha = pi lower(Re A)`, `beta = upper(|Re B|)` and `gamma = upper(Re C)`.
   - Completing the square, `-alpha t^2 + beta |t| = -alpha t^2/2 - (alpha/2)(|t| - beta/alpha)^2 +
     beta^2/(2 alpha) <= -alpha t^2/2 + beta^2/(2 alpha)`.
   - `|t|^j exp(-alpha t^2/2)` is largest at `t^2 = j/alpha`, with the value `(j/(alpha e))^(j/2) = T_j` (and
     `T_0 = 1`).
   - So `|phi(t)| <= R = sum over terms of exp(gamma + beta^2/(2 alpha)) sum_j upper(|p_j|) T_j`, and `phi(t)`
     lies in `[-R, R] + i [-R, R]`.
   - `R` decreases in `alpha` and increases in `beta`, `gamma` and `|p_j|`. So evaluating it in ball arithmetic at
     the bounds (a lower bound of `alpha`; upper bounds of the others) and taking the upper bound of the result
     bounds every member.
   - A term with `P = 0` contributes 0; the zero rfun gives `R = 0` and the exact 0.
   - Check: `none_bounds`: the 5 rfuns of the vectors and three more at 1000 sampled points each, among them
     `exp(-pi x^2 + 4 x)`, whose maximum `exp(4/pi) = 3.57` exceeds the bound without the factor
     `exp(beta^2/(2 alpha))`. Also `sball_cases`: the Gaussian gives `R = 1`.
5. Integrals and norms (P4). The integral of `f` is `(1/M) sum f[j]`, and that of `|f|^2` is
   `(1/M) sum |f[j]|^2` (P4:157-158; Haar measure with `Zhat` of volume 1, each coset of `M Zhat` of volume
   `1/M`, conventions 6.2).
   - The norm is summed as `Re^2 + Im^2` per entry, divided by `M`, then intersected with `[0, infinity)` by
     `arb_nonnegative_part` (`arb.rst:417-423`). The true value is nonnegative and lies in the enclosure, so the
     intersection keeps it. A provably negative enclosure is an internal defect: `flint_abort` (api-4.md:288).
   - `tensor_integral` is the product of the real integral (slice 4e) and the finite one. `tensor_norm2` is the
     product of the two norms, since `|phi(x) f(y)|^2 = |phi(x)|^2 |f(y)|^2`; it is clipped again after the
     product. Both rest on Fubini on the product measure of `R x A_f` [source pending: Fubini/Tonelli for the
     product of Lebesgue and Haar measure, the analytic import listed at docs/proofs/analysis.md:30-66].
   - Check: `integral_vectors`: 14 functions with exact rational results from the generator; the result is exact
     for `M` in `{1, 4}`; Parseval against `adf_ffun_fourier` (P4:158, weight `1/D`); a value `0 +/- 1` gives a
     nonnegative norm; `faults_44`: 7 and 91/3. `tensor_vectors`: against `rvalue_ball` of the oracle transform
     at 0 and the ordered pairs `rterm_product(t_k, conj t_l)`.

## Decisions where the design is silent

1. Work units of `tensor_eval_sball`: one per index and supplied prime, at least one per index; more than `2^20`
   gives `LIMIT` (D1). `ffun_eval` uses one per index, so the array cap bounds it.
2. Bits: the raw `A`, `H` and `d` of a finite ball, and the numerator and denominator of an lball centre, have at
   most `2^20 - 128` bits. Then `A D`, `H D`, `M d D` and `j d` stay within `2^20` bits, since the factors are
   below `2^62`.
3. The hull is formed over all the selected balls at once, with an exact radius where possible. Chained pairwise
   `acb_union`, and `arb_set_interval_arf`, of the installed FLINT 3.0.1 round the radius up by one mag ulp at
   each step: `[1, 5]` becomes `3 +/- (2 + 2^-28.4)`, and the union of `2 +/- 1` with 5 becomes
   `(3 - 2^-30) +/- (2 + 2^-27)`. The set is the same rectangle as the design's `acb_union`, up to rounding.
4. `tensor_norm2` intersects the product with `[0, infinity)` again: the product of two clipped balls can have a
   negative lower bound.
5. The order of statuses is that of the list above; `tensor_eval` computes the finite hull before the real value.
6. Driver: `ffun_eval F with B` also accepts a rational `B` (an exact finite ball); `tensor_eval_sball` reads the
   sball text of conventions 9.2.

## Cost

- `ffun_eval`: the global triple (a CRT and a gcd for local input), one gcd, then `L` products and exact
  divisibility tests on integers of the size of `A`, `d`, and `L` endpoint comparisons.
- `tensor_eval`: that, plus `adf_rfun_eval`.
- `tensor_eval_sball`: `L` times the number of supplied primes valuation tests on words (an exact rational
  difference only when the valuations are equal), plus `adf_rfun_eval`, or the bound: per term one exp, plus one
  log and one exp per nonzero coefficient.
- The integrals and norms: `O(L)` ball operations, plus slice 4e's integral and norm.
- Avoidable cost:
  - `tn_hull` recomputes `j d` per index; a running residue modulo `g` would do.
  - `tn_keep` recomputes `v_p(M)` and `v_p(D)` per index.
