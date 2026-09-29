# Lane s2-sources: notes

## 1. How the 22 files of refs/src/flint-3.0.1/ were fetched

`refs/fetch_sources.sh`, two loops over `doc/source/<m>.rst` at the tag v3.0.1:

- line 68 (first loop, "FLINT 3.0.1 documentation (.rst)"): `padic fmpz_mod nmod arb acb acb_dirichlet
  fmpq ulong_extras fmpz arf mag flint fmpz_vec memory` (13 files)
- line 139 (second loop, milestone S): `fmpz_mat fmpz_mod_mat nmod_mat padic_poly fmpz_poly arb_poly
  arb_calc arb_fmpz_poly` (8 files) plus `fmpq.rst` already in the first list = 22 in total.

URL: `https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/$m.rst`

## 2. What I added to the script

A third loop in the milestone S section, same form, with
`fmpz_mod_poly fmpz_mod_poly_factor fmpz_mod nmod_poly nmod_poly_factor`. `fmpz_mod.rst` was already
fetched by the first loop, so 4 new files appeared.

Sizes and sha256 (from `refs/manifest.sha256` after the run):

| file | bytes | sha256 |
|---|---|---|
| fmpz_mod_poly.rst | 94766 | e51f4d80c8c7d42d7c9ee3f6b4c1e67f149cf5b112bb7db56f1ffd0d3e3a2b72 |
| fmpz_mod_poly_factor.rst | 9867 | 0f81560d9263a135b0e40c1a0eb5ed5d2bba2664e6afab73bf4419e1c2fdc27a |
| nmod_poly.rst | 116652 | 2dd8bae5ae92f1762641f348e8edfaa2049271ad526609e13509524bc6cedafd |
| nmod_poly_factor.rst | 8519 | 59aa4c4c74f084051e0a74b7e99955480f3c0f065d7e94ea4770dadea6a60348 |
| fmpz_mod.rst | 5989 | dd75c63b7daa022f781b0ccbe2cf5ec6568a134622a65258b86bd08b9641ef8b (unchanged, 2026-09-27) |

`./refs/fetch_sources.sh` ran without error: 2785 files in manifest.sha256, 70 in manifest-extra.sha256.
`sha256sum -c manifest.sha256` in refs/: no failure reported.

## 3. What the documentation says (line numbers of the files on disk)

### fmpz_mod_poly.rst

- 690 `fmpz_mod_poly_powmod_x_fmpz_preinv(res, e, f, finv, ctx)`: 692 "Sets ``res`` to ``x`` raised to the power
  ``e``", 693 "modulo ``f``, using sliding window exponentiation. We require", 694 "``e >= 0``. We require
  ``finv`` to be the inverse of the reverse of", 695 "``". The text STOPS in the middle of a word.
- 681 `_fmpz_mod_poly_powmod_x_fmpz_preinv(res, e, f, lenf, finv, lenfinv, ctx)`: 684 "``e > 0``",
  685 finv is the inverse of the reverse of f, 687 "We require ``lenf > 2``", 688 room for lenf-1 coefficients.
  So the degree condition (deg f >= 2) is stated for the raw variant only.
- 1030 `fmpz_mod_poly_gcd`: 1032 "Sets `G` to the greatest common divisor", 1034-1036 "In general, the
  greatest common divisor is defined in the polynomial ring `(Z/(p Z))[X]` if and only if `p` is a prime number.
  Thus, this function assumes that `p` is prime." Monic: NOT said anywhere for this function.
- 1120 (in `_fmpz_mod_poly_xgcd`) "No attempt is made to make the GCD monic."; 1131-1134 (`fmpz_mod_poly_xgcd`)
  "Except in the case where the GCD is zero, the GCD `G` is made monic."
- 563-567 `fmpz_mod_poly_find_distinct_nonzero_roots`: returns 1 only if A has deg(A) distinct nonzero roots in
  F_p; 566 "It is assumed that ``A`` is nonzero and that the modulus of ``A`` is prime."; 567 "This function
  uses Rabin's probabilistic method via gcd's with `(x + \delta)^{(p-1)/2} - 1`."
- Nothing else in the file speaks of primality at the powmod entries; nothing says p is tested.

### fmpz_mod_poly_factor.rst

- 187 `fmpz_mod_poly_roots(r, f, with_multiplicity, ctx)`: 189 all distinct roots of a nonzero f in Z/pZ,
  190 "It is expected and not checked that the modulus of `ctx` is prime.", 191 multiplicity, 192 "This
  function throws if `f` is zero, but is otherwise always successful." No squarefree, no monic requirement.
  Nothing on randomisation.
- 194 `fmpz_mod_poly_roots_factored`: 197 "It is expected and not checked that `n` is a prime factorization of
  the modulus of `ctx`."
- 107 `fmpz_mod_poly_is_squarefree`, 148 `fmpz_mod_poly_factor_squarefree`, 152 `fmpz_mod_poly_factor`
  ("Factorises a non-constant polynomial ``f`` into monic irreducible factors"), 85-93
  `fmpz_mod_poly_is_irreducible_rabin_f`: 88-89 "or sets `r` to a nontrivial factor of `p`", 91-93 "This
  algorithm correctly determines whether `f` is irreducible over `\mathbb{Z}/p\mathbb{Z}`, even for composite
  `f`, or it finds a factor of `p`."

### fmpz_mod.rst

- 20-22 `fmpz_mod_ctx_init`: "Initialise ``ctx`` for arithmetic modulo ``n``, which is expected to be positive."
  No primality, no test. The only other prime statement in the file is 122 (discrete log, "It is assumed that
  `p` is prime").

### nmod_poly.rst

- 920 `nmod_poly_powmod_x_ui_preinv(res, e, f, finv)`: 922-925, same text with ``e >= 0`` and finv.
- 911 `_nmod_poly_powmod_x_ui_preinv`: 914 "``e > 0``", 915 finv, 917 "We require ``lenf > 2``".
- 936 `nmod_poly_powmod_x_fmpz_preinv(res, e, f, finv)`: 938-941, ``e >= 0`` and finv.
- 1716 `nmod_poly_gcd`: 1718-1721 "Computes the GCD of `A` and `B`. The GCD of zero polynomials is defined to be
  zero, whereas the GCD of the zero polynomial and some other polynomial `P` is defined to be `P`. Except in
  the case where the GCD is zero, the GCD `G` is made monic." Nothing about the modulus being prime.
- 1709 `_nmod_poly_gcd`: 1713 "No attempt is made to make the GCD monic."
- 2392-2396 `nmod_poly_find_distinct_nonzero_roots`: same as the fmpz one, 2395 "It is assumed that ``A`` is
  nonzero and that the modulus of ``A`` is prime.", 2396 "This function uses Rabin's probabilistic method via
  gcd's with `(x + \delta)^{(p-1)/2} - 1`."
- 2458 (Chinese Remaindering): "In all of these functions the moduli ... is assumed to match and be prime."

### nmod_poly_factor.rst

- 184 `nmod_poly_factor`: 186 "Factorises a general polynomial ``f`` into monic irreducible factors", 194-195
  "Currently Cantor-Zassenhaus is used by default unless the modulus is 2, in which case Berlekamp is used."
- 28 `.. function::` entries; the last is 197 `_nmod_poly_interval_poly_worker`, the file has 201 lines. There
  is NO "Root Finding" section and no entry named `nmod_poly_roots`: `grep -rn 'nmod_poly_roots' ` over the 26
  fetched .rst files returns nothing. The installed header /usr/include/flint/nmod_poly_factor.h:130 declares
  `nmod_poly_roots` and :133 `nmod_poly_roots_factored`; that header is a local reference, not a fetched file.

### Answers to the questions of the brief

1. `X^p` mod `g`, `p` an `fmpz`: `fmpz_mod_poly_powmod_x_fmpz_preinv` (fmpz_mod_poly.rst:690). Requirements
   stated: `e >= 0`, `finv` the inverse of the reverse of `f`; the degree condition `lenf > 2` is stated only
   for the raw variant (681, 687). The public entry's text is cut off after a lone "``" (695). Nothing is said
   about the modulus being prime or nonzero.
   For a one-word `p`: `nmod_poly_powmod_x_ui_preinv` (nmod_poly.rst:920), raw variant 911 with `lenf > 2` (917);
   `nmod_poly_powmod_x_fmpz_preinv` (936) for an `fmpz` exponent.
2. gcd: `fmpz_mod_poly_gcd` (1030), assumes `p` prime (1036), monicity NOT said. `nmod_poly_gcd` (1716) is
   monic except when zero (1721), nothing said about the modulus.
3. roots: `fmpz_mod_poly_roots` (fmpz_mod_poly_factor.rst:187) gives all distinct roots, with the exponent
   giving the multiplicity if asked; requires `f` nonzero and expects (does not check) a prime modulus; no
   squarefree or monic requirement; randomisation not said; nothing said about a composite modulus.
   For `nmod_poly` there is no documented root function: `[source pending]`.
4. Does any of them test that `p` is prime? No documentation says that any of these functions tests primality.
   `fmpz_mod_poly_roots` says the opposite ("expected and not checked"). `fmpz_mod_ctx_init` only expects `n`
   positive. `fmpz_mod_poly_is_irreducible_rabin_f` and the `_f` squarefree variants may return a nontrivial
   factor of `p`; that is a documented way of learning that the modulus is composite, not a primality test.
