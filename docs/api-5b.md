# Slice 5c: the Tate test vector and the global Tate integral in Re(s) > 1

The functions are those of `docs/api-5.md` section 3 (`:121-165`, decision D3) with the cutoffs, the quadrature
and the retry rule of section 4 (`:183-228`, decision D4) and the statements T3, T5 of section 6 (`:286-300`,
`:313-325`). Header `include/adelefeld/tate.h`, code `src/tate.c`, test `tests/test_tate.c`, vectors
`tests/ref/vectors/t5-slice2/tate.jsonl` (`lanes/t5-slice2/gen_vectors.py`, from `proto/tate_checks.py`), driver
fixture `tests/driver/tate`, Julia `tests/julia/tate.jl`. This file is new because `docs/api-5a.md` (lane t-slice1,
slice 5a) did not exist in this worktree; the orchestrator may merge the two.

Here P11, P12, P13, L14, P15, L6 are the statements of `docs/proofs/analysis.md` (`:452-495`, `:496-563`,
`:564-626`, `:627-651`, `:652-732`, `:213-259`); L8 and P5 are Lemma 8 (`:295-330`) and Proposition 5 (`:174-212`).
Every ball operation used contains the exact operation on every point of its inputs
(`refs/src/flint-3.0.1/arb.rst:6-12`, `acb.rst:6-17`); `arb_get_ubound_arf` and `arb_get_lbound_arf` round outward
(`arb.rst:430-443`); `arb_le` and `arb_gt` hold only if the comparison holds for all points (`arb.rst:705-718`).

## Statuses and order of checks

`adf_tate_vector(phi, f, chi, prec)`:

1. `prec > ADF_REAL_PREC_MAX`: `LIMIT`. 2. `C > ADF_CHAR_MOD_MAX = 65536`: `LIMIT`, before any character setup.
3. Under `INV`: `chi` canonical, `phi` and `f` distinct; a violation aborts.
4. A phase that `adf_char_chi` does not certify (`NOT_DETERMINED`), or a character setup failure (`UNSUPPORTED`):
   `NOT_DETERMINED`. Otherwise `OK`. Every status other than `OK` leaves both outputs untouched (built in
   temporaries, swapped in at the end).

`adf_tate_integral(z, chi, s, bits, prec)`:

1. `prec > ADF_REAL_PREC_MAX`: `LIMIT`.
2. `C > ADF_TATE_TRANSFORM_C_MAX = 1024`: `LIMIT` (D2: the direct finite transform needs `C^2 <= 2^20`,
   `api-5.md:45`; this also covers the character cap 65536).
3. `bits` outside `[0, 2^21]`: `DOMAIN`.
4. Under `INV`: `chi` canonical and `z != chi->s`; a violation aborts.
5. `s` not finite: `DOMAIN`. `arb_le(Re s, 1)` (every point has real part `<= 1`): `DOMAIN`. Otherwise, unless
   `arb_gt(Re s, 1)` (every point `> 1`): `NOT_DETERMINED`. The closed endpoint `upper(Re s) = 1` is `DOMAIN`.
6. The attempts (S6): `LIMIT` when the work counter would pass `ADF_TATE_WORK_MAX = 2^20`; `NOT_DETERMINED` by the
   retry rule. Every status other than `OK` leaves `z` untouched, also when `z` is `s` (the input is copied first).

## Statements

S1. The vector. `f` has `D = 1`, `M = C` and `f[j] = adf_char_chi(j)` at `max(prec, 2)` for `j` in `[0, C)`; `phi`
    is one term with `P = x^e` (`e` the parity), `A = 1`, `B = C_real = 0`. For `C = 1` every phase is 0 (L8:
    `chi(n) = 1` for every `n`), so `f[0] = 1`. A nonunit gives the exact 0 (char.h). The finite factor is the
    function `n -> chi(n)` on `Zhat` (D = 1, M = C: the value on `j + C Zhat`); at a prime `p | C` it is
    `chi_p(x_p) 1_(Z_p^x)` and elsewhere `1_(Z_p)`, which is `eta_p^(-1) 1_(Z_p^x)` with `eta_p = conj(chi_p)` of
    conventions 6.5 and P11. What the code promises about the entries: exact for the phases 0, 1/4, 1/2, 3/4, and an
    exact coordinate where `adf_phase_get_acb` gives one (e.g. `cos(2 pi/3) = -1/2`); otherwise a ball of that
    precision containing the value. `chi->s` is not read.
    Check: `vector_record` (18 vectors: every entry overlaps the 512-bit phase of the oracle, cardinal phases
    exact, other entries of radius `<= 2^-118` at prec 128; the term exactly; `chi->s = 5 + 7 i` gives identical
    outputs), `vector_statuses` (LIMIT order, sentinel bytes), `tests/driver/tate.cmd` (three vectors).

S2. The value and the split. For `Re(s) > 1`, P11 gives `I_chi(s) = pi^-z Gamma(z) L(s, chi)`, `z = (s + e)/2`,
    and `Lambda = C^z I`. P13 step 3 gives, for every `s`,
    `Lambda = integral_1^inf V_chi(t) t^(z-1) dt + W_chi integral_1^inf V_conj(chi)(t) t^(z'-1) dt
    + delta [1/(s-1) - 1/s]`, `V_chi(t) = sum_(n>=1) chi(n) n^e exp(-pi n^2 t/C)`, `z' = (1 - s + e)/2`,
    `delta = [C = 1]`. The code returns `C^-z` times an enclosure of this `Lambda` (`C^-z = exp(-z log C)`, an acb
    ball containing it for every `z` of the ball). The domain gate restricts the call to `Re(s) > 1`, where the
    value is the defining adelic integral (P11); the split is entire for `C > 1` and has simple
    poles at 0 and 1 for `C = 1` (P13). It is the 5d path.

S3. The dual coefficients from the transforms (design `:133-138`; P13 step 5). The code builds the balanced vector
    with the same setters: `f` as in S1 and the real term `x^e exp(-pi x^2/C)` (`A = 1/C`, a ball at the working
    precision). `adf_ffun_fourier` gives `g` with layout `(C, 1)`: `g[k] = (1/C) sum_j chi(j) E(-jk/C)`
    (ffun.h, P4), which is `(tau/C) (-1)^e conj(chi(k))` by L8 with `m = -k` and `chi(-1) = (-1)^e`.
    `adf_rfun_fourier` gives `Q(y) exp(-pi A' y^2)` with `A' = 1/A = C` and `Q(y) = A^(-1/2) H_e(2 pi i y)` (R1,
    P5): `Q(y) = sqrt(C)` for `e = 0` and `Q(y) = i C^(3/2) y` for `e = 1`. The coefficient of the dual theta at
    `k/C` is `b_k = g[k mod C] Q(k/C)`; for `e = 0` it is `sqrt(C) (tau/C) conj(chi(k)) = W conj(chi(k))`, for
    `e = 1` it is `i C^(3/2) (k/C) (tau/C) (-1) conj(chi(k)) = tau/(i sqrt(C)) k conj(chi(k))`, so in both cases
    `b_k = W_chi conj(chi(k)) k^e` with `W_chi = tau/(i^e sqrt(C))` (P13). Its Gaussian is
    `exp(-pi C (k/C)^2 t) = exp(-pi k^2 t/C)`, the same `a0 = pi/C` as the forward side. The code uses the exact
    `a0` (the ball of `A'` contains `C`; using the exact exponent is the same function). The forward coefficients
    are `a_n = f[n mod C] n^e = chi(n) n^e`. The computed balls `a_n`, `b_n` contain the exact coefficients
    (transform-generated widths included, design `:191`).
    Check: `theta` (for `C = 1, 4, 5, 16`, `n <= 3C`: `g[n mod C] Q(n/C)` overlaps `W conj(chi(n)) n^e` with `W`
    from `adf_char_root_number`, radius `<= 2^-100`; the balanced factor built there through
    `adf_rfun_dilate_idele` by the idele with real part `1/sqrt(C)` and the factor `C^(e/2)`, T5.1).

S4. The cutoffs (design `:194-205`; P15 with L6 and L14). `E_n(v, N) = exp(a0) S_e(a0, N) J(Re(v) - 1, a0, 1)` and
    `E_t(v, R) = exp(a0) S_e(a0, 0) J(Re(v) - 1, a0, R)` for `v = z, z'`. `S_e(c, N)` is L6 with `beta = 0` at the
    exact lower bound `c` of `pi/C` (the sum decreases in `c`); `J` is L14 with both branches, at the exact upper
    bound of `Re(v) - 1` and the exact lower bound of `a0` (for `t >= R >= 1`, `t^r exp(-b t)` increases in `r`
    and decreases in `b`), the branch test `b R >= 2 max(r, 0)` decided on exact dyadics, `t*` and `R0` as balls
    (`arb_max`, `arb_min`). P15 step 1 (`n^2 t >= n^2 + t - 1`) gives `|omitted n > N| <= E_n` and
    `|omitted t > R| <= E_t` for every point of the ball, with `|chi(n)| <= 1` and `|W| = 1` (P13) on the dual side.
    The code searches `N = 0, 1, 2, 4, ...` until `upper(E_n(z, N) + E_n(z', N)) <= epsilon/32` and
    `R = 1, 2, 4, ...` until `upper(E_t(z, R) + E_t(z', R)) <= epsilon/32`, with
    `epsilon = 2^-bits/max(1, upper|C^-z|)` (design `:205`; here `Re z > 1/2`, so `|C^-z| < 1` and
    `epsilon = 2^-bits`). The bounds are computed at 64 bits; any precision gives valid upper bounds.
    Check: `cutoff_record` (57 records: C = 1, 4, 5 at `s = 2`, bits 20, 53, 80; C = 1, 4, 5, 16 at
    `s = 2, 3, 9/8, 6`, bits 0, 4, 12): through the hidden hook `adf_tate_cutoffs`, `N`, `R` and every panel
    degree `J` of both sides equal those of the oracle's `continuation`; `gate` (the work cap: C = 1024 refused
    by the charge before the transform; zeta at bits 1400 refused by the sum of the panels while each fits).

S5. The quadrature (D4, T3). `[1, R]` with `R = 2^K` is split into `[2^k, 2^(k+1)]`, `k < K`: `m = 3 2^(k-1)`,
    `h = 2^(k-1)`, `d = m/2`, `q = h/d = 2/3`. For each `n` with a nonzero coefficient, `b = a0 n^2`,
    `c_0 = exp((z-1) log m - b m)`, `c_(k+1) = ((z - 1 - b m - k) c_k - b c_(k-1))/(m (k+1))`, and the panel
    integral `2h sum_(2j <= J) c_(2j) h^(2j)/(2j + 1)`, all in acb with `z` the input ball; powers of `h` are exact
    shifts. The majorant `B = sum_(n<=N) n^e exp(-a0 n^2 (m-d)) max((m-d)^r, (m+d)^r) exp(pi upper|Im z|/2)` uses
    the ball `r = Re(z) - 1` (so every `r` of the rectangle), the upper bounds of each factor; the panel error
    `6 h B (2/3)^(J+1) = 2h B q^(J+1)/(1-q)` with the least `J >= 0` whose upper bound is `<= epsilon/(64 K)`.
    T3 proves that the true panel integral of every term lies within this bound of the polynomial integral, for
    every `z` of the ball and every coefficient of modulus `<= n^e`; the ball arithmetic encloses the polynomial
    integral. The panels' bounds are summed for each side.
    Check: `piece_record` (18 records, the oracle's `check_certificates` cases: `C = 1, 4, 5`;
    `z = -3/4 + i/2, 9/4 - 5i/4`; `R = 1, 2, 8`; `N = 6`: the value `+-` the remainder contains mpmath's
    incomplete-Gamma integral at targets `2^-60` and `2^-6`; the remainder is `<= target`; `R = 1` gives the exact
    0; at `2^-6` all 12 records with `R > 1` need the remainder, i.e. the polynomial part alone misses).

S6. Assembly, width and retries (design `:218`, `:223-228`). `Lambda` = forward side + dual side + (for `C = 1`)
    `1/(s-1) - 1/s`; then `E_n + E_t` (both exponents) + both quadrature remainders, upper bound, are added to
    both coordinate radii (for a complex error `w` with `|w| <= E`, `|Re w|, |Im w| <= E`); then the product with
    the ball of `C^-z`. By S2 to S5 the result contains `I_chi(s)` for every `s` of the input rectangle. Width: the
    bounds add at most `3 epsilon/32` to each radius before the factor `|C^-z| <= 1`. The result is `OK` only if
    each coordinate diameter is `<= 2^-bits` after all of this (`tt_narrow`). Otherwise the working precision
    `w` (starting at `max(prec, 2)`) is doubled up to `ADF_REAL_PREC_MAX`; the call stops with `NOT_DETERMINED`
    when a doubling from a precision `>= 64` does not halve the largest coordinate radius (a nonfinite result
    counts as not halved), or at the cap. A dependency that does not certify at a low precision (a transform,
    `Re(A)` of the balanced term) counts as a nonfinite attempt. Repeated work is charged again.
    Check: `widths` (C = 1, 5, 16 at `2 + 3 i`: bits 20, 53, 80 at `prec = bits + 64`, each diameter `<= 2^-bits`,
    the largest radius decreasing: the SPEC 8 acceptance; at `prec = 2` the same targets met by the retries),
    `boundary` (zeta at `2 +- r`, 192 radii across the width `2^-20`: 65 OK, each narrow and containing
    `I(2 - r)` and `I(2 + r)`, 127 NOT_DETERMINED), `value_record` (the input balls of radius `2^-60`, `2^-70`:
    NOT_DETERMINED at bits 80, where two corner references differ by more than `2^-79`, N-D23), the witness
    (zeta on `[9/8, 5/4]`: NOT_DETERMINED at bits 0 and 20).

S7. The values against the references (SPEC 8 acceptance). Check: `value_record` (144 records: C = 1 and the 17
    golden characters at `s = 2, 3, 9/8, 3/2 + 14 i, 2 + 3 i, 10001/10000` and two balls; bits 53, and 20, 80
    for C = 1, 4, 5, 16: 256 OK calls; each result contains the 60-digit mpmath value and python-flint's L value
    completed by hand, 1024 points; each diameter `<= 2^-bits`), `hand` (`I(2) = pi/6`; chi_4 at 2:
    `G/(2 pi)` with Catalan's `G` and FLINT's `acb_dirichlet_l` completed by hand; `L(1, chi_4) = pi/4` is outside
    the domain: DOMAIN), `tests/driver/tate.cmd`, `tests/julia/tate.jl`.

S8. T5 at sample `t` (design `:140-144`, `:313-325`). The test compares both enclosures of `adf_tensor_poisson` on
    the vector with the real factor dilated by `u` (`adf_rfun_dilate_rat`, `u = 1, 3/2, 2/3`; theta variable
    `t = C u^2`; the finite factor unchanged) with the expansion `u^e Theta_chi(t)` and with
    `u^e W t^(-e-1/2) Theta_conj(chi)(1/t)` (P13), each a partial sum `|n| <= 40` plus the L6 tail.
    Check: `theta` (C = 1, 4, 5, 16; 12 comparisons, each side overlapping its expansion and each other).

## Decisions where the design is silent

1. The dual coefficients are `g[n mod C] Q(n/C)` from the two transforms (the generic transformed-factor
   expression), not `adf_char_root_number` times `conj(chi(n))`; the root number is used only in the test (S3).
2. The balanced real factor is set directly (`A = 1/C`, `P = x^e`) with `adf_rfun_set_terms`; the design's idele
   dilation by `1/sqrt(C)` followed by the scalar `C^(e/2)` is the same function (T5.1) and is what the test uses.
3. The exponent `a0 = pi/C` is used on both sides (S3), not the transformed `A'` ball; the pole terms of `C = 1` are
   the exact `1/(s-1) - 1/s` (P12: `a = b = 1`), not `g[0] Q(0)`.
4. The L6 kernel `pn_series` of `src/poisson.c` is static; `tt_series` is its `beta = 0` case written again (the
   design asks to consume the existing tail kernels: not possible without changing `poisson.c`).
5. Work units (D2): `C` (vector entries) + `C^2 + 2 L^2 - L + 1` (`L = e + 1`; the two transforms) + one per
   Lemma 6 step (prefix included) + one per `N` or `R` search step + `2N` (coefficient entries) +
   `(J + 1)` per nonzero coefficient and panel, charged before the panel is computed; on every attempt.
   `C = 1021` (`C^2 = 1042441`) passes the size check and is refused by the total within about 1.2 s.
6. The retry rule is that of `adf_tensor_poisson` (halving from a precision `>= 64`); no guard bits are added to the
   starting precision `max(prec, 2)`.
7. Two hidden functions (not exported from a shared object, as `src/roots.c:49-53`): `adf_tate_taylor_piece`
   gives the test the panel kernel of S5 with `a0 = pi/C`; `adf_tate_cutoffs` runs one attempt and returns `N`,
   `R`, the panel degrees of both sides and the work charged.
9. An attempt returns `OK` only after it wrote its output (a guard at its exit): a missing status assignment on
   an early exit cannot turn into `OK` with an unwritten result.
8. Driver: `tate_vector CHI` prints `PHI | F`; `tate_integral CHI with S with BITS` prints
   `VALUE | halfplane Re(s) > 1 | s=S` (design D1: the mode and `S` beside the value), with `S` a complex adele
   text whose finite coordinate is ignored, as for `local_zeta_factor_at`, and `prec` the driver setting.

## Cost

One attempt: `C` character phases (each with its character setup, char.h), the transform `O(C^2)`, the searches
(`O(log N + log R)` bound evaluations and the L6 prefixes), `O(N sum_panels (J + 1))` Taylor steps on each side
(about 6200 coefficients for zeta at bits 100, 25000 for `C = 5` at bits 80). The whole `test_tate` runs in about
3 s. Avoidable costs noted: the vector, the transforms and the cutoffs do not depend on the working precision
except through rounding and are recomputed on each retry; `C` separate `adf_char_chi` calls each set up the
character group.

# Slice 5d: the continuation and the completed function

The functions are those of `docs/api-5.md` section 4 (`:167-249`, the two comment blocks `:237-247`), with the pole
geometry of `:230-235`, the statement T4 of section 6 (`:302-312`) and the acceptance of section 7 (`:350-354`, the
independence test and the link-wrap). Header `include/adelefeld/tate.h` (two declarations appended), code
`src/tate.c` (the attempt `tt_attempt` and the body `tt_run` shared with `adf_tate_integral`, a mode), test
`tests/test_tate_continue.c` (a new binary, so that the link-wrap stays out of `test_tate`), vectors
`tests/ref/vectors/t5-slice4/continue.jsonl` (`lanes/t5-slice4/gen_vectors.py`, from `proto/tate_checks.py`), driver
fixture `tests/driver/tate-continue`, Julia `tests/julia/tate_continue.jl`. P11 to P15, L14 are cited as in "Slice
5c"; the general proof imports (identity theorem, holomorphic parameter integration) are those of
`docs/proofs/analysis.md:30-48`, with their sources pending there.

## Statuses and order of checks

`adf_tate_continue(z, chi, s, bits, prec)` and `adf_tate_completed(z, chi, s, bits, prec)`:

1. `prec > ADF_REAL_PREC_MAX`: `LIMIT`. 2. `C > ADF_TATE_TRANSFORM_C_MAX = 1024`: `LIMIT`. 3. `bits` outside
   `[0, 2^21]`: `DOMAIN`. 4. Under `INV`: `chi` canonical and `z != chi->s`; a violation aborts. 5. `s` not finite:
   `DOMAIN`.
6. For `C = 1` (S10): `s` exact and equal to 0 or 1: `DOMAIN`; otherwise a rectangle that contains 0 or 1, end
   points included: `NOT_DETERMINED`. For `C > 1` there is no pole test.
7. The attempts, as in S6 of slice 5c: `LIMIT` at the work cap; `NOT_DETERMINED` by the retry rule (a nonfinite
   attempt, or a width that stops halving). Every status other than `OK` leaves `z` untouched, also when `z` is `s`.

## Statements

S9. The split is an identity on the whole plane. Let `F(z) = integral_1^inf V_chi(t) t^(z-1) dt` and `G(z') =
    integral_1^inf V_conj(chi)(t) t^(z'-1) dt`. P13 step 3 (`analysis.md:602-612`) gives `Lambda(s) = F(z) + W_chi
    G(z') + delta [1/(s-1) - 1/s]` for `Re(s) > 1`, where the defining integral converges (step 2, termwise
    integration). P15 step 1 (`:700-704`) gives a majorant of both integrands, integrable locally uniformly in `z`;
    holomorphic parameter integration (`:40-44`) makes `F` and `G` entire (P13 step 3, `:609-612`). The right side
    is therefore meromorphic on the whole plane, with poles at most at 0 and 1 and only for `C = 1`. It agrees with
    `Lambda` on the half-plane; by the identity theorem (`:45-48`) it is the meromorphic continuation of `Lambda`,
    and it is what `Lambda(s)` means outside `Re(s) > 1` (SPEC 8: "computes the continuation"). P12 (`:505-512`,
    step 3 `:539-544`) states the same for the general adelic split with the signs of the pole terms. `I_chi = C^-z
    Lambda` with `C^-z = exp(-z log C)` entire and nonzero, so `C^-z` times the split is the continuation of
    `I_chi`. Nothing in S3 to S6 of slice 5c uses `Re(s) > 1` except the gate: S4's bounds take `r = upper(Re v) -
    1` for both exponents and L14 has a branch for every real `r` (`:630-639`); S5's T3 majorant uses `max((m-d)^r,
    (m+d)^r)`, the maximum of `t^r` on `[m-d, m+d]` for every real `r`; the budget `epsilon = 2^-bits/max(1,
    upper|C^-z|)` is also right for `Re(z) < 0`, where `|C^-z| > 1`. So the attempt of slice 5c, with the gate
    replaced by S10, encloses `I_chi(s)` for every `s` of every pole-free rectangle.
    Check: `values` (208 records: C = 1 and the 17 golden characters at `1/2`, `1/2 + 14 i`, `3/8 + 3i/4`, `-1`,
    `-2`, `-3`, `-7/2`, `2 + 3 i`, `1 - 2^-40`, `2^-40`, `1 + 2^-10 i` and the trivial zeros; both calls at bits 53,
    at 20 and 80 for C = 1, 4, 5, 16, at 4 for C = 1 (N = 1: one coefficient): 788 OK calls, each narrow and
    containing the mpmath and the FLINT value, 1576 points), `overlap` (continue = integral at `2, 3, 2 + 3i` for C
    = 1, 5, 16, both narrow at bits 53), `cutoffs` (27 records at `1/2`, `3/8 + 3i/4`, `-3/2`: N, R and every degree
    equal the oracle's).

S10. The pole geometry (design `:230-235`; P13 `:576-577`: for `C = 1` simple poles with residues -1 at 0 and +1 at
    1, entire for `C > 1`). An exact `s` equal to 0 or 1 is a proved pole: `DOMAIN` (conventions 3.2, row
    "Integrals, Poisson summation"; SPEC 9.3.7 G6). A rectangle that contains 0 or 1 contains points where
    `|Lambda|` is unbounded, so no finite enclosure exists: `NOT_DETERMINED`. The test is exact and needs no
    arithmetic: `x0` lies in the closed rectangle iff `arb_contains_si(Re s, x0)` and `arb_contains_zero(Im s)`
    (containment in the closed ball, `arb.rst:664-688`); exactness is `acb_is_exact` (`acb.rst:228-230`). An end
    point equal to `x0` counts. For `C > 1` nothing is tested: `Lambda` is entire (P13), so the poles of the local
    factor `Gamma(z)` at `z = -k`, `s = -e - 2k`, are not poles of the global function (the trivial zeros of `L`
    cancel them), and the code never forms `Gamma(z) L(s, chi)`.
    Check: `poles` (45 dyadic balls against the oracle's `pole_status` for C = 1, 5, 4: DOMAIN and NOT_DETERMINED
    exactly, z untouched; OK geometry gives OK at bits 4 when the radius is at most `2^-10`), `statuses` (exact 0,
    1; `1 +/- 2^-10`; end-point contact `[1 - 2^-9, 1]`, `[-2^-9, 0]`, `1 + [0, 2^-9] i`: NOT_DETERMINED, also at
    bits `2^21`, where an attempt would end in LIMIT: the geometry comes before the work), `overlap` (C > 1 at `s =
    0, 1` and at `s = -e, -e - 2`: OK).

S11. Near a pole (T4, `api-5.md:302-312`). The two reciprocal terms are computed by `tt_recip`: `c = s - x0` at the
    working precision contains every `x - x0`; its covering disk has the exact centre `m = mid(c)` and the radius `R
    = hypot(rad Re c, rad Im c)` (upper, `mag.rst:399-401`). If `D = lower|m|` exceeds `R` and `d = lower(D - R)` is
    positive, then for every `x` of the disk `|1/x - 1/m| = |m - x|/(|x| |m|) <= R/(D d)`, because `|x| >= |m| - R
    >= d` and `|m| >= D`; the term is `1/m` (an exact input to `acb_inv`) with `R/(D d)` added to both radii
    (rounded up, `mag.rst:12-13`). Otherwise (T4.2: the disk meets `x0`, the rectangle does not) `acb_inv(c)` gives
    an enclosure, possibly nonfinite; a nonfinite attempt fails the width. Adding the entire integrals and the exact
    residues gives the enclosure (T4.3). Near a pole the image of a fixed ball has a width of about `R/|s - x0|^2`;
    more precision does not reduce it, and the call returns `NOT_DETERMINED` when a doubling stops halving (N-D23).
    Check: `residues` (`(s - 1) Lambda(s)` at `1 + 2^-k` and `s Lambda(s)` at `2^-k`, `k = 10, 20, 40`, at bits 80:
    the product contains the 60-digit reference and lies within `2^(1-k)` of `+1` and `-1`: SPEC 8), `balls` (the
    pole-free rectangle `1 + 2^-40 +/- 2^-200` at bits 70: OK, containing both end values of mpmath and FLINT),
    `statuses` (`1 + 2^-10 i`: OK, imaginary part in `(-1050, -1000)`; `1 + 2^-30 (1 + i) +/- 2^-110`: OK; a
    rectangle that excludes 1 while its disk does not: NOT_DETERMINED by the width, not DOMAIN).

S12. The trivial zeros. At `s0 = -e - 2k` (`k >= 0`; `s0 != 0` for `C = 1`) the split S9 is evaluated as it stands:
    it contains no Gamma factor and is regular at the permitted `s0`. For `C > 1` it is entire; for `C = 1`
    its only poles are 0 and 1 (P13, analysis.md:576-577). Thus `Lambda(s0)` and `I(s0) = C^(-z0) Lambda(s0)`
    are finite by S9's local regularity. The quotient formula `Gamma(z) L(s)` would be infinity times 0 there.
    By api-5.md:124-126, `Lambda = (pi/C)^(-z) Gamma(z) L(s)`, with `z0 = -k`. Gamma's residue gives
    `Gamma(z(s)) ~ 2 (-1)^k/(k! (s-s0))`; hence the limit is `2 (-1)^k (pi/C)^k L'(s0)/k!`.
    Nonvanishing at `s0` is not used by the code; the tests check that the 40 enclosures of the vectors exclude 0.
    Check: `values` (40 trivial zeros: Lambda narrow, finite and not containing 0, and `I` overlapping `C^(-z0)`
    times Lambda; the references by the limit `2 (-1)^k (pi/C)^k L'(s0)/k!` (mpmath) and by the functional equation
    with FLINT's L at `1 - s0`, asserted equal by the generator), the driver lines for zeta at -2, `chi_4` at -1,
    `(5,4)` at 0.

S13. The completed function. `adf_tate_completed` runs the same attempt with the budget `epsilon = 2^-bits` for the
    cutoffs and panels (design `:205`) and without the factor `C^-z`; the width `2^-bits` is checked on `Lambda`
    itself. Multiplying a width-limited `I` by `C^z` (`|C^z| = C^(Re z)`, up to 16 at `2 + 3 i` for C = 16) would
    widen the result past the target without a recheck (design `:245-246`).
    Check: `overlap` (Lambda narrow at bits 53 and overlapping `C^z I`), `widths` (s = 1/2, -3/2 for C = 1, 5, 16 at
    bits 20, 53, 80: each narrow, the largest radius decreasing; the same targets from prec 2), `values`.

S14. Independence and the forbidden path. The two Mellin sides are computed by separate calls of the T3 kernel with
    separate exponents and coefficients. The hidden hook `adf_tate_trace` returns both sides (with the total error)
    and can replace one side by a disjoint sentinel before the assembly.
    Check: `sides` (6 records: each side contains its own 60-digit reference, the oracle's `numeric_piece` with `R =
    infinity`, independent of the other side; with the sentinel in one side the other side is bit for bit unchanged
    and the assembled value moves by the replacement), `forbidden` (the wrapped FLINT functions abort when called:
    the interposition is active).

## Decisions where the design is silent (slice 5d)

10. One body for the three calls: `tt_run(z, chi, s, bits, prec, mode)` with the modes half-plane, continuation of
    `I`, completed; `tt_attempt` takes a flag for `Lambda` (budget and no factor `C^-z`). `adf_tate_integral` keeps
    its gate, statuses and tests (22904 checks unchanged).
11. T4 is used for both reciprocal terms in every mode, also by `adf_tate_integral`; its values change only in the
    rounding of the pole terms near 1 (an enclosure either way).
12. The pole test comes after `s` finite and before any work (so that a ball meeting a pole is `NOT_DETERMINED` even
    at bits `2^21`). Exactness of a pole is `acb_is_exact(s)`: a ball of positive radius around 1, however small, is
    `NOT_DETERMINED`.
13. A rectangle whose pole geometry is OK but whose image is wider than `2^-bits` (near a pole, or wide) is
    `NOT_DETERMINED` by the retry rule, not by the geometry; the status alone does not tell the two apart.
14. The link-wrap of design `:353`: the Makefile has no per-test linker flags, so `tests/test_tate_continue.c`
    defines the forbidden symbols itself (each aborts); the library is linked statically into the test executable,
    so every reference from it binds to these definitions. Built with `-DADF_TATE_LINK_WRAP` and
    `-Wl,--wrap=<name>`, the same bodies are the `__wrap_` definitions. Wrapped: `acb_dirichlet_zeta`, `_zeta_rs`,
    `_zeta_jet`, `_zeta_jet_rs`, `_hurwitz`, `_hurwitz_precomp_eval`, `_eta`, `_xi`, `_ui_theta_arb`,
    `_chi_theta_arb`, `_root_number_theta`, `_l_euler_product`, `_l_hurwitz`, `_l`, `_l_vec_hurwitz`, `_l_jet`,
    `_l_fmpq`, `_l_fmpq_afe`, and `acb_zeta`, `acb_hurwitz_zeta`, `arb_zeta`, `arb_zeta_ui`, `arb_hurwitz_zeta`. The
    FLINT comparison values come from the vector file, not from a call in this binary.
15. The test seam is a hidden function (`adf_tate_trace`, as `src/roots.c:49-53`): no global state, no exported
    symbol; production calls pass no trace.
16. Driver: `tate_continue CHI with S with BITS` and `tate_completed CHI with S with BITS`, the grammar of
    `tate_integral`, printing `VALUE | continuation | s=S` and `VALUE | completed | s=S`.

## Cost (slice 5d)

As the half-plane call (design `:240`): one attempt is `C` phases, the `O(C^2)` transform, the searches and `O(N
sum_panels (J + 1))` Taylor steps on each side. Left of the critical line `Re(z')` grows and the second branch of
L14 takes over, but at the points tested the cutoffs stay those of the right half-plane: at bits 80, `N = 4, R = 32`
(zeta, about 4800 coefficients), `N = 8, R = 128` (`chi_4`, 7500), `N = 16, R = 128` (C = 5, 25300) at `s = 1/2`,
`3/8 + 3i/4` and `-3/2`. On high vertical lines the majorant factor `exp(pi |Im z|/2)` raises the panel degrees;
only absolute accuracy is promised (design `:228`). `test_tate_continue` runs in about 12 s.

# Slice 5e: the functional equation, computed by the caller

The computation is that of `docs/api-5.md` section 5 (`:250-260`), with the acceptance and faults of section 7 for
5.4 (`:346-352`) and the user call of section 8 item 5 (`:383-385`). There is no new library function and no new
C type: the equation is computed by the caller from `adf_tate_completed` (tate.h, "Slice 5d"), `adf_char_conj` and
`adf_char_root_number` (char.h:91-102). The caller is the driver command `tate_fe` (`tools/adf/adf.c`,
`adf_drv_tate_fe`) and, for the test, the function `fe_sides` of `tests/test_tate_fe.c`, which performs the same
steps. Vectors `tests/ref/vectors/t5-slice5/fe.jsonl` (`lanes/t5-slice5/gen_vectors.py`), driver fixture
`tests/driver/tate-fe`, Julia `tests/julia/tate_fe.jl`. P13 is `docs/proofs/analysis.md:564-626`.

## Statements

S15. What is computed. For a character `chi` (conductor `C <= 1024`, parity `e`), a ball `S` and `bits`:
    `A = adf_tate_completed(chi, S, bits)`; `B = adf_tate_completed(conj(chi), T, bits + 4)` with `T` the ball
    `1 - S` formed at `prec`; `W = adf_char_root_number(chi)` at `wp = max(prec, bits + m + 16)`, where `m >= 0` and
    `|B| < 2^m` (`acb_get_mag`, `acb.rst:290-292`; `arf_abs_bound_lt_2exp_si`, `arf.rst:393-398`); `R = W B` at
    `wp`. P13 (`analysis.md:574`) states `Lambda(s, chi) = W_chi Lambda(1 - s, conj(chi))` with `W_chi =
    tau(chi)/(i^e sqrt(C))` and the positive finite Gauss sum (`:569`, `:578`). The proof is step 4
    (`:613-614`): `W_chi W_conj(chi) = 1` (Lemma 8), and exchanging `z` and `z'` in the split of step 3
    (`:602-612`) gives the equation on the whole plane by continuation. `adf_char_root_number` computes exactly this
    `W_chi` (char.h:91-94; conventions 6.4). `adf_char_conj` gives `conj(chi)`, with the same conductor and parity
    (char.h:96-102). For every `x` in `S`: `A` contains `Lambda(x, chi)` (tate.h: the call encloses `Lambda` on its
    whole input ball). `T` contains `1 - x` (ball subtraction encloses, `arb.rst:6-12`), so `B` contains
    `Lambda(1 - x, conj(chi))`. `W` contains `W_chi`, and `acb_mul` encloses every product of points
    (`acb.rst:6-17`). Hence `R` contains `W_chi Lambda(1 - x, conj(chi)) = Lambda(x, chi)`. So when both sides are
    OK, `A` and `R` contain the same number and must overlap. A nonoverlap proves that an enclosure, a sign, the
    completion, the conjugation or a transform contract is wrong (design `:257-258`).
    Check: `values` (below), `tests/driver/tate-fe.cmd`, `tests/julia/tate_fe.jl`.

S16. The widths. `A` has each coordinate diameter at most `2^-bits`; this is the contract of `adf_tate_completed`,
    and nothing is applied to `A` after the call. `B` has diameters at most `2^-(bits+4)`. `W` has coordinate radii
    at most `32 C 2^-w` with `w = min(2^21, wp + ceil(log2 C) + 8)` (char.h:91-93, `docs/api-3d.md` "Gauss sum and
    root number"), so at most `2^-(wp+3)` below the cap. `R`'s diameters are checked against `2^-bits` after the
    product; this is the final width check of the design (`:254-255`). If the check fails, the status is
    `NOT_DETERMINED`, named `R`. The guard targets are chosen so that the check is expected to pass with a wide
    margin, by this reckoning. A coordinate of `W B` is a sum of two products. Its radius is about
    `(|Re W| + |Im W|) rad(B) + (|Re B| + |Im B|) rad(W)`, plus the rounding at `wp`. With `|W| = 1` (P13), the
    first term is at most `sqrt(2) 2^-(bits+5)` per coordinate. The second is at most `2^(m+1) 2^-(bits+m+19) =
    2^-(bits+18)`, and the rounding is about `2^-(bits+14)`. So the diameter of `R` is about `0.09 2^-bits`. [source
    pending: the radius propagated by `arb_mul` is not stated in `refs/src/flint-3.0.1/arb.rst`.] The guarantee is
    the run-time check, not this reckoning. The test asserts the stronger `2^-(bits+2)` on `R` as evidence that the
    guards suffice. Observed over the 280 cases of `values`: the largest diameter is `0.094 2^-bits` for `A` and
    `0.0061 2^-bits` for `R` (`B` is far narrower than its target). The oracle also uses `bits + 4`, on both sides
    (`proto/tate_checks.py:418-424`). Here `A` takes `bits` itself, because no operation follows it.
    Check: `values` (`A` at `2^-bits`, `B` at `2^-(bits+4)`, `R` at `2^-(bits+2)`), `widths` (bits 20, 53, 80 at
    `1/4 + i` from prec 2, for C = 1, 5, 16: both sides narrow, overlapping, and the largest radius of each
    decreasing), `guard` (below).

S17. Why this is the acceptance of SPEC 8. SPEC 8 (`docs/SPEC.md:494-501`) states the equation with this `W_chi`.
    It asks for a comparison with FLINT's values completed by hand, a width that shrinks with precision, both
    parities, a non-real character, and points at and near the poles. `A` and `R` come from two separate
    quadratures, at different points and characters and at different targets; `R` is not formed from `A`. Two
    enclosures that overlap can still be two equal wrong values (design `:256-257`). So each side must also contain
    two references that share no code with the library and none with each other: FLINT's L completed by hand
    (`acb_dirichlet.rst:98-100`, `:524-534`, `:567-569`, through python-flint) and mpmath's Hurwitz sum. If `A` and
    `R` both contain the same 60-digit value, each is right within its width. An honest `LIMIT` or
    `NOT_DETERMINED` is a failed acceptance case, not a refutation of the equation (design `:258-259`).
    Check: `values` (136 records: the 17 characters of `tests/golden/gauss.tsv`, which include C = 1, at `3/8 +
    3i/4`, `1/2`, `1/2 + 14 i`, `-1`, `2 + 3 i`, `1/4 + i`, `-e - 2` and `3 + e`; bits 53 for all, and 20 and 80 for
    C = 1, 4, 5, 15, 16; 280 OK cases; each side contains both references of `Lambda(s, chi)`, `B` contains both
    references of `Lambda(1 - s, conj(chi))`, and `W` overlaps the golden `W`: 1680 containments). `balls` (the
    radius `2^-60` at `3/8 + 3i/4` for the 17 characters, at bits 40: both sides contain all four corner
    references; at bits 80, `NOT_DETERMINED` named `A`, N-D23). `goldens` (the 17 root numbers overlap the golden
    `W`). `hand` (zeta at -1 and at 2: both sides contain `pi/6`, design `:338`).

S18. Statuses. The steps run in the order `A`, `B`, `W`, `R`, and the first failure stops. The driver prints the
    step's letter and the status, as `A: error: DOMAIN`. In the test, `fe_sides` returns the status and the letter.
    `bits + 4` is formed only after `A` is OK, so `bits <= 2^21` there and it cannot overflow. For `bits` in
    `(2^21 - 4, 2^21]`, `B` would be `DOMAIN`; `A` ends in `LIMIT` at the work cap before that. For conductor 1,
    at `s = 0` and `s = 1`, `A` is `DOMAIN`; `B`, at `1 - s = 1` and `0`, is `DOMAIN` too. Both are statuses, not
    values (design `:259`). A ball that meets a pole gives `NOT_DETERMINED` on both sides, because the poles of
    conductor 1 are symmetric under `s -> 1 - s`. A conductor above 1 has no pole: `chi_4` at `s = 0` and `1` is OK
    on both sides, and both sides contain `Lambda(1, chi_4) = Lambda(0, chi_4) = 1` (design `:338`). `bits` outside
    `[0, 2^21]` gives `DOMAIN` from `A`. A precision above the cap, or `C > 1024`, gives `LIMIT` from `A`. `W` fails
    only with `LIMIT` (`wp` above the cap) or with `NOT_DETERMINED` from its own certificate. The guard target makes
    one case possible in which the equation fails although `A` alone succeeds: a ball whose image width lies between
    `2^-(bits+4)` and `2^-bits`.
    Check: `poles`, `statuses`, `guard` (zeta on `3/8 +/- 2^-k`, bits 20: 4 radii give `A` OK and `B`
    `NOT_DETERMINED`), each failure leaving the four outputs byte for byte untouched, and the driver lines.

S19. The trivial zeros. At `s0 = -e - 2k` (`s0 != 0` for C = 1), `A` is finite and excludes 0 (S12). `R` comes
    from `B` at `1 - s0 = 1 + e + 2k`, a regular point. The converse also holds: when `1 - s` is a trivial zero of
    `L(., conj(chi))` (the point `3 + e`), `B` is the finite value of S12. The references at a trivial zero do not
    use the functional equation: `Lambda = (C/pi)^z L(s)/rgamma(z)`, and both `L(s)` and `rgamma(z(s))` have a
    simple zero at `s0`, so the value is `L'(s0)/(rgamma'(z0)/2)`. FLINT gives it through `acb_series.dirichlet_l`
    and `acb_series.rgamma`; mpmath through the differentiated Hurwitz formula and `mp.diff`.
    Check: `values` (46 records with a trivial zero on one side; where it is on `A`'s side, `A` and `R` exclude 0).

S20. Independence. The sides share no evaluator: `A` is one call, and `R` is formed from a second call and from the
    root number. If `A`'s evaluator is replaced by the disjoint sentinel `10^6`, then `R`, `B` and `W` stay bit for
    bit those of the real run. If `B`'s evaluator is replaced by `-10^6 i`, then `A` stays bit for bit the real one,
    and `R` moves to `W` times the sentinel (design `:350-352`). A side copied from the other passes every value
    check (it is the right number). Only this test and the check `R != A` at zeta `1/2` detect it.
    Check: `independence` (C = 1, 4, 5, 13 at `3/8 + 3i/4`) and `hand` check the caller's `fe_sides`.
    `tests/driver/tate-fe.cmd` and `.out` check the production handler: at zeta `1/2`, A's imaginary radius
    prints as `2.1e-26` and R's as `1.5e-27`; replacing the product by a copy of A fails the driver fixture.

## Decisions where the design is silent (slice 5e)

17. `A` takes `bits`, `B` takes `bits + 4`, and `W` and the product take `wp = max(prec, bits + m + 16)` (S16).
    The width of `R` is checked against `2^-bits`. The oracle takes `bits + 4` for `A` as well; nothing in this
    computation follows `A`, so the target of the call is the final width.
18. The output line is `A | R | overlap=yes|no | s=S`, with `A` and `R` printed as `tate_completed` prints a
    value. Overlap is `acb_overlaps` (`acb.rst:267-269`), which is decided exactly on the two balls, so there is no
    "undecided". The widths show as the printed radii. Where a coordinate is not the exact 0, the printer's
    rounding at the `digits` setting hides radii below about `10^-digits`.
19. A failed step prints its letter before the caller's `error: STATUS`, the form of `tate_gamma`. The first
    failure stops the command; `B` and `W` are not computed after `A` fails. An error in the operands (syntax, kind,
    a non-integer `BITS`) is the plain `error: STATUS`, because no call was made.
20. `1 - s` is formed with `acb_add_ui` at `prec`. This is an enclosure, and it is exact for the dyadic inputs of
    the tests.
21. The raw golden pairs `(8,7)` and `(10,3)` are run as given. The constructor lowers them to `(4,3)` and `(5,3)`
    (the test checks the conductor, the label and the parity of every record), so no character of conductor 8 or
    10 is integrated. Their rows repeat the values of the lowered characters, and the driver line of `(8,7)` is
    that of `(4,3)`.
22. The FLINT and mpmath references at a trivial zero are quotients of derivatives, not the functional equation
    (S19), so the test does not assume what it checks.
23. `fe_sides` repeats the driver's steps; it is not code shared with the driver, which is a separate program.
    The six faults are planted in both (`lanes/t5-slice5/faults.py`).
24. The grammar is that of `tate_completed`, `tate_fe CHI with S with BITS`, with `prec` the driver setting (as
    decision 16), not the `--bits 80 --prec 160` flags written in design `:384`.

## Faults (design section 7, 5.4)

Each fault was planted in a scratch copy of `fe_sides` and in a scratch copy of `adf_drv_tate_fe`. 6 of 6 are
detected by the test (exit 1) and by the driver fixture (output differs); see `lanes/t5-slice5/faults.out`. The
section `values` alone (the acceptance) detects the first five, at the overlap of `A` and `R`. The sixth (one side
copied from the other) passes `values` and is detected by `hand` (`R != A` at zeta `1/2`) and by `independence`.

## Cost (slice 5e)

Two calls of `adf_tate_completed` (S13), one root number (`O(C)` phases), and one product. The `B` call at
`bits + 4` costs about as much as the `A` call: the cutoffs and the panel degrees grow by a few terms for the
extra 4 bits. `test_tate_fe` runs in about 6 s.
