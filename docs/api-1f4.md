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
   `proto/lfunc_checks.py`); rejected in C because it costs a lifting of the same size as the power and one division
   more, and because the tests then check two different routes against each other.
8. `exp` is a Horner sum with the one denominator `L!` (F4); `log` is the term-by-term sum of Proposition 8 with the
   count of Proposition 7b (the tight one, the default of PLAN 1F.7), with the lower bound of the valuation of `z`
   taken from the residue of `z` when `z = a^(p-1) - 1` (F5). No binary splitting, no rectangular splitting.
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
