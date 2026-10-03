# Draft for the brief of lane f-slice10 (WP 1F.8, all-places forms): facts, a cut into slices, brief of slice 1

Written by a research assistant on 2026-10-03 for the orchestrator. Reading and grep only: nothing was built or run.
Every `file:line` below was opened in this session. Where a passage is silent, it is under "Open questions".

## 1. Facts

### 1.1 What the plan and the previous lane say

- `docs/PLAN.md:278` (row 1F.8, quoted in full in the task); `docs/PLAN.md:293`: "| 1F.8 | not started | all-places
  forms |". `HANDOFF.md:88-90`: 1F.8 is next, "the rational-root contract and the rational power of an exact
  rational at all places need only `lroot.h` and `lpow.h`".
- `lanes/f-slice9/result.md:203-206` (last section, "Proposed next slice"): "**1F.8, the all-places forms.** This
  slice gives the local rational power. The rational-root contract of SPEC 9.3.3 (Proposition 16), and its extension
  to `x^(e/n)` of an exact rational at all places (the rational branch, by integer root tests), need only this slice
  and `lroot.h`. The same holds for `NOT_DETERMINED` for ideles of degree >= 2." The extension to `x^(e/n)` is NOT
  in
  the text of row 1F.8; it is the lane's proposal.
- `docs/api-1f6.md:323-325` ("Scope"): "Not done: the real place of both operations (`UNSUPPORTED`), all-places
  forms
  (1F.8), the quasi-character (milestone 3)." `docs/api-1f5.md:291`: "No rational-power API ... or all-places root
  API
  is added."
- `lanes/f-review7/progress.md:10-15`: three MINOR findings on f-slice9 and f-repair4 (a stale header sentence in
  `lpow.h`; `powunit` LIMIT for a small result; a false sentence in `lanes/f-repair4/result.md`). Nothing in them
  bears on 1F.8. Repairs are pending (HANDOFF.md:80-90).

### 1.2 SPEC passages (docs/SPEC.md)

- 9.3.1, lines 557-573. Three forms. Line 565: "| `f(x)` with no places | `f` at all places at once | an adele or
  idele; requires that the whole input is certified to lie in the domain, otherwise a status |". Lines 568-569: "If
  one place fails, the function returns the status with that place and no value; an optional variant returns a
  per-place map". Lines 571-573: every function encloses the image of the whole ball; "an exact input does not give
  an exactly representable output".
- 9.3.2, lines 588-600 ("On all primes at once", "`Log` at all places"). Lines 588-596: the common domain of `exp`,
  `sin`, `cos`, `sinh`, `cosh` is `D = 4 Z_2 x product over odd p of p Z_p`; "`D` contains no finite ball of
  positive radius, and the only rational in it is 0. So with our types the all-places form applies only to a value
  whose finite part is exactly 0 (any real part)." Lines 598-600: "`Log` at all places: the finite image of any
  idele
  lies in `4 Zhat`, which is the conservative result; it is refined at finitely many named primes by the table
  above.
  The real coordinate needs a positive input, or the separately named `log_abs`." The table is lines 577-586; its
  `Log` row: image `Log(a) + p^(N-m) Z_p`, at 2 for `N - m >= 2`; for `N - m = 1` the image is `4 Z_2`.
- 9.3.3, lines 613-646 (roots). Lines 636-638: "**At all places.** A unit coset leaves every unit possible at the
  primes outside its modulus, and some of those units are not squares. So an idele of finite precision can never be
  certified to have a square root at all places; the all-places root of degree `n >= 2` returns `NOT_DETERMINED` for
  ideles (degree 1 is the identity). Use `root_at`." Lines 640-646: "For an exact rational the all-places root
  returns
  the **rational** root: for odd `n` the one real root, for even `n` the non-negative one (optionally both), found
  by
  testing numerator and denominator for n-th powers; no factorisation is needed. It does not list all adelic roots,
  and cannot: the rational 1 has the square roots `+1` or `-1` chosen freely at every place, continuum many. All
  branches are listed only over a finite named set of places. ... Exact 0 has the single root 0."
- 9.3.4, lines 648-657: four powers; item 2 "Rational powers, through the roots of 9.3.3 with their branches" (no
  all-places statement).
- 9.3.6, line 678: "| rational functions | denominator certified non-zero at the named places; at all places: exact
  non-zero
  rational or idele |".

### 1.3 Decisions of SPEC 15.4 (lines 911-933) that bear on 1F.8

- N-D7 (line 925): when `LIMIT` is returned; `LIMIT` never carries a value.
- N-D8 (line 926): `ADF_REAL_PREC_MAX = ADF_IDELE_PREC_MAX = 2097152`; a real `prec` above it is `LIMIT`, decided
  from `prec` alone before every other status.
- N-D9 (line 927): the series at a prime are the library's own; FLINT's `padic_exp`, `padic_log` are a second
  opinion.
- N-D10 (line 928): the `_at` forms on a partial ball; `prec` at a prime is the absolute precision `N`; a place that
  is not a place of the ball is `DOMAIN` with that place; driver `project`, `exp_at`, `log_at`.
- N-D12 (line 930): `sin`, `cos`, `sinh`, `cosh` at a prime, `K = min(N, E)`, "Only the exact 0 gives an exact
  constant (`sin 0 = sinh 0 = 0`, `cos 0 = cosh 0 = 1`)"; driver `sin_at` and so on.
- N-D13 (line 931): `lroot.h` branches, identifiers, "the exact rational root where one exists (integer root tests
  of the unit part)"; driver `roots_at`, `root_at`.
- N-D14 (line 932): `K = min(N, E)` for a ball input of a root.
- N-D15 (line 933): `lpow.h`, `powrat`, `powunit`; driver `powrat_at`, `powunit_at`.
- N-D1..N-D6, N-D11: not relevant (solvers, ball arithmetic, printer). There is no decision yet on the all-places
  forms: the lane will have to propose N-D16 (the orchestrator records it).

### 1.4 docs/proofs/functions.md

- Proposition 12 (line 375), "simultaneous finite domains and logarithm enclosure". Statement lines 377-387, key
  sentences: "No finite ball a+R Zhat, a rational and R a positive rational, is contained in D. The diagonal
  rational
  points of D consist of 0 alone. For the supported finite rational singletons and rational balls, therefore, only
  the exact finite part 0 certifies the simultaneous domain. The real coordinate may be arbitrary." (lines 382-385)
  and "For any idele, its finite Iwasawa Log belongs to D, hence to 4 Zhat." (line 386). Step 1 (line 394): "At
  finite
  argument 0 their outputs are respectively 1,0,0,1,1" (the order of Definition 1, line 11 on: exp, sin, sinh, cos,
  cosh). Step 4 (lines 402-406): each local `Log(x_p) = log(u)` lies in `p^c Z_p`, "at 2 it is divisible by 4, and
  at odd p, 4 Z_p = Z_p, so D is contained in 4 Zhat". Line 408: "Check: `check_global`. Used by: SPEC 9.3.2 and
  PLAN
  1F.8." Python check: `proto/functions_checks.py:433` (`check_global`).
- Proposition 14 (line 446), real roots: odd `n`: one real root with the sign of `a`; even `n`: negative `a` none,
  zero one, positive two; "Selecting the nonnegative root in the even case is a branch convention." (lines 447-449)
- Proposition 16 (line 538), "all places and the rational-root contract". Lines 540-545: "Let n >= 2. An ordinary
  idele unit coset of finite precision, with only finitely many restricted primes, cannot certify n-th-power
  membership at all places. ... Degree 1 is the identity. For an exact rational a and n >= 1, having an n-th root at
  every place is equivalent to having a rational n-th root. For a=0 the sole adelic root is zero. For a!=0 an
  exact-rational operation may therefore return the rational branch from Proposition 14. It must not claim to
  enumerate all adelic branches: 1 has continuum many adelic square roots." Step 4 (lines 565-568): "testing
  numerator and denominator separately for exact integer n-th powers gives the same criterion and needs no prime
  factorisation." Python check: `proto/functions_checks.py:585` (`check_global_roots`), which tests the criterion
  for `n` 1..6 and `a = num/den` by independent integer searches (lines 587-600), and the 256 sign tuples of 1.
- Proposition 22 (line 725): a named-place function is the function after projection; "An unqualified all-place map
  requires every coordinate's domain condition and an adelic output. A stored uncertain zero does not certify an
  exact zero." (lines 739-740) "In the adele ring, an element is invertible exactly when it is an idele."
  (lines 733-735)

### 1.5 docs/conventions.md

- Section 3.1, lines 169-192: statuses. `NOT_DETERMINED` (value 1) "Valid inputs do not determine the requested
  quantity"; `NOT_UNIT` 6; `DOMAIN` 7 "proved: every point of the input lies outside the domain ... Reported with
  the
  place, where there is one"; `UNSUPPORTED` 8; `LIMIT` 10.
- Section 3.2, line 221: "Functions at places (`_at`) and all-places functions (`SPEC.md` 9.3) | `OK`, `DOMAIN`
  (with place), `NOT_DETERMINED`, `NEEDS_SPLIT`, `UNSUPPORTED`, `LIMIT`".
- Section 3.3, lines 234-247: over several places the combined status is the maximum in the numeric order; the place
  is the first (canonical order) with that status; "`ADF_OK` exactly when every part is `ADF_OK`".
- Section 2.1, line 122: "Functions at named places end in `_at` (`adf_adele_exp_at`); the all-places form has no
  suffix (`SPEC.md` 9.3.1)."
- Section 5.5 (line 563) `adf_adele`; 5.7 (line 621) `adf_idele`, `adf_idclass`; 5.9 (line 693) `adf_sball`.

### 1.6 Public headers (include/adelefeld/)

- `adele.h` (216 lines): `adf_adele_struct {arb_t inf; adf_fball_struct fin}` (lines 53-57). The coordinates are
  independent (line 12). `adf_adele_init` (80), `adf_adele_set_rat(y, q, prec)` (107), `adf_adele_set_arb_fball(y,
  r,
  f)` (117, `int`), `adf_adele_get_real(r, x)` (123), `adf_adele_get_fin(f, x)` (132), `adf_adele_mul_rat` (158).
- `fball.h` (275): `adf_fball_is_exact(x)` line 154 ("1 if the radius is 0 (H = 0; a single rational) ... A local
  value is never exact"); `adf_fball_contains_rat(x, q)` line 256; `adf_fball_get_fmpz3(A, H, d, x)` line 161;
  `adf_fball_set_rat(x, q)` line 120; `adf_fball_zero`, `adf_fball_one` line 111. THERE IS NO function "finite part
  is
  exactly 0"; the test is `adf_fball_is_exact(&x->fin) && adf_fball_contains_rat(&x->fin, zero)` (with
  `adf_rat_zero`,
  `rat.h:65`), or `get_fmpz3` with `A = H = 0`.
- `rat.h` (126): `adf_rat_t`; `adf_rat_set_fmpz2(x, num, den)` line 79 (`int`, reduces); `adf_rat_set_fmpq` 85,
  `adf_rat_get_fmpq` 87; `adf_rat_zero`, `adf_rat_one` 65-66.
- `sball.h` (232): `adf_sball_project(y, where, x, places, n)` line 149 (adele to named places);
  `adf_sball_get_lball`
  170; `adf_sball_get_arb` 175; `adf_sball_set_arb_lballs(y, where, r, loc, n)` line 136; `ADF_REAL_PREC_MAX` line
  65.
- `place.h` (63): `adf_place_inf()` 28, `adf_place_prime(v, p)` 36 (`int`), `adf_place_is_archimedean` 40,
  `adf_place_prime_get` 44.
- `lball.h` (365): `adf_lball_set_rat(x, v, q)` 126, `adf_lball_set_rat_ball(x, v, c, N)` 136 (`c + p^N Z_p`),
  `adf_lball_set_fball(x, v, f)` 144, `adf_lball_is_exact` 152, `adf_lball_pow_si` 338.
- `lfunc.h` (149): `adf_lball_exp` 83, `log` 90, `Log` 99, `sin` 120, `cos` 128, `sinh` 135, `cosh` 143, all
  `(y, x, slong N)`. Exact 0 gives exact 1 for exp (lines 80-82: "The exact 0 gives the exact 1") and by N-D12 exact
  0 for
  sin, sinh and exact 1 for cos, cosh.
- `lroot.h` (77): `adf_lball_root_count` 51, `adf_lball_root_seed(y, x, n, seed, N)` 56, `adf_lball_sqrt_seed` 60,
  `adf_lball_roots` 72. The exact rational root of a local exact input is found by the STATIC function
  `integer_root` in `src/lroot.c:99-106` (not callable; `src/lroot.c` is read-only for a lane):
  `if (fmpz_is_one(a)) ...; if (n >= bits(a)) return 0; return fmpz_root(r, a, (slong)n);`. A new slice must write
  its
  own (n is `ulong`, FLINT's is `slong`).
- `lpow.h` (123): `adf_lball_powrat(y, x, e, n, seed, N)` line 74; `adf_lball_powunit(y, u, s, N)` line 117.
- `rfunc.h` (205): real functions on `arb`: `adf_real_exp` 73, `log` 74, `log_abs` 75, `sin` 76, `cos` 77, `sqrt`
  78,
  `adf_real_root(y, x, n, prec)` 79 (`ulong n`); there is NO `adf_real_sinh`/`cosh` (the `_at` form calls
  `arb_sinh`, `arb_cosh` directly, comments at lines 143-150). `_at` forms on `adf_sball`, all
  `(y, where, x, v, prec)`: `exp_at` 120, `log_at` 121, `Log_at` 122, `log_abs_at` 123, `sin_at` 130, `cos_at` 136,
  `sinh_at` 143, `cosh_at` 150, `sqrt_at` 151, `root_at` 152 (UNSUPPORTED at a prime), `root_seed_at` 162,
  `sqrt_seed_at` 168, `roots_at` 178, `powrat_at` 189, `powunit_at` 198. At the real place `prec` is bits; at a
  prime
  it is the absolute precision `N` (lines 91-96).
- `ucoset.h` (151): `adf_ucoset_get_fmpz2(c, N, x)` 99, `adf_ucoset_is_exact` 102, `adf_ucoset_one`,
  `adf_ucoset_minus_one` 94-95. `idele.h` (231): `adf_idele_set_rat(x, q, prec)` 135; `adf_idele_content(r, x)` 217;
  `adf_idele_get_unit(u, x)` 221; `adf_idele_get_real(r, x)` 213; `adf_idele_valuation_at` 181. Struct fields: real
  ball `inf`, content `r`, unit `u` (header comment lines 1-24). `idmap.h` (104): `adf_adele_set_idele(y, x)` 53
  (the smallest finite ball containing the idele: `r c' + r lcm(N,2) Zhat`), `adf_idele_set_adele(y, x)` 65.
- There is NO function that gives the local component of an idele as an `adf_lball`, none that applies a function
  to an idele, and no all-places function of any kind (grep of `include/adelefeld/` for "all places": only comments
  in
  `rfunc.h:111` and `lroot.h`). The hull of `idmap.h:53` is NOT usable for `Log` at a prime `p` not dividing `N`: it
  is a ball `r c' + r lcm(N,2) Zhat` that contains non-units at `p`, so `adf_lball_Log` would give `NOT_DETERMINED`
  (ball containing 0) where the true image is `p Z_p` (`4 Z_2`).
- `common.h`/`status.h`: status names `ADF_OK`, `ADF_NOT_DETERMINED`, ... (conventions 3.1).

### 1.7 Driver (tools/adf/adf.c, 2837 lines; tools/adf/README.md, 509 lines)

- Operations table `adf_drv_ops[]`, lines 187-229, one row `{ name, ADF_DRV_x, arity }`; enum lines 140-175. Rows of
  the
  `_at` family: `project` 196, `exp_at` 197, `log_at` 198, `sin_at`..`cosh_at` 199-202, `roots_at` 203, `root_at`
  204
  (arity 4: value, prime, degree, seed), `powrat_at` 205 (arity 4), `powunit_at` 206 (arity 3).
- Dispatch: line 2495-2497 sends every `_at` op to `adf_drv_places(out, op, l, st)` (defined line 1890). Inside it,
  X is
  read by the typed parser as a rational, a finite ball or an adele (rational converted to the adele `(q ; q)` at
  the
  setting `prec`), projected by `adf_sball_project` (line 2003), then `root_at`/`roots_at` call
  `adf_drv_root_result` (line 2006), `powrat_at` calls `adf_drv_powrat_result` (line 2011; its definition near line
  1862), `powunit_at` reads the second operand `S` (lines 2016-2023), the others call `adf_sball_exp_at` etc. (lines
  2026-2038) and print by `adf_drv_put_sball`. The exponent `E/N` is read by `adf_drv_ratexp` (line 1821).
- Ideles in the driver (the section after `adf_drv_places`, from about line 2050; README 310-342): `idele X` (arity
  1, rational to idele), `hull`, `hullsimple`,
  `unitof`, `class`, `inv`, `pow`, `norm`, `valuation`, `abs`. The value text of an idele is `(X ; r * [c mod N])`
  as in `tests/driver/f-cross.cmd:11` (`(5 ; 5 * [1])`); of a class `<1 ; [1]>`.
- NO command named `exp`, `log`, `Log`, `sin`, `cos`, `sinh`, `cosh`, `sqrt`, `root`, `ratroot` exists: the table
  has only the `_at` forms (grep of lines 187-229). The name `root` is free; so are the names without `_at`.
- README: the table of operations lines 52-90 (rows `exp_at`, `log_at` line 73, `powrat_at` line 75); sections "The
  commands at places" 198, "Local roots and their branches (1F.5)" 444, "Powers at a prime (1F.6)" 479-509.
- Golden files: `tests/driver/NAME.cmd` and `NAME.out`, picked up by the glob of `tests/test_driver.sh:72-73`;
  examples
  `tests/driver/pow-values.cmd` (each expected line explained in a comment), `pow-status.cmd`, `root-values.cmd`.

### 1.8 Julia

- Pattern: `tests/julia/lpow.jl` (header comment lines 1-9 with the expected values and where they come from;
  `ccall`
  helpers `place`, `LB`, `rat`, `exact`, `ball`, `powrat`, `powunit`; `@testset`). It is run by a line in
  `tests/test_julia.sh`: `timeout 60 "$JULIA" --startup-file=no tests/julia/lpow.jl "$so" || exit 1`
  (`tests/test_julia.sh:185`; the lines for `lfunc_trig.jl` and `lroot.jl` are 183-184). `tests/julia/f_at.jl` (line
  179)
  shows the sball ccall helpers. A new header must also be added to `include/adelefeld.h` (lines 25, 58-59 show how
  `lpow.h` is listed) and its functions are checked by `test_exports.sh` (all declared functions exported).

### 1.9 Python reference and oracles already on disk

- `proto/functions_checks.py:433` `check_global`, `:585` `check_global_roots`, `:1450` `rf_iroot(x, n)` (Newton,
  exact floor of the n-th root); `proto/lpow_checks.py:70` `iroot(a, n)` (bisection; returns None when inexact),
  `:160` `root_branch(p, n1, U, seed, h)`; `proto/precision_rules.py:279` `rational_root2(q)` (square roots only).
  `tests/ref/adfref/` has `local_ref.py`, `membership.py`, `policies.py`; none has `all_places`, `Zhat`-root or
  `ratroot` functions (grep for `all_places|rational_root|ratroot|iroot` found only the files above).
- The pattern of an oracle in exact integers with a stated precision: `lanes/f-review6/oracle.py` (cited by the
  f-slice9 brief) and `proto/lpow_checks.py` with vectors under `tests/ref/vectors/f-slice9/` (909 KB by
  `lanes/f-slice9/progress.md:13`).

### 1.10 FLINT 3.0.1 (refs/src/flint-3.0.1/; the .rst files are in the top directory, not under doc/source)

- `fmpz.rst:983` `int fmpz_root(fmpz_t r, const fmpz_t f, slong n)`: "Set r to the integer part of the n-th root of
  f.
  Requires that n > 0 and that if n is even then f be non-negative, otherwise an exception is raised. The function
  returns 1 if the root was exact, otherwise 0." (lines 983-988)
- `fmpz.rst:990` `int fmpz_is_perfect_power(fmpz_t root, const fmpz_t f)`: returns `k` or 0; "No guarantee is made
  about r
  or k being the smallest possible"; negative `f` permitted (lines 990-996). Not a direct n-th root test.
- `fmpz.rst:972` `fmpz_sqrtrem(f, r, g)` ("behaviour is undefined if f and r are aliases"); `:979` `fmpz_is_square`;
  `:966` `fmpz_sqrt`; `:913` `fmpz_pow_ui`; `:689` `fmpz_cmpabs`; `:715` `fmpz_is_pm1`.
- `fmpq.rst:132` `fmpq_is_zero`, `:136` `fmpq_is_one`, `:149` `fmpq_sgn`, `:111` `fmpq_neg`, `:185`
  `fmpq_set_fmpz_frac`, `:481` `fmpq_pow_si`. There is no `fmpq_root`.
- `arb.rst` (real part): `arb_root_ui` (line 979, cited in `rfunc.h`), `arb_exp` 1082, `arb_log` 1050,
  `arb_sin`/`arb_cos` 1101-1103, `arb_sinh` 1209, `arb_cosh` 1211 (all as cited in `rfunc.h`).

## 2. Open questions (alternatives given, not resolved)

1. **Which input types does an all-places function take?** SPEC 9.3.1 line 565: "an adele or idele". Prop 12 and
   SPEC 9.3.2 limit the five series to "finite part exactly 0" (an adele, since an idele has non-zero finite part),
   and
   `Log` to ideles. SPEC 9.3.3 mentions an "exact rational" and "ideles". Alternatives for the root: (a) input an
   `adf_rat` (pure exact integers), (b) an adele `(I ; q)` with exact finite part `q` and any real ball `I` (the two
   coordinates are independent, `adele.h:12`; output `(root of I ; rational root of q)`), (c) both. The text does
   not say.
2. **An exact idele and the root.** SPEC 9.3.3 line 637: "the all-places root of degree n >= 2 returns
   NOT_DETERMINED for ideles". SPEC 4.x and `ucoset.h:92-102` have the exact unit (modulus 0, `[1]`, `[-1]`), and
   `idele_set_rat` makes the idele of an exact rational (`idele.h:135`), which has a rational root when `q` is a
   perfect
   power. Prop 16 (line 540) says "An ordinary idele unit coset of finite precision ... cannot certify", which does
   not
   cover the exact unit. Alternatives: (a) `NOT_DETERMINED` for every idele of degree >= 2 (the literal text); (b)
   `NOT_DETERMINED` except for an idele with exact unit, where the rational root idele `(root X, r^(1/n), [sign])`
   is
   returned or `DOMAIN` when no rational root exists. PLAN 1F.8 repeats (a): "`NOT_DETERMINED` for roots of degree
   at
   least 2 of ideles".
3. **`DOMAIN` and its place for an exact rational without a rational root** (`2` with `n = 2`, `-4` with `n = 2`).
   Prop 16 says no adelic root exists, so `DOMAIN` is provable. Conventions 3.1 (line 183): "Reported with the
   place,
   where there is one". No place is named without factorising. Alternatives: `DOMAIN` with `where` untouched (as the
   root of degree 0, `rfunc.h`); `DOMAIN` with the real place when the sign is the cause (even `n`, `a < 0`) and
   untouched otherwise; `NO_SOLUTION` (conventions 3.1, value 5: "proved: no value satisfies the problem") instead
   of
   `DOMAIN`. SPEC 4.x gives `NO_SOLUTION` to reconstruction and solvers only (conventions 3.2 last rows).
4. **An adele with finite part of positive radius, or in the local backend** (`adf_fball_is_exact` is 0 for every
   local value, `fball.h:152-153`): `NOT_DETERMINED` (Prop 22, line 739: "A stored uncertain zero does not certify
   an
   exact zero"), or `UNIT_NOT_CERTIFIED`/`DOMAIN`? For the five series a ball `0 + R Zhat` contains 0 and points
   outside `D` (Prop 12 step 3, line 399); a ball not containing a point of `D` at all (it does not contain 0) is
   `DOMAIN` by the strict rule of conventions 3.1, but proving that every point lies outside `D` needs `R` and `a`
   (Prop 12 steps 2-3 give an odd prime `p` outside the support where the projection is `Z_p`, not a proof that NO
   point is
   in `D`). `NOT_DETERMINED` is the conservative answer. Not decided in the text.
5. **Both signs of an even root** ("both signs optional"). Alternatives: a parameter `sign` (+1 non-negative, -1
   non-positive) on a single-output function; a second function with two outputs and a length; always the
   non-negative
   root and a separate function for the negative one (as `adf_real_root` returns the non-negative one,
   `rfunc.h:79`). For `n` odd the root has the sign of `a` and `sign = -1` has no meaning (`DOMAIN`? the other root
   is not
   a root).
6. **Real part of the root of an adele when the finite part is a perfect power but the real ball is negative and `n`
   is
   even.** Independent coordinates give `DOMAIN` at the real place from `adf_real_root` (domain `t >= 0`) while the
   finite part is fine. The status and `where` follow conventions 3.3 (combined by maximum). Check the text accepts
   `DOMAIN` with `where` = the real place.
7. **The five series: output finite part.** With finite part exactly 0 the outputs are 1,0,0,1,1 exactly (Prop 12
   step 1; `lfunc.h` exact cases). SPEC 9.3.1 line 573: "an exact input does not give an exactly representable
   output", but SPEC 9.3.2 `lfunc.h` and N-D12 already return the exact constants at a prime. Alternative: return
   the finite part as the exact rational (consistent with N-D12), or as an `adf_fball` of radius > 0 (contradicts
   N-D12).
8. **`Log` on an idele: how is the local component built?** No header gives it (see 1.6). Alternatives: (a) a new
   function `adf_lball_set_idele_at` (or internal) that builds the ball `r c + p^(v_p(r)+k) Z_p` with `k = v_p(N)`
   and calls `adf_lball_Log`, and for `k = 0` returns the ball `p^c Z_p` centred at 0 (`c = 1` odd, `2` at 2; the
   whole
   image of `Log` on `Z_p^x`) without calling anything; (b) only the generic enclosure `4 Zhat` and `Log_at` through
   a
   hull (which is wrong for `p` not dividing `N`, see 1.6). Lemma 5 (`docs/proofs/ideles.md:92-110`) gives the local
   description `u_p = c modulo p^(a_p)`, `a_p = v_p(N)`. At 2 `adf_ucoset` has the factor-2 rule (`ideles.md:132`
   Lemma 7; `ucoset.h` normal form `N != 2 mod 4`): the image at 2 for `a_2 = 0` or `1` is `4 Z_2`.
9. **Result type of `Log` of an idele.** The finite image is an element of `4 Zhat`, a finite ball with centre 0 and
   radius 4 (an `adf_fball` `0 + 4 Zhat`); refinement "at named primes" gives local `adf_lball`s. Candidates: an
   `adf_adele` `(log of the real ball ; 0 + 4 Zhat)` for the unrefined form, and an `adf_sball` with the real
   component and the refined local components for the refined one (`_at` form on a set of places). The text does not
   say
   which type "an adele" carries when the real part uses `log` of the real ball of an idele that may be negative
   (the
   sign sits in the unit, `ucoset`; the real ball of an idele excludes 0 but may be negative: `idele.h` predicate):
   for a
   negative real ball, `Log` real is `DOMAIN` (`rfunc.h:109-112`), so the all-places `Log` of a negative idele is
   `DOMAIN` unless the real part is replaced by `log_abs`. SPEC 9.3.2 line 599 says "positive input, or the
   separately
   named `log_abs`".
10. **`x^(e/n)` of an exact rational at all places** (the f-slice9 proposal, not in the row of PLAN 1F.8). Prop 17
    (line 577) is stated at a prime. The extension: `x^(e/n)` exists at every place iff the reduced root of degree
    `n'`
    is rational (Prop 16) and then `x^(e/n) = (rational root)^(e')` with `e'` possibly negative. Not asked by PLAN;
    leave
    to the orchestrator.

## 3. Proposed cut into thin slices

Naming proposal, in the style of the existing headers: one new header `include/adelefeld/gfunc.h` ("functions at all
places"), source `src/gfunc.c`, test `tests/test_gfunc.c`, one `docs/api-1f8.md` (statements G1 ...), Julia
`tests/julia/gfunc.jl`, oracle `proto/gfunc_checks.py`. Each slice appends to the same files. Slices 1, 2, 3 are
independent in code (none calls another); they share the header and the driver file, so they are run one after the
other (or in separate lanes with disjoint headers, at the price of merge work in `adf.c`).

**Slice 1 (least machinery): the rational root of an exact rational, and the all-places root of an adele and of an
idele.**
- Functions: `int adf_rat_root(adf_rat_t y, const adf_rat_t a, ulong n, int sign)` (rational n-th root by
  `fmpz_root` on numerator and denominator, `sign` = +1 or -1 for even `n`); `int adf_adele_root(adf_adele_t y,
  const
  adf_adele_t x, ulong n, slong prec)` (finite part exactly rational: `(adf_real_root(I) ; rational root)`);
  `int adf_idele_root(adf_idele_t y, const adf_idele_t x, ulong n, slong prec)` (degree 1: copy; `n = 0`: DOMAIN;
  `n >= 2`: `NOT_DETERMINED`, subject to open question 2). Names are proposals.
- Header: new `include/adelefeld/gfunc.h`; source: new `src/gfunc.c`; also a line in `include/adelefeld.h`.
- Oracle: exact integers only (no p-adic library, no logarithm). For `n` in 1..8 and `num/den` over a grid with
  big cases (2^200-th powers, `(3^k)^n`, `n` above the bit length): the rational root by a different algorithm
  (`proto/lpow_checks.py:70` bisection `iroot`, independent of FLINT's `fmpz_root`) must agree; the existence
  criterion
  agrees with the local criterion for every prime up to a bound (`proto/functions_checks.py:585`, the valuations
  divisible
  by `n`, and the sign); the three named examples of PLAN row 1F.8 (`1` has `1` and `-1` at `n = 2`; `8` has `2` at
  `n = 3`; `2` has no square root); `0`; degree 1; the real part is checked as `root(I)^n` contains `I`.
- Driver command: `ratroot X with N` (X a rational; prints the non-negative or only real root, or `error: DOMAIN`),
  and `root X with N` for an adele (prints the adele). Pattern: one row in `adf_drv_ops[]` (`adf.c:187-229`), a case
  in the dispatcher near line 2495, README rows.
- Julia: `tests/julia/gfunc.jl` (8 has the cube root 2, 2 has no square root, 1 has square roots 1 and -1, ideles of
  degree 2 give NOT_DETERMINED); a line in `tests/test_julia.sh` after line 185.
- Files owned: `include/adelefeld/gfunc.h`, `src/gfunc.c`, `tests/test_gfunc.c`, `proto/gfunc_checks.py`,
  `tests/ref/vectors/f-slice10/`, `docs/api-1f8.md`, `tools/adf/adf.c`, `tools/adf/README.md`,
  `tests/driver/gfunc-*.cmd/.out`, `tests/julia/gfunc.jl`, `lanes/f-slice10/`; lines in `include/adelefeld.h`,
  `tests/test_julia.sh`.
- Depends on: `rfunc.h` (`adf_real_root`), `adele.h`, `idele.h`, `rat.h`; nothing in slices 2 and 3.

**Slice 2: the five series at all places on an adele whose finite part is exactly 0.**
- Functions: `adf_adele_exp`, `adf_adele_sin`, `adf_adele_sinh`, `adf_adele_cos`, `adf_adele_cosh` `(y, x, prec)`:
  `OK` only if `adf_fball_is_exact(&x->fin)` and the value is 0; the real part by the real function at `prec` bits
  (through `adf_sball_exp_at` at `adf_place_inf()` after `adf_sball_project`, which already has `sinh`, `cosh`); the
  finite part exactly 1, 0, 0, 1, 1 (Prop 12 step 1, `functions.md:394`). Otherwise `NOT_DETERMINED` (open question
  4).
- Header: `gfunc.h`; source `src/gfunc.c`; oracle: exact finite part (1,0,0,1,1); the real part against `mpmath` at
  high precision with the enclosure check (the ball contains the value) and a stated radius bound; negative, large,
  crossing-zero real balls; finite part `0 + 4 Zhat`, `1`, local backend: `NOT_DETERMINED`.
- Driver: new operations `exp X`, `sin X`, `sinh X`, `cos X`, `cosh X` (arity 1, X an adele): none exists now (1.7).
- Julia: `exp` of `(0.5 ; 0)` is `(1.6487... ; 1)`; of `(0 ; 1)` is NOT_DETERMINED.
- Depends on: nothing in slice 1; it needs `rfunc.h` and `sball.h` only.

**Slice 3 (most new machinery): `Log` on ideles, the enclosure `4 Zhat`, refined at named primes.**
- Functions: `adf_idele_Log(y, x, prec)` (an `adf_adele` `(log X ; 0 + 4 Zhat)` for `X > 0`, else `DOMAIN` or
  `NOT_DETERMINED` as `adf_real_log`, `rfunc.h:109-112`); `adf_idele_Log_at(y, where, x, places, n, prec_real, N)`
  giving
  an `adf_sball` with the refined local `Log` at each named prime. The refinement needs a new local component: the
  ball `r c + p^(v_p(r)+k) Z_p`, `k = v_p(N)` (Lemma 5, `ideles.md:92`), and for `k = 0` the unit group, whose `Log`
  image is `p Z_p` (`4 Z_2`): open question 8.
- Oracle: exact integers: the set `{Log(r u) mod p^H : u in c U(N)}` enumerated modulo `p^H`, compared with the
  returned ball
  (as `lanes/f-review6/oracle.py` does for roots); the image is the smallest ball or the stated enclosure; the claim
  "every `Log` lies in `4 Zhat`" (Prop 12 line 386) checked at primes up to 101 (`proto/functions_checks.py:433`).
- Driver: `Log X` (idele) and `Log_at X with PLACES`; Julia: an idele with `N = 6`, refined at 5 and 2 and 3.
- Depends on: `lfunc.h` (`adf_lball_Log`), `lball.h`, `idele.h`, `ucoset.h`; independent of slices 1 and 2. Last,
  because
  it needs a design decision (questions 8, 9) and a new proof statement (the image of `Log` over a unit coset at a
  prime).

**Not in the cut (decide separately):** `x^(e/n)` of an exact rational at all places (open question 10; it would be
a
fourth slice after slice 1, with `adf_lball_powrat` and `adf_rat_root` as the oracle pair); "branches over named
places" is already `roots_at` (`rfunc.h:178`, driver `roots_at`) and needs no new code.

## 4. Draft brief for slice 1 (copy to `lanes/f-slice10/brief.md` after the orchestrator's choices on questions 1-3,
5)

---

# Lane f-slice10: the rational root of an exact rational and the root at all places (milestone 1F, WP 1F.8, first
slice)

SPEC 9.3.3 (lines 636-646) fixes the root at all places. For an exact rational `a` and a degree `n >= 1` it returns
the RATIONAL root (for odd `n` the one real root, for even `n` the non-negative one, optionally both), found by
testing numerator and denominator for n-th powers; no factorisation. It does not list all adelic roots. For an idele
of degree `n >= 2` it returns `NOT_DETERMINED`; degree 1 is the identity. Exact 0 has the root 0. Proposition 16
(`docs/proofs/functions.md:538`) proves the contract. You build it as a thin working slice that a user can call
(library, driver, Julia):

1. `adf_rat_root` on `adf_rat`: the exact rational n-th root by integer root tests on numerator and denominator
   (`fmpz_root`, `refs/src/flint-3.0.1/fmpz.rst:983`: "returns 1 if the root was exact"). `n` is a `ulong`: FLINT's
   `n` is a `slong` and an even `n` needs a non-negative argument (`fmpz.rst:983-988`), so decide and test the cases
   `n` above the bit length (no root except for 0 and 1: see the static `integer_root`, `src/lroot.c:99-106`), `n`
   above
   `WORD_MAX`, and negative `a` with even `n`. Statuses: `OK`; `DOMAIN` (proved: no rational root, hence no adelic
   root,
   Proposition 16 step 3-4; `n = 0`); `LIMIT` if you need one. The sign for even `n` (open question 5 of the draft:
   decide
   and list).
2. `adf_adele_root(y, x, n, prec)`: `x = (I ; F)` with `F` exactly a rational `q`; `y = (real root of I at prec bits
   ;
   exact rational root of q)`. The real part is `adf_real_root` (`include/adelefeld/rfunc.h:79`; domain odd `n`: R,
   even `n`: `t >= 0`; `prec` above `ADF_REAL_PREC_MAX` is `LIMIT` first, N-D8). A finite part that is not exactly a
   rational (positive radius, or the local backend: `adf_fball_is_exact` is 0, `fball.h:152-153`) is
   `NOT_DETERMINED`
   (Proposition 22, `functions.md:739`: "A stored uncertain zero does not certify an exact zero"). Combined status
   by
   conventions 3.3 (maximum; `where`).
3. `adf_idele_root(y, x, n, prec)`: `n = 0`: `DOMAIN`; `n = 1`: `y = x`; `n >= 2`: `NOT_DETERMINED`, y untouched
   (SPEC 9.3.3 line 637; open question 2 of the draft: take (a) unless the orchestrator decided (b)).
4. Driver commands `ratroot X with N` and `root X with N` in the pattern of `powrat_at` (`tools/adf/adf.c:205`,
   `adf_drv_places` line 1890), `tools/adf/README.md`, a Julia example `tests/julia/gfunc.jl`.

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold for the
new
header and `gfunc.h` is yours; rule 5 as written), `docs/SPEC.md` 9.3.1 (lines 557-573), 9.3.3 (lines 613-646), 15.4
(N-D7, N-D8, N-D10, N-D13), `docs/proofs/functions.md` Proposition 14 (line 446), Proposition 16 (line 538),
Proposition 22
(line 725) and the Python checks `proto/functions_checks.py:585` (`check_global_roots`), `docs/conventions.md` 3.1
(lines 169-192), 3.2 (line 221), 3.3 (lines 234-247), `docs/PLAN.md` row 1F.8 (line 278),
`include/adelefeld/rfunc.h` (lines 73-79, 80-96), `rat.h`, `adele.h` (lines 53-57, 107, 117, 132), `fball.h` (lines
120, 152-161, 256), `idele.h` (lines 1-24, 135, 217, 221), `sball.h`, `src/lroot.c` (lines 99-106: the static
`integer_root` and its comment R3), `src/rfunc.c` (`adf_real_root`, line 247), `docs/api-1f6.md` (the style of the
statements P1 to P8, and their "Check:" lines), `lanes/f-slice9/brief.md` and `result.md`, `tests/test_lpow.c`,
`proto/lpow_checks.py` (line 70 `iroot`: an independent integer root), `tools/adf/adf.c` (lines 187-229 the table,
1890-2040 `adf_drv_places`, 2495-2497 the dispatch), `tools/adf/README.md` (479-509), `tests/julia/lpow.jl`,
`tests/test_julia.sh` (lines 183-185). `refs/src/` is on disk: FLINT is `refs/src/flint-3.0.1/` (`fmpz.rst:966-996`,
`fmpq.rst:132-185`).

**You own:** `include/adelefeld/gfunc.h` (new) and `src/gfunc.c` (new), `tests/test_gfunc.c` (new),
`tests/julia/gfunc.jl` (new), `proto/gfunc_checks.py` (new, exact integers only; the oracle of this slice),
`tests/ref/vectors/f-slice10/` (new, below 1 MB), `docs/api-1f8.md` (new: the statements G1 ... and their proofs, in
the style of `docs/api-1f6.md`), `tools/adf/adf.c`, `tools/adf/README.md`, `tests/driver/gfunc-*.cmd` and `.out`
(new),
`lanes/f-slice10/`. In `include/adelefeld.h` and `tests/test_julia.sh` you may add the lines your files need.
`src/rfunc.c`, `src/lroot.c`, `src/adele.c`, `src/fball*.c` and their headers are read-only: you call them. No git
command that changes state, no `bd`. At most 2 cores; every program under `timeout`; build into
`BUILD=lanes/f-slice10/build` while you work and into `build/` only for the final checks. Julia is on the PATH.

1. Header first: the comment block of every declaration (set statement; Proposition 14 and 16 with file and line;
   the sign convention for even `n`; the exact cases `0`, `1`, `-1`, degree 1; every status and which input gives
   it;
   what is untouched on a status; aliasing (`y` may be `x`); the limits).
2. Tests first, red then green (`lanes/f-slice10/redgreen.log`; a link error counts only for the first test).
   Oracles in exact integers, each with a stated output precision (here: exact; for the real part, the ball contains
   the true root and its radius is bounded by a stated factor of `2^-prec`):
   - the rational root: `n` from 1 to 12 and `n` large (above the bit length of numerator and of denominator, above
     `WORD_MAX`); `a = num/den` over a grid with signs and big values (numerator and denominator of thousands of
     bits,
     perfect powers and perfect powers plus or minus 1); the root by `iroot` of `proto/lpow_checks.py:70`
     (independent of FLINT) and the equality `root^n = a`; the criterion "valuations divisible by `n` at all primes
     up to
     a bound and the sign" of `proto/functions_checks.py:585` against the integer test; the named examples (`1`: `1`
     and `-1`; `8`: `2`; `2`: `DOMAIN`; `-4`, `n = 2`: `DOMAIN`; `-8`, `n = 3`: `-2`; `0`; `n = 1`);
   - the adele: the real part (ball contains the root of every point of `I`, even `n` and `I` crossing 0, odd `n`
     and `I` negative or around 0), the finite part, the combined statuses and `where`, `prec` above the limit,
     a finite part of positive radius, a local-backend finite part;
   - the idele: `n = 0, 1, 2, 3`, exact and inexact units, `y = x`;
   - every status with the state of the outputs the header promises; every aliasing combination;
   - driver golden files and the Julia example.
3. The code; the driver commands; the Julia example.
4. Five planted faults in a scratch copy under your build directory (among them: the sign of the even root taken for
   odd `n`; the denominator not tested; `n` above the bit length passed to `fmpz_root`; the real part computed from
   the
   finite part; the idele of degree 2 returned as `OK`). Mutation testing of `src/gfunc.c`, `lanes/COMMON-C.md` rule
   5
   (at most 60 mutants, `--seed 1`, 2 jobs, `--san`, `timeout 1300`; note the defect of the tool in
   `lanes/f-slice9/result.md`, "Defects of the mutation tool": give `SAN=1` in the `--make` command and do not put
   the
   string `ASAN_OPTIONS` there; check that the mutants COMPILED).
5. Final checks: `timeout 900 make -j2 check-all` once (in `build/`); one `ASAN_OPTIONS=detect_leaks=1` sanitizer
   build of `test_gfunc` in `BUILD=build/san` and its run. Give the last line of each.

Decisions where the specification is silent are yours, listed with the alternatives in the report (the orchestrator
records them in `docs/SPEC.md` 15.4 as N-D16): the types accepted, `DOMAIN` and `where` for an exact rational
without a root, the exact idele, the sign convention for even `n`. Report: `lanes/f-slice10/result.md`, written
ONCE,
AT THE END (the harness refuses the name `report.md` for a Claude subagent); running notes in
`lanes/f-slice10/progress.md` as you go. In it: functions built; decisions and alternatives; what is proved (the new
statements) and what is not; the numbers of the tests and what would have made a case fail; the fault table;
mutation
survivors one line each; findings against the specification; the next slice you propose (slice 2 or 3 of 1F.8).

---

## 5. What I did not do

- Nothing was built or run. I did not check that `tests/test_gfunc.c` or `gfunc.h` names are free (I saw no file
  with these names in `include/adelefeld/` or `src/`).
- I did not read `src/rfunc.c` beyond the lines cited, nor `docs/api-1f.md` beyond line 560-575, nor the README of
  the driver outside lines 1-25, 52-90, 198-247, 310-343 (only headings) and 479-509.
- Lines of `tools/adf/adf.c` marked "~" (1999-2003, 2080 on) are approximate; the others were seen.
- The claim that Julia `exp` of `(0.5 ; 0)` is `1.6487...` is arithmetic (e^0.5), not a run.
