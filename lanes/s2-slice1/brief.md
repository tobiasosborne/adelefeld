# Lane s2-slice1: roots at a prime, the first slice: the type and the seed function

`docs/workflow.md` rules 1 and 2: the smallest piece that a user can call and that computes something
true. Decisions S-D13 (the functions work on the normalised polynomial `g`), S-D14, S-D16 (scope SEED),
S-D17, S-D18 (checked `slong` arithmetic, `ADF_ROOTS_BITS_MAX`, `LIMIT` before allocation) are taken
(`docs/SPEC.md` 15.3).

Read first: `lanes/COMMON-C.md` (rule 6 does not hold for `roots.h`, which is yours; rule 5 is replaced by
item 5 below); `docs/proofs/solvers.md` section 3: Lemma 3.1, Definition and Proposition 3.2, Proposition
3.3, Proposition 3.12, 3.11 (the prime 2 and the examples), and Proposition 3.13(1); `docs/api-s.md`
section 4; `refs/src/conrad-hensel/` (examples 4.2 to 4.4); `proto/solvers_checks.py` (the reference of the
normalised polynomial, of the certificate, of the lifting and of the seed function; the `check_s2_*`
functions); `include/adelefeld/place.h`; `include/adelefeld/linsolve.h` and `src/linsolve.c` for the
style of a type that owns memory.

**You own:** `include/adelefeld/roots.h` (new), the one include line in `include/adelefeld.h`,
`src/roots.c` (new), `tests/test_roots_seed.c` (new; the pins of the layout go here, not into
`tests/test_abi.c`), `tests/ref/vectors/s2-slice1/` (new), `tests/julia/roots.jl` (new),
`tests/fuzz/diff_roots_seed.py` (new), `lanes/s2-slice1/`. Everything else is read-only.

## What is built

1. `include/adelefeld/roots.h`: the type `adf_rootlist` with ALL the fields of `docs/api-s.md` section 4
   (so that the layout does not change in later slices; the fields of the real place and of the unresolved
   classes stay empty in this slice), `init`, `clear`, `is_canonical`, and
   - `adf_root_padic_from_seed(L, f, p, a, prec_p)`: statuses and outputs as section 4 says;
   - `adf_rootlist_verify_entries(L, f)` for a list at a prime (at the real place it returns 0 in this
     slice, and the header says that this is temporary);
   - `adf_rootlist_length`, `adf_rootlist_is_complete`, `adf_rootlist_unresolved_length`,
     `adf_rootlist_place`, `adf_rootlist_scope`, `adf_rootlist_get_poly`, `adf_rootlist_get_cert`,
     `adf_rootlist_get_fball`.
   Nothing else of section 4 is declared in this slice.
2. The prime. Decision S-D10: the library accepts every prime, of any size; this function needs no roots
   modulo `p`, so it has no bound on `p`. Find out from `include/adelefeld/place.h`, `src/place.c` and
   `docs/conventions.md` section 7 what an `adf_place_t` guarantees: is `p` known to be prime, of what
   size can it be, who tested it? Write the answer, with file and line, into the report; the function
   relies on exactly what the place guarantees and says so in the header. Do not add a primality test.
3. Tests first, red then green (`lanes/s2-slice1/redgreen.log`), in `tests/test_roots_seed.c`:
   - Against an enumeration written in the test: for `p` in 2, 3, 5, 7, 11, polynomials of degree up to 4
     with coefficients in a small range and the products with planted integer roots, every seed `a` in
     `[0, p^4)`, `prec_p` from 1 to 6: `OK` exactly when the strong form holds for `g`; then the ball
     `a + p^K Z_p` of the certificate contains exactly one `x` modulo `p^(K+s+3)` with `g(x) = 0` modulo
     `p^(K+s+3)` up to the classes that Proposition 3.2 names (count them by enumeration, as the acceptance
     test of section 4 says), and that root is congruent to the seed modulo `p^(s+1)`.
   - Seeds with `v(g(a)) = 2 v(g'(a))` exactly are refused (`NOT_DETERMINED`, `L` untouched).
   - Conrad's examples 4.2, 4.3, 4.4 with their residues; `f = 27 X` at 3 gives the certificate
     `(0, K, 0)` for `g = X`; a polynomial with a repeated factor has `reduced = 1` and a simple root of
     `g`; the prime 2 (`solvers` 3.11).
   - `K = max(prec_p, s + 1)` and nested balls for growing `prec_p`.
   - Large: `p` of 64, 65 and 300 bits (a known prime, for example `2^64 + 13`, `2^89 - 1`, `2^521 - 1`),
     `prec_p` up to 2000; the limit `ADF_ROOTS_BITS_MAX` gives `LIMIT` before any allocation, and
     `prec_p = WORD_MAX` gives `LIMIT`, not an overflow.
   - Every certificate of the seed function passes `adf_rootlist_verify_entries`; changed lists (the
     centre moved off the root, `K` raised by one without the centre lifted, `s` changed, `g` replaced by
     `f` when they differ, two balls of one root) are refused; print how many were refused for each kind
     of change and fail if one kind has none.
   - Vectors from the reference (`lanes/s2-slice1/gen_vectors.py`, import, do not copy).
   - Statuses `DOMAIN` (`f = 0`, `prec_p < 1`, the real place), `L` untouched on every status other
     than `OK`.
4. `tests/julia/roots.jl`: the square root of 2 in `Z_7` to 20 digits from the seed 3, read through
   `adf_rootlist_get_cert`, squared in Julia. `tests/fuzz/diff_roots_seed.py --seconds N --seed S`: random
   polynomials, primes, seeds and precisions to the C function and to the reference, equal status and
   equal certificate. Run it for 180 seconds; that run is a smoke test.
5. Show that the tests bite, in a scratch copy under `build/`, one change at a time, recorded in
   `redgreen.log`: the strong form `>` changed to `>=`; `s` taken from `f` instead of `g`; one Newton
   step dropped; the final reduction of the centre modulo `p^K` dropped; each of (R1), (R2), (R3)
   removed from the verifier; one checked addition of exponents replaced by a plain one. No `make mutate`.
6. `make clean && make -j2 check`, the same with `SAN=1` and with `CC=clang`, `sh tests/test_exports.sh`
   pass; the Julia file passes (see `tests/test_julia.sh` for the `LD_PRELOAD`). Give the last line of each.

Not in this slice: `adf_roots_padic`, `adf_roots_padic_partial` (Algorithm P), `adf_roots_real`,
`adf_rootlist_verify_complete`, `get_unresolved`, `get_arb`, `set`, `swap`, `identical`, benchmarks.

If a statement of `docs/proofs/solvers.md` section 3 is wrong or cannot be implemented as written, give the
counterexample in the report.
