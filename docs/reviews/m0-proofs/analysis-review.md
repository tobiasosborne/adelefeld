# Review of `docs/proofs/analysis.md` (work package 0.3c)

Reviewer: Claude fable subagent (adversarial review, milestone 0). Saved to disk by the orchestrator from
the reviewer's final message, verbatim. Date: 2026-09-27.
Object: `docs/proofs/analysis.md` (15 statements) and `proto/analysis_checks.py`.
Own checks: `docs/reviews/m0-proofs/analysis_review_checks.py`, written without use of the author's functions.
The proof file was not edited.

Result: 9 VALID, 6 MINOR, 0 INVALID. No formula, sign or constant was refuted. Every inequality held in
every case tried, including one run with certified ball arithmetic. The defects are of three kinds:
bounds that are true but have rules without margin or are very loose; two proof steps that are asserted
rather than shown; references to a "Lemma 15" that does not exist. No finding against the specification.

What "not refuted" means here. Sections A to G, I, J, K of the checks are floating point computations with
mpmath at 25 to 50 digits. They are evidence, not proof. Section H is different: it uses arb balls at 1100
bits, and a comparison there counts only if it holds for every point of the balls. The proofs themselves
were read step by step.

## Summary

| No. | Statement | Verdict | One line |
|---|---|---|---|
| 1 | Conventions, measures, imports | VALID | Signs agree with SPEC 6 and 8; psi is 1 on 4000 rationals. |
| 2 | Character on balls, quotient | VALID | Images of 300 fractional balls are as stated. |
| 3 | Annihilators, self-dual measures | VALID | No gap found; inversion checked through 4 and 5. |
| 4 | Weighted finite transform | VALID | Weights 1/M, 1/D and the sign confirmed with D != M. |
| 5 | Polynomial-Gaussians, transform | VALID | Confirmed against the integral for complex A, B, degree 6. |
| 6 | Gaussian series bound | MINOR | True. Rule "rho < 1" has no margin; Kappa loses exp(beta^2/(4 alpha)). |
| 7 | Poisson summation, theta | VALID | Both sides, both truncation bounds, theta at three ideles. |
| 8 | Gauss sums | MINOR | True for 106 characters. Primitivity is used but never defined. |
| 9 | Local Tate integrals, gamma | VALID | 584 cases, conductors 9, 27, 16, 25 included. |
| 10 | Real Tate integral, gamma | VALID | Non-even shifted function, s left of the strip and near poles. |
| 11 | Global test vectors | VALID | Local constants multiply to i^(-e) tau(chi) C^(-s). |
| 12 | Splitting at norm 1 | MINOR | True with f(0) != F f(0). Convergence in step 1 only asserted. |
| 13 | Theta identity, functional equation | MINOR | Certified with FLINT. Cites a "Lemma 15" that does not exist. |
| 14 | Exponential integral bound | MINOR | True. Free choice of R0 can lose 1e7; finite piece can lose 1e17. |
| 15 | Truncation and quadrature | MINOR | True, certified. Needs R twice and N 1.4 times too large. |

## Details

### 1. Definition 1: VALID

Compared with SPEC 6 and 8: `psi_inf(x) = E(-x)`, `psi_p(x) = E(fp_p(x))`, transform against `conj(psi)`.
Hence the real kernel is `exp(+2 pi i x y)` and the finite kernel is `exp(-2 pi i {x y})`, as SPEC 6 says.
`d*x_p = (1 - 1/p)^(-1) dx_p / |x_p|_p` gives the units volume 1, as SPEC 8 says.
Tried: my own fractional part from the definition, 4000 random rationals with denominators up to 5000 and
numerators of both signs (check A1). The sum of the local fractional parts equals `q` modulo 1 every time.
The list of imported theorems is honest: each is marked source pending. I found no use of an import that is
not on the list. The attribution to Tate is marked pending by the author; that is correct.

### 2. Lemma 2: VALID

Read steps 1 to 5. Step 3 is correct: `N Zhat / A Zhat` is `Zhat / B Zhat`, the classes are `k N`, and
`psi_f(k N) = E(k A / B)` runs over all B-th roots because `gcd(A, B) = 1`. The gluing `(1, z) ~ (0, z - 1)`
is subtraction of the diagonal integer 1. It agrees with the example of SPEC 6.
Tried: 300 balls with random rational centre (both signs) and random rational radius, points
`a + N z` for integers `z` (check A2). The set of phases is the stated set each time.

### 3. Proposition 3: VALID

Read all seven steps. Step 3 is complete: compatibility `b_(m+1) = b_m mod p^m` follows from raising the
value at `p^(-m-1)` to the power `p`. Step 4 needs `y_p` in `Z_p` for almost all `p`, so that `y` is an
adele; the step says so. Step 6 for Schwartz functions is a sketch but each claim in it is correct; the
implemented class does not depend on it, because Proposition 5 step 5 proves inversion there directly.
Tried: double transform checked in A6 (finite) and B3 (real, complex A).

### 4. Proposition 4: VALID

The statement is SPEC 7 word for word, including the weight `1/D` of the second transform.
Tried (checks A3 to A6): D, M in (4,6), (6,4), (1,7), (9,2), (5,5), (1,1), (8,3), random complex arrays
without symmetry. The transform was computed from the integral, by refining the cosets until
`x -> psi_f(x y)` is constant, with `psi_f` from my own local fractional parts. Points `y = k/M + D w` with
`w` negative and positive test the period `D`; points `y` with denominators `2M`, `7M` test the support.
Max absolute difference 9.8e-50. The wrong weight `1/D` and the wrong sign give different numbers.

### 5. Proposition 5: VALID

Verified by hand: translation and dilation parameters; `z^2/(4 pi A)` expands to the stated new parameters
`1/A`, `i B/A`, `C + B^2/(4 pi A)`; the branch claim in step 5 holds because `arg(1/A) = -arg(A)` and both
lie in `(-pi/2, pi/2)`; `B'' = i (i B/A)/(1/A) = -B` and `C'' = C`.
Tried (checks B1 to B5): six functions, among them degree 6 with complex coefficients, `A = 0.05 + 0.3 i`,
`A = 0.02 - 0.1 i`, `A = 2 - 3 i`, `B = -6 + 4 i`, `B = 0.5 + 9 i`. The defining integral with kernel
`E(+x y)` was computed by trapezoid sums. I wrote the transform from the moments of a complex Gaussian,
not from the recurrence, and then compared the recurrence of the text with it. Agreement 1.2e-50 relative
to the integral of `|f|`; the smallest transform value tested is 7e-43 of that integral, so about 7 digits
of that one value are tested, and 40 or more of the others. `F(x exp(-pi x^2)) = + i y exp(-pi y^2)`
confirmed.

### 6. Lemma 6: MINOR

The inequality is true. Tried: `S_j(c, T)` for `j = 0..12`, `c` from 0.001 to 10, `T` from 0 to 30 (468
cases, check C1), and the lattice bound for 400 random cases with `Re(A)` down to 0.01, `|Re B|` up to 40,
degree up to 9, shifts, negative and small `h`, `T` from 0 to 40 (check C4). No violation.
The factor 2 for the two sides is needed: for `exp(-0.2 pi x^2)` the quotient bound/true is 1.936 (C7).

Three defects.

(a) The statement bounds `|sum phi(a + h n)|`. Propositions 7, 12 and 15 need the bound for
`sum |phi(a + h n)|`. The proof gives this; the statement does not.

(b) The rule "increment K to the first K0 with `rho < 1`" has no margin. If `rho` is just below 1 the factor
`1/(1 - rho)` is arbitrarily large. Found (check C2):

| j | c | T | bound/true with `rho < 1` | with `rho <= 1/2` |
|---|---|---|---|---|
| 3 | 1.0000001 | 0 | 2.4e6 | 1.002 |
| 6 | 2.000000001 | 0 | 2.9e8 | 1.000 |
| 1 | 0.33333334 | 0 | 2.5e7 | 1.048 |

On the grid of C1 the quotient reaches 1617 with the rule of the text and 1.58 with `rho <= 1/2`.
In ball arithmetic the rule of the text is also not decidable when the ball for `rho` contains 1.

(c) The step `beta |n| <= alpha n^2 / 2 + beta^2 / (2 alpha)` is valid but costs twice. It halves the
decay rate, and it replaces the true maximum `exp(beta^2/(4 alpha))` of `exp(-alpha n^2 + beta |n|)` by
`exp(beta^2/(2 alpha))`. For `A = 0.01 - 1.96 i`, `B = -40 + 0.15 i`, `h = -0.3`, `T = 0` the bound is
2.5e5560 times the true sum (check C4). For `exp(-pi x^2)` the cutoff computed from the bound is `T = 6`
for a target 1e-30 where `T = 4` suffices, and 12 against 8 for 1e-100 (check C5). For a shifted Gaussian
with its peak at `n0 = beta/(2 alpha)` the bound becomes small only for `T` beyond about `2 n0`.
The repair R1 applies the ratio test to `n^j exp(-alpha n^2 + beta n)` directly. On the same 400 cases its
quotient bound/true lies between 1.28 and 6.2e6 (check C6); the large values come from cancellation inside
`P(a + h n)` at degree 9, which no bound by coefficients can see.

### 7. Proposition 7: VALID

Read steps 1 to 4. The sign bookkeeping in step 2 is right: the coefficient of `E(-n t/M)` is
`phi_hat(n/M)/M` with the positive real kernel, and at `t = j/D` the sum over `j` is `g_n` with the negative
finite kernel. The truncation bounds use Lemma 6 in the form for absolute values (see 6 (a)).
To apply `B_phi` to `phi_hat`, the prefactor `A^(-1/2)` and the polynomial in `z = B + 2 pi i y` must first
be written as a polynomial in `y`; the statement leaves this to the reader.
Tried (checks D1 to D4): (D, M) = (2,3), (3,2), (4,6), (1,5), (5,1); `A = 0.6 + 0.5 i`, `B = 1.4 - 0.7 i`,
degree 2, complex arrays. Both sides agree to 2.5e-50. Truncation at `T = 0, 1, 2.5, 5`: the actual error
never exceeds the bound; smallest quotient bound/error 10.4 (left) and 10.5 (right). Theta identity at
three ideles with `r = 3/2, 1/9, 8`, units 5, 1, 11 and `x_inf = -0.7, 1.9, 0.31`: agreement 8.9e-46.

### 8. Lemma 8: MINOR

All identities are true. Tried (check E1): every primitive character of the conductors 1, 3, 4, 5, 7, 8, 9,
12, 15, 16, 20, 24, 25, 27, 32, 45, 63, 72 (106 characters, built by me; primitivity tested from the
definition), every `m` from `-C` to `2C`, 11191 values. Max absolute difference 3.6e-48. Check E2 shows
that the hypothesis is needed: for an imprimitive character modulo 9 and `m = 3` the sum has absolute value
5.196 while `conj(chi(3)) tau(chi) = 0`.

Defect: the file never defines "primitive". Step 2 says "Primitivity means chi is nontrivial on its
kernel", which is not a meaningful sentence. What step 2 uses is: for every proper divisor `C'` of `C`
there is a unit `u = 1 mod C'` with `chi(u) != 1`. With that definition the step is correct. The sentence
about surjectivity of the reduction of units is not used and can be deleted. Replacement text R2.

### 9. Proposition 9: VALID

Read steps 1 to 6. Verified by hand: `F f(p^(-a) v) = p^(-a) eta_0(v) G_minus`; the integral over the shell
`v_p(y) = -a` gives `alpha^a p^(-a s) G_minus`; the law `T(D_b f) = eta(b)^(-1) |b|^(-s) T(f)`; in step 5
the multiplier for `b = p` has absolute value `p^Re(s) / |alpha|`, larger than 1 exactly in the stated half
plane. The uniqueness argument of steps 4 and 5 is complete for locally constant functions.
Tried (check F1): my own shell-by-shell integral, arrays `f(j / p^d)` constant modulo `p^m` with `d != m`,
random complex values; (p, a) = (2,0), (3,0), (5,0), (2,2), (2,3), (2,4), (3,1), (3,2), (3,3), (5,1),
(5,2), (7,1); up to three primitive characters each; `alpha = 1.1 E(1/9)` and `0.6 E(-3/10)`; `s` inside
the strip, and at `-1.3 + 0.5 i` and `2.6 + 1.5 i` outside it. 584 cases, max relative difference 6.3e-50.

### 10. Proposition 10: VALID

Verified by hand: the kernel in step 2 and its limits `K_0`, `K_1`; the quotient of the two values on
`phi_e`; the residues. Check G1 confirms that `K_e(s)` equals the stated Gamma quotient at four points,
one at distance 0.001 from the pole `s = 1`.
Tried (check G2): `f = (1 + (0.4 - 0.2 i) x + (0.1 + 0.3 i) x^2) exp(-pi (0.9 + 0.4 i) x^2 + (0.8 - 0.5 i) x)`,
neither even nor odd, both parities, `s = 0.3 + 2 i`, `0.8 - 5 i` (strip), `-2.5 + i` (left),
`3.7 + 0.5 i` (right), `-0.97 + 0.02 i` and `0.04 - 0.03 i` (near poles). The continuation was computed as
in step 4, with the Taylor series on `(0,1)` integrated term by term. Max relative difference 8.3e-29.
Small remarks, no repair needed: step 2 writes `Z_inf(F f, eta, 1-s)` where the statement has `eta^(-1)`
(the same character here). The poles of the odd factor at `s = -1, -3, ...` are not listed; SPEC 9.3.7 asks
only for the trivial character.

### 11. Proposition 11: VALID

Verified by hand: invariance of `t` and `u'` under `Q^x`; `u_l = 1/p` at `l != p` for the idele with `p` at
`p`; hence `omega = chi(p)` for `p` not dividing `C` and `alpha_p = product over l != p of chi_l(p)`.
The decisive test of `alpha_p` and of the conjugation is global consistency: the product of `i^e` and of
the ramified `gamma_p(s)` of Proposition 9, with `alpha_p` from this proposition and `eta_0^(-1) = chi_p`,
must equal `i^(-e) tau(chi) C^(-s)`, the constant of Proposition 13 step 5. Check F2 confirms this for
conductors 5, 8, 9, 15, 20, 24, 45, 63, 72, odd and even, real and non-real, 36 cases, 1.4e-50.

### 12. Proposition 12: MINOR

The formula, the signs of the pole terms and the functional equation are true.
Tried (checks I0 to I4): `f` with real part of degree 2, `A = 0.8 + 0.3 i`, `B = 0.6 - 0.4 i`, and random
complex finite arrays; the trivial character with (D, M) = (2, 3); an odd non-real character of conductor 5
with (2, 5); a character of conductor 9 with (3, 3). Reference: the defining integral over the ideles for
`Re(s) > 1`, written directly from the measure (`x_f = r u`, every shell of mass 1) as a Dirichlet series
times the two real Mellin integrals. This does not use unfolding, Poisson summation or theta functions.
`J_(f,chi)(s)` agrees with it at `s = 1.3 + 2 i` and `4 - i`; and `J_(f,chi)(1 - s)`, a point left of the
strip, agrees with the defining integral of `(F f, conj chi)` at `s`. 12 cases, max relative difference
1.6e-21 at 30 digits. For conductor 1, `a = f(0) = 0.738 - 0.677 i` and `b = F f(0) = 0.591 - 0.334 i`
differ, and the residues are `+b` at 1 and `-a` at 0. The bound of step 4 holds (smallest quotient 9.5).

Defects.
(a) Step 4 cites "Lemma 15 below". There is no Lemma 15; meant are Lemma 14 and Proposition 15.
(b) Step 1 asserts absolute convergence for `Re(s) > 1` "as in Proposition 11". Proposition 11 treats
product vectors; a general `f` needs an estimate for small `t`, and unfolding needs it before the sum and
the integral are exchanged. The estimate is elementary; replacement text R3.

Remark, not a defect: if the conductor does not divide a modulus at which `f_fin` is invariant under units,
the unit average vanishes and `Z(f, omega_chi, s)` is identically 0. My first attempt used (D, M) = (2, 3)
with conductor 5 and met exactly this. An implementation should expect it.

### 13. Proposition 13: MINOR

The theta identity, the root number `W_chi = tau(chi) / (i^e sqrt(C))` and the functional equation are
true. Verified by hand: the constant `(1/C) i^e (-1)^e tau C^(e+1/2) C^(-e) = tau / (i^e sqrt(C))`; the
exponent `e + 1/2 - z - 1 = z' - 1`; the pole terms; step 5.
Tried (checks H1, H2, certified): the split formula of step 3, each term integrated exactly with FLINT's
incomplete Gamma function in ball arithmetic, against `(C/pi)^((s+e)/2) Gamma((s+e)/2) L(s, chi)` with
FLINT's `L`. Conductors 1, 5, 7, 8, 9, 15, 45, 101; up to four characters each; `s = -6.5 + 0.3 i`,
`-0.4 + 1.2 i`, `0.5 + 30 i`, `0.5 + 14.13 i`, `1e-9 - 1e-9 i`, `1 + 1e-9 + 1e-9 i`, `2.5`, `25 + 3 i`,
`0.5 + 80 i`. The functional equation holds with relative tolerance 1e-50 in every case, with certainty.
Note on independence: the project's acceptance test also compares with FLINT, so an error common to FLINT
and to the split formula would not be seen. Check I2 (Dirichlet series) and the author's Hurwitz reference
are two further references that do not use FLINT's L-functions.

Defect: step 3 says "Lemma 15 proves both integrals entire". There is no Lemma 15.

### 14. Lemma 14: MINOR

The inequality is true: 112 cases, `r` from -5 to 40, `b` from 0.01 to 15, `R` from 1 to 100 (check J1).
Defects, all about looseness that the statement permits.
(a) "Choosing R0 >= R with `b > rplus/R0`" leaves `R0` free. For `r = 3`, `b = 0.5`, `R = 1` the choice
`R0 = 6.0000001` is admissible and gives a bound 1.3e7 times the integral (J2).
(b) The direct bound has the same problem when `b` is just above `rplus/R`: `r = 3`, `b = 3.0000001`,
`R = 1` gives 1.0e7 times the integral (J3).
(c) The finite piece uses `max(R^r, R0^r) exp(-b R)`, the product of two separate maxima. With `r = 40`,
`b = 0.01`, `R = 1` the bound is 5.9e17 times the integral. With the true maximum of `t^r exp(-b t)` and
`R0 = max(R, 2 rplus/b)` the quotient is at most 401 on the same grid.
Where it matters: `r = Re(z) - 1` is large for `s` far left or right, and `b = pi/(2C)` is small for large
conductor. The loss is a constant factor in an error bound: it costs more terms, not correctness.

### 15. Proposition 15: MINOR

All bounds are true.
Tried, certified (check H1): for the 765 combinations of character, `s` and cutoffs
`(N, R) = (1,1), (2,3), (5,10), (12,40), (40,400)` the distance between the truncated split formula and the
FLINT value is at most `E_sum + E_integral`, with certainty, 0 violations. The smallest quotient
bound/actual error is 2.43. Midpoint rule (check K1): K = 1, 2, 7, 64, 512 subintervals, `|Im z|` up to
60, conductor up to 45: 30 cases, smallest quotient 3.7. General adelic bound (check I4): smallest
quotient 1289.

Defects.
(a) Looseness. `n^2 t >= (n^2 + t)/2` halves both decay rates. For `n, t >= 1` the sharper inequality
`n^2 t >= n^2 + t - 1` holds, because `(n^2 - 1)(t - 1) >= 0`. It keeps both rates and costs the factor
`exp(a0)`. Certified in check H4. Cutoffs needed for conductor 45, `z = 0.25 + 7 i` (check H3):

| target | text: N | text: R | sharper: N | sharper: R |
|---|---|---|---|---|
| 1e-30 | 46 | 2021 | 32 | 996 |
| 1e-100 | 82 | 6611 | 58 | 3291 |
| 1e-300 | 141 | 19781 | 99 | 9876 |

(b) The midpoint rule has order `h^2`. For an error 1e-30 on `[1, 40]` it needs about 1e15 nodes. Every
term `exp(-a0 n^2 t) t^(z-1)` has the exact integral
`(a0 n^2)^(-z) (Gamma(z, a0 n^2) - Gamma(z, a0 n^2 R))`, which FLINT encloses; check H1 uses this and needs
no quadrature. With `R = infinity` the term `E_integral` is not needed either.
(c) "Terms n divisible by the conductor may be omitted" is true but too weak: the vanishing terms are those
with `gcd(n, C) > 1`.
(d) The bounds are absolute and do not depend on `Im(s)`, while `|Lambda(s, chi)|` decays like
`exp(-pi |Im s| / 4)`. At `s = 0.5 + 80 i`, conductor 101, the bound at `(40, 400)` is 2.6e26 times
`|Lambda|`. Inherent in the method; the statement should say that relative accuracy needs about
`0.34 |Im s|` more decimal digits.
(e) The rules of Lemma 6 (b) and Lemma 14 (a), (b) enter here through `S_e` and `J_bound`.

## The author's checks

Run: `python3 proto/analysis_checks.py`, 3131 assertions, 0 failures, 15.6 s on this machine.

What agreement "to 1e-55" establishes. The author compares two mpmath computations at 55 digits with a
relative tolerance 1e-32. Agreement shows that the two formulas give the same floating point number at the
sample points. It rules out a wrong sign, a missing factor or a wrong conjugation, provided the sample
point is one where the error shows. It does not give an enclosure: mpmath's Gamma, incomplete Gamma and
Hurwitz zeta carry no error bound, and both sides of several comparisons call the same mpmath functions.
It establishes nothing about an inequality beyond the sampled cases.

Would a wrong formula pass? I ran the author's checks against 11 mutants of bounds and formulas
(section M; the author's own 8 mutants concern signs and constants of identities and were all killed).
8 of my 11 are killed, 3 survive:
1. The factor 2 of the lattice bound dropped. No test of the author is within a factor 2 of the bound.
2. The continuation error bound multiplied by 1e-6.
3. The continuation error bound computed with conductor 1 in place of `C`.
Reason for 2 and 3: `check_continuation` tests `|result - expected| < bound + 1e-44 max(1, |expected|)`
with `N = 24`, `R = 420`. There the bound is far below the slack, so the inequality tests the slack.
`check_continuation_bounds` tests the two factors at `N = 2`, `R = 3`, but only for real `z`, conductors
1, 5, 7, and without the function that assembles the error.

Not covered by the author's checks, covered here: the truncation bound for the right side of Poisson
summation (D3); the theta identity at an idele other than 1 (D4); Proposition 12 for a function that is not
the special test vector, with `a != b` (I2, I3); the general adelic bound of Proposition 15 (I4);
conductors 9, 16, 25, 27 and composite conductors (`alpha_p` is tested by the author only through
`conj(chi(1/p)) = chi(p)`, which holds for any multiplicative function of modulus 1) (F2); `s` with large
imaginary part (H1). `check_global_integral` uses `s = 36 + 0.3 i`, where `L(s, chi)` differs from 1 by
1e-11: it tests the Gamma factor far more than the Dirichlet series.

## Repairs

### R1. Lemma 6, statement (replaces the text from "If rho<1" to "B_phi(a,h,T)=...")

    If rho <= 1/2, set S_j(c,T) = K^j exp(-c K^2)/(1-rho). If rho > 1/2, increment K to the first K0 with
    rho_j(c,K0) <= 1/2, add the explicit terms for floor(T)+1 <= n < K0, and use that geometric bound at
    K0. Then sum_(n>T) n^j exp(-c n^2) <= S_j(c,T). This terminates for every c>0. In ball arithmetic
    "rho <= 1/2" is tested on the upper bound of the ball for rho.

    For alpha>0, beta>=0 define in the same way S_j(alpha,beta,T) from
        rho_j(alpha,beta,K) = exp(j/K - alpha(2K+1) + beta),
        first term K^j exp(-alpha K^2 + beta K),
    so that sum_(n>T) n^j exp(-alpha n^2 + beta n) <= S_j(alpha,beta,T).

    With alpha = pi Re(A'), beta = |Re(B')|, a proved bound for the two-sided lattice tail of absolute
    values, including negative n, is
        sum_(|n|>T) |phi(a+hn)| <= B_phi(a,h,T),
        B_phi(a,h,T) = 2 exp(Re(C')) sum_j |q_j| S_j(alpha,beta,T).
    The same bound holds for |sum_(|n|>T) phi(a+hn)|.

Replacement for proof steps 1 and 2:

    1. For n >= K the ratio of successive terms of n^j exp(-alpha n^2 + beta n) is
       (1+1/n)^j exp(-alpha(2n+1) + beta) <= exp(j/n - alpha(2n+1) + beta), since log(1+1/n) <= 1/n.
       This is decreasing in n, hence at most rho_j(alpha,beta,K). Sum the geometric majorant. The
       exponent tends to minus infinity, so rho <= 1/2 is reached.
    2. |phi(a+hn)| <= exp(Re(C')) sum_j |q_j| |n|^j exp(-alpha n^2 + beta |n|), because
       Re(B') n <= beta |n|.

The bound of the present text remains true and may be kept as a corollary.

### R2. Lemma 8, definition and step 2

Add to the statement, after the first sentence:

    Primitive of conductor C means: chi is a character modulo C, and for every divisor C' of C with
    C' < C there is an integer u with gcd(u,C)=1, u = 1 mod C' and chi(u) != 1.

Replace step 2 by:

    2. Suppose d = gcd(m,C) > 1. Then C' = C/d is a divisor of C with C' < C. By primitivity choose a
       unit u = 1 mod C/d with chi(u) != 1. Since d divides m and C/d divides u-1, C divides m(u-1), so
       E(m u a/C) = E(m a/C). Substituting a -> u a in the sum multiplies it by conj(chi(u)) != 1 and
       leaves it unchanged, so the sum is zero. This covers m = 0 for C > 1 (then d = C, C' = 1).
       C = 1 is a one-term sum and is checked directly.

### R3. Proposition 12, step 1, last sentence, and step 4

Replace "Absolute convergence for Re(s)>1 follows as in Proposition 11, ..." by:

    Absolute convergence for Re(s)>1: with F0 = max_j |f_j| and the majorant of Lemma 6 step 2,
    sum_(q != 0) |f(q(t,u))| <= F0 sum_(n != 0) |phi(t n/D)|
        <= 2 F0 Kappa sum_j |p_j| sum_(n>=1) (t n/D)^j exp(-alpha t^2 n^2/(2 D^2)),
    uniformly in u. The function x^j exp(-alpha x^2/2) on x >= 0 has one maximum m_j and integral I_j,
    so a sum over the points x = t n/D, of spacing t/D, is at most m_j + (D/t) I_j. Hence the sum over
    q != 0 is at most c_1 + c_2/t for all t > 0, and by step 4 it decays faster than every power for
    t >= 1. The integral of (c_1 + c_2/t) t^(Re(s)-1) over (0,1) is finite exactly for Re(s) > 1.
    This justifies the unfolding (Fubini for a nonnegative integrand, then for the integrand itself).

In step 4 replace "Lemma 6 and Lemma 15 below" by "Lemma 6, Lemma 14 and Proposition 15 below".

### R4. Proposition 13, step 3, last sentence

    Replace "Lemma 15 proves both integrals entire." by
    "Proposition 15 step 1 gives a majorant that is integrable locally uniformly in z; by holomorphic
    parameter integration (Definition 1) both integrals are entire."

### R5. Lemma 14, statement

    If b >= 2 rplus/R then
        J(r,b,R) <= R^r exp(-b R)/(b-rplus/R) <= 2 R^r exp(-b R)/b = J_bound(r,b,R).
    Otherwise put R0 = 2 rplus/b > R and t* = min(max(r/b, R), R0) (here r > 0). Then
        J(r,b,R) <= (R0-R) (t*)^r exp(-b t*) + 2 R0^r exp(-b R0)/b.
    For r <= 0 the first case always applies.

Replacement for proof step 2:

    2. The derivative of r log t - b t is r/t - b, so t^r exp(-b t) increases up to t = r/b and
       decreases after it. Its maximum on [R,R0] is at t*. Add (R0-R) times this maximum to the bound
       of step 1 at R0, where b - rplus/R0 = b/2.

### R6. Proposition 15, first display and proof step 1

    E_sum      <= exp(a0) S_e(a0,N) J_bound(r,a0,1),
    E_integral <= exp(a0) S_e(a0,0) J_bound(r,a0,R).

    Proof step 1: for n >= 1 and t >= 1, (n^2-1)(t-1) >= 0 gives n^2 t >= n^2 + t - 1. Hence
    exp(-a0 n^2 t) <= exp(a0) exp(-a0 n^2) exp(-a0 t). Then use Lemmas 6 and 14.

For the general adelic bound, with `c = alpha/(2 D^2)` as before:

    n^2 t^2 >= n^2 + t^2 - 1 >= n^2 + t - 1 for n, t >= 1. Omitting |n|>N costs at most
    sum_j K_j exp(c) S_j(c,N) J_bound(r_j,c,1); omitting t>R costs at most
    sum_j K_j exp(c) S_j(c,0) J_bound(r_j,c,R).

Further replacements in the statement:

    Replace "Terms n divisible by the conductor may be omitted" by
    "Terms with gcd(n,C) > 1 vanish and may be omitted".

    Add after "All bounds tend to zero as the corresponding cutoffs tend to infinity.":
    "The bounds are absolute and independent of Im(s). Since |Lambda(s,chi)| decays like
    exp(-pi |Im s|/4), a relative accuracy needs cutoffs and working precision raised by about
    0.34 |Im s| decimal digits."

    Add to the paragraph on the midpoint rule:
    "The rule has order h^2 and is a fallback only. Each term has the exact integral
    (a0 n^2)^(-z) (Gamma(z, a0 n^2) - Gamma(z, a0 n^2 R)) in terms of the upper incomplete Gamma
    function; with R = infinity the term E_integral is not needed."
    [source pending: definition and enclosure of the incomplete Gamma function, FLINT documentation]

### R7. Checks of the author (suggestion)

In `check_continuation`, test the bound where it is larger than rounding: cutoffs such as (2, 3) and
(5, 10), complex `s`, no additive slack other than the rounding of the reference. Add one lattice case
within a factor 2 of the bound, for instance `exp(-0.2 pi x^2)`, `T = 0`.

## Checks

Commands, from the repository root. Three invocations, so that none runs longer than 3 minutes. One core.
Python 3.12.3, mpmath 1.3.0, python-flint 0.8.0.

    python3 docs/reviews/m0-proofs/analysis_review_checks.py ABCDEFG     (93 s)
    python3 docs/reviews/m0-proofs/analysis_review_checks.py HJK         (66 s)
    python3 docs/reviews/m0-proofs/analysis_review_checks.py IM          (106 s)

Every section seeds its random numbers itself. Result: 0 failed checks. "ok" on a line about looseness
means that the numbers are as described in this review, not that the looseness is acceptable. Output:

    [ok] A1 psi_f(q) = E(q) on 4000 rationals, so psi(q) = 1 (includes p = 2, negative q)
    [ok] A2 image of a + (A/B) Zhat is E(a) times the B-th roots of unity, 300 balls
    [ok] A3 transform formula against the integral, D != M, arrays without symmetry: 28 values, max abs diff 9.802e-50
    [ok] A4 transform vanishes off (1/M) Zhat: max abs 3.582e-51
    [ok] A5 norm identity with weights 1/M and 1/D: max abs diff 6.415e-50
    [ok] A6 second transform with weight 1/D is f(-x): max abs diff 8.261e-50
    [ok] B1 transform (moment formula) against trapezoid sums of the defining integral, kernel E(+xy): 24 values,
         max |diff|/int|f| = 1.208e-50, min |F|/int|f| = 7.285e-43
    [ok] B2 recurrence H_j of the text against the moment formula: max rel diff 4.991e-49
    [ok] B3 second transform is phi(-x), complex A (square root branches cancel): max rel diff 1.567e-49
    [ok] B4 F(x exp(-pi x^2))(0.4) = +0.4 i exp(-0.16 pi): value (0.0 + 0.24196903j)
    [ok] B5 translation, dilation, and F(D_h phi)(y) = |h|^(-1) F phi(y/h), h < 0 included: max rel diff 3.386e-49
    [ok] C1 S_j(c,T) >= tail, j = 0..12, c = 0.001..10, T = 0..30: 468 cases, bound/true between 1.0 and 1617.0
    [note] C2 j=3 c=1.0000001 T=0: bound/true = 2.368e+6 with rho<1; 1.002 with rho<=1/2
    [note] C2 j=6 c=2.000000001 T=0: bound/true = 2.877e+8 with rho<1; 1.0 with rho<=1/2
    [note] C2 j=1 c=0.33333334 T=0: bound/true = 2.534e+7 with rho<1; 1.048 with rho<=1/2
    [ok] C2 rule 'rho < 1' is valid but unbounded in looseness; 'rho <= 1/2' is within a factor 2.1
    [ok] C3 repaired rule on the same grid: bound/true between 1.0 and 1.583
    [ok] C4 lattice bound B_phi >= sum of |phi(a + h n)| over |n| > T (absolute values), Re(A) down to 0.01,
         |Re B| up to 40, degree up to 9, T = 0 .. 40: 400 cases, min bound/true 2.00877;
         at T = 0 max bound/true 2.538e+5560
    [note] C4 loosest case at T = 0: degree 2, A=(0.01 - 1.964j), B=(-40.0 + 0.1523j), a=1.922, h=-0.3
    [ok] C6 repaired lattice bound (ratio test on n^j exp(-alpha n^2 + beta n)) on the same cases:
         bound/true between 1.28025 and 6.196e+6 for every T
    [ok] C7 the factor 2 of B_phi is needed: exp(-0.2 pi x^2), a = 0, h = 1, T = 0: bound/true = 1.93633
    [note] C5 exp(-pi x^2), h=1, target 1e-30: cutoff from the bound T=6, from the true tail T=4
    [note] C5 exp(-pi x^2), h=1, target 1e-100: cutoff from the bound T=12, from the true tail T=8
    [ok] D1 Poisson summation, both sides, D != M, complex A and B, no symmetry: 5 cases, max rel diff 2.524e-50
    [ok] D2 truncation bound of the left side, T = 0, 1, 2.5, 5: min bound/error 10.39
    [ok] D3 truncation bound of the right side, T = 0, 1, 2.5, 5: min bound/error 10.54
    [ok] D4 Theta_f(x) = |x|^(-1) Theta_(F f)(1/x) at three ideles with r != 1, u != 1, x_inf < 0:
         max rel diff 8.881e-46
    [ok] E1 Gauss sum identities for all primitive characters of 18 conductors (8, 9, 16, 25, 27, 32, 72):
         106 characters, 11191 values of m, max abs diff 3.649e-48
    [ok] E2 primitivity is necessary: imprimitive character modulo 9, m = 3 gives a non-zero sum:
         |sum| = 5.196, |conj(chi(3)) tau| = 0.0
    [ok] F1 local functional equation on arrays without symmetry, d != m, conductors 4 8 16 3 9 27 5 25 7,
         s inside and outside the strip: 584 cases, max rel diff 6.316e-50
    [ok] F2 gamma_inf-constant i^e times product of ramified gamma_p equals i^(-e) tau(chi) C^(-s)
         (composite conductors, alpha_p of Proposition 11): 36 cases, max rel diff 1.356e-50
    [ok] G1 K_0, K_1 of the proof equal the stated Gamma quotient (four s, one at distance 0.001 from the
         pole s = 1): max rel diff 4.717e-29
    [ok] G2a the continued Mellin integral equals the plain integral inside the strip: abs diff 5.028e-31
    [ok] G2 real functional equation for a shifted non-even polynomial-Gaussian with complex A, both parities,
         s in the strip, left and right of it, near poles: 12 cases, max rel diff 8.272e-29
    [ok] G3 Z_inf(phi_e) and the residues 2 (-1)^k pi^k / k! at s = -2k: max rel diff 8.61e-26
    TOTAL failed=0 seconds=92.9

    [ok] H1 certified with arb at 1100 bits: |split formula with cutoffs (N,R) - FLINT value| <= E_sum +
         E_integral; conductors 1 5 7 8 9 15 45 101, nine s, five cutoffs: 765 cases, 0 violations,
         0 undecided; bound/actual error between 2.43 and 2.27e+56
    [note] H1 largest bound/|Lambda| at (N,R) = (1, 1): 4.16e+30 (conductor 101, s = (0.5+80j))
    [note] H1 largest bound/|Lambda| at (N,R) = (2, 3): 2.15e+30 (conductor 101, s = (0.5+80j))
    [note] H1 largest bound/|Lambda| at (N,R) = (5, 10): 7.87e+29 (conductor 101, s = (0.5+80j))
    [note] H1 largest bound/|Lambda| at (N,R) = (12, 40): 1.39e+29 (conductor 101, s = (0.5+80j))
    [note] H1 largest bound/|Lambda| at (N,R) = (40, 400): 2.55e+26 (conductor 101, s = (0.5+80j))
    [ok] H2 certified: Lambda(s,chi) = W_chi Lambda(1-s,conj chi) with FLINT's L-values, relative 1e-50:
         0 failures
    [note] H3 conductor 45, target 1e-30: text needs N=46, R=2021; with n^2 t >= n^2 + t - 1: N=32, R=996
    [note] H3 conductor 45, target 1e-100: text needs N=82, R=6611; with n^2 t >= n^2 + t - 1: N=58, R=3291
    [note] H3 conductor 45, target 1e-300: text needs N=141, R=19781; with n^2 t >= n^2 + t - 1: N=99, R=9876
    [ok] H4 certified: the sharper bounds exp(a0) S_e(a0,N) J(r,a0,1) and exp(a0) S_e(a0,0) J(r,a0,R) hold:
         0 failures
    [ok] J1 Lemma 14 (direct bound, and general algorithm with the first integer R0) >= integral: 112 cases,
         bound/true between 1.0 and 5.927e+17; loosest at r, b, R = (40, '0.01', 1)
    [note] J1 repaired rule (R0 = max(R, 2 rplus/b), prefix with the true maximum): bound/true at most 401.3
    [note] J2 r=3, b=0.5, R=1, admissible R0=6.0000001: bound/true = 1.347e+7
    [note] J3 r=3, b=3.0000001, R=1 (direct bound applies): bound/true = 1.038e+7
    [ok] K1 midpoint bound (R-1) h^2 M2 / 24, K = 1 .. 512, |Im z| up to 60, conductors up to 45: 30 cases,
         bound/error between 3.725 and 6.863e+7
    [note] K2 the rule has order h^2: an error 1e-30 on [1,40] needs about 1e15 midpoints, 1e-100 about 1e50
    TOTAL failed=0 seconds=65.9

    [ok] I0 conductor 1: unit average of the term q = 0 is delta f(0)
    [ok] I1 conductor 1: Hurwitz form of the Dirichlet series against 200000 plain terms: abs diff 1.72e-16
    [ok] I0 conductor 5: unit average of the term q = 0 is delta f(0)
    [ok] I1 conductor 5: Hurwitz form of the Dirichlet series against 200000 plain terms: abs diff 4.711e-21
    [ok] I0 conductor 9: unit average of the term q = 0 is delta f(0)
    [ok] I1 conductor 9: Hurwitz form of the Dirichlet series against 200000 plain terms: abs diff 4.529e-21
    [ok] I2 J_(f,chi)(s) equals the defining idele integral for Re(s) > 1, and J_(f,chi)(1-s) equals the
         defining integral of (F f, conj chi) at s; f without symmetry, f(0) != F f(0), conductors 1 5 9:
         12 cases, max rel diff 1.631e-21
    [ok] I3 conductor 1: residue +b at s = 1 and -a at s = 0 with a = f(0), b = F f(0), a != b:
         a = (0.7382 - 0.6772j), b = (0.5908 - 0.3344j)
    [ok] I4 bound of step 4 for |H - a| and the general truncation bounds of Proposition 15 hold:
         min bound/actual: pointwise 9.527, truncation 1289.0
       mutant: series bound: first term only (times 1.001): killed by check_tail_bounds
       mutant: series bound: explicit prefix terms dropped: killed by check_tail_bounds
       mutant: lattice bound: factor 2 for the two sides dropped: SURVIVES
       mutant: lattice bound: Kappa with beta^2/(4 alpha): killed by check_tail_bounds
       mutant: lattice bound: S_j(alpha,T) in place of S_j(alpha/2,T): killed by check_tail_bounds
       mutant: integral bound: finite piece [R,R0] dropped: killed by check_tail_bounds
       mutant: integral bound: R^r exp(-bR)/b for every r: killed by check_tail_bounds
       mutant: continuation error bound times 1e-6: SURVIVES
       mutant: continuation error bound computed with conductor 1: SURVIVES
       mutant: unramified gamma: alpha in place of 1/alpha in the denominator: killed by check_local_gamma
       mutant: ramified gamma: alpha^1 in place of alpha^a: killed by check_local_gamma
    [ok] M1 the author's checks against 11 wrong bounds and formulas: 8 killed, 3 survive
    TOTAL failed=0 seconds=105.7

## Not done

- No source under `refs/` was read for this review: the proof file quotes none, and its own sources are
  marked pending by its author. My verdicts rest on the derivations read step by step and on computation.
- The imported theorems of Definition 1 were not reviewed.
- Parameter balls (Proposition 15 step 5) were not tested: there is no implementation yet.
- Sections A to G and I to K of the checks are not certified. Only H is.
- The repaired bounds R1, R5 were tested numerically (C3, C6, J1) and R6 with certainty (H4); their proofs
  are the short texts above and have not been reviewed by a second reader.
