# Slice 1F.8: functions at all places

Lane f-slice10, 2026-10-03. The contract is the comment block of each declaration in `include/adelefeld/gfunc.h`.
The implementation is `src/gfunc.c`, the driver commands are in `tools/adf/adf.c`. This document adds the
statements G1 to G4 (the root at all places) and G5, G6 (the five series at all places), in the style of
`docs/api-1f6.md`. It does not change SPEC 9.3.

Sources used, all on disk: `docs/proofs/functions.md` Definition 1 (line 11), Proposition 4 (line 92), Proposition
6 (line 140), Proposition 12 (line 375), Proposition 13 (line 410), Proposition 14 (line 446), Proposition 16 (line
538), Proposition 22 (line 725); `docs/proofs/ideles.md` (the set of an idele, through `include/adelefeld/idele.h`);
`docs/api-1f5.md` R3 (lines 109-129, integer roots of large degree); `docs/conventions.md` 3.1, 3.3 (lines
234-247); `refs/src/flint-3.0.1/fmpz.rst:983-988` (`fmpz_root`). Notation: `c = 1` for odd `p`, `c = 2` at 2;
`A_f` the finite adeles; a rational `a = A/B` in lowest terms with `B > 0`.

## Interface and decisions (proposed as N-D16)

| Operation | Result |
|---|---|
| `adf_rat_root(y, where, a, n, sign)` | the rational `n`-th root of `a` on the branch `sign`, or `DOMAIN` |
| `adf_adele_root(y, where, x, n, sign, prec)` | `(J ; rho)`: the real root of the real ball, the rational root of the exact finite part |
| `adf_idele_root(y, where, x, n, sign, prec)` | `(J, |rho|, [sign rho])` for an exact unit; `NOT_DETERMINED` for a unit of finite precision |
| driver `root X with N [with SIGN]` | the same, one line |

1. Types. An `adf_rat`, an `adf_adele` with an exact finite part and ANY real ball (the coordinates are independent,
   `adele.h:12`), an `adf_idele`. Alternatives (draft, question 1): the rational alone; the adele alone. The adele
   form is needed for SPEC 9.3.1 ("an adele or idele"); the rational form is the plain contract of 9.3.3.
2. The branch. A parameter `sign` in `{+1, -1}`; `+1`: the non-negative root for even `n`, the only root for odd
   `n`; `-1`: the non-positive root for even `n`, in every coordinate (the same branch at all places). For odd `n`
   the value `-1` names no branch and is `DOMAIN`, as `lroot.h` treats a seed that names no branch; the exact 0 of
   the type (the rational 0, the adele `(0 ; 0)`) has the root 0 for either sign. Any other value is `DOMAIN` for
   `n >= 2`. Degree 1 ignores `sign`, as `lroot.h` ignores the seed. Alternatives (draft, question 5): a function
   with two outputs; a second function for the negative root.
3. No rational root: `DOMAIN` (G2: then no adelic root exists, so every point of the input is outside the domain).
   `where` is the real place when the real coordinate fails (even `n`, a negative rational or a real ball certified
   negative); when only the finite part fails, `where` is untouched: the first failing prime cannot be named
   without a factorisation (2 and 7 fail first at 2 for `n = 2`, 17 first at 3), and conventions 3.1 reports a
   place "where there is one". Alternatives: `NO_SOLUTION` (conventions 3.1 value 5; its class in 3.2 is solvers
   and reconstruction, not functions); the first failing prime by a search (a factorisation in the worst case).
4. A finite part that is not exactly a rational (a positive radius, or the local backend): `NOT_DETERMINED`,
   without examining the primes of the modulus. For `2 + 4 Zhat` and `n = 2` a look at 2 would prove `DOMAIN`
   (every point has `v_2 = 1`); the header says so.
5. Ideles. A unit coset of modulus `N >= 1`: `NOT_DETERMINED` (SPEC 9.3.3 line 637, Proposition 16 steps 1-2). An
   exact unit: the rational contract (G4). Alternative (the literal text of SPEC 9.3.3 and PLAN 1F.8):
   `NOT_DETERMINED` for every idele with `n >= 2`. G4 proves the exact case; the literal text speaks of "an idele
   of finite precision".
6. Combination: conventions 3.3, the maximum, the real place first. So an idele with a negative real ball and even
   `n` is `DOMAIN` at the real place also when its unit is inexact (the maximum of `DOMAIN` and `NOT_DETERMINED`).
7. Degree 0: `DOMAIN`, `where` untouched (as `adf_real_root` and `lroot.h`). `prec` above `ADF_REAL_PREC_MAX`:
   `LIMIT`, `where` = the real place, before everything else (N-D8). `n = 1`: the identity, an exact copy (the real
   ball is not rounded to `prec`).

## G1 (the rational root by two integer roots)

Let `a = A/B != 0` in lowest terms, `B > 0`, and `n >= 2`.
(a) `a` has a rational `n`-th root exactly when `|A| = s^n` and `B = t^n` for integers `s, t >= 1` and (`n` odd or
    `a > 0`). The rational roots are then `sign(a) s/t` for odd `n` and `s/t`, `-s/t` for even `n`, each in lowest
    terms.
(b) For an integer `q >= 2` and `n >= bits(q)`, `q` is not an `n`-th power. 0 and 1 are `n`-th powers for every
    `n >= 1`.
(c) The function computes (a) with (b) first and `fmpz_root` only for `n < bits(q)`, a degree that fits an `slong`.

Proof.

1. (a) If `(C/D)^n = A/B` with `C/D` in lowest terms, `D > 0`, then `C^n/D^n` is in lowest terms too (a prime
   dividing `C^n` and `D^n` divides `C` and `D`), with a positive denominator; the reduced form of a rational is
   unique, hence `B = D^n` and `A = C^n`. So `|A| = |C|^n` and `B = D^n`. For even `n`, `A = C^n >= 0`, and
   `A != 0`, so `a > 0`. Conversely, if `|A| = s^n`, `B = t^n`, then `(s/t)^n = |a|`; for odd `n`
   `(sign(a) s/t)^n = a`, for even `n` and `a > 0` both `(s/t)^n` and `(-s/t)^n` equal `a`. `gcd(s, t) = 1`
   because a common prime would divide `|A|` and `B`. By Proposition 14 (functions.md:446) these are all the real
   roots, so all the rational ones. This is R3 steps 1-2 (`docs/api-1f5.md:117-122`) with `p^m` removed.
2. (b) If `q = r^n` with an integer `r`, then `|r| >= 2`, not `r >= 2`: `|r| <= 1` would give `q <= 1`, but
   `q >= 2`; the sign of `r` is free (`q = 4`, `n = 2`, `r = -2` has `q = r^n` and `r < 2`). Finding F4 of
   `docs/reviews/f1/review-gfunc.md`: this step as it stood said `r >= 2`, which is false for that input. With
   `|r| >= 2` the two conclusions are unchanged: `q = |r|^n >= 2^n` and `bits(q) >= n + 1`
   (R3 step 4, `api-1f5.md:124-126`).
3. (c) `fmpz_root(r, f, n)` "returns 1 if the root was exact" and requires `n > 0` and `f >= 0` for even `n`
   (`fmpz.rst:983-988`); it is called with `f = |A|` or `B`, both `>= 2`, and `2 <= n < bits(f) <= WORD_MAX`.
   What is on disk about the two ends of that chain: `fmpz_bits` returns `flint_bitcnt_t` and "the number of bits
   required to store the absolute value of `f`" (`fmpz.rst:605-608`); `flint_bitcnt_t` is `ulong`
   (`refs/src/flint-src-3.0.1/flint.h.in:112`) and "a bit offset within an array of limbs" (`flint.rst:110-111`);
   `WORD_MAX` is `LLONG_MAX` (`flint.h.in:176`); the limbs of an `fmpz` are allocated through `flint_malloc`,
   which wraps the system `malloc` (`memory.rst:16-21`) with a `size_t` argument (`flint.rst:121`). No file
   under `refs/` gives a bound on the number of limbs of an `fmpz`, so the step
   `[source pending: explicit FLINT/GMP representation bound implying fmpz_bits(f) <= WORD_MAX on this target]`
   is not closed here; it is the same source as the review lists for G1(c). The cast `(slong) n` is therefore
   proved safe under that bound only, and `n` is read before the cast.

Check: `rat_root_vectors` (5814 rows of `proto/gfunc_checks.py`: two independent integer roots, Newton and the
bisection of `proto/lpow_checks.py:70`; for `|A|, B <= 2^20` also the valuation criterion), `rat_root_named_cases`.

## G2 (existence at every place, and at every finite place)

Let `a != 0` be rational and `n >= 2`.
(a) `a` has an `n`-th root in `Q_p` for every prime `p` (that is, in `A_f`) exactly when `a` has a rational `n`-th
    root. The same holds with the real place added.
(b) If `n` is even, `a < 0` and `|a|` is a rational `n`-th power, then `a` has no `n`-th root in `Q_3` (and none
    in `R`).
(c) Hence, when `a` has no rational `n`-th root, the adele with finite coordinate `a` (any real coordinate) has no
    `n`-th root in `A`, and the idele with finite part `a` has none in the ideles: every point of the input lies
    outside the domain, which is `DOMAIN` (conventions 3.1).

Proof.

1. A rational root embeds at every place, which proves "if" in (a).
2. "Only if": write `a = epsilon prod p^(e_p)` (finitely many `e_p != 0`, `epsilon = +-1`). A root in `Q_p` forces
   `n | e_p` (Proposition 13, functions.md:410, first condition). So `|a| = r^n` with `r = prod p^(e_p/n)` a
   positive
   rational: Proposition 16 step 3 (functions.md:562-564). If `n` is odd, `a = (epsilon r)^n`. If `n` is even and
   `a > 0`, `a = r^n`. There remains `n` even, `a = -r^n`: by (b) there is no root in `Q_3`, so this case does not
   occur under the hypothesis of (a).
3. (b) If `b^n = -r^n` in `Q_3` with `r` rational, then `(b/r)^n = -1`, so `z = (b/r)^(n/2)` satisfies `z^2 = -1`
   in `Q_3`. But `-1` is a unit with residue 2 modulo 3, which is not a square modulo 3 (the squares modulo 3 are
   0 and 1), so `-1` is not a square in `Q_3` (Proposition 13, functions.md:423-424: the square criterion at odd
   `p`). With the real place (Proposition 14): `a < 0`, `n` even has no real root either.
4. (c) The adele `(t, a)` would need a root `(j, b)` with `b^n = a` in `A_f`, which (a) excludes; likewise the
   finite part of an idele. "Every point" is literal: the finite coordinate is a single point.

So the existence test "two integer roots and the sign" of G1 decides existence at all places and at the finite
places, as SPEC 9.3.3 lines 643-645 state ("a non-zero rational with an n-th root at every place has all valuations
divisible by n and the right sign, so it has a rational root"). The first failing place is not computed: for
`a = 2`, `n = 2` it is 2 (odd valuation); for `a = 17` it is 3 (17 = 1 mod 8 is a square in `Q_2`, Proposition 13
step 3; 17 = 2 mod 3 is not a square modulo 3); in general finding it needs the primes of `A B`.

Check: `rat_root_vectors` (the valuation criterion agrees with the integer test on every row with `|A|, B <= 2^20`,
in `proto/gfunc_checks.py`, as `check_global_roots` of `proto/functions_checks.py:585` does for `n <= 6`).

## G3 (the adele)

Let `x = (I ; F)` be an adele (`adele.h`: the set `I x F`), `n >= 2`, `sign` a valid selector (decision 2), and
`F = {q}` exact. Let `rho` be the rational root of `q` on `sign` (G1) and `J` the ball of `adf_real_root(I, n,
prec)` (`rfunc.h:79`), negated for even `n` and `sign = -1`.
(a) If `adf_real_root` returns `OK` and `rho` exists, then for every point `(t, q)` of `x`, the point
    `(s t^(1/n), rho)` lies in `y = (J ; rho)` and is an `n`-th root of `(t, q)`, where `t^(1/n)` is the real root
    of Proposition 14 and `s = sign` for even `n`, `s = 1` for odd `n`. For even `n` and `sign = -1` both
    coordinates are the non-positive roots, for `sign = +1` the non-negative ones: one branch at all places.
(b) The statuses: the real coordinate's status is that of `adf_real_root` (DOMAIN for even `n` and `I` negative,
    NOT_DETERMINED for even `n` and `I` meeting both signs); the finite coordinate's is DOMAIN when `rho` does not
    exist (G2 (c)), NOT_DETERMINED when `F` is not exact. The combined status is the maximum, reported at the real
    place when the real status is the maximum (conventions 3.3), else with `where` untouched.
(c) A finite part `F` of positive radius gives `NOT_DETERMINED`, which is correct whatever `F` is: it is never a
    proved status, and Proposition 22 (functions.md:739-740) requires every coordinate's domain condition to be
    certified ("A stored uncertain zero does not certify an exact zero").

Proof.

1. (a) By `rfunc.h` (the enclosure of the image of the whole ball), `adf_real_root(I)` contains `t^(1/n)` for every
   `t` in `I`; negation is exact in arb. `rho^n = q` by G1. Componentwise, `(s t^(1/n))^n = t` (for even `n`
   `s^n = 1`) and `rho^n = q`, so the point is a root in `A = R x A_f`.
2. (b) Each status is a statement about one coordinate of the set `I x F`: a coordinate with no root at any of its
   points makes every point of `x` fail, which is `DOMAIN`. The combination is conventions 3.3 applied to the two
   parts (the real place, then the finite part as a whole).
3. (c) A ball `F` of positive radius contains rationals of every sign class at an odd prime outside its modulus
   (the projection is `Z_p` there, Proposition 12 step 2, functions.md:395-398), so it is never certified to have
   a root; whether `DOMAIN` could be proved by a look at a prime of the modulus is not examined (decision 4).

Check: `adele_root_real_vectors` (558 real rows times 5 finite parts times 2 signs: the image `[lo, hi]` of
`proto/functions_checks.py` rf_image at 800 bits, the identity with `adf_real_root`, the exact finite root),
`adele_root_rat_vectors`, `adele_root_statuses`.

## G4 (the idele)

Let `x = (X, r, u)` be an idele (`idele.h`: the set of `(xi, r w)`, `xi` in `X`, `w` in `u`), `n >= 2`, `sign`
valid.
(a) If `u` has modulus `N >= 1`, no `n`-th root of `x` at all places can be certified: some point of `x` has no
    `n`-th root at some prime (Proposition 16 steps 1-2, functions.md:547-557). `NOT_DETERMINED` is returned (it is
    not `DOMAIN`: other points of `x`, for example `r * 1` with `r` an `n`-th power, may have roots).
(b) If `u = [c]` is exact, the finite part of every point of `x` is the rational `q = c r`. Its finite coordinate
    has an `n`-th root in `A_f` exactly when `q` has a rational root `rho` (G2 (a)); then
    `y = (J, |rho|, [sign(rho)])`, with `J` as in G3, is an idele that contains a root of every point of `x`, on the
    one branch `sign`; when `rho` does not exist, `DOMAIN` (G2 (c)).
(c) `J` must exclude 0 for `y` to be an idele (`idele.h`, the predicate); the real root of a ball that excludes 0
    excludes 0 as a set, but arb's enclosure need not: then `NOT_DETERMINED` at the real place (measured: the cube
    root of `2^999 +- 2^960` at 2 bits contains 0, FLINT 3.0.1).

Proof.

1. (a) Proposition 16 step 1 constructs a prime `q'` outside the modulus and the scale primes of the coset with an
   `ell`-th power map that is not surjective on `F_q'^x`; step 2 changes one unit coordinate of a point of the coset
   to a non-`n`-th power. That point has no root; the point with unit 1 at every prime outside `N` may have one.
2. (b) `[c]` is the single unit `c` (`ucoset.h`, `N = 0`), so the finite part is `r c`. `rho^n = q`, `|rho|` is a
   positive rational and `[sign(rho)]` an exact unit, so `(|rho| [sign rho])^n = |rho|^n [sign(rho)^n] = q` as a
   finite idele. The real coordinate as in G3 (a). For odd `n` the sign of `rho` is that of `q`; for even `n`
   `q > 0` is required, so `c = 1`, and `sign` sets the sign of `rho` and of `J`.
3. (c) `arb_is_nonzero` is the test of the predicate (`idele.h`); a ball that fails it is not stored.

Check: `idele_root_rat_vectors` (every non-zero rational of `rat_root.jsonl` as its idele), `idele_root_statuses`.

## Decisions of slice B (the five series; proposed as part of N-D16)

8. Type: an `adf_adele` `(I ; F)`; the all-places form applies only when `F` is exactly 0 (SPEC 9.3.2 lines
   593-595), with any real ball `I`. The driver also reads a rational `q` as `(q ; q)`.
9. The finite part of the result is the exact constant `f(0)`: 1 for `exp`, `cos`, `cosh`, 0 for `sin`, `sinh`, as
   at a prime (N-D12: "Only the exact 0 gives an exact constant"). Alternative: a ball of positive radius (SPEC
   9.3.1 line 573: "an exact input does not give an exactly representable output"); rejected, the argument 0 is the
   exception already made at a prime.
10. An exact `q != 0`: `DOMAIN` with `where` the first prime outside the local domain (G6). A finite part of
    positive radius or in the local backend: `NOT_DETERMINED`, no prime examined (`0 + 4 Zhat` cannot be decided; `1
    + 4 Zhat` could be proved `DOMAIN` at 2 and is not).
11. The real coordinate is computed through `adf_sball_exp_at` ... at the real place (`rfunc.h` has no
    `adf_real_sinh`, `adf_real_cosh`; the `_at` forms call `arb_sinh`, `arb_cosh`, `rfunc.h:143-150`), one path for
    the five functions.

## G5 (the five series at all places)

Let `f` be one of `exp`, `sin`, `sinh`, `cos`, `cosh`, and `x = (I ; F)` an adele.
(a) If `F = {0}`, then for every point `(t, 0)` of `x`, `f((t, 0)) = (f(t), f(0))` lies in `y = (J ; f(0))`, where
    `J` is the ball of the real function at the real place, and `f(0) = 1, 0, 0, 1, 1` respectively.
(b) If `F = {q}`, `q != 0`, every point of `x` is outside the domain `R x D`: `DOMAIN`.
(c) If `F` has positive radius, `NOT_DETERMINED` is a correct answer (never a false proof): the ball contains points
    outside `D` (Proposition 12 step 2) and it may contain 0, which is in `D` (`0 + 4 Zhat`).

Proof.

1. (a) The adele functions act coordinatewise (Proposition 22, functions.md:737-739; SPEC 9.3.1). At the real place
   `rfunc.h` encloses the image of the ball (`J` contains `f(t)` for every `t` in `I`). At every prime the series at
   0 take their constant terms (Proposition 6, functions.md:143-144: "At zero the factorial series take their
   constant terms"; Proposition 12 step 1, functions.md:394), 1 for `exp`, `cos`, `cosh` and 0 for `sin`, `sinh`
   (Definition 1, functions.md:22-26). The diagonal rational `f(0)` is that finite adele.
2. (b) Proposition 12 step 3 (functions.md:399-401): a non-zero rational fails the condition of `D` at an odd prime
   outside its numerator and denominator. G6 names the first failing prime.
3. (c) Proposition 12 step 2 (functions.md:395-398): the ball is not contained in `D`; `NOT_DETERMINED` claims
nothing. 
Check: `series_real_vectors` (327 real balls, image by mpmath intervals at 800 bits, identity with the function at
the real place, the radius bound from 24 bits), `series_statuses`.

## G6 (the first prime outside the domain, and the bound of the search)

Let `q = A/B != 0` in lowest terms. The local domain of the five series at `p` is `p^c Z_p` (Proposition 6,
functions.md:142).
(a) `v_2(q) >= 2` exactly when 4 divides `A`; at an odd prime `p`, `v_p(q) >= 1` exactly when `p` divides `A`.
(b) The first prime `p` with `q` outside `p^c Z_p` is 2 if 4 does not divide `A`, else the first odd prime that does
    not divide `A`; it exists.
(c) The search 3, 5, 7, ... ends after at most `floor(log_3 |A|) + 1` odd primes.
(d) The prime that the search returns is below `2^64`, so it fits a `ulong`, for every numerator `A` of at most
    `3.66 * 10^17` bits. Above that size the code has no guard; see the proof and the note after it.

Proof.

1. (a) If 2 divides `B`, `A` is odd and `v_2(q) < 0`; else `v_2(q) = v_2(A)`, which is `>= 2` exactly when `4 | A`.
   At odd `p` the same with `c = 1`.
2. (b) Order the primes; (a) decides each. Existence: by (c).
3. (c) If the first `k` odd primes all divide `A`, their product divides `|A|`; it is at least `3^k`, so
   `3^k <= |A|`, `k <= log_3 |A|`. So among the first `floor(log_3 |A|) + 1` odd primes one does not divide `A`.
4. (d) Let `B = bits(A)` and let `k` be the number of odd primes the search tries. By (c),
   `k <= floor(log_3 |A|) + 1`, and `|A| < 2^B`, so `k <= 0.6309297536 * B + 1` (`log_3 2 = 0.6309297535714574`,
   rounded up here).
   The last candidate is the k-th prime `p_k`. With
   `[source pending: the inequality p_k < 2 k ln k for k >= 3, for which no file under refs/ was found]`:
   `k -> 2 k ln k` is increasing, `2 * 230700252851841000 * ln(230700252851841000) = 18446744073709551582` and
   `2 * 230700252851841001 * ln(230700252851841001) = 18446744073709551664`, while `2^64 = 18446744073709551616`
   (the three numbers computed with 30 decimal digits), so `p_k < 2^64` for every `k <= 230700252851841000`, and
   hence for every `B` with `0.6309297536 * B + 1 <= 230700252851841000`, that is `B <= 365651249660515264` bits,
   about `4.57e16` bytes of numerator.
   Note on what is proved. No file under `refs/` bounds the memory of a machine below that size, and the library's
   own cap `ADF_LBALL_BITS_MAX = 67108864` (`lball.h:68`) bounds the bits of the powers `p^k` it forms, not the
   numerator of an exact rational, which `adf_fball` holds at any size that fits in memory. So (d) is a limit of
   the code, not a theorem about every numerator the library can hold; what the code does beyond it: nothing. The
   loop of `src/gfunc.c` calls `n_nextprime(p, 1)`, which "Assumes the result will fit in an `ulong`"
   (`ulong_extras.rst:688-692`), so beyond the limit that assumption fails. Before it is reached the search would
   try about `2.3e17` primes, each a division of the whole numerator by a word, so no machine finishes it.
   `[source pending: an explicit FLINT/GMP or platform bound on the number of limbs of an fmpz that would make
   (d) hold for every representable numerator instead of for every numerator below the stated size]`.

Check: `series_place_vectors` (988 rationals; the oracle `first_failing_prime` of `proto/gfunc_checks.py` tries the
primes up to 2000 with exact valuations, and asserts the bound (c) on every row; numerators `4 * 3 * 5 * ... * p_k`
up to `k = 59` make the C search walk 60 odd primes).

## Scope

G1 to G6 are proved here. Not done: `Log` on ideles at all places (the next lane; draft questions 8 and 9), the
rational power `x^(e/n)` of an exact rational at all places (draft question 10), the optional per-place variant of
SPEC 9.3.1. No source is pending.
