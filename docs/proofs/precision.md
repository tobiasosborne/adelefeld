# Proofs of the arithmetic rules for finite balls

Status: proofs taken from the design review of 2026-09-27 (findings M1, M2, M3, D1), rewritten here stepwise. They
have been read by us and checked numerically by `proto/precision_rules.py`; they have not yet been reviewed by a
third party (work package 0.3).

Notation. `Zhat` is the ring of profinite integers. A finite ball is `a + N Zhat`, `a` rational, `N` a rational,
`N >= 0`. For rationals `x_1, ..., x_k`, `gcd(x_1, ..., x_k)` is the non-negative generator of the subgroup of `Q`
they generate; it is 0 exactly when all are 0.

## Lemma 1. `R Zhat ∩ Q = R Z` for a positive rational `R`.

Dividing by `R`, it suffices to show `Zhat ∩ Q = Z`. A rational number in `Zhat` is a p-adic integer for every
prime `p`, so no prime divides its denominator, so it is an integer.

Consequence: for rationals `x` and `R > 0`, `x` lies in `R Zhat` exactly when `x/R` is an integer.

## Lemma 2. `N Zhat + M Zhat = gcd(N, M) Zhat`.

Let `G = gcd(N, M)`. If `G = 0` both sides are `{0}`. Otherwise `N/G` and `M/G` are integers with no common factor
(if one of `N`, `M` is 0 the other equals `G`). By Bezout there are integers `x, y` with `x N + y M = G`, so
`G Zhat` is inside the left side. Conversely `N` and `M` are integer multiples of `G`, so the left side is inside
`G Zhat`.

## Proposition 1 (sum).

    (a + N Zhat) + (b + M Zhat)  =  (a + b) + gcd(N, M) Zhat.

The left side is `(a + b) + (N Zhat + M Zhat)`; apply Lemma 2. The result is the exact set of sums, hence the
smallest ball containing them.

## Proposition 2 (product).

Let `G = gcd(a M, b N, N M)`. Then every product of an element of `a + N Zhat` with an element of `b + M Zhat` lies
in `a b + G Zhat`, and no ball with a radius `R` such that `R Zhat` is properly inside `G Zhat` contains all of them.

*Enclosure.* For `u, v` in `Zhat`,

    (a + N u)(b + M v) - a b  =  a M v + b N u + N M u v.

Each of the three coefficients is an integer multiple of `G`, so the right side lies in `G Zhat`.

*Tightness.* Take `(u, v) = (0,0), (0,1), (1,0), (1,1)`. The differences from `a b` are `0`, `a M`, `b N`,
`a M + b N + N M`. A ball `c + R Zhat` that contains the four products contains `a b`, so it equals
`a b + R Zhat`, and `R Zhat` contains `a M`, `b N` and `N M`. By Lemma 1 these three are integer multiples of `R`
(if `R = 0` they vanish), hence so is `G`. So `G Zhat` is inside `R Zhat`.

If `G = 0` all three coefficients vanish and the product set is the single point `a b`.

No sign or positivity of `a`, `b` was used; rational radii are covered by the definition of `gcd`.

*Remark.* "Tight" refers to independent inputs. It says nothing about expressions in which the same unknown occurs
twice.

## Proposition 3 (set predicates). For `N, M > 0`:

1. `a + N Zhat = b + M Zhat` exactly when `N = M` and `(a - b)/N` is an integer.
2. The two balls meet exactly when `(a - b)/gcd(N, M)` is an integer.
3. `a + N Zhat` is inside `b + M Zhat` exactly when `N/M` and `(a - b)/M` are integers.

*Proof.* (2) The balls meet when `a - b` lies in `N Zhat + M Zhat = gcd(N, M) Zhat` (Lemma 2); apply Lemma 1.
(3) If the first is inside the second, then `a` is in the second, so `(a - b)/M` is an integer, and
`N Zhat` is inside `M Zhat`, so `N` is in `M Zhat`, so `N/M` is an integer. The converse is immediate. (1) follows
from (3) applied in both directions: `N/M` and `M/N` both positive integers gives `N = M`.

## Proposition 4 (every basic neighbourhood is one ball).

Let `S` be a finite set of primes, and for each `p` in `S` let a ball `c_p + p^(n_p) Z_p` in `Q_p` be given; at the
other primes take `Z_p`. Then the product of these sets is `a + N Zhat` with `N = product of p^(n_p)` and a rational
`a`.

*Proof.* Choose an integer `d`, a product of powers of primes in `S`, such that `d c_p` and `d p^(n_p)` are p-adic
integers for every `p` in `S`. For each `p` in `S` choose an integer `A_p` congruent to `d c_p` modulo
`d p^(n_p) Z_p`. By the Chinese remainder theorem there is one integer `A` congruent to `A_p` modulo the power of
`p` in question for all `p` in `S`. Put `a = A/d`. At `p` in `S`, `a` lies in the given ball. At the other primes
`d` is a unit, so `a` is a p-adic integer. Hence the product set is `a + N Zhat`.

## Proposition 5 (scaled residues).

Let `K >= 1` be an integer, `s, t` positive rationals, `u, v` integers, `x = s (u + K Zhat)`, `y = t (v + K Zhat)`.
Let `g = gcd(s, t)`, `A = s/g`, `B = t/g`.

1. `x + y = g ((A u + B v) + K Zhat)`.
2. `x y` is inside `s t (u v + K Zhat)`.

*Proof.* (1) `x + y = g (A u + B v) + g (A K Zhat + B K Zhat)`, and `gcd(A K, B K) = K` since `gcd(A, B) = 1`;
apply Lemma 2. (2) By Proposition 2 the tight radius of `x y` is `gcd(s u t K, t v s K, s K t K) = s t K gcd(u, v, K)`,
an integer multiple of `s t K`; apply Proposition 3(3).

## Counterexample (a fixed radius is not a policy).

`1 + 2 Zhat` multiplied by the exact rational `1/2` is `1/2 + Zhat` (Proposition 2 with `N = 0` for the scalar).
The ball `1/2 + 2 Zhat` does not contain `3/2`, which is in `1/2 + Zhat`. So replacing the radius of a result by a
fixed radius 2 can exclude true values.

## Still to be proved here (work package 0.3)

Rules for exact zero and exact scalars in the scaled policy; conversion between tight and scaled values and between
contexts; the unit-coset statements of `SPEC.md` section 5; the splitting of a fractional radius (section 6); the
weighted finite Fourier transform (section 7); the functional equation with the chosen signs (section 8).
