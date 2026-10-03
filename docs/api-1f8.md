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

## Log on ideles (lane f-slice11, slice A)

This section implements the choices in the assigned brief, to be recorded as N-D17.
The design docs/design/idele-log.md IL1-IL8 is unchanged. The earlier scope paragraph
predates this section. Log and log_abs return an adele. A named-place call returns an sball.
No public function claims that an additive hull is the exact local component of an idele.

### G7 (conservative image and real coordinate)

adf_idele_Log and adf_idele_log_abs return (J ; 0 + 4 Zhat). The real ball J is identical
to adf_sball_Log_at or adf_sball_log_abs_at at infinity on I. At a prime, both functions
enclose the finite Log of every point. The finite answer is constant even for exact units.

Proof.

1. The idele predicate makes I finite and excludes zero. At infinity the real-place API
   applies Log to positive I, or log_abs to either sign. Its stored ball is copied unchanged.
2. IL2-IL4 put each finite value in p^d Z_p. IL5 identifies its integral tuple with a finite
   adele and proves inclusion in 4 Zhat. Its canonical global triple is (0,4,1).
3. The real and finite coordinates are independent. Their product encloses the whole image.
   The result can contain real zero, which the adele predicate permits.
4. Only the real part can fail. The real API's statuses and place are retained. Excess real
   precision is LIMIT at infinity before allocation or invariant checks. A temporary is
   swapped into y only on OK. No cross-type alias is permitted by conventions 4.1.

Check: conservative_Log, real_selection, slice_A_status_and_limits and the driver and Julia calls.

### G8 (private descriptor and local cases)

At p the descriptor stores exact=(M=0), m=v_p(r), k=v_p(M) for M>0, r'=r/p^m and
K=N for exact input, min(N,max(k,d)) otherwise. It never stores an additive shell as an input
ball and never forms m+k. adf_idele_Log_at returns the IL2-IL4 image enclosed at K.
adf_idele_log_abs_at is the real-only function; at p its status is UNSUPPORTED.

Proof.

1. Removing p from the numerator and denominator gives positive coprime p-free integers a,b.
   Their two nonnegative valuations fit slong, and their difference also fits slong. The
   ratio a/b is r'. fmpz_remove has this contract in refs/src/flint-3.0.1/fmpz.rst:1142-1149.
2. IL1 gives the exact point for M=0, the restricted coset for k>=1, and the unrestricted
   shell for k=0. At 2 the stored k=1 is that same shell. IL2-IL4 give the descriptor's E.
   This handles the valid stored (1,2) pair without normalising or rejecting it.
3. Exact r'=1 gives exact local zero for either sign, independent of N (IL4). No other
   branch returns an exact value. Unrestricted input gives centre zero; at 2 k=1 does too.
4. If K<=d all Log values agree with zero modulo p^K. A rational centre equal to 1 has zero
   Log at every K. These branches form no modular power. They still return a finite ball.
5. Otherwise K>d. Form q=p^K and the unit residue A=a c b^(-1) mod q. The inverse exists
   because b is p-free. A is nonzero and prime to p. It defines an exact rational unit.
6. If t is the true rational centre r'c, then t/A belongs to 1+p^K Z_p. Lemma 9 and
   Proposition 11 give Log(t)-Log(A) in p^K Z_p. Thus any higher digits supplied by the
   integer representative A give the correct centre modulo p^K. Call adf_lball_Log at K.
7. The existing local evaluator returns a ball at K or exact zero for this representative.
   Exact zero is replaced by 0+p^K Z_p. This prevents a compact representative equal to
   1 from accidentally turning an inexact input into a singleton. For exact input with
   r'!=1 the value is also a ball, even if its compact representative is torsion.
8. For M>0 the image is Log(t)+p^E Z_p. If K=E the result is exactly that image; if K<E
   it is its unique enclosing ball of exponent K. Exact input is enclosed at N (IL8).

Check: local_selection compares all 6784 selected rows, including the eight design witnesses,
both exact signs and
positive/negative content valuations. Its 20 complete H=5 images compare membership in both
directions. large_prime_routes compares a restricted input at N=3, additivity at N=3 and the
witness Log(1+p)=p mod p^2 at p=65537 and p=2^64-59. There is no enumeration at these primes.

### G9 (working precision, limits and preservation)

The wrapper bounds |m| and the resulting |K| by ADF_LBALL_EXP_MAX. The exact-zero _at
shortcut ignores N after checking m. Required compact and working powers obey N-D7.

Proof.

1. The descriptor uses comparisons and max/min; it never adds m+k or two arbitrary signed
   exponents. Its valuation subtraction is justified in G8 step 1. Checking K with lower
   and upper comparisons avoids negating WORD_MIN. k need not be bounded just to cap it.
2. Before the compact power is formed, K<=ADF_LBALL_BITS_MAX/bits(p) is required. The
   division expresses the product bound without overflowing slong. It precedes that power.
3. At the compact exact unit the existing local Log evaluator computes its actual truncation
   degree and W=K+floor(log_p(T)) as in F5-F6 and Proposition 8. It tests W bits(p) before
   forming the working power. Agreement modulo K, proved in G8, makes this higher working
   precision valid even though the arbitrary compact lift differs at higher digits.
4. No full input ball or power p^k is constructed. An unrestricted prime requires only
   factor removals, comparisons and a zero-centred finite output, even for very large N.
5. The named-place result contains only that place. A temporary sball is committed on OK;
   every failure retains y and names v. Reusing an initialised output is valid; input/output
   overlap is forbidden because their types differ. NULL where is accepted.
6. Infinity calls the existing real-place API, with the N-D8 precision check first. At a
   prime N is passed unchanged. log_abs_at returns UNSUPPORTED there, independent of N.

Check: slice_A_status_and_limits tests exponent and compact-power refusals, a working-power
refusal, exact zero with WORD_MIN/WORD_MAX N, huge N with small E, and a 100000-bit stored
modulus with N=4. Failure checks compare every field and the struct bytes. Finite valid input
cannot induce DOMAIN or NOT_DETERMINED. The real evaluator's non-finite failure is tested
by an injected scratch evaluator, recorded in the lane report.

Sources pending inherited from the design: the conventional names and normalisation of Iwasawa
Log and Teichmueller, and the real analytic facts of functions.md Lemma 2. The finite proofs use
the defined series and the stepwise proofs. No nonzero rational-value theorem is required.

### G10 (bounded named intersections and exact-zero rounding)

adf_idele_Log_refine and adf_idele_log_abs_refine return (J ; a + R Zhat), exactly the
intersection C_S of IL5 for the rounded local enclosures. At p in S, K=min(N,E) for
finite M, K=N for exact M=0, and L=max(K,beta_p), beta_2=2 and beta_p=0 at odd p.
An exact local zero is rounded before this intersection. The global result remains
inside 4 Zhat, including for N<=0 and an empty list.

Proof.

1. IL2-IL4 and G8 give the exact local images and their unique enclosing balls at K.
   At M=0 a singleton zero has no positive-radius global finite-ball representation
   as a condition at only one prime. Replace it by 0+p^N Z_p as IL5 step 8 prescribes.
2. Each exact image belongs to p^d Z_p, hence to p^beta_p Z_p. If K<=beta_p, its enclosing
   ball contains the baseline factor, and the intersection leaves that factor unchanged.
   Otherwise its centre is integral and gives the stronger congruence modulo p^K.
3. Begin with a=0, R=4. At 2 with L>2 replace this baseline by b+2^L Z_2; its centre b
   is divisible by 4. At L<=2 retain the baseline. The sorted list puts 2 first.
4. At each odd prime with L>0 impose z=b modulo p^L. The current modulus has only
   distinct previously named primes and 2, so it is coprime to p^L. Integer CRT gives
   one congruence class modulo their product. Its nonnegative representative is unique
   (refs/src/baker-padic/padicnotes.txt:374-387; FLINT fmpz.rst:1292-1304).
5. At odd L=0 no congruence is imposed. Induction gives R=2^L_2 product_(odd p in S) p^L_p,
   with L_2=2 if 2 is absent or less refined, and 0<=a<R. Both directions of CRT membership
   hold. Thus the finite output is precisely C_S, with canonical global triple (a,R,1).
6. In particular a and R are divisible by 4. At unlisted odd primes R is a unit and a is
   integral, so the output factor is Z_p. This is the advertised enclosure, without claiming
   the smaller p Z_p condition there. n=0 leaves exactly the conservative result.
7. The real ball is G7's ball, independent of N and the prime list. The product therefore
   encloses the whole idele image. log_abs_refine changes only the real evaluator.

Check: refinement_selection reads all 280 CRT rows, compares every global triple, projects
525 named factors and the unlisted factor at 11, and compares 73056 memberships over two
full periods for R<=4096. Larger rows are triple and projection checks. Negative real balls
use log_abs_refine with the same finite result. refinement_witness demands (12,36,1).

### G11 (preflight bounds and canonical failure reports)

The prime list has at most 65536 entries. The conservative aggregate bound is
sum L_p bits(p)<=2^26, including the baseline 2^2. The preflight and actual evaluators
preserve the maximum/canonical-place status contract.

Proof.

1. Excessive real prec is LIMIT at infinity first. Negative n is DOMAIN with no place;
   n>65536 is LIMIT with no place. Both length checks precede array allocation. A sorted
   copy then rejects infinity or the first canonical repeated prime, before evaluating x.
2. The bounded copy uses at most 65536*sizeof(adf_place_t) bytes. Its entries are borrowed
   from valid place handles. The sorted copy is owned for the call and freed on every path.
3. The descriptor checks |m| and the rounded K, also for exact zero. A necessary compact
   power is checked by K<=2^26/bits(p), before forming it. Centre-free branches avoid it.
   This distinguishes exact-zero _at from finite rounding in _refine at extreme N.
4. Known local refusals precede the aggregate test. If one is found, earlier sorted primes
   are evaluated to detect an earlier actual working-power LIMIT as well. All their compact
   powers already passed preflight. This preserves the first-prime tie rule. LIMIT outranks
   every possible real status once the real precision check has passed.
5. Charge the baseline by 2 bits(2)=4. For named 2 charge only (L_2-2) bits(2) in addition;
   for odd p charge L_p bits(p). Every charge is nonnegative. Compare L against the remaining
   budget divided by bits(p) before multiplying. Once a charge fails, keep the failure flag
   and continue the local preflight. All additions that are made remain within the bound.
6. If no local preflight refusal exists and the aggregate exceeds its bound, return LIMIT
   with where untouched, before evaluations and all CRT powers. This is a refusal of the
   combined modulus. The design does not specify its order relative to an uncomputed local
   working-power refusal. The chosen order prevents work after a known aggregate refusal.
7. On passing preflight, evaluate real and sorted local components. A local working-power
   LIMIT still dominates real DOMAIN or NOT_DETERMINED. No finite status exceeds LIMIT;
   after the first finite LIMIT the remaining primes cannot change the canonical report.
8. Every CRT power and product is bounded: log2(R)<=sum L_p log2(p)<sum L_p bits(p).
   The larger conservative bit sum was checked before those powers. G9's local working bound
   still applies separately, because a centre may need W>K. No m+k is ever constructed.
9. Real DOMAIN/NOT_DETERMINED with no finite failure names infinity. Success leaves where
   untouched. Whole-modulus failure leaves it untouched. All value writes are a final swap
   after both components succeed. The other output and input overlap rules are unchanged.

Check: refinement_status_and_limits includes all shape errors, canonical repeats, the length
ceiling, exact-zero rounding outside the exponent limit, positive exponent boundary versus
aggregate refusal, local compact and working bounds, aggregate size, negative-real/prime
precedence, earlier working/later compact ties, and real prec at its ceiling. Outputs are
compared by fields and struct bytes on every failure, including NULL where calls.

### G12 (implementation choices and proof scope)

The six functions are proved by G7-G11 and IL1-IL8. The descriptor is private. Compact
centres use only p^K; working precision is delegated to the proved local Log evaluator.
The list and CRT limits follow the brief's selected recommendations. The implementation
uses no copied helper from gfunc.c and adds no public local-component function.

The design leaves the following ordering choice implicit: known local exponent/compact
refusals precede aggregate refusal; an aggregate refusal precedes uncomputed actual local
working-power evaluation. The alternative is to evaluate every local centre before checking
the aggregate, which can do substantial work for a request whose combined modulus is refused.
If a known local refusal is present, earlier actual local refusals are still checked to keep
canonical place ties. The header states this choice.

The driver uses space-separated primes, matching project, with none for the empty list.
Comma-separated primes would require a second place-list grammar. The setting prec supplies
both finite N and real bits, while the library keeps the arguments separate. The spelling
logabs is an alias for log_abs, as requested in the brief. Only idele operands are accepted
by the new driver commands; the existing lower-case log_at keeps its additive-series meaning.

Avoidable costs: descriptor removals are repeated in preflight and evaluation; real wrapping
copies the input/output balls; each prime has an independent local evaluation and sequential
CRT step. No optimisation or cost bound beyond the explicit resource limits is claimed.
The lane report records mutation survivors and the full-INV/LeakSanitizer environment blocks.
