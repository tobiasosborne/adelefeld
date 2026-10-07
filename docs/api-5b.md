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
    value is the defining adelic integral (P11); the split itself is entire (P13) and is the 5d path.

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
