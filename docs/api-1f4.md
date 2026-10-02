# adelefeld: slice 1F.4-a, `exp`, `log` and `Log` at a prime (lane f-slice4, 2026-09-30)

This is the section of slice 1F.4-a of the interface of milestone 1F (`docs/api-1f.md` is written section by section;
this slice keeps its section in this file because it does not own `api-1f.md`). The contract of a function is the
comment block above its declaration in the header; this file lists the functions, the decisions, and the statements
F1 to F7 that the code adds to `docs/proofs/functions.md`, with their proofs.

Header: `include/adelefeld/lfunc.h`. Implementation: `src/lfunc.c`. Tests: `tests/test_lfunc.c` (vectors
`tests/ref/vectors/f-slice4/lfunc_cases.jsonl` and `lfunc_points.jsonl` from `lanes/f-slice4/gen_vectors.py`, which
imports the reference `proto/lfunc_checks.py`), `tests/julia/lfunc.jl`. Sources: `docs/SPEC.md` 9.3.1, 9.3.2;
`docs/conventions.md` 3.1 (line 173), 4.3; `docs/proofs/functions.md` Definition 1 (line 11), Lemma 5 (117),
Proposition 6 (140), 7 (165), 7b (198), 8 (227), Lemma 9 (265), Propositions 10 (299), 11 (336), 12 (375);
`docs/api-1f.md` L0 and L8; `refs/src/flint-3.0.1/fmpz.rst` (cited by line in `src/lfunc.c`).

## Functions

| Function | Result | Statuses |
|---|---|---|
| `adf_lball_exp(y, x, N)` | encloses `exp(t)` for every `t` in `x`; exact 1 for the exact 0 | `OK`, `DOMAIN`, `NOT_DETERMINED`, `LIMIT` |
| `adf_lball_log(y, x, N)` | encloses `log(t)`, series on `1 + p Z_p`; exact 0 for the exact 1 and, at 2, the exact `-1` | `OK`, `DOMAIN`, `NOT_DETERMINED`, `LIMIT` |
| `adf_lball_Log(y, x, N)` | encloses `Log(t) = log(u)`, `t = p^m w u`; exact 0 for the exact `+-p^m` | `OK`, `DOMAIN` (exact 0), `NOT_DETERMINED` (ball containing 0), `LIMIT` |

`N` is the requested absolute precision. A ball result is `f(a) + p^K Z_p`, `a` the exact input or the centre of the
input ball, `K = N` for an exact input and `K = min(N, E)` for a ball, `E` the exponent of the image (F6).

## Decisions taken in this slice (the orchestrator's, and the lane's)

1. The series are evaluated by the library's own code on `fmpz`, modulo a power `p^W` of `p` (orchestrator). FLINT's
   `padic_exp`, `padic_log` and `padic_teichmuller` are used in the tests only, as a second opinion at the centre.
   FLINT's C source of `padic` is not on disk and is not needed.
2. `N` is an ABSOLUTE precision, the same kind as the field `N` of `adf_lball` and as the exponents of SPEC 9.3.2
   ("absolute precision drops by `m` digits"). Alternative: a relative precision (digits after the valuation of the
   result); rejected for this slice because the result of `log` near 1 would then need the valuation of the result
   before the sum, and because Propositions 7, 7b and 8 are stated with an absolute `n`. A relative variant can be
   a separate function later.
3. A ball input gives `K = min(N, E)`: the requested `N` caps the result, and never raises it above what the input
   determines (SPEC 9.3.1: "an enclosure of the image of the whole input ball"; Proposition 8, step 4: "extra working
   digits cannot recover missing input digits"). With `N >= E` the result is the image itself, which is a ball
   (Propositions 10, 11), so it is the smallest ball. Alternative: always return exponent `N` and a status when
   `N > E`; rejected, because the result `f(a) + p^E Z_p` is the true answer and a status would lose it.
4. Exact results only where the value is a rational proved exactly: `exp(0) = 1`, `log(1) = 0`, `log(-1) = 0` at 2,
   `Log(+-p^m) = 0` (F2). Every other value is a ball of exponent `K`, also for an exact input (SPEC 9.3.1: "an exact
   input does not give an exactly representable output"). Alternative: always a ball; rejected because `Log(p) = 0`
   is a statement of SPEC 9.3.2 and the exact 0 says it without a precision.
5. The domain test is made with the predicates `adf_lball_contains` and `adf_lball_overlaps` of `lball.h` against the
   domain written as a ball (F1): `OK` inside, `DOMAIN` disjoint, `NOT_DETERMINED` meeting both (conventions 3.1).
   An input beyond the exponent bounds is `LIMIT` before the domain is tested (as `adf_lball_neg`, `_add`, `_mul`).
6. `log` accepts its whole domain `1 + p Z_p`, also `3 + 4 Z_2` at 2, where FLINT's `padic_log` refuses
   (`padic.rst:506-507`). On that domain `log = Log` (Proposition 11, step 3), and the code computes `log` as `Log`.
7. `Log` never forms the root of unity `w` (F3): at odd `p` it takes `log(a^(p-1)) / (p - 1)` for the unit part `a`
   (or `log(a)` when `a = 1` modulo `p`), at 2 `log(s a)` with `s = +-1`, `s a = 1` modulo 4. Alternative: the
   Teichmueller representative by Hensel lifting or by powering, as the reference does (statement T of
   `proto/lfunc_checks.py`); rejected in C because the measured lift costs more than the power and needs one division more,
   and because the tests then check two different routes against each other. The f-slice5 cost comparison
   at p = 2^64-59 and K = 10000 measured 0.66 s for the power and 3.08 s for the lift (power.log).
8. `exp` is a Horner sum with the one denominator `L!` (F4); `log` is the term-by-term sum of Proposition 8 with the
   count of Proposition 7b (the tight one, the default of PLAN 1F.7), with the lower bound of the valuation of `z`
   taken from the residue of `z` when `z = a^(p-1) - 1` (F5). The tagged-word log loop is retained. Other sums with K <= 64 use F8; larger sums use F9.
   Exp retains its original Horner sum. The F7 status limit is unchanged.
9. Limits: the bounds of `lball.h`; `LIMIT` for an input or a result beyond them, and for a working power `p^W` with
   `W bits(p) > ADF_LBALL_BITS_MAX` (F7). The last is the only limit set by an intermediate value; the lball rule
   ("never because of an intermediate value") cannot hold for a sum that must be formed modulo `p^W`, `W > K`. No
   power is formed where the centre is known without a sum, so `Log(1 + 5^E Z_5) = 5^E Z_5` for `E = 2^60`. There is
   no bound on the time: it grows with `N` as stated in the header, and `N` is the caller's choice (as `prec` of
   `rfunc.h`). Alternative: a smaller bound on the number of terms; not taken, because no limit of time exists
   elsewhere in the library.

## Statements F1 to F7 (to be merged into functions.md)

Notation as in `functions.md` Definition 1 and `api-1f.md` L0 to L8: `p` a prime, `v = v_p`, `c = 1` for odd `p` and
`c = 2` for `p = 2`; a ball is `a + p^M Z_p` in the canonical form of conventions 5.8 (`a = 0` or `v(a) < M`).
`e(k)` is the largest `e` with `p^e <= k` (Proposition 7b).

**F1 (the domain test).** Let `D` be `p^c Z_p` (for `exp`) or `1 + p Z_p` (for `log`). Both are balls: `D = 0 + p^c
Z_p` and `D = 1 + p^1 Z_p`, canonical. For a value `x` (an exact rational or a canonical ball) the set `x` lies inside
`D` iff `adf_lball_contains(x, D)`, and meets `D` iff `adf_lball_overlaps(x, D)`. So the status `OK` (inside),
`DOMAIN` (disjoint), `NOT_DETERMINED` (otherwise) is the rule of conventions 3.1 (line 173). For `Log` the domain is
`Q_p \ {0}`: the exact 0 is disjoint from it, a ball `p^M Z_p` around 0 contains 0 and `p^M != 0`, and a canonical
ball with centre `a != 0` has only elements of valuation `v(a)` (L7), none of which is 0.

*Proof.* The domains are those of Proposition 6 (line 142: exp, sin, sinh, cos, cosh on `p^c Z_p`; log on
`1 + p Z_p`, also at 2). `D` is canonical: centre 0 with `u = 0, v = 0`, or centre 1 with `u = 1 < p^1`. L8 (3) and
(2) state exactly that `contains` decides "inside" and `overlaps` decides "meets", for exact values and balls. The
three cases of the status are then the definition. For `Log`, L7 gives the valuations. *Check:* `check_domains` of
`proto/lfunc_checks.py` (1593 balls against sampled points), the grid statuses of `ball_enumeration_over_the_grid`.

**F2 (exact values).** `exp(0) = 1`; `log(1) = 0`; `log(-1) = 0` at `p = 2`; `Log(p^m) = Log(-p^m) = 0` for every
integer `m`. Conversely, for a rational `x != 0`, `Log(x) = 0` only if `x = +-p^m`, and for a rational `x` in
`1 + p Z_p`, `log(x) = 0` only if `x = 1` or (`p = 2` and `x = -1`).

*Proof.* `exp(0) = 1` and `log(1) = 0` are the constant terms (Definition 1; Proposition 6: "At zero the factorial
series take their constant terms; log(1) = 0"). `log(-1) = 0` at 2 is Proposition 11 (line 338). For `x = +-p^m`: at
odd `p`, `(+-1)^(p-1) = 1` since `p - 1` is even, so `+-1` is the root of unity `w` of Proposition 4 and `u = 1`; at
2, `w = +-1` by Proposition 4, step 2, and `u = 1`. So `Log(x) = log(1) = 0`. Conversely, `Log(x) = log(u)` with
`u` in `1 + p^c Z_p`, and `log` is a bijection from `1 + p^c Z_p` onto `p^c Z_p` (Lemma 9), so `log(u) = 0` forces
`u = 1`, and `x / p^m = w`. A rational `w` with `w^(p-1) = 1` (odd `p`) or `w = +-1` (`p = 2`) is `+-1`: its real
absolute value is 1. On `1 + p Z_p`, `log = Log` (Proposition 11, step 3), and `+-p^m` lies in `1 + p Z_p` only for
`m = 0` and `+1`, or `-1` at 2. *Check:* `exact_results_and_shortcut_values`, the exact rows of the case vectors.
The converse is not used by the code for correctness; it says that the exact results are all the ZERO values
of `log` and `Log` at rationals. Whether `log` or `Log` takes a nonzero rational value at a rational argument
is not decided here (review f-review3, R2): such a value would be returned as a ball, which is an enclosure.
`[source pending: a proof that log and Log take no nonzero rational value at a rational argument]` For `exp` at a rational `x != 0` no such statement is made: a ball is returned.

**F3 (Log without the root of unity).** Let `x = p^m a`, `a` a unit of `Z_p`.
(a) Odd `p`: `a^(p-1)` lies in `1 + p Z_p` and `Log(x) = log(a^(p-1)) / (p - 1)`. If `a = 1` modulo `p`, then
    `Log(x) = log(a)`.
(b) `p = 2`: with `s = 1` if `a = 1` modulo 4 and `s = -1` if `a = 3` modulo 4, `s a` lies in `1 + 4 Z_2` and
    `Log(x) = log(s a)`.

*Proof.* (a) Proposition 4: `a = w u`, `w^(p-1) = 1`, `u` in `1 + p Z_p`. So `a^(p-1) = u^(p-1)`, in `1 + p Z_p` (a
group under multiplication). Lemma 9 (`log(ab) = log a + log b` on `1 + p Z_p`) and induction give
`log(u^(p-1)) = (p - 1) log(u)`. `p - 1` is prime to `p`, a unit, so `log(u) = log(a^(p-1)) / (p - 1)`, and
`Log(x) = log(u)` is the definition (Proposition 11). If `a = 1` modulo `p`, then `w = 1` modulo `p`; the polynomial
`T^(p-1) - 1` has the root 1 with residue 1 and derivative `p - 1`, a unit, so by Lemma 3.2 its only root with
residue 1 is 1, and `w = 1`, `u = a`. (b) Proposition 4, step 2: `w = s` (the choice by `a` modulo 4) and
`u = a / w = s a`, in `1 + 4 Z_2`. *Check:* the grid of `lfunc_points.jsonl`, whose reference computes `Log` through
the Teichmueller representative at odd `p` and through `log(a^2)/2` at 2 (`proto/lfunc_checks.py`, statement T),
and FLINT's `padic_teichmuller` and `padic_log` in `flint_second_opinion`.

**F4 (exp with one common denominator).** Let `x` be a rational with `w = v(x) >= c`, `K > w`, `Kt` the count of
Proposition 7 for `n = K` and `v = w`, `L = Kt - 1`, `D = v_p(L!)`, `W = K + D`, and `x_r` an integer with
`x_r = x` modulo `p^W`. Define integers modulo `p^W` by `F_(L+1) = 1`, `A_(L+1) = 1` and, for `k = L, ..., 1`,
`F_k = k F_(k+1)`, `A_k = F_k + x_r A_(k+1)`. Then `A_1` and `F_1` are divisible by `p^D` (as residues in
`[0, p^W)`), `F_1 / p^D` is prime to `p`, and `r = (A_1 / p^D) (F_1 / p^D)^(-1)` modulo `p^K` satisfies
`exp(x) = r` modulo `p^K`.

*Proof.* (i) `L >= 1`: `K > w` gives `(p-1)K - 1 > (p-1)w - 1 >= 1`, so `Kt >= 2`. (ii) By induction from `k = L + 1`
down, `A_k(X) = sum_(i=0)^(L-k+1) (L! / (k-1+i)!) X^i` as a polynomial with integer coefficients and `F_k = L!/(k-1)!`:
the case `k = L + 1` is `1 = L!/L!`, and `F_k + X A_(k+1) = L!/(k-1)! + sum_(i=1)^(L-k+1) (L!/(k-1+i)!) X^i`. So
`A_1(X) = sum_(i=0)^L (L!/i!) X^i`, `F_1 = L!`, and `sum_(i=0)^L x^i/i! = A_1(x) / L!`. (iii) `A_1` has integer
coefficients and `x_r - x` lies in `p^W Z_p`, so `A_1(x_r) = A_1(x)` modulo `p^W` (Proposition 8, step 1). Each term
`(L!/i!) x^i` has valuation `D - v_p(i!) + i w >= D`, because `v_p(i!) <= (i-1)/(p-1) < i <= i w` for `i >= 1`
(Lemma 5) and the term `i = 0` is `L!`. So `v(A_1(x)) >= D`, and the residue of `A_1(x_r)` modulo `p^W`, `W >= D`, is
divisible by `p^D`, with `A_1(x_r)/p^D = A_1(x)/p^D` modulo `p^(W - D) = p^K`. (iv) `L! = p^D U`, `U` prime to `p`
(the definition of `D`, Lemma 5), and `L!` modulo `p^W` is `p^D (U` modulo `p^K)`, so `F_1 / p^D = U` modulo `p^K`,
invertible. (v) `A_1(x) / L! = (A_1(x)/p^D) / U = r` modulo `p^K`, and `exp(x) - A_1(x)/L!` lies in `p^K Z_p`
(Proposition 7, the tail after `Kt` terms). The residue modulo `p^W` loses no precision in the sum: every operation
is a ring operation on integers. When `K <= w` the value is `1` modulo `p^K`, because `v(exp(x) - 1) = v(x) = w`
(Lemma 9, item 5); the code returns the centre 1 then, and for the centre 0 of a ball. *Check:* the fault "one
term too few" (`lanes/f-slice4/faults.log`, fault 2), the grid and case vectors, FLINT's `padic_exp`.

**F5 (the sum of log, and a lower bound of the valuation from a residue).** Let `z` be in `Z_p` with `v(z) >= b >= 1`,
`b < K`, `T` the count of Proposition 7b for `n = K`, `v = b`, and `W >= K + e(T)`, `z_r = z` modulo `p^W`. Then the
sum of Proposition 8, `sum_(k=1)^T (-1)^(k+1) (z_r^k mod p^W) / p^(e_k) (k / p^(e_k))^(-1)`, `e_k = v_p(k)`, is
`log(1 + z)` modulo `p^K`. Moreover:
(a) the count `T` and `e(T)` do not increase when `b` increases, so a `W` formed with a smaller lower bound `b'` is
    valid for the larger `b`;
(b) with `z_r` the residue modulo `p^W` and `b = min(v_p(z_r), W)` (`b = W` for `z_r = 0`), `v(z) >= b`; if `b >= K`
    then `log(1 + z) = 0` modulo `p^K`, provided `v(z) >= c`.

*Proof.* The sum is Proposition 8 with `n = K`, `D = e(T)` (line 234: "For a nonempty log sum with largest degree L,
take D = floor(log_p L)") and `W >= max(b, K + D)` (`b < K`). Proposition 8 states `W` as sufficient; a larger `W`
is sufficient as well, because its proof (step 2) uses only `W >= n + D > e`. (a) For `b' <= b`,
`k b - e(k) >= k b' - e(k)`, so the least `k` of Proposition 7b for `b` is at most that for `b'`, and `e` is
nondecreasing. (b) `z - z_r` lies in `p^W Z_p`; if `v_p(z_r) < W` then `v(z) = v_p(z_r)` by the strict ultrametric
inequality, otherwise `v(z) >= W`. If `v(z) >= K` and `v(z) >= c`, Lemma 9, item 5 gives `v(log(1 + z)) = v(z) >= K`.
In the code `z = a^(p-1) - 1` or `z = a - 1` at odd `p` (`v(z) >= 1 = c`), `z = s a - 1` at 2 (`v(z) >= 2 = c`).
*Check:* faults 3 and 4, the case and grid vectors, FLINT's `padic_log`.

**F6 (the precision of a result).** Let `B = a + p^M Z_p` be a canonical ball inside the domain of `f`, `E` the exponent
of lfunc.h (exp: `M`; log: `M` if `M >= c`, else 2; Log: `r = M - v(a)` if `r >= c`, else 2), `K = min(N, E)`, and
`r_K` an integer with `r_K = f(a)` modulo `p^K` (`r_K = 0` for `K <= 0`). Then `f(B)` lies in `r_K + p^K Z_p`; if
`N >= E`, `f(B) = f(a) + p^E Z_p`, and `r_K + p^K Z_p` is the smallest ball that contains `f(B)`. For an exact input
`a`, `f(a)` lies in `r_N + p^N Z_p`.

*Proof.* The image: Proposition 10 (exp: `f(B)` has smallest enclosing ball `exp(a) + p^M Z_p`; it is the image
itself, because `exp(a + p^M Z_p) = exp(a) exp(p^M Z_p) = exp(a) (1 + p^M Z_p)` by Lemma 9 and `exp(a)` is a unit);
Proposition 11, lines 345-352 (`Log(a + p^M Z_p) = Log(a) + p^r Z_p` for `r >= c`, `= 4 Z_2` for `p = 2, r = 1`; the
series log on a ball of its domain: `log(a) + p^M Z_p` for `M >= c`, `4 Z_2` for `p = 2, M = 1`). In the cases
`4 Z_2`, `f(a)` is in `4 Z_2` (Proposition 12, step 4), so the image is `f(a) + p^2 Z_2` as well. So `f(B) =
f(a) + p^E Z_p`, and for `K <= E` it lies in `f(a) + p^K Z_p = r_K + p^K Z_p`. With `N >= E`, `K = E` and the result
equals the image, a ball, which is the smallest ball containing itself. The centre `r_K` is computed by F4 (exp), by
F3 and F5 (log, Log), or is 0 when `K <= c` (log, Log: `f(a)` in `p^c Z_p`, Proposition 12, step 4) or when the unit
part of `a` is 1 (`Log(a) = 0`, F2), or 1 for exp as in F4. *Check:* `ball_enumeration_over_the_grid` (7600 calls:
every grid point value inside, the exponent `min(N, E)`, 3852 smallest-ball witnesses), `check_ball_enumeration` of
the reference, faults 1 and 6.

**F7 (limits).** The functions return `ADF_LIMIT` exactly in the cases of the header: an input beyond
`ADF_LBALL_EXP_MAX`; a ball result with `|K| > ADF_LBALL_EXP_MAX`; a sum that needs `p^W` with `W bits(p) >
ADF_LBALL_BITS_MAX`. Every exponent formed is within `3 * 2^60` in absolute value, so no `slong` overflows.

*Proof.* The inputs are within `2^60`; `r = M - v(a)` is within `2^61`; `K = min(N, E)` is compared with the bound
before it is used; `W = K + D` is formed only after `pow_ok(p, K)` holds (`K <= 2^25`), and `D <= L < Kt <= 2K` for
exp (`(p-1)w - 1 >= (p-1)/2` for odd `p` and `w >= 1`, and `>= 1 = p - 1` at 2 for `w >= 2`) and `e(T) <= 26` for
log (`T <= 2K <= 2^26`). `count_exp` forms `(p - 1) K` in `fmpz`.
*Check:* `every_status` (the limits of each kind, and the no-power cases at `E = 2^60`).

## What the oracles are

The reference `proto/lfunc_checks.py` sums the series exactly with `Fraction` and a tail bound with three digits of
margin (so its count is longer than the counts of Propositions 7 and 7b), computes `log` by the raw series (also at 2
for `x = 3` modulo 4), and `Log` through the Teichmueller representative (odd `p`, statement T, proved there) or
`log(a^2)/2` (at 2), so that it does not share the routes of F3. Its ball results are checked by enumeration of the
points of each ball (`check_ball_enumeration`: 1020 balls, 13281 points). The C tests compare with the reference
field by field, enclose the value of every grid point of every grid ball, and compare with FLINT's `padic` at the
centre for 1000 random inputs up to precision 200 and at precision 2000.

## F8 (working precision of each log term; lane f-slice5)

Let z be in p Z_p, v(z) >= v >= 1, K > v, and z = p^v b. For degree k >= 1 put

    e_k = v_p(k), d_k = k / p^e_k, h_k = k v - e_k, H_k = K - h_k.

If H_k <= 0 the term is zero modulo p^K. Otherwise compute b^k modulo p^H_k, divide by the unit d_k
in that ring, and multiply by p^h_k. This gives z^k/k modulo p^K. The input z need only be known
modulo p^K. A convenient nonincreasing modulus for the recurrence of the unit powers is

    p^G_k, G_k = K - k v + floor(log_p k).

The sum must keep K digits even when a term needs fewer digits. The tail count remains Proposition 7b.
The original working-power limit F7 remains checked before this computation.

Proof, step by step.

1. v_p(k) <= floor(log_p k) and k v > e_k (Lemma 5). Hence h_k >= 0. A term with h_k >= K is zero
   modulo p^K. Multiplication by a unit changes no valuation.
2. An error divisible by p^H_k in b^k/d_k becomes divisible by p^(H_k+h_k) = p^K after multiplication
   by p^h_k. Thus H_k digits of the unit power suffice. This is a sufficient precision, not a necessary
   one for every special input.
3. If z is replaced modulo p^K, b changes by p^(K-v). For k >= 1 write b' = b + delta. In the binomial
   expansion of b'^k-b^k, the term of degree i in delta has valuation at least
   (i(K-v) + v_p(k) - v_p(i)). Indeed i binom(k,i) = k binom(k-1,i-1).
   Since K-v >= 1 and v_p(i) <= i-1, this is at least K-v+e_k. After multiplication by p^(k v-e_k),
   its valuation is at least K+(k-1)v. Thus the input precision K is sufficient for every term.
4. floor(log_p k) increases by at most 1 from k to k+1, while v >= 1. Hence G_k is nonincreasing,
   and G_k >= H_k. Given b^(k-1) modulo p^G_(k-1), multiplication by b followed by reduction modulo
   p^G_k gives the required b^k. Through the last retained degree G_k > 0 by the definition of the count.
5. For the unit division let Q = p^H_k and 0 <= r < Q. Since gcd(Q,d_k)=1, choose
   j = (-r mod d_k) (Q mod d_k)^(-1) mod d_k. Then r+j Q is divisible by d_k, and
   0 <= (r+j Q)/d_k < Q. This quotient is exactly r/d_k modulo Q. d_k <= k fits in a word, so the
   inverse, remainder and exact division use word operations. The case d_k=1 needs no inverse.
6. Add the terms with their signs while preserving K digits of the sum. Each term error and the omitted
   tail are in p^K Z_p, so their total error is in p^K Z_p. Reducing the sum at G_k would lose digits.

Checks: all 2000 stored old-code cases, including aliasing, and the existing reference vectors.
Three faults in private copies reduce a term by one extra digit, divide by the wrong unit integer, or
reduce the sum at G_k. Each is rejected by the stored comparison. Commands and counts are in the lane report.

## F9 (balanced factors and an exact splitting tree)

For an integral principal unit u = 1 modulo p^v, v >= c, known modulo p^K, take m = min(2v,K).
Let l be its least nonnegative residue modulo p^m. Then l is a unit, l = 1 modulo p^v, and
u/l = 1 modulo p^m. Modulo p^K,

    log(u) = log(l) + log(u/l).

Repeat with v replaced by m, until m=K. A factor equal to 1 contributes zero. Every other factor has
z = l-1 of valuation w >= v, and bit size less than m log2(p). Use Proposition 7b's count T for that w.
Compute its finite sum exactly by the following tree, with q=-z and 1 <= a < b <= T+1:

    B_(a,b) = product_(k=a)^(b-1) k,
    R_(a,b) = q^(b-a),
    A_(a,b) / B_(a,b) = sum_(k=a)^(b-1) q^(k-a)/k.

A leaf is A=1, B=a, R=q. At a split a < d < b combine the left and right triples by

    A = A_left B_right + R_left A_right B_left,
    B = B_left B_right, R = R_left R_right.

Then z A_(1,T+1)/B_(1,T+1) is the signed log partial sum. Remove the p part of B, divide A z by
that same p power exactly, invert the remaining unit denominator modulo p^K, and reduce. The p valuation
of B here is v_p(T!), not the guard exponent of F5. This tree uses exact integers and needs no guard
precision; the original F7 limit is still checked from F5 before selecting this route.

Proof.

1. l is congruent to u modulo p^m and is a unit. Therefore u/l-1 = (u-l)/l lies in p^m Z_p.
   Both l and u/l lie in 1+p^c Z_p. Lemma 9's product identity proves the logarithm identity.
2. Replacing a principal unit by another modulo p^K changes its log by p^K Z_p: the quotient is in
   1+p^K Z_p, and Lemma 9 gives the valuation of its log. Therefore modular inverses and products in the
   factorisation preserve the result modulo p^K. For the residual write u-l = p^m d, an exact integer
   division. Then u/l = 1 + p^m d/l. Only K-m digits of l^(-1) are needed: an inverse error in
   p^(K-m) becomes an error in p^K after multiplication by p^m d. This avoids a full-size inverse.
   At the last step the remaining quotient is 1 modulo p^K.
3. Each step doubles the known valuation, unless it reaches K. Thus there are finitely many steps.
   The integer factor has less than m digits, and the term count decreases as v increases.
4. The leaf identities hold directly. Splitting the stated sum at d gives the left sum plus
   q^(d-a) times the right sum. Putting this over B_left B_right gives precisely the combine rule.
   Induction proves the tree identities without rounding.
5. Since q=-z, z q^(k-1) = (-1)^(k-1) z^k. Thus the final ratio is the required finite sum.
   Every summand is p-integral since k w > v_p(k), so their sum is p-integral. Consequently the integer
   numerator A z is divisible by the entire p part of B. The remaining denominator is a unit.
6. Proposition 7b bounds each omitted infinite tail by p^K Z_p. Reducing each exact partial sum and then
   adding the factor logarithms retains the complete log modulo p^K. F6 therefore gives the same ball,
   canonical centre, exponent and exact flag as before. Domain tests, shortcuts and F7 are unchanged.

The tree and factor proof above are our own. FLINT describes rectangular log splitting in
`refs/src/flint-3.0.1/padic.rst:509-516`, and balancing chunk size against valuation for exp in
`:440-447`. No FLINT padic function is called by the library (N-D9).

## Slice 1F.7: the four parity series (lane f-slice7)

Functions: adf_lball_sin, adf_lball_cos, adf_lball_sinh, adf_lball_cosh; the corresponding
adf_sball functions with suffix _at; driver commands sin_at, cos_at, sinh_at, cosh_at.
The header blocks give their complete contracts. The old exp, log and Log implementations are unchanged.

Decision: use E = M for sin and sinh. For cos and cosh use E = 2M - v_p(2) on the centred
ball p^M Z_p, and E = M elsewhere. The alternative is the safe exponent M on every ball.
This choice uses the centred hull already proved in Proposition 10. It makes no tightness claim
for a noncentred cosine ball. All four cap E by requested N, and only exact zero gives an exact result.
At a prime the _at precision is absolute N; at real it is arb working bits, as for exp_at (N-D10).

## F10 (counts for the four parity series)

Let w = v_p(a) >= c for a nonzero centre a and let K be the requested centre precision. Put

    d = (p-1)w - 1,
    C = max(1, ceildiv((p-1)K - 1, d)),
    T_odd = floor(C/2), T_even = ceildiv(C,2).

Sin and sinh retain odd degrees 1,3,...,2T_odd-1; cos and cosh retain even degrees
0,2,...,2T_even-2. A zero odd count means an empty sum. Thus L, when a nonconstant
sum is needed, is the greatest integer below C of the selected parity. These are precisely
Proposition 7's counts (docs/proofs/functions.md:165-195), including the constant term.

Proof.

1. Lemma 5 (functions.md:117-138) gives v_p(a^k/k!) >= k w - (k-1)/(p-1)
   = (k d+1)/(p-1) for every k >= 1. The sign of a coefficient does not change this bound.
2. d > 0 on the domain. For k >= C the bound is at least K and tends to infinity with k.
3. The first omitted odd degree is 2 floor(C/2)+1 >= C. The first omitted even degree is
   2 ceildiv(C,2) >= C. Hence every omitted term in each of the four sums lies in p^K Z_p.
4. Every finite tail has valuation at least K by the ultrametric inequality. Completeness and
   closedness of p^K Z_p give the same bound for the infinite tail. Cancellation cannot worsen it.

Check: the exact Fraction oracle uses the degree bound with three extra digits, independent of
this closed count. The fixtures specify N on every row. The planted short-count fault removes
one whole retained parity term, rather than an absent coefficient of the other parity.

## F11 (parity Horner sum and working precision)

Assume a nonconstant partial sum of F10, largest degree L, K > w >= c. Write
D = v_p(L!), W = K+D, and a_r = a modulo p^W. For each degree j let epsilon_j be 0
for the other parity, 1 for a hyperbolic term, or (-1)^floor(j/2) for a circular term.
Start F = 1, A = epsilon_L. For k = L,L-1,...,1, do

    F = k F, A = a_r A + epsilon_(k-1) F,

reducing each result modulo p^W. Then p^D divides the final residues A and F, F/p^D is
a unit modulo p^K, and (A/p^D) (F/p^D)^(-1) modulo p^K is f(a) modulo p^K.
The code performs these steps two at a time; F15 proves that the residues are the same.

Proof.

1. Before reduction, induction on descending k gives F = L!/(k-1)! and
   A = sum_(j=k-1)^L epsilon_j (L!/j!) a_r^(j-k+1). At the start the degree-L
   expression is epsilon_L. Multiplying by a_r and adding the next coefficient proves the step.
2. At the end F=L! and A=sum_(j=0)^L epsilon_j (L!/j!) a_r^j. Division by L!
   is exactly the selected finite factorial sum, with its signs and missing degrees.
3. This numerator polynomial has integer coefficients. Replacing a by a_r modulo p^W
   changes its value by p^W Z_p, by factoring each difference of powers. Reducing any
   intermediate sum or product modulo p^W changes no final residue. Signs cause no precision loss.
4. For j>=1, j w-v_p(j!) >= 0 by Lemma 5. Thus every nonzero summand
   (L!/j!) a^j has valuation at least D. The possible constant term L! also has valuation D.
   W=K+D>D, so the least nonnegative residue A is divisible by p^D as an integer.
5. L!=p^D U with U a unit. The residue F/p^D is U modulo p^K, so is invertible.
   Dividing A and F by p^D loses exactly D available absolute digits. Unit inversion loses none.
   The ratio is the exact finite sum modulo p^K. F10 supplies the infinite-tail error in p^K Z_p.
6. This is Proposition 8's working precision (functions.md:227-263): the largest retained
   denominator valuation is v_p(L!), and max(w,K+D)=K+D because K>w. It works for rational
   centres: reduce their unit numerator times the inverse of the unit denominator, then multiply by p^w.

The implementation makes no call to FLINT's padic module. It uses fmpz exact division, nonnegative
modulus and modular inverse (refs/src/flint-3.0.1/fmpz.rst:852-859,880-883,1154-1160).

## F12 (whole-ball enclosure, exact values and constant-centre shortcuts)

For B=a+p^M Z_p contained in p^c Z_p, with canonical centre a, define E as follows:

    sin, sinh: E=M;
    cos, cosh: E=2M-v_p(2) if a=0, E=M otherwise.

A ball input returns f(a)+p^K Z_p, K=min(N,E); an exact nonzero input returns
f(a)+p^N Z_p. Exact zero returns exactly 0 for sin/sinh and 1 for cos/cosh, at every N.
For sin/sinh the ball at K=E is the image itself. For centred cos/cosh it is the smallest
ball containing the image. The latter statement does not assert that every point of the hull occurs.

Proof.

1. Proposition 6 (functions.md:140) gives the common domain. F1 applies its inside/disjoint/overlap
   test to all four. In particular 2 Z_2 meets both the domain and its complement, whereas exact 2
   is outside: NOT_DETERMINED and DOMAIN, respectively.
2. Proposition 10 (functions.md:299-334) gives distance preservation for sin/sinh, exponent M
   safe for cos/cosh, and the centred cosine hull exponent 2M-v_p(2). Therefore each image lies
   in f(a)+p^E Z_p. Replacing the centre by its residue modulo p^K, K<=E, preserves enclosure.
3. For completeness the odd image is onto its hull. Write f(t)=t+h(t). The degree-difference
   estimate in Proposition 10's step 1 gives v_p(h(u)-h(t)) >= v_p(u-t)+1 on the domain.
   For b in f(a)+p^M Z_p iterate t_0=a, t_(j+1)=b-h(t_j). If t_j is in B then
   t_(j+1)-a=(b-f(a))-(h(t_j)-h(a)) lies in p^M Z_p, so every iterate is in B.
   Successive differences gain at least one valuation at each step. Completeness gives a limit t in B;
   the same estimate gives continuity of h and t=b-h(t), hence f(t)=b. This proves surjectivity
   on B, without requiring a globally chosen inverse. Distance preservation also proves hull minimality.
4. On p^M Z_p, the even quadratic term has valuation at least 2M-v_p(2). Every higher even
   term has greater valuation, as proved in Proposition 10 steps 4-5. The values at 0 and p^M
   differ with valuation exactly 2M-v_p(2). They witness the smallest hull. For noncentred balls
   only the safe E=M is claimed. Tests explicitly require this exponent as well as the centred gain.
5. At exact zero all positive degree terms vanish, so the constant terms are exact. A ball centred
   at zero contains nonzero points. The witnesses in steps 3-4 forbid replacing its image by a singleton.
   No claim about other rational values of these functions is needed: nonzero exact inputs give balls.
6. Setting one argument to zero in Proposition 10 shows v_p(sin(a))=v_p(sinh(a))=w.
   The quadratic dominance argument gives v_p(cos(a)-1)=v_p(cosh(a)-1)=2w-v_p(2).
   Thus K<=w for odd functions, or K<=2w-v_p(2) for even functions, determines the centre
   as 0 or 1 without a sum. A zero centre also needs no sum. For K<=0 all four values are integral,
   and their canonical residue is 0. If the even shortcut does not apply, K>2w-v_p(2)>=w;
   the odd case also has K>w. F11's hypotheses therefore hold whenever it is called.

Checks: all domain balls with c<=M<=c+2 at p=2,3,5,7; every representative modulo p^(c+3),
with oracle output precision at least E+1. The tests demand containment and the exact promised K;
when smallest hulls are claimed they also demand two values different modulo p^(E+1).
The independent values cos(4)=9 mod 16 at 2, sin(3)=3 mod 9 at 3, and v_3(cos(3)-1)=2
are tested at absolute output precisions 4, 2, and 5, respectively.

## F13 (limits and output transaction)

The limits are those of lfunc.h: input |v| or |M| beyond 2^60 first; then the domain;
then exact zero; then |K| beyond 2^60; then a required working power whose W bits(p)
exceeds 2^26. Outputs are untouched on failure, including y=x. Known-centre shortcuts need no power.

Proof.

1. After the input check, E=2M-v_p(2) has magnitude at most 2^61+1, within slong.
   Domain membership ensures M>=c when this centred-ball expression is used. The threshold
   2v(a)-v_p(2) is also safe. N is only compared, never added or negated, so LONG_MIN is allowed
   as an argument. In the order above it returns LIMIT only for an input that passes the input limits and
   the domain test and is not exact zero: at p=2 exact 2 gives DOMAIN and 2 Z_2 gives NOT_DETERMINED.
2. K is bounded before arithmetic with it. Before forming a nonconstant sum, K bits(p)<=2^26
   is checked, so K<=2^25. The count satisfies C<=2K (F7); L<C and D<=L imply W<=3K.
   The products (p-1)K and (p-1)w are formed in fmpz, including at p=2^64-59. No word product
   at that prime can overflow the count. W is tested before forming any power p^W.
3. The centred hull can have E>2^60 while K remains valid when N<=2^60. Test the bounded
   result K, not the uncapped E. With N>=E>2^60 return LIMIT. This distinguishes input and result limits.
4. All computation is in a temporary local ball. Only OK swaps it into y. Input data remains alive
   until the final swap, so distinct and aliased calls give identical results and failures change nothing.

## F14 (partial balls and real hyperbolic functions)

S8 of docs/api-1f.md extends to sin, cos, sinh and cosh: at a prime of a partial ball, call the
local function on that component with unchanged absolute N, and on OK retain only that prime.
Failures set where to the requested prime and preserve y; OK preserves where. At real, sinh_at
and cosh_at use arb_sinh and arb_cosh with the precision and finiteness rules of exp_at.

Proof.

1. Projection to the named prime reads exactly the component whose image F12 encloses.
   Constructing a partial ball with that one component is the one-place tuple of Proposition 22
   (functions.md:725). The other components do not occur in the output. Copying the component
   before writing y proves the same rule when y=x. There is no rounding or clamping of local N.
2. At real, refs/src/flint-3.0.1/arb.rst:1209-1219 specifies the hyperbolic calls. The enclosure
   convention for arb arithmetic is refs/src/flint-3.0.1/arb.rst:4-12. A finite arb result contains
   the image, including its propagated input error; this wrapper makes no smallest-interval claim.
   The existing real_apply rules clamp prec below 2, reject it above ADF_REAL_PREC_MAX, and map
   a non-finite computed result to NOT_DETERMINED. Only finite results are swapped into y.
3. The existing at_place check order and status reporting are unchanged. A missing prime is DOMAIN;
   a COMPLEX tag affects only the real place. At real, LIMIT from prec is checked before place membership.

Tests compare every new oracle case through _at, including aliasing, and real sinh/cosh with
arb at the same precision and with three image points evaluated at 250 bits. Overflow at input 2^1000
must be NOT_DETERMINED with no output, the same loss rule as exp_at.

## F15 (paired parity Horner steps; lane f-repair3)

Under the hypotheses of F11, the following loop ends with the same integers A and F as the loop of F11.
Put x2 = a_r^2 modulo p^W. Start F = 1, A = epsilon_L. For k = L, L-2, ..., while k >= 2, do

    F = k (k-1) F, A = x2 A + epsilon_(k-2) F,

reducing each result modulo p^W. If L is odd, finish with A = a_r A modulo p^W.

Proof.

1. L has the retained parity (F10), and the F11 step at k leads to degree k-1. From k = L down the steps
   therefore alternate: the step at k with k = L modulo 2 adds epsilon_(k-1) F, and epsilon_(k-1) = 0
   because k-1 has the other parity; the step at k-1 adds epsilon_(k-2) F, with k-2 of the retained parity.
2. Let (F, A) be the state before the step at k, k = L modulo 2, k >= 2. The first step gives F' = kF and
   A' = a_r A. The second gives F'' = (k-1) F' = k(k-1) F and A'' = a_r A' + epsilon_(k-2) F''
   = a_r^2 A + epsilon_(k-2) F''. These are identities of integers before any reduction.
3. Reduction modulo p^W is a ring homomorphism from Z onto Z/p^W Z. Both loops evaluate the same
   expressions of a_r, k and epsilon in that ring, so after each pair their states are congruent modulo p^W.
   Both store the least nonnegative residue after every operation (fmpz_mod,
   refs/src/flint-3.0.1/fmpz.rst:880-883), so the stored integers are equal.
4. For even L the pairs cover k = L, ..., 2 and end at degree 0, as F11 does. For odd L they cover
   k = L, ..., 3 and end at degree 1. The last F11 step, at k = 1, gives F = 1 F and A = a_r A + epsilon_0 F
   with epsilon_0 = 0 (degree 0 is even): the final multiplication by a_r. For L = 1 there is no pair.
5. The division by p^D and the inverse of F/p^D modulo p^K act on the same integers, so the result is the
   same residue. L, D, W, K, the domain test, the exponent rule and the statuses are fixed before the loop.

The product k(k-1)F is formed by fmpz_mul2_uiui (refs/src/flint-3.0.1/fmpz.rst:758-760), with no word
product of k and k-1. A pair costs one multiplication of A, one of F and two reductions; F11's two steps
cost two of each. Check: lanes/f-repair3/compare.c runs the code of F11 (src/lfunc.c before this change)
against the paired loop on 3300 random and 14 fixed inputs of the four functions, distinct and aliased;
the residues pinned in tests/test_lfunc_trig.c (pinned_residues_odd_and_even_top_degree) were computed
before the change.
